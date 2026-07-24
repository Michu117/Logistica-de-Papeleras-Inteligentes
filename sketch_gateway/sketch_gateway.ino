#include <WiFi.h>
#include <esp_now.h>
#include <PubSubClient.h>
#include <ESPmDNS.h>
#include <esp_wifi.h>

//================= WiFi =================
const char* ssid = "MoranSanchez";
const char* password = "0702594508";

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
volatile uint8_t macRecibido[6];

#define MAX_NODOS 20
struct {
  uint8_t mac[6];
  bool alerta;
} nodosAlerta[MAX_NODOS];
int numNodos = 0;

WiFiClient espClient;
PubSubClient client(espClient);

//=========================================
typedef struct {
  int nivel;
} Mensaje;

Mensaje datos;

//=========================================
int findOrCreateNodo(const uint8_t *mac) {
  for (int i = 0; i < numNodos; i++) {
    if (memcmp(nodosAlerta[i].mac, mac, 6) == 0) return i;
  }
  if (numNodos < MAX_NODOS) {
    memcpy(nodosAlerta[numNodos].mac, mac, 6);
    nodosAlerta[numNodos].alerta = false;
    return numNodos++;
  }
  return -1;
}

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
  memcpy((void*)macRecibido, recv_info->src_addr, 6);
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

  Serial.println("Gateway listo (ID por MAC)");
}

//=========================================
void loop() {
  if (!client.connected())
    conectarMQTT();
  client.loop();

  if (datosRecibidos) {
    datosRecibidos = false;

    char macStr[13];
    snprintf(macStr, sizeof(macStr), "%02X%02X%02X%02X%02X%02X",
             macRecibido[0], macRecibido[1], macRecibido[2],
             macRecibido[3], macRecibido[4], macRecibido[5]);

    char topicNivel[64];
    char topicAlerta[64];
    snprintf(topicNivel, sizeof(topicNivel), "%s/nivel", macStr);
    snprintf(topicAlerta, sizeof(topicAlerta), "%s/alerta", macStr);

    Serial.print("MAC ");
    Serial.print(macStr);
    Serial.print(" nivel: ");
    Serial.print(nivelRecibido);
    Serial.println("%");

    char mensaje[8];
    sprintf(mensaje, "%d", nivelRecibido);
    client.publish(topicNivel, mensaje);

    bool enAlerta = nivelRecibido > 80;

    uint8_t macCopy[6];
    memcpy(macCopy, (const uint8_t*)macRecibido, 6);
    int idx = findOrCreateNodo(macCopy);
    if (idx >= 0) {
      nodosAlerta[idx].alerta = enAlerta;
    }

    if (enAlerta) {
      client.publish(topicAlerta, "Contenedor lleno");
      Serial.print("***** ALERTA MAC ");
      Serial.println(macStr);
    } else {
      client.publish(topicAlerta, "Contenedor normal");
    }

    bool algunAlerta = false;
    for (int i = 0; i < numNodos; i++) {
      if (nodosAlerta[i].alerta) { algunAlerta = true; break; }
    }
    digitalWrite(LED, algunAlerta ? HIGH : LOW);
  }
}
