// Define los pines para Trigger y Echo
const int trigPin = 13;
const int echoPin = 12;

// Variables para guardar el tiempo del eco y la distancia
long duracion;
int distancia;

void setup() {
  // Inicia la comunicación serie para ver los resultados en la PC
  Serial.begin(9600);
  
  // Configura los pines
  pinMode(trigPin, OUTPUT); // Trigger es un pin de SALIDA
  pinMode(echoPin, INPUT);  // Echo es un pin de ENTRADA
  
  Serial.println("Sensor de Ultrasonido HC-SR04 listo.");
}

void loop() {
  // --- 1. Generar el pulso de Trigger (el sonido) ---
  
  // Aseguramos que el Trigger esté apagado al inicio
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  // Enviamos el pulso ultrasónico (de 10 microsegundos)
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // --- 2. Medir el "Tiempo de Vuelo" (el eco) ---
  
  // La función pulseIn() mide el tiempo (en microsegundos) 
  // que el pin echoPin permanece en ALTO.
  duracion = pulseIn(echoPin, HIGH);

  // --- 3. Calcular la distancia ---
  
  // Fórmula: (tiempo * 0.0343) / 2
  // O, simplificado: tiempo / 58
  distancia = duracion / 58;

  // --- 4. Mostrar el resultado ---
  
  Serial.print("Distancia: ");
  Serial.print(distancia);
  Serial.println(" cm");

  // Pequeña pausa antes de la siguiente medición
  delay(500); 
}