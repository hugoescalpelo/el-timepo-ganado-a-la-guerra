#include <WiFi.h>

// Datos de la red WiFi
const char* ssid = "INFINITUMD2AC";       
const char* password = "PCwGdtcV9D";      

// Pin y configuración de lectura analógica
const int analogPin = 15;  // Pin analógico al que leeremos el voltaje (GPIO15)
const float referenceVoltage = 3.3;  // Voltaje de referencia del ESP32
const int adcMaxValue = 4095;  // Resolución del ADC (12 bits)

// Duración de las fases de lectura en milisegundos (10 segundos)
const unsigned long phaseDuration = 10000;

void setup() {
  // Inicializar el monitor serial
  Serial.begin(115200);
  
  // Configurar el pin D15 como entrada analógica
  pinMode(analogPin, INPUT);
  
  // Desconectar cualquier conexión WiFi al inicio para garantizar un estado limpio
  WiFi.disconnect(true); 
}

void loop() {
  // Fase 1: Lectura sin conexión WiFi
  Serial.println("Iniciando lectura sin conexión a WiFi");
  for (unsigned long startTime = millis(); millis() - startTime < phaseDuration;) {
    leerVoltaje();
    delay(1000);
  }

  // Fase 2: Conectarse a WiFi
  Serial.println("Conectando a WiFi...");
  WiFi.mode(WIFI_STA); // Asegurar que estamos en modo estación
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

  // Fase 3: Lectura con conexión a WiFi
  Serial.println("Iniciando lectura con conexión a WiFi");
  for (unsigned long startTime = millis(); millis() - startTime < phaseDuration;) {
    // Temporalmente deshabilitar el WiFi para evitar problemas con el ADC
    WiFi.mode(WIFI_OFF);
    leerVoltaje();
    delay(100);  // Pequeña pausa para garantizar la estabilidad del ADC
    WiFi.mode(WIFI_STA); // Volver a activar el WiFi
    delay(900); // Resto del segundo
  }

  // Desconectarse de WiFi y limpiar recursos
  WiFi.disconnect(true);
  Serial.println("Desconectado de WiFi, repitiendo ciclo...");
}

// Función para leer y mostrar el voltaje en el pin analógico
void leerVoltaje() {
  // Leer el valor analógico del pin D15
  int analogValue = analogRead(analogPin);

  // Convertir el valor leído en voltaje
  float voltage = (analogValue * referenceVoltage) / adcMaxValue;

  // Imprimir el voltaje en el monitor serial
  Serial.print("Voltaje en el pin D15: ");
  Serial.print(voltage);
  Serial.println(" V");
}

