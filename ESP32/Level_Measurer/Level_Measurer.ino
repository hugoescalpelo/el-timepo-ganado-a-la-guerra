#include <WiFi.h>

// Datos de la red WiFi
const char* ssid = "INFINITUMD2AC";       
const char* password = "PCwGdtcV9D";      

// Pin y configuración de lectura analógica
const int analogPin = 15;  // Pin analógico al que leeremos el voltaje (GPIO15)
const float referenceVoltage = 5;  // Voltaje de referencia del ESP32
const int adcMaxValue = 4095;  // Resolución del ADC (12 bits)

// Configuración del circuito divisor de voltaje
const float resistor1 = 10000.0;  // 10k ohms
const float resistor2 = 33000.0;  // 33k ohms

// Umbral de batería (20% de 12V)
const float batteryFullVoltage = 12.6;  // Voltaje total de la batería
const float batteryThreshold = 0.2;  // 20% de la carga total
const float batteryMinVoltage = 12.18;  // 20% del voltaje de la batería

// Variables de tiempo no bloqueantes
unsigned long timeNow = 0;
unsigned long timeLastCheck = 0;
unsigned long timeLastCounter = 0;

// Intervalos de tiempo
const unsigned long intervalCheck = 10000; // Comprobar voltaje cada 1 minuto (60,000 ms)
const unsigned long intervalCounter = 1000; // Incrementar contador cada 1 segundo (1000 ms)

// Variables de contador e indicador de conexión
int counter = 0;
bool wifiConnected = false;

void setup() {
  // Inicializar el monitor serial
  Serial.begin(115200);

  // Configurar el pin D15 como entrada analógica
  pinMode(analogPin, INPUT);
  
  // Desconectar cualquier conexión WiFi al inicio
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);  // Apagar el WiFi al inicio para asegurar estado limpio

  // Inicializar los tiempos
  timeLastCheck = millis();
  timeLastCounter = millis();
}

void loop() {
  // Obtener el tiempo actual
  timeNow = millis();

  // Comprobar el voltaje de la batería una vez por minuto
  if (timeNow - timeLastCheck >= intervalCheck) {
    timeLastCheck = timeNow;
    float batteryVoltage = leerBateria();

    Serial.print("Voltaje de la batería: ");
    Serial.print(batteryVoltage);
    Serial.println(" V");

    // Comprobar si el voltaje está por debajo del 20%
    if (batteryVoltage < batteryMinVoltage) {
      // Si la batería está por debajo del 20%, desconectar WiFi si está conectado
      if (wifiConnected) {
        Serial.println("Batería por debajo del 20%, desconectando WiFi...");
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);  // Apagar el WiFi para ahorrar energía
        wifiConnected = false;
      }
    } else {
      // Si la batería está por encima del 20%, conectar a WiFi si no está conectado
      if (!wifiConnected) {
        Serial.println("Batería por encima del 20%, conectando a WiFi...");
        WiFi.mode(WIFI_STA);  // Cambiar a modo estación
        WiFi.begin(ssid, password);

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
      }
    }
  }

  // Imprimir el contador cada segundo mientras esté conectado a WiFi
  if (wifiConnected && timeNow - timeLastCounter >= intervalCounter) {
    timeLastCounter = timeNow;
    counter++;
    Serial.print("Contador: ");
    Serial.println(counter);
  }
}

// Función para leer el voltaje de la batería a través del divisor de voltaje
float leerBateria() {
  // Temporalmente deshabilitar el WiFi para evitar problemas con el ADC
  WiFi.mode(WIFI_OFF);
  
  // Leer el valor analógico del pin D15
  int analogValue = analogRead(analogPin);

  // Convertir el valor leído en voltaje real (teniendo en cuenta el divisor de voltaje)
  float measuredVoltage = (analogValue * referenceVoltage) / adcMaxValue;

  // Calcular el voltaje real de la batería utilizando la fórmula del divisor de voltaje
  float batteryVoltage = measuredVoltage / (resistor1 / (resistor1 + resistor2));

  // Volver a activar el WiFi si estaba activado antes
  WiFi.mode(WIFI_STA);

  return batteryVoltage;
}
