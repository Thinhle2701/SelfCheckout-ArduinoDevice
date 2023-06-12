/**
   BasicHTTPClient.ino

    Created on: 24.05.2015

*/
#include <MFRC522.h>

#include <Arduino.h>
#include <SPI.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266HTTPClient.h>
#include <string>
#include <WiFiClient.h>


MFRC522::MIFARE_Key key;          //create a MIFARE_Key struct named 'key', which will hold the card information
MFRC522::StatusCode status;
#define SERVER_IP "192.168.1.128:8000" 
constexpr uint8_t RST_PIN =  0;          // Configurable, see typical pin layout above 18
constexpr uint8_t SS_PIN =  15;         // Configurable, see typical pin layout above  16
MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance
byte blockcontent[5] = {"P001"}; 
int blockWrite = 6;  
bool isReading = false;
// WiFi
const char *ssid = "BaoLong";               // Enter your WiFi name
const char *password = "lelong0987654321";  // Enter WiFi password

// MQTT Broker
const char *mqtt_broker = "broker.emqx.io";
const char *topic = "CheckoutRFID";
// const char *mqtt_username = "emqx";
// const char *mqtt_password = "public";
const int mqtt_port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);
String newTopic = "";
void setup() {
  WiFi.begin(ssid, password);
  Serial.begin(9600);
  // Serial.setDebugOutput(true);
  delay(1000);
  Serial.println("Setup");
  
  while (!Serial);    // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
  SPI.begin();      // Init SPI bus
  mfrc522.PCD_Init();   // Init MFRC522
  mfrc522.PCD_DumpVersionToSerial();  // Show details of PCD - MFRC522 Card Reader details
  Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));
  Serial.println("Setup done");
  Serial.println();
  Serial.println();
  Serial.println();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the WiFi network");
  //connecting to a mqtt broker
  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callback);
  while (!client.connected()) {
    String client_id = "esp8266-client-";
    client_id += String(WiFi.macAddress());
    Serial.printf("The client %s connects to the public mqtt broker\n", client_id.c_str());
    if (client.connect(client_id.c_str(), "thinh", "thinhbeo2801")) {
      Serial.println("Public emqx mqtt broker connected");
    } else {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
  }
  client.subscribe(topic);

  // for (uint8_t t = 4; t > 0; t--) {
  //   Serial.printf("[SETUP] WAIT %d...\n", t);
  //   Serial.flush();
  //   delay(1000);
  // }
  Serial.println("Ready to Scan RFID");


}

void callback(char *topic, byte *payload, unsigned int length) {
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  String messageTopic = "";
  for (int i = 14; i < length; i++) {
      messageTopic += (char)payload[i];
  }

  if (messageTopic ==  String(ESP.getChipId(),HEX))
  {
    if (strstr(message.c_str(), "start")) {
      isReading = true;
      int indexContent = 0;
      newTopic = messageTopic;
    }
    
    if (strstr(message.c_str(), "stop")) {
      isReading = false;
    }
  }


  Serial.println(newTopic);

  Serial.println();
  Serial.println("-----------------------");
}

String ReadRFID(MFRC522 mfrc522)
{
  byte byteCount;
	byte buffer[18];
	byte i;
	String uuid = "";
  String dataBlock = "";
  for (byte page = 0; page < 16; page +=4) { // Read returns data for 4 pages at a time.
      // Read pages
    byteCount = sizeof(buffer);
    status = mfrc522.MIFARE_Read(page, buffer, &byteCount);
    if (status != MFRC522::STATUS_OK) {
      Serial.print(F("MIFARE_Read() failed: "));
      Serial.println(mfrc522.GetStatusCodeName(status));
      break;
    }
    if (page < 4){
      for (byte index = 0; index < 4; index++) {
        i = index;
        uuid.concat(String(buffer[i] < 0x10 ? " 0":" "));
        uuid.concat(String(buffer[i],HEX));
			}
    }else if(page >= 4 && page < 8){
      for (byte index = 0; index < 4; index++) {
        int y = 2 * 4 + index;
        dataBlock += (char)buffer[y];
			}
    }
  }
      // Dump data
  uuid.toUpperCase();
  dataBlock.toUpperCase();
  // Serial.print("UUID: ");
  // Serial.print(uuid);
  // Serial.println();
  // Serial.print("Data block: ");
  // Serial.print(dataBlock);
  // Serial.println();

  if (dataBlock.indexOf("P") < 0){
    return "";
  }
  else
  {
    String result = uuid + "||" + dataBlock + "||" + String(ESP.getChipId(),HEX);
    return result;
  }
}


void loop() {
  client.loop();
  if (isReading == true) {
    if (!mfrc522.PICC_IsNewCardPresent()) {
      //delay(50);
      return;
    }

    // Select one of the cards
    if (!mfrc522.PICC_ReadCardSerial()) {
      //delay(50);
      return;
    }
    Serial.println("Scan a tag to Read Data....");
    String UUID = ReadRFID(mfrc522);
    if (newTopic == String(ESP.getChipId(),HEX)){
      client.publish(newTopic.c_str(),(char*)UUID.c_str());
    }

    delay(5000);
  }
  Serial.println(newTopic);
}

void sendRFIDtoBE(String RFID){
    WiFiClient client;
    HTTPClient http;
    
    Serial.print("[HTTP] begin...\n");

    http.begin(client, "http://" SERVER_IP "/api/rfid/sendtag"); //HTTP
    http.addHeader("Content-Type", "application/json");

    Serial.print("[HTTP] POST...\n");

    String httpRequestData = "{"
      "\"error\":\"true\","
      "\"message\":\"this is a message of Arduino\","
      "\"data\":" 
        "{\"RFID\":\"" + RFID + "\",\"deviceID\":\"" + String(ESP.getChipId(),HEX)+ "\",\"device_Name\":\"" + "Wemos D1" + "\"}"
    "}";
    int httpCode = http.POST(httpRequestData);

    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTP] POST... code: %d\n", httpCode);
      // file found at server
      if (httpCode == HTTP_CODE_OK) {
        const String& payload = http.getString();
      }
    }
    else {
      Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
}


bool WriteRFID(int blockNumber, byte arrayAddress[]){
  status = mfrc522.MIFARE_Write(blockNumber, arrayAddress, 16);
  if (status != MFRC522::STATUS_OK) {
           Serial.print("MIFARE_Write() failed: ");
           Serial.println(mfrc522.GetStatusCodeName(status));
           return false;
  }
  Serial.println("block was written");
  return true;
}



