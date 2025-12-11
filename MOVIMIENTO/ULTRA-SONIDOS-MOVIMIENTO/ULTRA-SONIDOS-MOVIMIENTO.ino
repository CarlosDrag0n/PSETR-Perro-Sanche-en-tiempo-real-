#include <Thread.h>
#include <ThreadController.h>

const int trigPin = 13;
const int echoPin = 12;

#define PIN_Motor_STBY  3
#define PIN_Motor_AIN_1 7
#define PIN_Motor_PWMA  5
#define PIN_Motor_BIN_1 8
#define PIN_Motor_PWMB  6 

long duracion;
int distancia = 999; 

// --- VARIABLES DEL REGULADOR PROPORCIONAL ---
const int distanciaObjetivo = 12;       // Meta: Parar a 7 cm
const int distanciaInicioFrenado = 20; // Empieza a frenar a los 20 cm
const float Kp = 12.0;                 // Ganancia Proporcional
const int velocidadMinima = 55;        

enum movimiento {
  LINEA_ENCONTRADA,           
  BUSCANDO_LINEA,
  PARANDO,
  FINALIZADO 
};
movimiento estadoActual = LINEA_ENCONTRADA; 

ThreadController controlador = ThreadController();
Thread hilo_ultra_sonido = Thread();
Thread hilo_motor = Thread();

void mover(int velocidadIzq, int velocidadDer) {
  velocidadIzq = constrain(velocidadIzq, -255, 255);
  velocidadDer = constrain(velocidadDer, -255, 255);

  if (velocidadIzq >= 0) {
    digitalWrite(PIN_Motor_BIN_1, HIGH);
    analogWrite(PIN_Motor_PWMB, velocidadIzq);
  } else {
    digitalWrite(PIN_Motor_BIN_1, LOW);
    analogWrite(PIN_Motor_PWMB, abs(velocidadIzq));
  }

  if (velocidadDer >= 0) {
    digitalWrite(PIN_Motor_AIN_1, HIGH);
    analogWrite(PIN_Motor_PWMA, velocidadDer);
  } else {
    digitalWrite(PIN_Motor_AIN_1, LOW);
    analogWrite(PIN_Motor_PWMA, abs(velocidadDer));
  }
}

void callback_ultra_sonido() {
  if (estadoActual == FINALIZADO){
    return;
  }

  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  duracion = pulseIn(echoPin, HIGH);

  if (duracion == 0) {
    distancia = 999; 
  } else {
    distancia = duracion / 58;
  }

  // Lógica de transición
  if (distancia <= distanciaInicioFrenado){
    estadoActual = PARANDO;
  } else {
    estadoActual = estadoActual; 
  }
}

void callback_motor() {
  // 1. BLOQUEO DE SEGURIDAD
  if (estadoActual == FINALIZADO) {
    mover(0, 0); // Aseguramos que los motores sigan parados
    return;      // Salimos de la función
  }

  int velocidadCalculada = 0;

  if (estadoActual == PARANDO) {
    int error = distancia - distanciaObjetivo; 
    Serial.print(distancia);
    Serial.println(" cm");

    // --- PUNTO DE NO RETORNO ---
    if (error <= 0) {
      // Hemos llegado a la meta (7cm o menos)
      estadoActual = FINALIZADO; // Cambiamos al estado de bloqueo
      mover(0, 0);               // Frenazo final
      return;                    // Salimos
    } 
    
    // Si aún no hemos llegado, aplicamos el regulador P
    velocidadCalculada = (error * Kp) + velocidadMinima;
    mover(velocidadCalculada, velocidadCalculada);

  } else if (estadoActual == LINEA_ENCONTRADA) {
    mover(150, 150); 
  } else if (estadoActual == BUSCANDO_LINEA) {
    mover(100, 100);
  }
}

void setup() {
  Serial.begin(9600);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  pinMode(PIN_Motor_STBY, OUTPUT);
  pinMode(PIN_Motor_AIN_1, OUTPUT);
  pinMode(PIN_Motor_PWMA, OUTPUT);
  pinMode(PIN_Motor_BIN_1, OUTPUT);
  pinMode(PIN_Motor_PWMB, OUTPUT);

  digitalWrite(PIN_Motor_STBY, HIGH);

  hilo_ultra_sonido.onRun(callback_ultra_sonido);
  hilo_ultra_sonido.setInterval(60); 

  hilo_motor.onRun(callback_motor);
  hilo_motor.setInterval(30); 

  controlador.add(&hilo_ultra_sonido);
  controlador.add(&hilo_motor);
}

void loop() {
  controlador.run();
  if (estadoActual == FINALIZADO){
    return;
  }
}