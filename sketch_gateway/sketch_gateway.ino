#include <WiFi.h>
#include <esp_now.h>
#include <PubSubClient.h>
#include <ESPmDNS.h>
#include <esp_wifi.h>

//================= WiFi =================
const char* ssid = "MoranSanchez"; // MoranSanchez  Internet_UNL
const char* password = "0702594508"; // 0702594508  UNL1859WiFi

//================= MQTT =================
const int mqtt_port = 1883;
const char* mqtt_user = "Ruiz26";
const char* mqtt_pass = "RelaxedChar206";
const char* mqtt_host = "michu117-pc";
IPAddress mqtt_server;

//================= Pines =================
const int LED = 2;

//================= Variables =============
volatile bool datosRecibidos = false;
volatile int nivelRecibido = 0;
volatile int nodoIdRecibido = 0;

WiFiClient espClient;
PubSubClient client(espClient);

//=========================================
typedef struct {
  int nivel;
  int nodo_id;
} Mensaje;

Mensaje datos;

//=========================================
void conectarWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Conectando al WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    vTaskDelay(pdMS_TO_TICKS(500));
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado");
  Serial.print("Canal: ");
  Serial.println(WiFi.channel());
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  esp_wifi_set_ps(WIFI_PS_NONE);

  MDNS.begin("esp32-gateway");
  for (int i = 0; i < 5; i++) {
    mqtt_server = MDNS.queryHost(mqtt_host);
    if (mqtt_server != IPAddress(0, 0, 0, 0)) break;
    vTaskDelay(pdMS_TO_TICKS(1000));
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
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
      Serial.println("Conectado");
    } else {
      Serial.print("Error: ");
      Serial.println(client.state());
      vTaskDelay(pdMS_TO_TICKS(2000));
    }
  }
}

//=========================================
void OnDataRecv(const esp_now_recv_info_t *recv_info,
                const uint8_t *incomingData,
                int len) {
  memcpy(&datos, incomingData, sizeof(datos));
  nivelRecibido = datos.nivel;
  nodoIdRecibido = datos.nodo_id;
  datosRecibidos = true;
}

//=========================================
void setup() {
  Serial.begin(9600);

  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);

  WiFi.mode(WIFI_AP_STA);
  conectarWiFi();

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error iniciando ESP-NOW");
    return;
  }
  esp_now_register_recv_cb(OnDataRecv);

  client.setServer(mqtt_server, mqtt_port);

  Serial.print("MAC Gateway: ");
  Serial.println(WiFi.macAddress());

  Serial.println("Gateway listo (auto-ID por ESP-NOW)");
}

//=========================================
void loop() {
  if (!client.connected())
    conectarMQTT();
  client.loop();

  if (datosRecibidos) {
    datosRecibidos = false;

    if (nodoIdRecibido == 0) {
      Serial.println("nodo_id=0 ignorado");
      return;
    }

    char topicNivel[50];
    char topicAlerta[50];
    snprintf(topicNivel, sizeof(topicNivel), "contenedor%d/nivel", nodoIdRecibido);
    snprintf(topicAlerta, sizeof(topicAlerta), "contenedor%d/alerta", nodoIdRecibido);

    Serial.print("Nodo ");
    Serial.print(nodoIdRecibido);
    Serial.print(" nivel: ");
    Serial.print(nivelRecibido);
    Serial.println("%");

    char mensaje[8];
    sprintf(mensaje, "%d", nivelRecibido);
    client.publish(topicNivel, mensaje);

    if (nivelRecibido > 80) {
      digitalWrite(LED, HIGH);
      client.publish(topicAlerta, "Contenedor lleno");
      Serial.print("***** ALERTA nodo ");
      Serial.println(nodoIdRecibido);
    } else {
      digitalWrite(LED, LOW);
      client.publish(topicAlerta, "Contenedor normal");
    }
  }
}
