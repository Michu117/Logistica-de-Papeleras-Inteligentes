import paho.mqtt.client as mqtt
from datetime import datetime
import database

MQTT_HOST = "localhost"
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

            ultimo_estado = database.get_ultimo_estado(nodo_id)
            if ultimo_estado != nuevo_estado:
                database.insert_alerta(fecha, nivel, nuevo_estado, nodo_id)
                print(f"[DB] {nodo_id}: {nuevo_estado} ({nivel}%)")
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
