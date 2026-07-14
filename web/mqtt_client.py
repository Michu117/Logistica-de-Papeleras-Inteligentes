import paho.mqtt.client as mqtt
from datetime import datetime
import database

MQTT_HOST = "localhost"
MQTT_PORT = 1883
MQTT_USER = "Ruiz26"
MQTT_PASS = "RelaxedChar206"

TOPICS = ["papelera/nivel", "papelera/alerta", "papelera/test"]


def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("MQTT conectado")
        for topic in TOPICS:
            client.subscribe(topic)
    else:
        print(f"Error MQTT: rc={rc}")


def on_message(client, userdata, msg):
    payload = msg.payload.decode()
    fecha = datetime.now().isoformat()

    if msg.topic == "papelera/nivel":
        try:
            nivel = int(payload)
            nuevo_estado = "alerta" if nivel > 80 else "normal"
            ultimo_estado = database.get_ultimo_estado()
            if ultimo_estado != nuevo_estado:
                database.insert_alerta(fecha, nivel, nuevo_estado)
                print(f"[DB] Cambio de estado: {nuevo_estado} ({nivel}%)")
            else:
                print(f"[INFO] Nivel {nivel}% (sin cambio)")
        except ValueError:
            print(f"[WARN] payload no numérico: {payload}")

    elif msg.topic == "papelera/alerta":
        print(f"[ALERTA] {payload}")

    elif msg.topic == "papelera/test":
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
