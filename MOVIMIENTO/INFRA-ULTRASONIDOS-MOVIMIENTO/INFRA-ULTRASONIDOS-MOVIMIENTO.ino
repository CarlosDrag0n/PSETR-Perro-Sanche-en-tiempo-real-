#include <Thread.h>
#include <ThreadController.h>
#include "FastLED.h"

// ==========================================
// 1. CONFIGURACIÓN DE PINES
// ==========================================

#define PIN_ITR_LEFT   A2
#define PIN_ITR_MIDDLE A1
#define PIN_ITR_RIGHT  A0

const int trigPin = 13;
const int echoPin = 12;

#define PIN_Motor_STBY  3
#define PIN_Motor_AIN_1 7
#define PIN_Motor_PWMA  5
#define PIN_Motor_BIN_1 8
#define PIN_Motor_PWMB  6 

#define PIN_RBGLED 4
#define NUM_LEDS 1
CRGB leds[NUM_LEDS];

// ==========================================
// 2. AJUSTES PARA VELOCIDAD Y RECUPERACIÓN AGRESIVA
// ==========================================

// --- PID DE ALTA VELOCIDAD ---
// Kp: 0.4 es agresivo, está bien para curvas rápidas.
float kp_linea = 0.4; 
// Kd: Subimos a 3.0 para frenar el tembleque que genera un Kp de 0.4
float kd_linea = 2.5;  

int error_linea, prev_error_linea, derivate_linea;

// Velocidad de crucero
int velocidadBase = 120; 

// --- BÚSQUEDA / RECUPERACIÓN (TURBO) ---
// Para recuperar rápido, necesitamos mucha diferencia entre ruedas.
int velocidadGiroRapida = 200; // Muy rápido para cerrar la curva ya
int velocidadGiroLenta = -20;  // Pequeño freno activo para clavar la rueda interior y pivotar
int umbralNegro = 400; 

enum Lado { IZQUIERDA, DERECHA };
Lado ultimoLadoVisto = DERECHA; 

int valLeft, valMiddle, valRight;

// --- FRENADO ---
long duracion;
int distancia = 999;
const int distanciaObjetivo = 7;       
const int distanciaInicioFrenado = 35; 
const float kp_freno = 10.0;           
const int velocidadMinimaFreno = 0;    

// Led
bool change_led = true;
// --- ESTADOS ---
enum movimiento
{
  SEGUIR_LINEA,
  BUSCANDO_LINEA,
  PARANDO_OBSTACULO,
  FINALIZADO 
};
movimiento estadoActual = SEGUIR_LINEA; 

ThreadController controlador = ThreadController();
Thread hilo_infra_rojos = Thread();
Thread hilo_ultra_sonido = Thread();
Thread hilo_motor = Thread();

// ==========================================
// 3. FUNCIONES
// ==========================================

void go_state(int new_state)
{
  estadoActual = new_state;
  change_led = true;
}

uint32_t Color(uint8_t r, uint8_t g, uint8_t b)
{
  return (((uint32_t)r << 16) | ((uint32_t)g << 8) | b);
}

void led_color(int r, int g, int b)
{
  if (change_led) {
    FastLED.showColor(Color(r, g, b));
    change_led = false;
  }
}
void mover(int velocidadIzq, int velocidadDer)
{
  velocidadIzq = constrain(velocidadIzq, -255, 255);
  velocidadDer = constrain(velocidadDer, -255, 255);

  if (velocidadIzq >= 0) {
    digitalWrite(PIN_Motor_BIN_1, HIGH); analogWrite(PIN_Motor_PWMB, velocidadIzq);
  } else {
    digitalWrite(PIN_Motor_BIN_1, LOW); analogWrite(PIN_Motor_PWMB, abs(velocidadIzq)); 
  }

  if (velocidadDer >= 0) {
    digitalWrite(PIN_Motor_AIN_1, HIGH); analogWrite(PIN_Motor_PWMA, velocidadDer);
  } else {
    digitalWrite(PIN_Motor_AIN_1, LOW); analogWrite(PIN_Motor_PWMA, abs(velocidadDer));
  }
}

// ==========================================
// 4. CALLBACKS
// ==========================================

void callback_infra_rojos()
{
  valLeft = analogRead(PIN_ITR_LEFT);
  valMiddle = analogRead(PIN_ITR_MIDDLE);
  valRight = analogRead(PIN_ITR_RIGHT);

  // Actualizamos la memoria
  if (valLeft > umbralNegro) {
    ultimoLadoVisto = IZQUIERDA;
  } 
  else if (valRight > umbralNegro) {
    ultimoLadoVisto = DERECHA;
  }
}

