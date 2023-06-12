#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <MFRC522.h>
#include <string>


MFRC522::MIFARE_Key key;  //create a MIFARE_Key struct named 'key', which will hold the card information
MFRC522::StatusCode status;
constexpr uint8_t RST_PIN = 0;  // Configurable, see typical pin layout above 18
constexpr uint8_t SS_PIN = 15;  // Configurable, see typical pin layout above  16
MFRC522 mfrc522(SS_PIN, RST_PIN);
//write
int blockWrite = 6;
byte blockcontent[5] = { "P001" };
bool isWriting = false;
// WiFi
const char *ssid = "BaoLong";               // Enter your WiFi name
const char *password = "lelong0987654321";  // Enter WiFi password

// MQTT Broker
const char *mqtt_broker = "broker.emqx.io";
const char *topic = "rfid";
// const char *mqtt_username = "emqx";
// const char *mqtt_password = "public";
const int mqtt_port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  // Set software serial baud to 115200;
  Serial.begin(9600);
  // connecting to a WiFi network
  WiFi.begin(ssid, password);
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
  // publish and subscribe
  client.subscribe(topic);

  SPI.begin();                        // Init SPI bus
  mfrc522.PCD_Init();                 // Init MFRC522
  mfrc522.PCD_DumpVersionToSerial();  // Show details of PCD - MFRC522 Card Reader details
}

bool WriteRFID(int blockNumber, byte arrayAddress[]) {
  status = mfrc522.MIFARE_Write(blockNumber, arrayAddress, 16);
  if (status != MFRC522::STATUS_OK) {
    Serial.print("MIFARE_Write() failed: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    return false;
  }
  Serial.println("block was written");
  return true;
}

void callback(char *topic, byte *payload, unsigned int length) {
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  if (strstr(message.c_str(), "start")) {
    isWriting = true;
    int indexContent = 0;
    for (int i = 15; i < length; i++) {
      blockcontent[indexContent] = (char)payload[i];
      indexContent++;
    }
  }

  if (strstr(message.c_str(), "stop")) {
    isWriting = false;
  }

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
    }
  }
      // Dump data
  uuid.toUpperCase();
  return uuid;
}

void loop() {
  client.loop();
  if (isWriting == true) {
    if (!mfrc522.PICC_IsNewCardPresent()) {
      //delay(50);
      return;
    }

    // Select one of the cards
    if (!mfrc522.PICC_ReadCardSerial()) {
      //delay(50);
      return;
    }
    Serial.println("Scan a tag to write Data....");
    bool checkWrite = WriteRFID(blockWrite, blockcontent);
    String UUID = ReadRFID(mfrc522);
    String topic = "rfid/" + String((char*)blockcontent);
    client.publish((char*)topic.c_str(),(char*)UUID.c_str());
    delay(5000);
  }
}