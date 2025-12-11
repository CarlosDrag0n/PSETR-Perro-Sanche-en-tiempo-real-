#include <Thread.h>
#include <ThreadController.h>

// ==========================================
// 1. CONFIGURACIÓN DE PINES
// ==========================================

// --- Sensores Infrarrojos ---
#define PIN_ITR_LEFT   A2
#define PIN_ITR_MIDDLE A1
#define PIN_ITR_RIGHT  A0

// --- Sensor Ultrasonido ---
const int trigPin = 13;
const int echoPin = 12;

// --- Motores (Driver) ---
#define PIN_Motor_STBY  3
#define PIN_Motor_AIN_1 7
#define PIN_Motor_PWMA  5
#define PIN_Motor_BIN_1 8
#define PIN_Motor_PWMB  6 

// ==========================================
// 2. VARIABLES Y AJUSTES
// ==========================================

// --- PID LÍNEA ---
float kp_linea = 0.15; 
float kd_linea = 2.0;
int error_linea, prev_error_linea, derivate_linea;
int velocidadBase = 90; // Velocidad crucero

// Variables lectura IR
int valLeft, valMiddle, valRight;

// --- CONTROL DE FRENADO ---
long duracion;
int distancia = 999;
const int distanciaObjetivo = 7;       // AL LLEGAR AQUÍ SE ACABA TODO
const int distanciaInicioFrenado = 25; // Empieza a frenar un poco antes
const float kp_freno = 8.0;            // Ganancia (Si choca, súbelo a 10 o 12)
const int velocidadMinimaFreno = 0;    // Dejamos que el PWM baje hasta 0 si es necesario

// --- ESTADOS ---
enum movimiento {
  SEGUIR_LINEA,           
  PARANDO_OBSTACULO,
  FINALIZADO 
};
movimiento estadoActual = SEGUIR_LINEA; 

// --- HILOS ---
ThreadController controlador = ThreadController();
Thread hilo_infra_rojos = Thread();
Thread hilo_ultra_sonido = Thread();
Thread hilo_motor = Thread();

// ==========================================
// 3. FUNCIONES DE MOVIMIENTO
// ==========================================

void mover(int velocidadIzq, int velocidadDer) {
  // Limitamos rango PWM
  velocidadIzq = constrain(velocidadIzq, -255, 255);
  velocidadDer = constrain(velocidadDer, -255, 255);

  // Motor Izquierdo
  if (velocidadIzq >= 0) {
    digitalWrite(PIN_Motor_BIN_1, HIGH); 
    analogWrite(PIN_Motor_PWMB, velocidadIzq);
  } else {
    digitalWrite(PIN_Motor_BIN_1, LOW); 
    analogWrite(PIN_Motor_PWMB, abs(velocidadIzq)); 
  }

  // Motor Derecho
  if (velocidadDer >= 0) {
    digitalWrite(PIN_Motor_AIN_1, HIGH); 
    analogWrite(PIN_Motor_PWMA, velocidadDer);
  } else {
    digitalWrite(PIN_Motor_AIN_1, LOW); 
    analogWrite(PIN_Motor_PWMA, abs(velocidadDer));
  }
}

// ==========================================
// 4. CALLBACKS (LÓGICA)
// ==========================================

// --- Lectura IR ---
void callback_infra_rojos() {
  valLeft = analogRead(PIN_ITR_LEFT);
  valMiddle = analogRead(PIN_ITR_MIDDLE);
  valRight = analogRead(PIN_ITR_RIGHT);
}

// --- Lectura Ultrasonido ---
void callback_ultra_sonido() {
  if (estadoActual == FINALIZADO) return; // Si acabó, no leemos más

  digitalWrite(trigPin, LOW); delayMicroseconds(2);
  digitalWrite(trigPin, HIGH); delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  duracion = pulseIn(echoPin, HIGH, 30000); // Timeout corto

  if (duracion == 0) distancia = 999;
  else distancia = duracion / 58;

  // Solo cambiamos estado si NO hemos terminado
  if (estadoActual != FINALIZADO) {
    if (distancia <= distanciaInicioFrenado) {
      estadoActual = PARANDO_OBSTACULO;
    } else {
      estadoActual = SEGUIR_LINEA;
    }
  }
}

// --- Control Motores ---
void callback_motor() {
  
  // 1. SI YA TERMINAMOS -> STOP TOTAL
  if (estadoActual == FINALIZADO) {
    mover(0, 0);
    return;
  }

  // 2. LÓGICA DE FRENADO (OBSTÁCULO)
  if (estadoActual == PARANDO_OBSTACULO) {
    
    // Si estamos a la distancia objetivo O MENOS (nos pasamos de frenada)
    // CORTAMOS INMEDIATAMENTE. No corregimos hacia atrás.
    if (distancia <= distanciaObjetivo) {
      mover(0, 0);
      estadoActual = FINALIZADO; // Bloqueo final
      Serial.println("--- META ALCANZADA: FIN ---");
      return; 
    }

    // Cálculo proporcional SOLO HACIA ADELANTE
    int error = distancia - distanciaObjetivo;
    int velocidadFreno = (error * kp_freno);

    // Aseguramos una velocidad mínima para vencer la inercia del motor
    // pero solo si estamos lejos.
    if (velocidadFreno < 45) velocidadFreno = 45; 
    
    // Si por error de cálculo sale negativa, la forzamos a 0 (nunca atrás)
    if (velocidadFreno < 0) velocidadFreno = 0;

    // Avanzar recto frenando
    mover(velocidadFreno, velocidadFreno);
    
    // Debug
    Serial.print("Dist: "); Serial.print(distancia); 
    Serial.print(" | Vel: "); Serial.println(velocidadFreno);
  } 
  
  // 3. SEGUIMIENTO DE LÍNEA (PID)
  else if (estadoActual == SEGUIR_LINEA) {
    
    error_linea = valRight - valLeft;
    derivate_linea = error_linea - prev_error_linea;
    prev_error_linea = error_linea;

    int ajuste = (kp_linea * error_linea) + (kd_linea * derivate_linea);

    int speed_left = velocidadBase + ajuste;
    int speed_right = velocidadBase - ajuste;

    // Si se sale de la línea (todo blanco), paramos por seguridad 
    // o puedes poner aquí lógica de recuperación
    if (valLeft < 300 && valMiddle < 300 && valRight < 300) {
       // mover(0, 0); // Descomenta si quieres que pare al perder línea
       // Por defecto sigue con la última instrucción o recto
    } else {
       mover(speed_left, speed_right);
    }
  }
}

// ==========================================
// 5. SETUP
// ==========================================
void setup() {
  Serial.begin(9600);

  // Pines
  pinMode(PIN_Motor_STBY, OUTPUT);
  pinMode(PIN_Motor_AIN_1, OUTPUT);
  pinMode(PIN_Motor_PWMA, OUTPUT);
  pinMode(PIN_Motor_BIN_1, OUTPUT);
  pinMode(PIN_Motor_PWMB, OUTPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  digitalWrite(PIN_Motor_STBY, HIGH);

  // Hilos
  hilo_infra_rojos.onRun(callback_infra_rojos);
  hilo_infra_rojos.setInterval(10);

  hilo_ultra_sonido.onRun(callback_ultra_sonido);
  hilo_ultra_sonido.setInterval(50); // Lectura cada 50ms

  hilo_motor.onRun(callback_motor);
  hilo_motor.setInterval(20); 

  controlador.add(&hilo_infra_rojos);
  controlador.add(&hilo_ultra_sonido);
  controlador.add(&hilo_motor);
}

void loop() {
  controlador.run();
}