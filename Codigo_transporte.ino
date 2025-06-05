#include <WiFi.h>
#include <PubSubClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "DHT.h"

#define DHTPIN 15      
#define DHTTYPE DHT22  

const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* mqtt_server = "broker.emqx.io"; 

WiFiClient espClient;
PubSubClient client(espClient);
DHT dht(DHTPIN, DHTTYPE);

// Configuração NTP
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000); // Fuso horário UTC

void setup_wifi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi conectado!");
  timeClient.begin(); // Inicia o cliente NTP
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Conectando ao MQTT...");
    if (client.connect("ESP32Client")) {
      Serial.println("Conectado!");
    } else {
      Serial.print("Falha, tentando em 5s...");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  dht.begin();       
  setup_wifi();      
  client.setServer(mqtt_server, 1883); 
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  timeClient.update();
  unsigned long timestamp = timeClient.getEpochTime() * 1000; 

  float temp = dht.readTemperature();
  float hum = dht.readHumidity();

  if (!isnan(temp) && !isnan(hum)) {
    String payloadAmbiente = 
      "{\"temperatura_B\":" + String(temp) + 
      ", \"humidade_B\":" + String(hum) +     
      ", \"timestamp\":" + String(timestamp) + "}";
      
    client.publish("IPB/IOT/sensor/ambiente/transporte", payloadAmbiente.c_str());
    Serial.println("Ambiente: " + payloadAmbiente);
  }

  delay(10000);
}
