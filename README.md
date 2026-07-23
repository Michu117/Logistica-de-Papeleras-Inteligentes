# Logística de Contenedores Inteligentes

## Visión general

Sistema IoT para monitorear el nivel de llenado de contenedores en tiempo real. Sensores ultrasónicos (simulados con potenciómetros) en ESP32 envían datos vía ESP-NOW a un gateway, que los publica por MQTT a un broker Mosquitto. Una aplicación Python (Flask + Paho MQTT + SQLite) consume los mensajes, provee un dashboard web con múltiples vistas por contenedor y notifica alertas por Telegram.

## Topología

```
Nodo 1 ──ESP-NOW──┐
(contenedor1)     │
                  ├──> Gateway (ESP32) ──MQTT──> Flask (Python)
Nodo 2 ──ESP-NOW──┘
(contenedor2)
                                                 │
                                            ┌────┴────┐
                                            │         │
                                            ▼         ▼
                                       ┌────────┐ ┌──────────┐
                                       │ SQLite │ │Dashboard │
                                       └────────┘ └──────────┘
                                                            │
                                                            ▼
                                                       Telegram
```

Se implementó una **topología en estrella**: múltiples nodos ESP32 (contenedores) se comunican por ESP-NOW punto a punto con un único Gateway, que centraliza los datos y los publica a un broker MQTT local (Mosquitto en `localhost:1883`). Una aplicación Flask (Python) se suscribe al broker, actualiza un diccionario en memoria con las lecturas en vivo de cada contenedor y persiste solo los cambios de estado (alerta ↔ normal) en SQLite. El mismo Flask sirve un dashboard web con API REST que el frontend consulta periódicamente vía AJAX, y ante cada cambio de estado envía una notificación a Telegram. Cada nodo se autoidentifica enviando un `nodo_id` en el struct ESP-NOW, lo que permite agregar nuevos contenedores sin modificar el Gateway ni hardcodear direcciones MAC.

## Componentes hardware

### Nodo contenedor (sketch_contenedor1/, sketch_contenedor2/)
- ESP32 con potenciómetro en pin 34
- Lee nivel (0-100%) cada 2 segundos
- Envía struct con `nodo_id` y nivel por ESP-NOW al Gateway
- `MI_ID` define el identificador del contenedor (1 → `contenedor1`, 2 → `contenedor2`)

### Gateway (sketch_gateway/)
- ESP32 con LED de alerta (pin 2)
- Recibe datos por ESP-NOW desde múltiples nodos
- Publica en `contenedor{ID}/nivel` con el valor numérico
- Publica en `contenedor{ID}/alerta` según el nivel
- LED encendido mientras al menos un contenedor tenga nivel > 80%

### Mosquitto
- Broker MQTT local (localhost:1883)
- Credenciales: `usuario` / `contraseña` (configurar según el broker local)
- Topics dinámicos: `contenedor1/nivel`, `contenedor1/alerta`, `contenedor2/nivel`, etc.

## Componentes software (`web/`)

### Estructura

```
web/
├── app.py              # Servidor Flask (puerto 5000)
├── database.py         # Capa de datos SQLite
├── mqtt_client.py      # Cliente MQTT (Paho)
├── telegram_bot.py     # Notificador Telegram
├── requirements.txt    # flask, paho-mqtt, requests
├── contenedores.db     # Base de datos SQLite
├── static/
│   ├── style.css       # Estilos del dashboard
│   └── dashboard.js    # Lógica del dashboard (modal, tabs, chart, tabla)
└── templates/
    └── dashboard.html  # Interfaz web (HTML mínimo)
```

### app.py
- `GET /` — Dashboard HTML con datos en tiempo real
- `GET /api/alertas` — Endpoint JSON con últimas alertas y estadísticas (filtrable por `?nodo=`)
- `GET /api/estado` — Endpoint JSON con lecturas en vivo por contenedor
- `GET /api/contenedores` — Lista de contenedores registrados
- `POST /api/contenedores` — Registrar nuevo contenedor (el ID se sanitiza eliminando espacios)
- `PUT /api/contenedores/<id>` — Editar ubicación de contenedor
- Inicia el cliente MQTT en un thread aparte

### database.py
- Tabla `alertas(id, fecha, nivel, estado, nodo_id)` — historial de cambios de estado
- Tabla `contenedores(id, lat, lon, ubicacion)` — registro de contenedores
- Solo persiste cambios de estado (alerta ↔ normal); lecturas normales quedan en memoria

### mqtt_client.py
- Se suscribe a `+/nivel` y `+/alerta` (comodín MQTT, QoS 0)
- Mantiene `lecturas_en_vivo` (dict global) con último nivel por contenedor
- Inserta en BD solo cuando hay cambio de estado
- Llama a `telegram_bot.enviar_alerta()` en cada cambio de estado

### telegram_bot.py
- Lee `TELEGRAM_TOKEN` y `TELEGRAM_CHAT_ID` desde el archivo `.env` en la raíz del proyecto
- Expone `enviar_alerta(mensaje)` que envía un mensaje de texto con formato a Telegram vía Bot API
- Si no hay token configurado, la función retorna silenciosamente sin enviar
- Se activa únicamente en transiciones de estado (alerta ↔ normal), no en cada lectura

