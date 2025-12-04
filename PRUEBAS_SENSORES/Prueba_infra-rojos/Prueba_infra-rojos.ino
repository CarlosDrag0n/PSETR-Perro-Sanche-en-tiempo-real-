// --- Configuración de Pines ---
#define PIN_ITR20001_LEFT   A2
#define PIN_ITR20001_MIDDLE A1
#define PIN_ITR20001_RIGHT  A0

void setup() {
  // Iniciamos comunicación serial a alta velocidad para no perder datos
  Serial.begin(9600);
  
  // Nota: No es estrictamente necesario declarar pinMode como INPUT 
  // para lecturas analógicas (analogRead), pero no hace daño.
}

void loop() {
  // 1. Leemos el valor analógico puro (Rango 0 - 1023 en ADC de 10-bits)
  int valLeft = analogRead(PIN_ITR20001_LEFT);
  int valMiddle = analogRead(PIN_ITR20001_MIDDLE);
  int valRight = analogRead(PIN_ITR20001_RIGHT);

  // 2. Imprimimos los valores con etiquetas para el Serial Plotter
  // Formato: "Etiqueta:Valor,Etiqueta:Valor,Etiqueta:Valor"
  
  Serial.print("Izquierda:");
  Serial.print(valLeft);
  Serial.print(","); // Separador importante
  
  Serial.print("Centro:");
  Serial.print(valMiddle);
  Serial.print(",");
  
  Serial.print("Derecha:");
  Serial.println(valRight); // Salto de línea al final

  // Pequeña pausa para estabilizar la lectura visual, 
  // pero lo suficientemente rápida para detectar cambios
  delay(500); 
}