// --- Definición de Pines ---
#define PIN_Motor_STBY  3
// Grupo A (Derecha)
#define PIN_Motor_AIN_1 7
#define PIN_Motor_PWMA  5
// Grupo B (Izquierda)
#define PIN_Motor_BIN_1 8
#define PIN_Motor_PWMB  6 

void setup() {
  // Configuración de pines como Salida
  pinMode(PIN_Motor_STBY, OUTPUT);
  pinMode(PIN_Motor_AIN_1, OUTPUT);
  pinMode(PIN_Motor_PWMA, OUTPUT);
  pinMode(PIN_Motor_BIN_1, OUTPUT);
  pinMode(PIN_Motor_PWMB, OUTPUT);

  // Habilitar la controladora de motores
  digitalWrite(PIN_Motor_STBY, HIGH);
}

void loop() {
  // 1. ACELERAR HACIA DELANTE (5 Segundos)
  // Ambos motores velocidad positiva
  for (int i = 0; i <= 255; i++) {
    mover(i, i); 
    delay(20); // Pequeña pausa para crear el efecto de rampa (20ms * 255 pasos ≈ 5 seg)
  }
  pararYEsperar();

  // 2. ACELERAR HACIA ATRÁS (5 Segundos)
  // Ambos motores velocidad negativa
  for (int i = 0; i <= 255; i++) {
    mover(-i, -i); 
    delay(20);
  }
  pararYEsperar();

  // 3. GIRO PROGRESIVO IZQUIERDA (Sobre su propio eje)
  // Izquierda Retrocede (-), Derecha Avanza (+)
  for (int i = 0; i <= 255; i++) {
    mover(-i, i); 
    delay(20);
  }
  pararYEsperar();

  // 4. GIRO PROGRESIVO DERECHA (Sobre su propio eje)
  // Izquierda Avanza (+), Derecha Retrocede (-)
  for (int i = 0; i <= 255; i++) {
    mover(i, -i); 
    delay(20);
  }
  
  // Pausa larga antes de repetir todo el ciclo
  pararYEsperar();
  delay(2000); 
}

// --- Funciones Auxiliares ---

void pararYEsperar() {
  mover(0, 0);
  delay(1000); // Esperar 1 segundo para proteger los engranajes antes de cambiar dirección
}

// Función universal para mover motores
// Valores positivos = Adelante / Valores negativos = Atrás
void mover(int velocidadIzq, int velocidadDer) {
  
  // --- Motor IZQUIERDO (Grupo B) ---
  if (velocidadIzq >= 0) {
    digitalWrite(PIN_Motor_BIN_1, HIGH); // Adelante
    analogWrite(PIN_Motor_PWMB, velocidadIzq);
  } else {
    digitalWrite(PIN_Motor_BIN_1, LOW);  // Atrás
    analogWrite(PIN_Motor_PWMB, abs(velocidadIzq)); // Usamos valor absoluto para el PWM
  }

  // --- Motor DERECHO (Grupo A) ---
  if (velocidadDer >= 0) {
    digitalWrite(PIN_Motor_AIN_1, HIGH); // Adelante
    analogWrite(PIN_Motor_PWMA, velocidadDer);
  } else {
    digitalWrite(PIN_Motor_AIN_1, LOW);  // Atrás
    analogWrite(PIN_Motor_PWMA, abs(velocidadDer));
  }
}