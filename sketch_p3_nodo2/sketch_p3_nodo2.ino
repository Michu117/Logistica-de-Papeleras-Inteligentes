#include <WiFi.h>
#include <esp_now.h>
#include <PubSubClient.h>
#include <ESPmDNS.h>

//================= WiFi =================
const char* ssid = "Internet_UNL"; // Internet_UNL - Fabricio's A56 - MoranSanchez
const char* password = "UNL1859WiFi"; // UNL1859WiFi - holamundo100 - 0702594508

//================= MQTT =================
const int mqtt_port = 1883;
IPAddress mqtt_server;

const char* topicNivel = "papelera/nivel";
const char* topicAlerta = "papelera/alerta";

//================= Pines =================
const int LED = 2;
const int BUZZER = 15;
const int BOTON = 4;

//================= Variables =============
bool buzzerSilenciado = false;
bool alertaActiva = false;

WiFiClient espClient;
PubSubClient client(espClient);

//=========================================
// Estructura recibida desde Nodo 1
typedef struct {
  int nivel;
} Mensaje;

Mensaje datos;

//=========================================
void conectarWiFi() {

  WiFi.begin(ssid, password);

  Serial.print("Conectando al WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi conectado");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  MDNS.begin("esp32-gateway");

  for (int i = 0; i < 5; i++) {
    mqtt_server = MDNS.queryHost("michu117-pc");
    if (mqtt_server != IPAddress(0, 0, 0, 0)) break;
    delay(1000);
  }

  if (mqtt_server == IPAddress(0, 0, 0, 0)) {
    mqtt_server = IPAddress(192, 168, 100, 161);
  }

  Serial.print("Broker MQTT: ");
  Serial.println(mqtt_server.toString());
}

//=========================================
void conectarMQTT() {

  while (!client.connected()) {

    Serial.print("Conectando MQTT...");

    String clientId = "Gateway-";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {

      Serial.println("Conectado");

    } else {

      Serial.print("Error: "); 
      Serial.println(client.state());

      delay(2000);
    }
  }
}

//=========================================
void OnDataRecv(const esp_now_recv_info_t *recv_info,
                const uint8_t *incomingData,
                int len) {

  memcpy(&datos, incomingData, sizeof(datos));

  Serial.print("Nivel recibido: ");
  Serial.print(datos.nivel);
  Serial.println("%");

  // Publicar siempre el porcentaje
  char mensaje[5];
  sprintf(mensaje, "%d", datos.nivel);
  client.publish(topicNivel, mensaje);

  if (datos.nivel > 80) {

    digitalWrite(LED, HIGH);

    // Activar buzzer únicamente si no fue silenciado
    if (!buzzerSilenciado) {
      digitalWrite(BUZZER, HIGH);
    }

    // Publicar alerta una sola vez
    if (!alertaActiva) {

      client.publish(topicAlerta, "Papelera llena");

      Serial.println("***** ALERTA ENVIADA *****");

      alertaActiva = true;
    }

  } else {

    digitalWrite(LED, LOW);
    digitalWrite(BUZZER, LOW);

    // Rearmar sistema
    buzzerSilenciado = false;
    if (alertaActiva) {
      client.publish(topicAlerta, "Papelera normal");
      Serial.println("Alerta desactivada");
    }
    alertaActiva = false;
  }
}

//=========================================
void setup() {

  Serial.begin(115200);

  pinMode(LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(BOTON, INPUT_PULLUP);

  digitalWrite(LED, LOW);
  digitalWrite(BUZZER, LOW);

  // ESP-NOW
  WiFi.mode(WIFI_AP_STA);

  if (esp_now_init() != ESP_OK) {

    Serial.println("Error iniciando ESP-NOW");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);

  // WiFi + MQTT
  conectarWiFi();

  client.setServer(mqtt_server, mqtt_port);

  Serial.print("MAC Gateway (copiar a Nodo 1): ");
  Serial.println(WiFi.macAddress());
  Serial.println("Gateway listo");
}

//=========================================
void loop() {

  if (!client.connected())
    conectarMQTT();

  client.loop();

  // Pulsador para silenciar el buzzer
  if (digitalRead(BOTON) == LOW) {

    if (datos.nivel > 80 && !buzzerSilenciado) {

      buzzerSilenciado = true;

      digitalWrite(BUZZER, LOW);

      Serial.println("Buzzer silenciado");
    }

    delay(250);   // Antirrebote
  }
}