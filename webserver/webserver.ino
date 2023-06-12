#include <ESP8266WiFi.h>
#include <WiFiClient.h>
//ESP Web Server Library to host a web page
#include <ESP8266WebServer.h>
//---------------------------------------------------------------
//Our HTML webpage contents in program memory
//Main
const char MAIN_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<body>
<center>
<h1>State of LED: __</h1><br>
<a href="ledOn"><button>ON</button></a><br>
<a href="ledOff"><button>OFF</button></a><br>
</center>
</body>
</html>
)=====";
//Led on
const char LEDON_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<body>
<center>
<h1>State of LED: ON</h1><br>
<a href="ledOn"><button>ON</button></a><br>
<a href="ledOff"><button>OFF</button></a><br>
</center>
</body>
</html>
)=====";
//Led off
const char LEDOFF_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<body>
<center>
<h1>State of LED: OFF</h1><br>
<a href="ledOn"><button>ON</button></a><br>
<a href="ledOff"><button>OFF</button></a><br>
</center>
</body>
</html>
)=====";
//---------------------------------------------------------------
//On board LED Connected to GPIO2
#define LED LED_BUILTIN
//SSID and Password of your WiFi router
const char* ssid = "BaoLong";
const char* password = "lelong0987654321";
//Declare a global object variable from the ESP8266WebServer class.
ESP8266WebServer server(80); //Server on port 80
//===============================================================
// This routine is executed when you open its IP in browser
//===============================================================
void handleRoot() {
Serial.println("You called root page");
String html = MAIN_page; //Read HTML contents
server.send(200, "text/html", html); //Send web page
}
void handleLEDon() {
Serial.println("LED on page");
digitalWrite(LED,LOW); //LED is connected in reverse
String html = LEDON_page; //Read HTML contents
server.send(200, "text/html", html); //Send ADC value only to client ajax request
}
void handleLEDoff() {
Serial.println("LED off page");
digitalWrite(LED,HIGH); //LED off
String html = LEDOFF_page; //Read HTML contents
server.send(200, "text/html", html); //Send ADC value only to client ajax request
}
//==============================================================
// SETUP
//==============================================================
void setup(){
Serial.begin(9600);
WiFi.begin(ssid, password); //Connect to your WiFi router
Serial.println("");
//Onboard LED port Direction output
pinMode(LED,OUTPUT);
//Power on LED state off
digitalWrite(LED,HIGH);
// Wait for connection
while (WiFi.status() != WL_CONNECTED) {
delay(500);
Serial.print(".");
}
//If connection successful show IP address in serial monitor
Serial.println("");
Serial.print("Connected to ");
Serial.println(ssid);
Serial.print("IP address: ");
Serial.println(WiFi.localIP()); //IP address assigned to your ESP
server.on("/", handleRoot); //Which routine to handle at root location. This is display page
server.on("/ledOn", handleLEDon); //as Per <a href="ledOn">, Subroutine to be called
server.on("/ledOff", handleLEDoff);
server.begin(); //Start server
Serial.println("HTTP server started");
}
//==============================================================
// LOOP
//==============================================================
void loop(){
server.handleClient(); //Handle client requests
}