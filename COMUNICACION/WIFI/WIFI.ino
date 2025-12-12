#include <WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

#define RXD2 33
#define TXD2 4

// --- CREDENCIALES EDUROAM ---
#define EAP_ANONYMOUS_IDENTITY "20220719anonymous@urjc.es"
#define EAP_IDENTITY "wifi.fuenlabrada.acceso@urjc.es" 
#define EAP_PASSWORD "EstasenFuenlabrada.00"         
#define EAP_USERNAME "wifi.fuenlabrada.acceso@urjc.es" 
const char* ssid = "eduroam";

// --- CREDENCIALES MOVIL LUIS ---
const char* luis_ssid = "MOVISTAR-WIFI6-4BC0";
const char* luis_password = "****************";

// --- MQTT ---
#define mqtt_server "193.147.79.118"
#define mqtt_port 21883
#define mqtt_topic "/SETR/2025/si/"

// ---MESSAGES ---
String team_name = "PSETR";
String id = "13";
String action;
char caracter;
//float time;
//float distance;
//float value;

WiFiClient espClient;

Adafruit_MQTT_Client mqtt(&espClient, mqtt_server, mqtt_port);

Adafruit_MQTT_Publish mqtt_pub = Adafruit_MQTT_Publish(&mqtt, mqtt_topic);

String sendBuff;
String message;

String selectMessage(String buff)
{
  caracter = buff[0];

  switch(caracter) {
    case 'a':
      action = "hola";
      break;
  }

return String("{") + "\"team_name\": \"" + team_name + "\", \"id\": \"" + id + "\", \"action\": \"" + action + "\"}";
}

void reconnectMQTT()
{

   if (mqtt.connected()) {
    Serial.println("¡Conectado a MQTT!");
    return;
  }

  while (!mqtt.connected()) {
    Serial.println("Intentando conexión MQTT...");
    int8_t ret = mqtt.connect();

    if (ret == 0) {
      Serial.println("¡Conectado a MQTT!");
      return;
    }

    Serial.print("Error MQTT = ");
    Serial.println(mqtt.connectErrorString(ret));

    mqtt.disconnect();
    delay(3000);
  }
}

void setup()
{
  Serial.begin(9600); 
  
  delay(10);
  Serial.println();
  Serial.print(F("Connecting to network: "));
  Serial.println(ssid);
  
  WiFi.disconnect(true); 

  // Eduroam
  //WiFi.begin(ssid, WPA2_AUTH_PEAP, EAP_IDENTITY, EAP_USERNAME, EAP_PASSWORD); 

  // Movil luis
  WiFi.begin(luis_ssid, luis_password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(F("."));
  }
  
  Serial.println("\nWiFi is connected!");
  Serial.println(WiFi.localIP());

  // Iniciar comunicación con Arduino
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);

  delay(2000); 
  Serial.println("Enviando señal de arranque al Arduino...");
  Serial2.print("{ 'start': 1 }"); 
}

void loop()
{
  // 1. Mantener conexión MQTT viva
  if (!mqtt.connected()) {
    reconnectMQTT();
  }
  mqtt.processPackets(10);
  mqtt.ping();

  // 2. Escuchar al Arduino
  if (Serial2.available()) {
    char c = Serial2.read();
    sendBuff += c;
    
    // Si encontramos el final del mensaje JSON
    if (c == '}')  {
      // --- CREAR FUNCIÓN DEPENDIENDO DEL CARACTER MANDE UN MENSAJE U OTRO ---
      message = selectMessage(sendBuff);
      Serial.print("Recibido de Arduino: ");
      Serial.println(sendBuff);
      
      // Enviamos al MQTT
      if (mqtt_pub.publish(message.c_str())) {
         Serial.println("--> Publicado en MQTT correctamente");
      } else {
         Serial.println("--> Fallo al publicar");
      }

      // Limpiamos el buffer para el siguiente mensaje
      sendBuff = "";
    } 
  }
}
