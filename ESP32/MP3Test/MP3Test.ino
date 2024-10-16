
#include <SoftwareSerial.h>

// Definir los pines para la comunicación con el YX5300
#define RX_PIN 22 // RX del ESP32 conectado a TX del YX5300
#define TX_PIN 23 // TX del ESP32 conectado a RX del YX5300
#define RELAY_PIN 21 // Pin conectado al relé que controla el amplificador

SoftwareSerial mp3(RX_PIN, TX_PIN);

// Variables de tiempo
unsigned long previousMillis = 0;
const long intervalRelayOn = 1000;   // 1 segundo para activar el relé
const long intervalPlayTime = 480000; // 8 minutos en milisegundos
const long intervalRelayOff = 1000;  // 1 segundo para apagar el relé

// Estados del ciclo
enum State { WAITING, RELAY_ON, PLAYING, RELAY_OFF };
State currentState = WAITING;

void sendCommand(uint8_t command, uint8_t arg1 = 0, uint8_t arg2 = 0) {
  uint8_t commandBuffer[8] = {0x7E, 0xFF, 0x06, command, 0x00, arg1, arg2, 0xEF};
  mp3.write(commandBuffer, 8);
}

void setup() {
  mp3.begin(9600);

  // Configurar el pin del relé como salida
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW); // Inicialmente apagado

  // Configurar el volumen al 50%
  sendCommand(0x06, 0x00, 0x3F); // 0x3F es el máximo volumen

  // Seleccionar la tarjeta SD como fuente de audio
  sendCommand(0x09, 0x00, 0x02); 
}

void loop() {
  unsigned long currentMillis = millis();

  switch (currentState) {
    case WAITING:
      // Esperar 1 segundo antes de activar el relé
      if (currentMillis - previousMillis >= intervalRelayOn) {
        previousMillis = currentMillis;
        digitalWrite(RELAY_PIN, HIGH); // Activar el relé
        currentState = RELAY_ON;
      }
      break;

    case RELAY_ON:
      // Activar el relé y empezar a reproducir el MP3
      sendCommand(0x03, 0x00, 0x01);  // Reproducir el archivo "wave.mp3" desde el principio
      previousMillis = currentMillis; // Reiniciar el temporizador
      currentState = PLAYING;
      break;

    case PLAYING:
      // Reproducir durante 8 minutos (480,000 ms)
      if (currentMillis - previousMillis >= intervalPlayTime) {
        sendCommand(0x16); // Detener la reproducción del MP3
        previousMillis = currentMillis; // Reiniciar el temporizador
        currentState = RELAY_OFF;
      }
      break;

    case RELAY_OFF:
      // Apagar el relé 1 segundo después de detener la reproducción
      if (currentMillis - previousMillis >= intervalRelayOff) {
        digitalWrite(RELAY_PIN, LOW); // Apagar el relé
        previousMillis = currentMillis; // Reiniciar el temporizador
        currentState = WAITING; // Reiniciar el ciclo
      }
      break;
  }
}
