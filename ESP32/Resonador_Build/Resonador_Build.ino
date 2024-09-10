#include <WiFi.h>

// Datos de la red WiFi
const char* ssid = "INFINITUMD2AC";       
const char* password = "PCwGdtcV9D";      

// Pin y configuración de lectura analógica
const int analogPin = 15;  // Pin analógico al que leeremos el voltaje (GPIO15)
const float referenceVoltage = 5.0;  // Voltaje de referencia del ESP32
const int adcMaxValue = 4095;  // Resolución del ADC (12 bits)

// Configuración del circuito divisor de voltaje
const float resistor1 = 10000.0;  // 10k ohms
const float resistor2 = 33000.0;  // 33k ohms

// Umbral de batería (20% de 12V)
const float batteryFullVoltage = 12.0;  // Voltaje total de la batería
const float batteryThreshold = 0.20;  // 20% de la carga total
const float batteryMinVoltage = batteryFullVoltage * batteryThreshold;  // 20% del voltaje de la batería

void setup() {
  // Inicializar el monitor serial
  Serial.begin(115200);

  // Configurar el pin D15 como entrada analógica
  pinMode(analogPin, INPUT);
  
  // Desconectar cualquier conexión WiFi al inicio
  WiFi.disconnect(true); 
}

void loop() {
  // Leer el voltaje actual de la batería
  float batteryVoltage = leerBateria();

  // Mostrar el voltaje de la batería
  Serial.print("Voltaje de la batería: ");
  Serial.print(batteryVoltage);
  Serial.println(" V");

  // Comprobar si el voltaje está por debajo del 20%
  if (batteryVoltage < batteryMinVoltage) {
    // Si la batería está por debajo del 20%, desconectar WiFi si está conectado
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Batería por debajo del 20%, desconectando WiFi...");
      WiFi.disconnect(true);
    }
  } else {
    // Si la batería está por encima del 20%, conectar a WiFi si no está conectado
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("Batería por encima del 20%, conectando a WiFi...");
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
    }
  }

  // Esperar 1 segundo antes de la siguiente lectura
  delay(1000);
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
