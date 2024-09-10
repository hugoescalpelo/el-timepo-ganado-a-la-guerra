#include <WiFi.h>
#include <PubSubClient.h>
#include <SoftwareSerial.h>

// Configuración WiFi
const char* ssid = "INFINITUMD2AC";
const char* password = "PCwGdtcV9D";
const char* mqtt_server = "192.168.1.101";
IPAddress local_IP(192, 168, 1, 120); // IP fija
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

// Configuración del cliente MQTT
WiFiClient espClient;
PubSubClient client(espClient);

// Pines y hardware
#define MP3_RX 22
#define MP3_TX 23
#define RELAY_PIN 21
#define BATTERY_PIN 15

SoftwareSerial mp3(MP3_RX, MP3_TX); // Comunicación con el MP3 usando SoftwareSerial

// Configuración del voltaje de la batería
const float maxBatteryVoltage = 12.6;  // Voltaje máximo de la batería
const float minBatteryVoltage = 10.5;  // Voltaje mínimo de la batería
const float batteryDividerRatio = (10.0 + 33.0) / 10.0; // Relación del divisor
const float adcMaxValue = 4095.0;      // Valor máximo del ADC
const float referenceVoltage = 3.3;    // Voltaje de referencia del ADC

// Variables globales
unsigned long lastReconnectAttempt = 0;
unsigned long lastInternetCheck = 0;
unsigned long lastVoltagePrint = 0;
const unsigned long internetCheckInterval = 2 * 60 * 1000; // 2 minutos
const unsigned long voltagePrintInterval = 1000; // 1 segundo
bool playingAudio = false;

void setup() {
  // Configuración de pines
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW); // Inicialmente apagado
  
  // Configuración del SoftwareSerial para el MP3
  mp3.begin(9600);

  // Configuración del monitor serial
  Serial.begin(115200);
  delay(1000); // Espera a que se inicie el Serial

  // Configuración WiFi
  WiFi.config(local_IP, gateway, subnet);
  WiFi.begin(ssid, password);

  // Configuración del cliente MQTT
  client.setServer(mqtt_server, 1883);
  client.setCallback(mqttCallback);

  // Conectar al WiFi
  connectToWiFi();

  // Inicializar el relé según el estado de la batería
  checkBattery();
}

void loop() {
  if (!client.connected()) {
    unsigned long now = millis();
    if (now - lastReconnectAttempt > internetCheckInterval) {
      lastReconnectAttempt = now;
      if (reconnect()) {
        lastReconnectAttempt = 0;
      } else {
        ESP.restart(); // Reinicia si no se puede reconectar
      }
    }
  } else {
    client.loop();
  }

  // Monitorear el voltaje de la batería
  checkBattery();
  delay (1000);

  // Imprimir el voltaje cada segundo
  unsigned long now = millis();
  if (now - lastVoltagePrint >= voltagePrintInterval) {
    lastVoltagePrint = now;
    printBatteryVoltage();
  }
}

void connectToWiFi() {
  Serial.println("Conectando a WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("Conectado a WiFi!");
  Serial.print("IP asignada: ");
  Serial.println(WiFi.localIP());
}

boolean reconnect() {
  Serial.println("Intentando conectar a MQTT...");
  if (client.connect("ESP32Client")) {
    Serial.println("Conectado a MQTT!");
    client.subscribe("cartas/play");
    client.subscribe("cartas/stop");
  }
  return client.connected();
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.print("Mensaje recibido en el tópico: ");
  Serial.println(topic);
  Serial.print("Contenido del mensaje: ");
  Serial.println(message);

  if (String(topic) == "cartas/play" && !playingAudio) {
    playAudio();
  } else if (String(topic) == "cartas/stop" && playingAudio) {
    stopAudio();
  }
}

void sendCommand(uint8_t command, uint8_t arg1 = 0, uint8_t arg2 = 0) {
  uint8_t commandBuffer[8] = {0x7E, 0xFF, 0x06, command, 0x00, arg1, arg2, 0xEF};
  mp3.write(commandBuffer, 8);
}

void playAudio() {
  sendCommand(0x03, 0x00, 0x01); // Comando para reproducir la pista 1
  playingAudio = true;
  Serial.println("Reproduciendo audio...");
}

void stopAudio() {
  sendCommand(0x16); // Comando para detener el audio
  playingAudio = false;
  Serial.println("Audio detenido.");
}

void checkBattery() {
  int rawValue = analogRead(BATTERY_PIN);
  float batteryVoltage = (rawValue / adcMaxValue) * referenceVoltage * batteryDividerRatio;
  
  if (batteryVoltage <= minBatteryVoltage) {
    digitalWrite(RELAY_PIN, LOW);  // Apaga el relé
    WiFi.disconnect();             // Desconecta del WiFi
    Serial.println("Batería baja, desconectado de WiFi y relé apagado.");
  } else {
    digitalWrite(RELAY_PIN, HIGH); // Enciende el relé
  }
}

void printBatteryVoltage() {
  int rawValue = analogRead(BATTERY_PIN);
  float batteryVoltage = (rawValue / adcMaxValue) * referenceVoltage * batteryDividerRatio;
  Serial.print("Voltaje de la batería: ");
  Serial.print(batteryVoltage);
  Serial.println("V");
}
