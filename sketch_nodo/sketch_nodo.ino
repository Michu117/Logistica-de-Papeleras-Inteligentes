#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

uint8_t gatewayMAC[] = { 0x00, 0x70, 0x07, 0x82, 0xE9, 0xA0 };

const int potPin = 34;

typedef struct {
  int nivel;
} Mensaje;

Mensaje datos;

void setup() {
  Serial.begin(115200);
  pinMode(potPin, INPUT);

  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(7, WIFI_SECOND_CHAN_NONE);
  vTaskDelay(pdMS_TO_TICKS(100));

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error ESP-NOW");
    return;
  }

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, gatewayMAC, 6);
  peerInfo.channel = 7;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Error agregar peer");
    return;
  }

  Serial.print("Nodo MAC ");
  Serial.print(WiFi.macAddress());
  Serial.println(" listo (canal 7)");
}

void loop() {
  int lectura = analogRead(potPin);
  datos.nivel = map(lectura, 0, 4095, 0, 100);

  esp_now_send(gatewayMAC, (uint8_t *)&datos, sizeof(datos));

  Serial.print("Nivel: ");
  Serial.println(datos.nivel);

  vTaskDelay(pdMS_TO_TICKS(2000));
}
