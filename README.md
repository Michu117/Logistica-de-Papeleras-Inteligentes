# Logistica-de-Papeleras-Inteligentes

## Visión general

Sistema IoT para monitorear el nivel de llenado de papeleras en tiempo real. Sensores ultrasónicos (simulados con potenciómetros) en ESP32 envían datos vía ESP-NOW a un gateway, que los publica por MQTT a un broker Mosquitto. Una aplicación Python (Flask + Paho MQTT + SQLite) consume los mensajes y provee un dashboard web.

## Arquitectura

```
Nodo1 (ESP32) ──ESP-NOW──> Gateway (ESP32) ──MQTT──> Mosquitto ──> Flask (Python) ──> SQLite + Dashboard Web
```

## Componentes hardware

### Nodo 1 (sketch_p3_nodo1/)
- ESP32 con potenciómetro en pin 34
- Lee nivel (0-100%) cada 2 segundos
- Envía por ESP-NOW al Gateway

### Nodo 2 - Gateway (sketch_p3_nodo2/)
- ESP32 con LED (pin 2), buzzer (pin 15), botón (pin 4)
- Recibe datos por ESP-NOW
- Publica en `papelera/nivel` con el valor numérico
- Publica en `papelera/alerta` ("Papelera llena" / "Papelera normal")
- Activa LED y buzzer cuando nivel > 80%
- Botón para silenciar buzzer

### Mosquitto
- Broker MQTT local (localhost:1883)
- Credenciales: `Ruiz26` / `RelaxedChar206`
- Topics: `papelera/nivel`, `papelera/alerta`, `papelera/test`

## Componentes software (`web/`)

### Estructura

```
web/
├── app.py              # Servidor Flask (puerto 5000)
├── database.py         # Capa de datos SQLite
├── mqtt_client.py      # Cliente MQTT (Paho)
├── requirements.txt    # flask, paho-mqtt
├── papeleras.db        # Base de datos SQLite
├── static/
│   └── style.css       # Estilos del dashboard
└── templates/
    └── dashboard.html  # Interfaz web
```

### app.py
- `/` — Dashboard HTML con datos en tiempo real
- `/api/alertas` — Endpoint JSON con últimas alertas y estadísticas
- Inicia el cliente MQTT en un thread aparte

### database.py
- Tabla `alertas(id, fecha, nivel, estado)`
- `init_db()` — crea la tabla si no existe
- `insert_alerta(fecha, nivel, estado)` — inserta un registro
- `get_alertas(limit)` — últimas N alertas
- `get_ultimo_estado()` — último estado registrado (para evitar duplicados)
- `get_stats()` — total de registros y último nivel de alerta

### mqtt_client.py
- Se conecta al broker MQTT local con las credenciales del sistema
- Se suscribe a `papelera/nivel`, `papelera/alerta`, `papelera/test`
- Solo almacena en BD cuando hay cambio de estado (nivel > 80 → `alerta`, ≤ 80 → `normal`)
- Consulta el último estado desde la BD para evitar duplicados

### Dashboard web
- **Banner de estado**: muestra "Papelera normal" (verde) o "Papelera llena" (⚠ rojo)
- **Total de alertas** y **último nivel máximo registrado**
- **Gráfico de líneas** (Chart.js): línea roja > 80%, verde ≤ 80%, con línea punteada de umbral en 80%
- **Tabla** con últimas 20 lecturas (ID, fecha, nivel, estado)
- Actualización automática cada 10 segundos
- Botón ↻ para refrescar manualmente

## Flujo de datos

1. Nodo 1 lee potenciómetro y envía nivel (0-100%) por ESP-NOW
2. Gateway recibe y publica en `papelera/nivel`
3. Si nivel > 80%, publica alerta y activa LED/buzzer
4. Mosquitto recibe y distribuye a suscriptores
5. `mqtt_client.py` recibe el mensaje, compara con último estado en BD
6. Si hay cambio de estado, inserta en SQLite
7. Dashboard Flask consulta la BD y muestra los datos

## Instalación y ejecución

```bash
# Instalar dependencias
pip install -r web/requirements.txt

# Iniciar Mosquitto (si no está corriendo)
# El dashboard asume Mosquitto en localhost:1883 con credenciales configuradas

# Ejecutar la aplicación
python web/app.py

# Abrir en navegador
http://localhost:5000
```

## Notas

- Las credenciales MQTT están hardcodeadas en `mqtt_client.py`
- No se requiere modificar los ESP32 existentes
- El dashboard requiere JavaScript (Chart.js vía CDN)
