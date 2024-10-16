#include <WiFi.h>
#include <PubSubClient.h>

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

// Pin y configuración de lectura analógica
const int analogPin = 15;  // Pin analógico al que leeremos el voltaje (GPIO15)
const float referenceVoltage = 5;  // Voltaje de referencia del ESP32
const int adcMaxValue = 4095;  // Resolución del ADC (12 bits)

// Configuración del circuito divisor de voltaje
const float resistor1 = 10000.0;  // 10k ohms
const float resistor2 = 33000.0;  // 33k ohms

// Umbral de batería (20% de 12.6V)
const float batteryFullVoltage = 12.6;  // Voltaje total de la batería
const float batteryThreshold = 0.2;  // 20% de la carga total
const float batteryMinVoltage = 12.18;  // 20% del voltaje de la batería

// Pin del relé
const int relayPin = 21;  // Pin donde está conectado el relé

// Variables de tiempo no bloqueantes
unsigned long timeNow = 0;
unsigned long timeLastCheck = 0;
unsigned long timeLastCounter = 0;

// Intervalos de tiempo
const unsigned long intervalCheck = 20000; // Comprobar voltaje cada 20 segundos (20,000 ms)
const unsigned long intervalCounter = 1000; // Incrementar contador cada 1 segundo (1000 ms)

// Variables de contador e indicador de conexión
int counter = 0;
bool wifiConnected = false;

// Cliente WiFi y MQTT
WiFiClient espClient;
PubSubClient client(espClient);

// Función para generar un ID único para el cliente MQTT
String generateUniqueID() {
  String id = "";
  const char alphanum[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
  for (int i = 0; i < 6; i++) {
    id += alphanum[random(0, sizeof(alphanum) - 1)];
  }
  return id;
}

// Función de callback para mensajes MQTT
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mensaje recibido en el tema: ");
  Serial.println(topic);
  Serial.print("Mensaje: ");
  if (length == 1 && (payload[0] == '0' || payload[0] == '1')) {
    Serial.println((char)payload[0]);
  } else {
    Serial.println("Mensaje no válido");
  }
}

// Función para configurar la conexión MQTT
void reconnectMQTT() {
  while (!client.connected() && wifiConnected) {
    String clientId = "carta-" + generateUniqueID();  // Generar nuevo ID en cada intento
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

void setup() {
  // Inicializar el monitor serial
  Serial.begin(115200);

  // Inicializar generador de números aleatorios
  randomSeed(micros());

  // Configurar el pin D15 como entrada analógica
  pinMode(analogPin, INPUT);
  
  // Configurar el pin del relé como salida
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);  // Inicialmente apagar el relé
  
  // Desconectar cualquier conexión WiFi al inicio
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);  // Apagar el WiFi al inicio para asegurar estado limpio

  // Configurar el cliente MQTT
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  // Inicializar los tiempos
  timeLastCheck = millis();
  timeLastCounter = millis();
}

void loop() {
  // Obtener el tiempo actual
  timeNow = millis();

  // Comprobar el voltaje de la batería cada 20 segundos
  if (timeNow - timeLastCheck >= intervalCheck) {
    timeLastCheck = timeNow;

    // Desconectar WiFi si está conectado
    if (wifiConnected) {
      //Serial.println("Batería por debajo del 20%, desconectando WiFi...");
      WiFi.disconnect(true);
      WiFi.mode(WIFI_OFF);  // Apagar el WiFi para ahorrar energía
      wifiConnected = false;
    }
    float batteryVoltage = leerBateria();

    Serial.print("Voltaje de la batería: ");
    Serial.print(batteryVoltage);
    Serial.println(" V");

    // Control del relé basado en el voltaje de la batería
    if (batteryVoltage < batteryMinVoltage) {
      // Si el voltaje de la batería está por debajo del 20%, apagar el relé y WiFi
      digitalWrite(relayPin, LOW);
      Serial.println("Voltaje bajo: Relé apagado.");

      // if que apaga el wifi se esta bajo el voltaje
    } else {
      // Si no está conectado a WiFi, intentar reconectar
      if (!wifiConnected) {
        Serial.println("Intentando reconectar a WiFi...");
        
        // Configurar IP fija antes de cada reconexión a WiFi
        if (!WiFi.config(local_IP, gateway, subnet)) {
          Serial.println("Configuración de IP fallida.");
        }

        WiFi.mode(WIFI_STA);  // Cambiar a modo estación
        WiFi.begin(ssid, password);  // Intentar conectarse a la red WiFi

        // Esperar hasta conectarse
        while (WiFi.status() != WL_CONNECTED) {
          delay(500);
          Serial.print(".");
        }
        Serial.println();
        Serial.println("Conectado a WiFi");
        Serial.print("Dirección IP: ");
        Serial.println(WiFi.localIP());
        wifiConnected = true;
        counter = 0; // Reiniciar el contador cuando se conecte

        delay(1000);  // Esperar 1 segundo antes de intentar conectarse al broker MQTT

        // Conectar al broker MQTT después de conectar a WiFi con un nuevo ID
        reconnectMQTT();
      }
      // Si el voltaje de la batería está por encima del 20%, encender el relé
      digitalWrite(relayPin, HIGH);
      Serial.println("Voltaje suficiente: Relé encendido.");
    }
  }

  // Mantener la conexión con el broker MQTT activa si está conectado
  //if (wifiConnected && !client.connected()) {
  //  reconnectMQTT();
  //}

  // Imprimir el contador cada segundo mientras esté conectado a WiFi
  if (wifiConnected && timeNow - timeLastCounter >= intervalCounter) {
    timeLastCounter = timeNow;
    counter++;
    Serial.print("Contador: ");
    Serial.println(counter);
  }

  // Mantener la conexión activa
  if (client.connected()) {
    client.loop();
  }
}

// Función para leer el voltaje de la batería a través del divisor de voltaje
float leerBateria() {
  // Leer el valor analógico del pin D15
  int analogValue = analogRead(analogPin);

  // Convertir el valor leído en voltaje real (teniendo en cuenta el divisor de voltaje)
  float measuredVoltage = (analogValue * referenceVoltage) / adcMaxValue;

  // Calcular el voltaje real de la batería utilizando la fórmula del divisor de voltaje
  float batteryVoltage = measuredVoltage / (resistor1 / (resistor1 + resistor2));

  return batteryVoltage;
}
