#include <WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <Thread.h>

#define RXD2 33
#define TXD2 4

Thread hilo_ping = Thread(); // NUEVO: Hilo para PING
unsigned long start_time = 0;
bool should_ping = false;

// --- CREDENCIALES EDUROAM ---
#define EAP_ANONYMOUS_IDENTITY "20220719anonymous@urjc.es"
#define EAP_IDENTITY "wifi.fuenlabrada.acceso@urjc.es" 
#define EAP_PASSWORD "EstasenFuenlabrada.00"         
#define EAP_USERNAME "wifi.fuenlabrada.acceso@urjc.es" 
const char* ssid = "wifi.fuenlabrada.acceso@urjc.es";
const char* password = "EstasenFuenlabrada.00";
// --- CREDENCIALES MOVIL LUIS ---
const char* luis_ssid = "LuisMovible";
const char* luis_password = "luis_pass";

// --- MQTT ---
#define mqtt_server "193.147.79.118"
#define mqtt_port 21883
#define mqtt_topic "/SETR/2025/13/"

// ---MESSAGES ---
String team_name = "PSETR";
String id = "13";
char caracter;
String value;

WiFiClient espClient;

Adafruit_MQTT_Client mqtt(&espClient, mqtt_server, mqtt_port);

Adafruit_MQTT_Publish mqtt_pub = Adafruit_MQTT_Publish(&mqtt, mqtt_topic);

String sendBuff;
String message;

String selectMessage(char caracter, String float_val)
{

  String action;
  String measurement;

  switch(caracter) {
    case 's':
      action = "START_LAP";
      break;

    case 'f':
      action = "END_LAP";
      measurement = "time";
      break;

    case 'o':
      action = "OBSTACLE_DETECTED";
      measurement = "distance";
      break;

    case 'l':
      action = "LINE_LOST";
      break;

    case 'p':
      action = "PING";
      measurement = "time";
      break;
    
    case 'b':
      action = "INIT_LINE_SEARCH";
      break;
    
    case 'e':
      action = "STOP_LINE_SEARCH";
      break;

    case 'w':
      action = "LINE_FOUND";
      break;

    case 'v':
      action = "VISIBLE_LINE";
      measurement = "value";
      break;
    
    default:
      return "";
  }

  String json = "\n{";
  json += "\n\"team_name\": \"" + team_name + "\",";
  json += "\n\"id\": \"" + id + "\",";
  json += "\n\"action\": \"" + action + "\"";

  if (measurement != "" and float_val != "") {
    json += ",\n\"" + measurement + "\": " + float_val;
  }

  json += "\n}\n";

  // Reinicio 
  measurement = "";

return json;
}

void callback_ping() {
  // Solo enviar PING si la carrera ha empezado y no ha terminado
  if (should_ping) {
    unsigned long current_lap_time = millis() - start_time;
    message = selectMessage('p', String(current_lap_time));

    if (mqtt_pub.publish(message.c_str())) {
      Serial.println("--> Publicado en MQTT correctamente");
    } else {
      Serial.println("--> Fallo al publicar");
    }
  }
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
  Serial.println("Enviando señal de arranque al Arduino...");
  Serial2.print("{ 'start': 1 }"); 

  hilo_ping.enabled = true;
  hilo_ping.onRun(callback_ping);
  hilo_ping.setInterval(4000);

  delay(2000);
}

void loop()
{
  // 1. Mantener conexión MQTT viva
  if (!mqtt.connected()) {
    reconnectMQTT();
  }
  mqtt.processPackets(10);
  mqtt.ping();

  // 2. Mandamos el pin si ha pasado el tiempo necesario
  if (hilo_ping.shouldRun()) {
    hilo_ping.run();
  }

  // 3. Escuchar al Arduino
  if (Serial2.available()) {
    char c = Serial2.read();
    sendBuff += c;
    
    // Si encontramos el final del mensaje JSON
    if (c == '}')  {
      //Cojo los caracteres importantes del String enviado
      sendBuff.replace("}", "");
      sendBuff.trim();

      if (sendBuff.length() > 0) {
        caracter = sendBuff[0];

        if (caracter == 's') {
          start_time = millis(); 
          should_ping = true;
        }

        if (caracter == 'f') {
          should_ping = false;
          hilo_ping.enabled = false;
        }
        
        if (sendBuff.length() > 1) {
          value = sendBuff.substring(1);
        } else {
          value = "";
        }

        // Creo el String a mandar
        message = selectMessage(caracter, value);

        Serial.print("Recibido de Arduino: ");
        Serial.println(sendBuff);
        
        if (message != "") {
        // Enviamos al MQTT
          if (mqtt_pub.publish(message.c_str())) {
            Serial.println("--> Publicado en MQTT correctamente");
          } else {
            Serial.println("--> Fallo al publicar");
          }
        }
      }
      // Limpiamos el buffer para el siguiente mensaje
      sendBuff = "";
    } 
  }
}