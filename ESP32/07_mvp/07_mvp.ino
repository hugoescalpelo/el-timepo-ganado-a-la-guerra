#include <WiFi.h>
#include <PubSubClient.h>
#include <SoftwareSerial.h>

// Datos de la red WiFi
const char* ssid = "INFINITUMD2AC";
const char* password = "PCwGdtcV9D";

// Datos del broker MQTT
const char* mqtt_server = "192.168.1.81";
const char* mqtt_topic = "tgg/carta";

// Configuración de IP fija
IPAddress local_IP(192, 168, 1, 32);
IPAddress gateway(192, 168, 1, 254);
IPAddress subnet(255, 255, 255, 0);

// Pines y configuración de lectura analógica
const int analogPin = 15;  // Pin analógico para leer el voltaje (GPIO15)
const float referenceVoltage = 5;  // Voltaje de referencia del ESP32
const int adcMaxValue = 4095;  // Resolución del ADC (12 bits)

// Configuración del divisor de voltaje
const float resistor1 = 10000.0;  // 10k ohms
const float resistor2 = 33000.0;  // 33k ohms

// Umbral de batería
const float batteryFullVoltage = 12.6;  // Voltaje total de la batería
const float batteryThreshold = 0.2;     // 20% de la carga total
const float batteryMinVoltage = batteryFullVoltage * batteryThreshold;  // 20% del voltaje de la batería

// Pines del relé y MP3
const int relayPin = 21;  // Pin del relé
const int mp3RxPin = 22;  // RX del ESP32 conectado a TX del YX5300
const int mp3TxPin = 23;  // TX del ESP32 conectado a RX del YX5300

// Variables de tiempo
unsigned long timeNow = 0;
unsigned long timeLastCheck = 0;
unsigned long timeLastCounter = 0;

// Intervalos de tiempo
const unsigned long intervalCheck = 20000;    // Comprobar voltaje cada 20 segundos
const unsigned long intervalCounter = 1000;   // Incrementar contador cada 1 segundo
const unsigned long mp3PlaybackDuration = 480000;  // 8 minutos en milisegundos

// Variables de estado
int counter = 0;
bool wifiConnected = false;
bool mp3Playing = false;
unsigned long mp3PlaybackStartTime = 0;

// Cliente WiFi y MQTT
WiFiClient espClient;
PubSubClient client(espClient);

// Comunicación con el MP3
SoftwareSerial mp3Serial(mp3RxPin, mp3TxPin);

// Función para generar un ID único para MQTT
String generateUniqueID() {
  String id = "";
  const char alphanum[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
  for (int i = 0; i < 6; i++) {
    id += alphanum[random(0, sizeof(alphanum) - 1)];
  }
  return id;
}

// Función para enviar comandos al MP3
void sendCommand(uint8_t command, uint8_t arg1 = 0, uint8_t arg2 = 0) {
  uint8_t commandBuffer[8] = {
    0x7E, 0xFF, 0x06, command, 0x00, arg1, arg2, 0xEF
  };
  mp3Serial.write(commandBuffer, 8);
}

// Función de callback para mensajes MQTT
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mensaje recibido en el tema: ");
  Serial.println(topic);
  Serial.print("Mensaje: ");
  if (length == 1 && (payload[0] == '0' || payload[0] == '1')) {
    char msg = (char)payload[0];
    Serial.println(msg);
    if (msg == '1') {
      if (!mp3Playing) {
        startMp3Playback();
      } else {
        Serial.println("MP3 ya está reproduciéndose.");
      }
    } else if (msg == '0') {
      if (mp3Playing) {
        stopMp3Playback();
      } else {
        Serial.println("MP3 no está reproduciéndose.");
      }
    }
  } else {
    Serial.println("Mensaje no válido");
  }
}

