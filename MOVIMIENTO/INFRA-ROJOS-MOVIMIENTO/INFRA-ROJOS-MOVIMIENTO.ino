#include <Thread.h>
#include <ThreadController.h>

#define PIN_ITR20001_LEFT   A2
#define PIN_ITR20001_MIDDLE A1
#define PIN_ITR20001_RIGHT  A0

// Máquina de estados del motor
enum movimiento {
  LINEA_ENCONTRADA,           
  BUSCANDO_LINEA            
};
movimiento estadoActual = LINEA_ENCONTRADA; 

// 1. Instanciamos el Controlador (opcional, pero recomendado para manejar muchos hilos)
ThreadController controlador = ThreadController();

// 2. Creamos los Hilos
Thread hilo_infra_rojos = Thread();
Thread hilo_motor = Thread();

int valLeft = analogRead(PIN_ITR20001_LEFT);
int valMiddle = analogRead(PIN_ITR20001_MIDDLE);
int valRight = analogRead(PIN_ITR20001_RIGHT);

void callback_infrarojo() {
  valLeft = analogRead(PIN_ITR20001_LEFT);
  valMiddle = analogRead(PIN_ITR20001_MIDDLE);
  valRight = analogRead(PIN_ITR20001_RIGHT);
  if (PIN_ITR20001_LEFT < 100 && PIN_ITR20001_RIGHT< 100){
    estadoActual = BUSCANDO_LINEA;
  }else{
    estadoActual = LINEA_ENCONTRADA;
  }
}

bool motoor = false;
void callback_motor() {
  motoor = true;
}

void setup() {
  Serial.begin(9600);
  pinMode(13, OUTPUT);

  // 4. Configuramos el Hilo del LED
  hilo_infra_rojos.onRun(callback_infrarojo);    // Qué función ejecutar
  hilo_infra_rojos.setInterval(50);      // Cada cuánto (500ms)

  // 5. Configuramos el Hilo del Serial
  hilo_motor.onRun(callback_motor);
  hilo_motor.setInterval(50);  // Cada cuánto (1000ms)

  // 6. Añadimos los hilos al controlador
  controlador.add(&hilo_infra_rojos);
  controlador.add(&hilo_motor);
}

void loop() {
  // 7. Ejecutamos el controlador (revisa todos los hilos automáticamente)
  controlador.run();
  
  if (motoor == true){
    if (movimiento == LINEA_ENCONTRADA) {

  } else if (movimiento == BUSCANDO_LINEA){

  }
}
























