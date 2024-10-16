#include <PicoMQTT.h>
#include <WiFi.h>

// Pines de conexión para el LD2420
#define RX_PIN 23   // Conectar al TX del LD2420
#define TX_PIN 22   // Conectar al RX del LD2420

// Definir el WiFi y el broker MQTT
#define WIFI_SSID "INFINITUMD2AC"
#define WIFI_PASSWORD "PCwGdtcV9D"
#define MQTT_TOPIC "tgg/carta"

// Instanciar el servidor MQTT
PicoMQTT::Server mqtt;

// Inicializamos una instancia de HardwareSerial
HardwareSerial ld2420(1);  // Utilizamos UART1

// Variables para la distancia y el control de tiempo
unsigned long lastSendTime = 0;
unsigned long sendIntervalShort = 5000;  // Intervalo para enviar '1' cada 5 segundos
unsigned long sendIntervalLong = 5000;  // Intervalo para enviar '0' cada 1 minuto
int distanceThreshold = 50;  // Umbral de detección de distancia
bool personDetected = false;
String sensorData = "";  // Para almacenar los datos recibidos

void setup() {
    // Inicializar el monitor serial
    Serial.begin(115200);

    // Inicializar el puerto serial para el LD2420
    ld2420.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);

    // Conectar a WiFi
    Serial.printf("Connecting to WiFi %s\n", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
    }
    Serial.printf("WiFi connected, IP: %s\n", WiFi.localIP().toString().c_str());

    // Inicializar el servidor MQTT
    mqtt.begin();
}

void loop() {
    mqtt.loop();

    // Comprobar si hay datos disponibles en el LD2420
    if (ld2420.available()) {
        char c = ld2420.read();
        sensorData += c;

        // Si encontramos una nueva línea, procesamos los datos recibidos
        if (c == '\n') {
            if (sensorData.indexOf("Range") != -1) {
                int rangeValue = extractRangeValue(sensorData);  // Extraer el valor de rango
                Serial.printf("Distancia detectada: %d cm\n", rangeValue);

                // Actualizar la detección de persona
                if (rangeValue < distanceThreshold) {
                    personDetected = true;
                } else {
                    personDetected = false;
                }
            }
            sensorData = "";  // Reiniciar la variable para la siguiente lectura
        }
    }

    // Controlar el envío de mensajes MQTT basados en la detección de distancia
    unsigned long currentTime = millis();

    if (personDetected && currentTime - lastSendTime > sendIntervalShort) {
        mqtt.publish(MQTT_TOPIC, "1");
        Serial.println("Enviando 1 al tema tgg/carta");
        lastSendTime = currentTime;
    } else if (!personDetected && currentTime - lastSendTime > sendIntervalLong) {
        mqtt.publish(MQTT_TOPIC, "0");
        Serial.println("Enviando 0 al tema tgg/carta");
        lastSendTime = currentTime;
    }

    delay(100);  // Pequeño retardo para no saturar el procesador
}

// Función para extraer el valor de rango de los datos recibidos
int extractRangeValue(String data) {
    int rangeIndex = data.indexOf("Range");
    if (rangeIndex != -1) {
        String rangeValueStr = data.substring(rangeIndex + 6);  // Extraer el valor después de "Range "
        rangeValueStr.trim();  // Eliminar espacios en blanco
        return rangeValueStr.toInt();  // Convertir a entero
    }
    return -1;  // Valor inválido si no se encuentra "Range"
}
