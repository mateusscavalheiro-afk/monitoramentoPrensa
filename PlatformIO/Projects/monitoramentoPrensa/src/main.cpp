#include <Arduino.h> // Obrigatório no PlatformIO
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h> 
#include <LiquidCrystal_I2C.h> 

// Configurações de conexão
const char* ssid = "Wokwi-GUEST"; // Padrão do Wokwi
const char* password = "";
const char* mqtt_server = "broker.hivemq.com"; // Corrigido de 'broke' para 'broker'

// Definição de pinos
#define PIN_DHT 15 
#define PIN_POT 34 
#define LED_G 12 
#define LED_Y 14 
#define LED_R 27 

// Objetos
WiFiClient espClient;
PubSubClient client(espClient);
DHT dht(PIN_DHT, DHT22);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// FUNÇÃO DE CALLBACK (Corrigida: unsigned int e ponteiro de payload)
void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  lcd.clear(); 
  lcd.setCursor(0,0);
  lcd.print("STATUS: ");
  lcd.setCursor(0,1);
  lcd.print(message);

  // ATIVAÇÃO DOS LEDS (Corrigida a lógica de comparação ==)
  digitalWrite(LED_G, (message == "VERDE") ? HIGH : LOW);
  digitalWrite(LED_Y, (message == "AMARELO") ? HIGH : LOW);
  digitalWrite(LED_R, (message == "VERMELHO") ? HIGH : LOW);
}

void setup() {  
  Serial.begin(115200);
  
  pinMode(LED_G, OUTPUT);
  pinMode(LED_Y, OUTPUT);
  pinMode(LED_R, OUTPUT);

  dht.begin();
  lcd.init(); 
  lcd.backlight(); 

  // Conexão Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Conectado!");

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Tentando MQTT...");
    if (client.connect("ESP32_Prensa_01")) {
      Serial.println("Conectado!");
      client.subscribe("senai/prensa/comando");
    } else {
      Serial.print("falha, rc=");
      Serial.print(client.state());
      delay(5000);
    }
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Leitura dos sensores
  float temperatura = dht.readTemperature();
  int leituraAnalogica = analogRead(PIN_POT);
  int pressaoPorcentagem = map(leituraAnalogica, 0, 4095, 0, 100);

  // Exibição Serial
  Serial.printf("Temp: %.1fC | Pres: %d%%\n", temperatura, pressaoPorcentagem);

  // Exibição LCD
  lcd.setCursor(0,0);
  lcd.print("Temp: ");
  lcd.print(temperatura, 1);
  lcd.print("C   "); 

  lcd.setCursor(0,1);
  lcd.print("Pres: ");
  lcd.print(pressaoPorcentagem);
  lcd.print("%   ");

  // Publicação MQTT
  String payload = "{\"temp\":" + String(temperatura) + ",\"press\":" + String(pressaoPorcentagem) + "}";
  client.publish("senai/prensa/telemetria", payload.c_str());

  delay(1000);
}