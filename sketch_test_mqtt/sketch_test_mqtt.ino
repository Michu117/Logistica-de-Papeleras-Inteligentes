#include <WiFi.h>
#include <PubSubClient.h>
#include <ESPmDNS.h>

const char* ssid = "Internet_UNL"; // Internet_UNL - Fabricio's A56 - MoranSanchez
const char* password = "UNL1859WiFi"; // UNL1859WiFi - holamundo100 - 0702594508

const int mqtt_port = 1883;
IPAddress mqtt_server;

const char* topicNivel = "papelera/nivel";
const char* topicAlerta = "papelera/alerta";

WiFiClient espClient;
PubSubClient client(espClient);

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

  MDNS.begin("esp32-test");
  for (int i = 0; i < 5; i++) {
    mqtt_server = MDNS.queryHost("michu117-pc");
    if (mqtt_server != IPAddress(0, 0, 0, 0)) break;
    delay(1000);
  }
  if (mqtt_server == IPAddress(0, 0, 0, 0)) {
    mqtt_server = IPAddress(10, 20, 137, 186);
  }

  Serial.print("Broker MQTT: ");
  Serial.println(mqtt_server.toString());
}

void conectarMQTT() {

  while (!client.connected()) {

    Serial.print("Conectando MQTT...");

    String clientId = "Test-";
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

void setup() {

  Serial.begin(115200);

  conectarWiFi();

  client.setServer(mqtt_server, mqtt_port);

  Serial.println("Test MQTT listo");
}

int contador = 0;
int nivel = 0;

void loop() {

  if (!client.connected())
    conectarMQTT();

  client.loop();

  nivel = random(0, 101);

  char mensaje[8];
  sprintf(mensaje, "%d", nivel);

  client.publish(topicNivel, mensaje);
  Serial.print("Publicado papelera/nivel: ");
  Serial.print(nivel);
  Serial.println("%");

  if (nivel > 80) {

    client.publish(topicAlerta, "Papelera llena");
    Serial.println("Publicado papelera/alerta: Papelera llena");

  } else {

    client.publish(topicAlerta, "Papelera normal");
    Serial.println("Publicado papelera/alerta: Papelera normal");
  }

  contador++;

  if (contador % 5 == 0) {

    client.publish("papelera/test", "Test OK");
    Serial.println("Publicado papelera/test: Test OK");
  }

  delay(3000);
}
