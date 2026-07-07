#include <WiFi.h>
#include <esp_now.h>

// ========= MAC del Gateway =========
// Reemplazar con la MAC del Nodo 2
uint8_t gatewayMAC[] = { 0x00, 0x70, 0x07, 0x82, 0xE9, 0xA0 };

const int potPin = 34;

typedef struct {
  int nivel;
} Mensaje;

Mensaje datos;

void OnDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {

  Serial.print("Estado envio: ");

  if (status == ESP_NOW_SEND_SUCCESS)
    Serial.println("OK");
  else
    Serial.println("ERROR");
}

void setup() {

  Serial.begin(115200);

  pinMode(potPin, INPUT);

  WiFi.mode(WIFI_STA);

  Serial.print("MAC Nodo 1: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error inicializando ESP-NOW");
    return;
  }

  esp_now_register_send_cb(OnDataSent);

  esp_now_peer_info_t peerInfo = {};

  memcpy(peerInfo.peer_addr, gatewayMAC, 6);

  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("No se pudo agregar el Gateway");
    return;
  }

  Serial.println("Nodo 1 listo.");
}

void loop() {

  int lectura = analogRead(potPin);

  datos.nivel = map(lectura, 0, 4095, 0, 100);

  esp_now_send(gatewayMAC, (uint8_t *)&datos, sizeof(datos));

  Serial.print("Nivel enviado: ");
  Serial.print(datos.nivel);
  Serial.println("%");

  delay(3000);
}