### Dashboard web
- **Pestañas**: "Todos" (vista general con grid de tarjetas) + una pestaña por contenedor
- **Colores dinámicos**: cada contenedor recibe un color único (rojo, azul, amarillo, verde, etc.) según su número
- **Grid resumen** (vista "Todos"): tarjetas por contenedor con nivel, barra de progreso y ubicación
- **Banner de estado** (vista individual): muestra nivel actual, estado y ubicación del contenedor
- **Gráfico de líneas** (Chart.js): multicolor, con umbral punteado en 80%. No se destruye/recrea en cada actualización
- **Estadísticas**: total de alertas (>80%) y contenedores activos o último nivel máximo
- **Tabla** con últimas lecturas: ID, contenedor, ubicación, fecha, nivel (barra de color), estado
- **Paginación**: 10 filas iniciales, botón "Mostrar más" para ver el historial completo
- **Modal para agregar contenedor**: el ID se genera automáticamente (`contenedor{N+1}`) y el campo es readonly
- Auto-refresh cada 10 segundos sin recargar la página

## Flujo de datos

1. Cada nodo lee el potenciómetro y envía un struct ESP-NOW con `nodo_id` y nivel
2. Gateway recibe, identifica el nodo y publica en `contenedor{ID}/nivel` (QoS 0) y `contenedor{ID}/alerta`
3. Si algún contenedor supera 80%, publica alerta y enciende LED hasta que todos vuelvan a normal
4. Mosquitto distribuye a suscriptores
5. `mqtt_client.py` recibe, actualiza `lecturas_en_vivo` y persiste solo si hay cambio de estado
6. Si hubo cambio de estado, `telegram_bot.enviar_alerta()` envía notificación al chat configurado
7. Dashboard consulta `/api/estado` (en vivo) y `/api/alertas` (historial) cada 10s

## Nivel de QoS

Todo el sistema utiliza **QoS 0** (at most once / fire-and-forget):
- Gateway (PubSubClient): `client.publish(topic, payload)` sin argumento QoS → 0
- Python (Paho): `client.subscribe("+/nivel")` sin argumento QoS → 0
- No hay sesiones persistentes ni reenvío de mensajes

## Notificaciones Telegram

Un bot de Telegram envía un mensaje cada vez que un contenedor cambia de estado (normal → alerta o alerta → normal).

### Configuración

1. Crea un bot con [@BotFather](https://t.me/BotFather) y obtén el token
2. Obtén tu chat ID (ej: con [@userinfobot](https://t.me/userinfobot))
3. Crea un archivo `.env` en la raíz del proyecto:

```
TELEGRAM_TOKEN=tu_token_aqui
TELEGRAM_CHAT_ID=tu_chat_id_aqui
```

4. El archivo `.env` está en `.gitignore` para no exponer credenciales

### Formato del mensaje

```
⚠ ALERTA - contenedor1
Nivel: 95%
Fecha: 16/07/26 10:30:00
```

O al volver a estado normal:

```
✓ NORMAL - contenedor1
Nivel: 45%
Fecha: 16/07/26 10:35:00
```

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

## Agregar un nuevo contenedor

El sistema está diseñado para escalar sin modificar el Gateway ni el servidor. Solo dos pasos:

### 1. Firmware del nuevo nodo

Copia `sketch_contenedor1/` y renómbralo, o edita directamente `sketch_contenedor1/sketch_contenedor1.ino` cambiando `MI_ID`:

```cpp
#define MI_ID 3   // ID único para el nuevo contenedor
```

La MAC del Gateway ya está en el código; no necesita cambios.  
El nodo enviará `nodo_id=3` en el struct ESP-NOW y el Gateway publicará automáticamente en `contenedor3/nivel`.

### 2. Registro en el dashboard

Abre el dashboard en `http://localhost:5000`, haz clic en **+** (pestaña final). El ID del contenedor se genera automáticamente. Solo completa:
- Latitud / Longitud: coordenadas GPS
- Dirección: ubicación descriptiva

Esto guarda el contenedor en la tabla `contenedores` de SQLite y la pestaña aparece automáticamente.

### Topics MQTT asignados

El Gateway genera los topics dinámicamente según `nodo_id`:

| Topic | Formato | Ejemplo |
|---|---|---|
| Nivel | `contenedor{ID}/nivel` | `contenedor3/nivel` |
| Alerta | `contenedor{ID}/alerta` | `contenedor3/alerta` |

El backend Flask se suscribe a `+/nivel` y `+/alerta` (comodín MQTT), por lo que cualquier contenedor nuevo es detectado automáticamente sin reiniciar.

## Notas

- Los nodos se autoidentifican por `MI_ID` en el firmware — no requieren MACs hardcodeadas
- Los contenedores se registran desde el dashboard (modal con ID auto-generado readonly)
- Las credenciales MQTT están en `mqtt_client.py`
- Las credenciales Telegram van en `.env` (gitignored)
- El dashboard requiere Chart.js (vía CDN)
- La base de datos `contenedores.db` se crea automáticamente al iniciar