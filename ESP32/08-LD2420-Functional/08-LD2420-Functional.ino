// Pines de conexión para el LD2420
#define RX_PIN 23   // Conectar al TX del LD2420
#define TX_PIN 22  // Conectar al RX del LD2420

// Inicializamos una instancia de HardwareSerial
HardwareSerial ld2420(1);  // Utilizamos UART1

void setup() {
  // Inicializar el puerto serial para el LD2420
  ld2420.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);

  // Inicializar el monitor serial para depuración
  Serial.begin(115200);
  Serial.println("Iniciando comunicación con el sensor LD2420...");
}

void loop() {
  // Comprobamos si hay datos disponibles en el LD2420
  if (ld2420.available()) {
    // Leemos los datos y los mostramos en el monitor serial
    while (ld2420.available()) {
      char c = ld2420.read();
      Serial.print(c);  // Imprime el carácter recibido
      
    }
    Serial.println();  // Nueva línea después de cada paquete de datos
    Serial.println("H");
  }

  // Puedes agregar lógica adicional aquí para procesar los datos recibidos
  // del LD2420 según tus necesidades específicas.

  delay(100);  // Pequeño retardo para no saturar el procesador
}

