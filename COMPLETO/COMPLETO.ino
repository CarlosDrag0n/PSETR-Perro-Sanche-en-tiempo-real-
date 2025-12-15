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
// 2. VARIABLES GLOBALES
// ==========================================

unsigned long start_time = 0; // Para medir el tiempo de la vuelta

// PID
float kp_linea = 0.4; 
float kd_linea = 2.5;  
int error_linea, prev_error_linea, derivate_linea;
int velocidadBase = 110; 

// Recuperación
int velocidadGiroRapida = 180; 
int velocidadGiroLenta = -20;  
int umbralNegro = 400; 

enum Lado { IZQUIERDA, DERECHA };
Lado ultimoLadoVisto = DERECHA; 

int valLeft, valMiddle, valRight;

// Ultrasonidos
long duracion;
int distancia = 999;
const int distanciaObjetivo = 7;       
const int distanciaInicioFrenado = 25; 
const float kp_freno = 10.0;           

// Led
bool change_led = true;

// Estados
enum movimiento
{
  SEGUIR_LINEA,
  BUSCANDO_LINEA,
  PARANDO_OBSTACULO,
  FINALIZADO 
};
movimiento estadoActual = SEGUIR_LINEA; 

// Threads
ThreadController controlador = ThreadController();
Thread hilo_infra_rojos = Thread();
Thread hilo_ultra_sonido = Thread();
Thread hilo_motor = Thread();

// Flags de comunicación
bool send_start = true;
bool send_line_lost = true;
bool send_obstacle = true;
bool send_finish = true;

int error;
int velocidadFreno;
bool lineaPerdida ;
unsigned long total_time;
int ajuste;
int speed_left;
int speed_right;


// ==========================================
// 3. FUNCIONES AUXILIARES
// ==========================================

void go_state(int new_state)
{
  // Si cambiamos DE Buscando A Seguir, significa que encontramos la línea
  if (estadoActual == BUSCANDO_LINEA && new_state == SEGUIR_LINEA) {
    // Enviar LINE_FOUND ('w') y STOP_LINE_SEARCH ('e')
    Serial.print("w"); // Line found
    Serial.print("}");
    Serial.print("e"); // Stop search
    Serial.print("}");
  }

  estadoActual = new_state;
  change_led = true;
  
  // Reiniciar flags de envío único por estado si es necesario
  if(new_state == BUSCANDO_LINEA) send_line_lost = true;
  if(new_state == PARANDO_OBSTACULO) send_obstacle = true;
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
    else if (estadoActual == PARANDO_OBSTACULO && distancia > distanciaInicioFrenado) go_state(SEGUIR_LINEA); 
  }
}

void callback_motor()
{
  lineaPerdida = (valLeft < umbralNegro && valMiddle < umbralNegro && valRight < umbralNegro);
  // --- FIN DE VUELTA ---
  switch(estadoActual){
    case FINALIZADO:
      if (send_finish == true) {
        total_time = millis() - start_time;
        Serial.print("f"); // END_LAP
        Serial.print(total_time); // Tiempo real calculado
        Serial.print("}");
        send_finish = false;
      }
      led_color(0, 0, 255);
      mover(0, 0);
      break;

    case PARANDO_OBSTACULO:
      if (distancia <= distanciaObjetivo) {
        mover(0, 0);
        if (send_obstacle == true) {
          Serial.print("o"); // OBSTACLE_DETECTED
          Serial.print(distancia);
          Serial.print("}");
          send_obstacle = false;
        }
        go_state(FINALIZADO);
        return; 
      }
      error = distancia - distanciaObjetivo;
      velocidadFreno = (error * kp_freno);
      if (velocidadFreno < 45) velocidadFreno = 45; 
      if (velocidadFreno < 0) velocidadFreno = 0;
      mover(velocidadFreno, velocidadFreno);
      break;

    case BUSCANDO_LINEA:
      if (!lineaPerdida) {
        go_state(SEGUIR_LINEA);
        break;
      }
      if (send_line_lost == true) {
        Serial.print("l"); // LINE_LOST
        Serial.print("}");
        
        Serial.print("b"); // INIT_LINE_SEARCH
        Serial.print("}");
        send_line_lost = false;
      }
      
      led_color(255, 0, 0);
      if (ultimoLadoVisto == IZQUIERDA) {
        mover(velocidadGiroLenta, velocidadGiroRapida);
      } else {
        mover(velocidadGiroRapida, velocidadGiroLenta);
      }
      break;

    case SEGUIR_LINEA:
      if (lineaPerdida){
        go_state(BUSCANDO_LINEA);
        break;
      }

      led_color(0, 255, 0);
      error_linea = valRight - valLeft;
      
      derivate_linea = error_linea - prev_error_linea;
      prev_error_linea = error_linea;

      ajuste = (kp_linea * error_linea) + (kd_linea * derivate_linea);

      if (abs(ajuste) < 10) ajuste = 0;
      ajuste = constrain(ajuste, -velocidadBase, velocidadBase);

      speed_left = velocidadBase + ajuste;
      speed_right = velocidadBase - ajuste;

      mover(speed_left, speed_right);
      break;
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

  // Configuración de Hilos
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

  // Espera a que la ESP32 dé la orden de inicio (Handshake)
  String sendBuff;
  while(1) {
    if (Serial.available()) {
      char c = Serial.read();
      sendBuff += c;
      if (c == '}')  { 
        sendBuff = "";
        break;
      } 
    }
  }
  
  // INICIO DE VUELTA
  if (send_start == true) {
    start_time = millis(); // Guardamos el tiempo de inicio
    Serial.print("s"); // START_LAP
    Serial.print("}");
    send_start = false;
  }
}

void loop()
{
  controlador.run();
}