void callback_ultra_sonido()
{
  if (estadoActual == FINALIZADO) return; 

  digitalWrite(trigPin, LOW); delayMicroseconds(2);
  digitalWrite(trigPin, HIGH); delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duracion = pulseIn(echoPin, HIGH, 30000); 

  if (duracion == 0) distancia = 999;
  else distancia = duracion / 58;

  if (estadoActual != FINALIZADO) {
    if (distancia <= distanciaInicioFrenado) go_state(PARANDO_OBSTACULO);
    else if (estadoActual == PARANDO_OBSTACULO) go_state(SEGUIR_LINEA); 
  }
}

void callback_motor()
{
  if (estadoActual == FINALIZADO) {
    led_color(0, 0, 255);
    mover(0, 0);
    return;
  }

  // --- OBSTÁCULO ---
  if (estadoActual == PARANDO_OBSTACULO) {
    if (distancia <= distanciaObjetivo) {
      mover(0, 0);
      go_state(FINALIZADO);
      return; 
    }
    int error = distancia - distanciaObjetivo;
    int velocidadFreno = (error * kp_freno);
    if (velocidadFreno < 45) velocidadFreno = 45; 
    if (velocidadFreno < 0) velocidadFreno = 0;
    mover(velocidadFreno, velocidadFreno);
  } 
  
  // --- SEGUIR LÍNEA / RECUPERACIÓN ---
  else {
    bool lineaPerdida = (valLeft < umbralNegro && valMiddle < umbralNegro && valRight < umbralNegro);

    if (lineaPerdida) go_state(BUSCANDO_LINEA);
    else go_state(SEGUIR_LINEA);

    // --- MODO RECUPERACIÓN (LATIGAZO) ---
    if (estadoActual == BUSCANDO_LINEA) {

      //Enciendo el led de rojo
      led_color(255, 0, 0);
      if (ultimoLadoVisto == IZQUIERDA) {
        // La línea se fue a la izquierda -> Giro brusco a la izquierda
        // Rueda IZQ frena (-20), Rueda DER a tope (200)
        mover(velocidadGiroLenta, velocidadGiroRapida);
      } else {
        // La línea se fue a la derecha -> Giro brusco a la derecha
        // Rueda IZQ a tope (200), Rueda DER frena (-20)
        mover(velocidadGiroRapida, velocidadGiroLenta);
      }
    }
    
    // --- MODO PID (RECTA/CURVA SUAVE) ---
    else if (estadoActual == SEGUIR_LINEA) {

      //Eciendo el led de verde
      led_color(0, 255, 0);
      error_linea = valRight - valLeft;
      
      derivate_linea = error_linea - prev_error_linea;
      prev_error_linea = error_linea;

      int ajuste = (kp_linea * error_linea) + (kd_linea * derivate_linea);

      // Zona muerta para estabilidad en recta
      if (abs(ajuste) < 10) ajuste = 0;
      
      ajuste = constrain(ajuste, -velocidadBase, velocidadBase);

      int speed_left = velocidadBase + ajuste;
      int speed_right = velocidadBase - ajuste;

      mover(speed_left, speed_right);
    }
  }
}

void setup()
{
  Serial.begin(9600);
  pinMode(PIN_Motor_STBY, OUTPUT);
  pinMode(PIN_Motor_AIN_1, OUTPUT);
  pinMode(PIN_Motor_PWMA, OUTPUT);
  pinMode(PIN_Motor_BIN_1, OUTPUT);
  pinMode(PIN_Motor_PWMB, OUTPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  digitalWrite(PIN_Motor_STBY, HIGH);

  // Tiempos ultra-rápidos
  hilo_infra_rojos.onRun(callback_infra_rojos);
  hilo_infra_rojos.setInterval(5); 

  hilo_ultra_sonido.onRun(callback_ultra_sonido);
  hilo_ultra_sonido.setInterval(45); 

  hilo_motor.onRun(callback_motor);
  hilo_motor.setInterval(10); 

  controlador.add(&hilo_infra_rojos);
  controlador.add(&hilo_ultra_sonido);
  controlador.add(&hilo_motor);

  FastLED.addLeds<NEOPIXEL, PIN_RBGLED>(leds, NUM_LEDS);
  FastLED.setBrightness(20);
}

void loop()
{
  controlador.run();
}