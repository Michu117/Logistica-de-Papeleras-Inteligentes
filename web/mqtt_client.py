import paho.mqtt.client as mqtt
from datetime import datetime
import re
import database
import telegram_bot

MQTT_HOST = "localhost"

def es_mac(valor):
    return bool(re.fullmatch(r'[0-9A-F]{12}', valor))

MQTT_PORT = 1883
MQTT_USER = "Ruiz26"
MQTT_PASS = "RelaxedChar206"

lecturas_en_vivo = {}


def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("MQTT conectado")
        client.subscribe("+/nivel")
        client.subscribe("+/alerta")
        client.subscribe("contenedor/test")
    else:
        print(f"Error MQTT: rc={rc}")


def on_message(client, userdata, msg):
    payload = msg.payload.decode()
    fecha = datetime.now().isoformat()

    parts = msg.topic.split('/')
    if len(parts) == 2:
        nodo_id = parts[0]
    else:
        nodo_id = "desconocido"

    if msg.topic.endswith("/nivel"):
        try:
            nivel = int(payload)
            nuevo_estado = "alerta" if nivel > 80 else "normal"

            lecturas_en_vivo[nodo_id] = {
                "nivel": nivel,
                "fecha": fecha,
                "estado": nuevo_estado
            }

            if es_mac(nodo_id) and not database.get_nodo(nodo_id):
                nombre = f"Contenedor {len(database.get_nodos()) + 1}"
                database.add_nodo(nodo_id, nombre=nombre)

            ultimo_estado = database.get_ultimo_estado(nodo_id)
            if ultimo_estado != nuevo_estado:
                database.insert_alerta(fecha, nivel, nuevo_estado, nodo_id)
                print(f"[DB] {nodo_id}: {nuevo_estado} ({nivel}%)")
                icono = "⚠ ALERTA" if nuevo_estado == "alerta" else "✓ NORMAL"
                nodo = database.get_nodo(nodo_id)
                nombre = nodo["nombre"] if nodo and nodo["nombre"] else nodo_id
                telegram_bot.enviar_alerta(f"{icono} - {nombre}\nID: {nodo_id}\nNivel: {nivel}%\nFecha: {fecha[:19]}")
            else:
                print(f"[LIVE] {nodo_id}: {nivel}% ({nuevo_estado})")
        except ValueError:
            print(f"[WARN] payload no numérico: {payload}")

    elif msg.topic.endswith("/alerta"):
        print(f"[ALERTA] {nodo_id}: {payload}")

    elif msg.topic.endswith("/test"):
        print(f"[TEST] {payload}")


def start_client():
    client = mqtt.Client()
    client.username_pw_set(MQTT_USER, MQTT_PASS)
    client.on_connect = on_connect
    client.on_message = on_message

    try:
        client.connect(MQTT_HOST, MQTT_PORT, 60)
        client.loop_forever()
    except Exception as e:
        print(f"Error conectando MQTT: {e}")
