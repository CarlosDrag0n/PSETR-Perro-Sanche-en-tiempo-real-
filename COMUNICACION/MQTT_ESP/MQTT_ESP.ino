/*
* Copyright (C) 2022 by Roberto Calvo-Palomino
*
*
*  This programa is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation, either version 3 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with RTL-Spec.  If not, see <http://www.gnu.org/licenses/>.
*
*   Authors: Roberto Calvo-Palomino <roberto.calvo [at] urjc [dot] es>
*/

// Subject: Sistemas Empotrados y de Tiempo Real
// Universidad Rey Juan Carlos, Spain

// Define specific pins for Serial2.
#define RXD2 33
#define TXD2 4


void setup() {

  // Regular serial connection to show traces for debug porpuses
  Serial.begin(9600);

  // Serial port to communicate with Arduino UNO
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  
  delay(5000);
  
  Serial2.print("{ 'test': " + String(millis()) + " }");
  Serial.print("Messase sent! to Arduino");
     
  
}

String sendBuff;

void loop() {

  // We always check if there is data in the serial buffer (max: 64 bytes)

  if (Serial2.available()) {

    char c = Serial2.read();
    sendBuff += c;
    
    if (c == '}')  {            
      Serial.print("Received data in serial port from Arduino: ");
      Serial.println(sendBuff);

      sendBuff = "";
    } 


  }
}