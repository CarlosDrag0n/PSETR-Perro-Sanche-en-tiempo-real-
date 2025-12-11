// --- Configuración de Pines Sensores ---
#define PIN_ITR20001_LEFT   A2
#define PIN_ITR20001_MIDDLE A1
#define PIN_ITR20001_RIGHT  A0

// --- Definición de Pines Motores ---
#define PIN_Motor_STBY  3
// Grupo A (Derecha)
#define PIN_Motor_AIN_1 7
#define PIN_Motor_PWMA  5
// Grupo B (Izquierda)
#define PIN_Motor_BIN_1 8
#define PIN_Motor_PWMB  6 

float kp = 0.001;
float kd = 2;

int error;
int speed_left;
int speed_right;

int derivate;
int prev_error;

int velocidadBase = 60;

// Enum para ver cual es el útimo infrarojos en detectar linea negra
enum LastInfra {
  LEFT,
  MIDDLE,
  RIGHT,
};

int maxspeed = 255;
int vals[3];
int speeds[2];

// Devuelve un puntero al array de lecturas
void read_infra()
{
  int valLeft = analogRead(PIN_ITR20001_LEFT);
  int valMiddle = analogRead(PIN_ITR20001_MIDDLE);
  int valRight = analogRead(PIN_ITR20001_RIGHT);

  // Debugging para Serial Plotter
  Serial.print("Izquierda:"); Serial.print(valLeft); Serial.print(", ");
  Serial.print("Centro:"); Serial.print(valMiddle); Serial.print(", ");
  Serial.print("Derecha:"); Serial.println(valRight);

  vals[0] = valLeft;
  vals[1] = valMiddle;
  vals[2] = valRight;
}

void pid(int* inf_vals)
{

  int left = inf_vals[0];
  int middle = inf_vals[1];
  int right = inf_vals[2];

  error = right - left;
  
  derivate = error - prev_error;
  prev_error = error;

  speed_left = velocidadBase + kp * error + kd * derivate;
  speed_right = velocidadBase - (kp * error + kd * derivate);

  speed_left = constrain(speed_left, 0, 255);
  speed_right = constrain(speed_right, 0, 255);

  speeds[0] = speed_left;
  speeds[1] = speed_right;

  if (left < 300 && middle < 300 && right < 300) {
    speeds[0] = 0;
    speeds[1] = 0;
  }

  Serial.print("Left: ");
  Serial.print(speeds[0]);
  Serial.print(" ");
  Serial.print("Right: ");
  Serial.print(speeds[1]);
  Serial.print(" ");
}

void move(int* speeds)
{
  speed_left = speeds[0];
  speed_right = speeds[1];

  digitalWrite(PIN_Motor_BIN_1, HIGH);
  analogWrite(PIN_Motor_PWMB, speed_left);

  digitalWrite(PIN_Motor_AIN_1, HIGH);
  analogWrite(PIN_Motor_PWMA, speed_right);
}

void setup()
{
  Serial.begin(9600);

  // Configuración de pines como Salida
  pinMode(PIN_Motor_STBY, OUTPUT);
  pinMode(PIN_Motor_AIN_1, OUTPUT);
  pinMode(PIN_Motor_PWMA, OUTPUT);
  pinMode(PIN_Motor_BIN_1, OUTPUT);
  pinMode(PIN_Motor_PWMB, OUTPUT);

  // Habilitar la controladora de motores
  digitalWrite(PIN_Motor_STBY, HIGH);
}

void loop()
{
   // 1. Leer sensores
  read_infra();
  
  // 2. Calcular PID
  pid(vals);

  //move(speeds);

}
