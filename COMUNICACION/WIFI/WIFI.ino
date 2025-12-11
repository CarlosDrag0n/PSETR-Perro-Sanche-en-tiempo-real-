#include <WiFi.h>
#include <PubSubClient.h>

#define RXD2 33
#define TXD2 4

// --- CREDENCIALES EDUROAM ---
#define EAP_ANONYMOUS_IDENTITY "20220719anonymous@urjc.es"
#define EAP_IDENTITY "wifi.fuenlabrada.acceso@urjc.es" 
#define EAP_PASSWORD "EstasenFuenlabrada.00"         
#define EAP_USERNAME "wifi.fuenlabrada.acceso@urjc.es" 
const char* ssid = "eduroam";

// --- MQTT ---
const char* mqtt_server = "193.147.79.118";
const int mqtt_port = 21883;
const char* mqtt_topic = "/SETR/2025/si/"; 

WiFiClient espClient;
PubSubClient client(espClient);

String sendBuff;
// Eliminamos 'payload' global porque la usaremos localmente al recibir

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print(F("Connecting to network: "));
  Serial.println(ssid);
  
  WiFi.disconnect(true); 
  WiFi.begin(ssid, WPA2_AUTH_PEAP, EAP_IDENTITY, EAP_USERNAME, EAP_PASSWORD); 

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(F("."));
  }
  
  Serial.println("\nWiFi is connected!");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Intentando conexión MQTT...");
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    
    if (client.connect(clientId.c_str())) {
      Serial.println("¡Conectado!");
    } else {
      Serial.print("falló, rc=");
      Serial.print(client.state());
      Serial.println(" reintentando en 5s");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(9600); 
  setup_wifi(); 
  client.setServer(mqtt_server, mqtt_port);

  // Iniciar comunicación con Arduino
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);

  delay(2000); 
  Serial.println("Enviando señal de arranque al Arduino...");
  Serial2.print("{ 'start': 1 }"); 
}

void loop() {
  // 1. Mantener conexión MQTT viva
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // 2. Escuchar al Arduino
  if (Serial2.available()) {
    char c = Serial2.read();
    sendBuff += c;
    
    // Si encontramos el final del mensaje JSON
    if (c == '}')  {            
      Serial.print("Recibido de Arduino: ");
      Serial.println(sendBuff);
      
      // Enviamos al MQTT
      if (client.publish(mqtt_topic, sendBuff.c_str())) {
         Serial.println("--> Publicado en MQTT correctamente");
      } else {
         Serial.println("--> Fallo al publicar");
      }

      // Limpiamos el buffer para el siguiente mensaje
      sendBuff = "";
    } 
  }
}
