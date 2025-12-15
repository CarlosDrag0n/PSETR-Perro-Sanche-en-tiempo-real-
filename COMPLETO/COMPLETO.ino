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
int velocidadBase = 120; 

// Recuperación
int velocidadGiroRapida = 200; 
int velocidadGiroLenta = -20;  
int umbralNegro = 400; 

enum Lado { IZQUIERDA, DERECHA };
Lado ultimoLadoVisto = DERECHA; 

int valLeft, valMiddle, valRight;

// Ultrasonidos
long duracion;
int distancia = 999;
const int distanciaObjetivo = 7;       
const int distanciaInicioFrenado = 35; 
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
Thread hilo_ping = Thread(); // NUEVO: Hilo para PING

// Flags de comunicación
bool send_start = true;
bool send_line_lost = true;
bool send_obstacle = true;
bool send_finish = true;


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

// --- NUEVO: Callback para PING cada 4 segundos ---
void callback_ping() {
  // Solo enviar PING si la carrera ha empezado y no ha terminado
  if (start_time > 0 && estadoActual != FINALIZADO) {
    unsigned long current_lap_time = millis() - start_time;
    
    Serial.print("p"); // Caracter para PING
    Serial.print(current_lap_time);
    Serial.print("}");
  }
}

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
  // --- FIN DE VUELTA ---
  if (estadoActual == FINALIZADO) {
    if (send_finish == true) {
      unsigned long total_time = millis() - start_time;
      Serial.print("f"); // END_LAP
      Serial.print(total_time); // Tiempo real calculado
      Serial.print("}");
      send_finish = false;
    }
    led_color(0, 0, 255);
    mover(0, 0);
    return;
  }

  // --- OBSTÁCULO ---
  if (estadoActual == PARANDO_OBSTACULO) {
    if (send_obstacle == true) {
      Serial.print("o"); // OBSTACLE_DETECTED
      Serial.print(distancia);
      Serial.print("}");
      send_obstacle = false;
    }
    
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

    // Transiciones de estado
    if (lineaPerdida && estadoActual == SEGUIR_LINEA) go_state(BUSCANDO_LINEA);
    else if (!lineaPerdida && estadoActual == BUSCANDO_LINEA) go_state(SEGUIR_LINEA);

    // --- MODO RECUPERACIÓN (LATIGAZO) ---
    if (estadoActual == BUSCANDO_LINEA) {
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
    }
    
    // --- MODO PID (RECTA/CURVA SUAVE) ---
    else if (estadoActual == SEGUIR_LINEA) {
      led_color(0, 255, 0);
      error_linea = valRight - valLeft;
      
      derivate_linea = error_linea - prev_error_linea;
      prev_error_linea = error_linea;

      int ajuste = (kp_linea * error_linea) + (kd_linea * derivate_linea);

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

  // Configuración de Hilos
  hilo_infra_rojos.onRun(callback_infra_rojos);
  hilo_infra_rojos.setInterval(5); 

  hilo_ultra_sonido.onRun(callback_ultra_sonido);
  hilo_ultra_sonido.setInterval(45); 

  hilo_motor.onRun(callback_motor);
  hilo_motor.setInterval(10); 

  // --- CONFIGURACIÓN PING ---
  hilo_ping.onRun(callback_ping);
  hilo_ping.setInterval(4000); // Cada 4 segundos (4000ms)

  controlador.add(&hilo_infra_rojos);
  controlador.add(&hilo_ultra_sonido);
  controlador.add(&hilo_motor);
  controlador.add(&hilo_ping); // Añadimos el ping al controlador

  FastLED.addLeds<NEOPIXEL, PIN_RBGLED>(leds, NUM_LEDS);
  FastLED.setBrightness(20);

  // Espera a que la ESP32 dé la orden de inicio (Handshake)
  String sendBuff;
  while(1) {
    Serial.print("Hola");
    if (Serial.available()) {
      Serial.print("Adios");
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