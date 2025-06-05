#include <WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"

#define DHTPIN 15      
#define DHTTYPE DHT22  
#define LDR_PIN 36    
#define PH_PIN 39    
#define SOLO_PIN 34     
#define BOMBA_PIN 2    

const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* mqtt_server = "broker.emqx.io"; 

WiFiClient espClient;
PubSubClient client(espClient);
DHT dht(DHTPIN, DHTTYPE);

unsigned long tempoBombaLigada = 0;
const unsigned long DURACAO_BOMBA = 30000;
bool bombaAtiva = false;

void setup_wifi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi conectado!");
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
  pinMode(BOMBA_PIN, OUTPUT);
  dht.begin();       
  setup_wifi();      
  client.setServer(mqtt_server, 1883); 
}

void loop() {
  if (!client.connected()) {
    reconnect();  
  }
  client.loop();

  unsigned long timestamp = millis(); 

  float temp = dht.readTemperature();
  float hum = dht.readHumidity();

  if (!isnan(temp) && !isnan(hum)) {
    String payloadAmbiente = 
      "{\"temperatura_A\":" + String(temp) +
      ", \"humidade_A\":" + String(hum) +
      ", \"timestamp\":" + String(timestamp) + "}";
    client.publish("IPB/IOT/sensor/ambiente", payloadAmbiente.c_str());
    Serial.println("Ambiente: " + payloadAmbiente);
  }

  int leituraLuz = analogRead(LDR_PIN); 
  float luz = (leituraLuz / 4095.0) * 100.0;  
  String payloadLuz = 
    "{\"luz_A\":" + String(luz, 1) +
    ", \"timestamp\":" + String(timestamp) + "}";
  client.publish("IPB/IOT/sensor/luz", payloadLuz.c_str());
  Serial.println("Luz: " + payloadLuz);

  int leituraPH = analogRead(PH_PIN);
  float ph = (leituraPH / 4095.0) * 14.0;
  String payloadPH = 
    "{\"ph_A\":" + String(ph, 2) +
    ", \"timestamp\":" + String(timestamp) + "}";
  client.publish("IPB/IOT/sensor/ph", payloadPH.c_str());
  Serial.println("pH: " + payloadPH);


  if (!bombaAtiva && hum < 35) {
    bombaAtiva = true;
    tempoBombaLigada = millis();
    digitalWrite(BOMBA_PIN, HIGH);
    Serial.println(">> Rega ATIVADA (temporizada)");
  }

  if (bombaAtiva && millis() - tempoBombaLigada >= DURACAO_BOMBA) {
    bombaAtiva = false;
    digitalWrite(BOMBA_PIN, LOW);
    Serial.println(">> Rega DESLIGADA após tempo");
  }

  delay(10000); 
}