// Función para configurar la conexión MQTT
void reconnectMQTT() {
  while (!client.connected() && wifiConnected) {
    String clientId = "carta-" + generateUniqueID();
    Serial.println("ID Generado: " + clientId);
    Serial.print("Conectando al broker MQTT con ID: ");
    Serial.println(clientId);
    if (client.connect(clientId.c_str())) {
      Serial.println("Conectado al broker MQTT");
      client.subscribe(mqtt_topic);
    } else {
      Serial.print("Fallo, rc=");
      Serial.print(client.state());
      Serial.println(" Intentando de nuevo en 5 segundos");
      delay(5000);
    }
  }
}

// Función para iniciar la reproducción del MP3
void startMp3Playback() {
  Serial.println("Iniciando reproducción de MP3...");
  sendCommand(0x03, 0x00, 0x01);  // Reproducir el primer archivo
  mp3PlaybackStartTime = millis();
  mp3Playing = true;
}

// Función para detener la reproducción del MP3
void stopMp3Playback() {
  Serial.println("Deteniendo reproducción de MP3...");
  sendCommand(0x16);  // Comando para detener
  mp3Playing = false;
}

void setup() {
  Serial.begin(115200);

  // Inicializar generador de números aleatorios
  randomSeed(micros());

  // Configurar pines
  pinMode(analogPin, INPUT);
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);

  // Configurar MP3
  mp3Serial.begin(9600);
  sendCommand(0x06, 0x00, 0x3F); // Volumen máximo
  sendCommand(0x09, 0x00, 0x02); // Seleccionar tarjeta SD

  // Configurar WiFi y MQTT
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  // Inicializar tiempos
  timeLastCheck = millis();
  timeLastCounter = millis();
}

void loop() {
  timeNow = millis();

  // Comprobar el voltaje de la batería cada 20 segundos
  if (timeNow - timeLastCheck >= intervalCheck) {
    timeLastCheck = timeNow;

    // Desconectar WiFi antes de leer el voltaje
    if (wifiConnected) {
      WiFi.disconnect(true);
      WiFi.mode(WIFI_OFF);
      wifiConnected = false;
    }

    float batteryVoltage = leerBateria();

    Serial.print("Voltaje de la batería: ");
    Serial.print(batteryVoltage);
    Serial.println(" V");

    if (batteryVoltage < batteryMinVoltage) {
      digitalWrite(relayPin, LOW);
      stopMp3Playback();
      Serial.println("Voltaje bajo: Relé apagado.");
    } else {
      if (!wifiConnected) {
        Serial.println("Intentando reconectar a WiFi...");
        if (!WiFi.config(local_IP, gateway, subnet)) {
          Serial.println("Configuración de IP fallida.");
        }

        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid, password);

        while (WiFi.status() != WL_CONNECTED) {
          delay(500);
          Serial.print(".");
        }
        Serial.println();
        Serial.println("Conectado a WiFi");
        Serial.print("Dirección IP: ");
        Serial.println(WiFi.localIP());
        wifiConnected = true;
        counter = 0;
        delay(1000);
        reconnectMQTT();
      }
      digitalWrite(relayPin, HIGH);
      Serial.println("Voltaje suficiente: Relé encendido.");
    }
  }

  // Mantener la conexión MQTT activa
  if (wifiConnected) {
    if (!client.connected()) {
      reconnectMQTT();
    }
    client.loop();
  }

  // Imprimir el contador cada segundo
  if (wifiConnected && timeNow - timeLastCounter >= intervalCounter) {
    timeLastCounter = timeNow;
    counter++;
    Serial.print("Contador: ");
    Serial.println(counter);
  }

  // Gestionar la reproducción del MP3
  if (mp3Playing) {
    if (millis() - mp3PlaybackStartTime >= mp3PlaybackDuration) {
      Serial.println("Reproducción de MP3 completada.");
      stopMp3Playback();
    }
  }
}

// Función para leer el voltaje de la batería
float leerBateria() {
  int analogValue = analogRead(analogPin);
  float measuredVoltage = (analogValue * referenceVoltage) / adcMaxValue;
  float batteryVoltage = measuredVoltage / (resistor1 / (resistor1 + resistor2));
  return batteryVoltage;
}
