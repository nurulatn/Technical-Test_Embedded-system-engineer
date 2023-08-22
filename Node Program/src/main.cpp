#include <Arduino.h>
#include <ModbusMaster.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#define RS485_1_RX      16
#define RS485_1_TX      17
#define RS485_1_RE_NEG  18
#define RS485_1_DE      19

#define RS485_2_RX      1
#define RS485_2_TX      3
#define RS485_2_RE_NEG  23
#define RS485_2_DE      25

ModbusMaster node1;
ModbusMaster node2;

uint8_t result;
uint16_t data_Temp, data_Volt, data_Curr, data_PA;
uint16_t registerAddresses_EM4M[3] = {0x0123, 0x0234, 0x0345}; //Volt, Curr, PA

unsigned long LoopingTime = 0;

float setpoint;
int suhu_normal = 27;
String fanStatus;

#define Relay_Pin 12

const char* ssid = "NamaWifi";
const char* password = "1234567890";

const char* mqtt_server = "2001:41d0:1:925e::1";
const int mqtt_port = 1883;
const char* mqtt_topic = "voltage/DATA/LOCAL/SENSOR/PANEL_1";
const char* mqtt_client_id = "ramadhani-nurul-atina";

WiFiClient espClient;
PubSubClient client(espClient);

void preTransmission() {
  digitalWrite(RS485_1_RE_NEG, 1);
  digitalWrite(RS485_1_DE, 1);
  digitalWrite(RS485_2_RE_NEG, 1);
  digitalWrite(RS485_2_DE, 1);
}

void postTransmission() {
  digitalWrite(RS485_1_RE_NEG, 0);
  digitalWrite(RS485_1_DE, 0);
  digitalWrite(RS485_2_RE_NEG, 0);
  digitalWrite(RS485_2_DE, 0);
}

void callback(char* topic, byte* payload, unsigned int length) {
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("MQTT connection...");
    if (client.connect(mqtt_client_id)) {
      Serial.println("connected");
      client.subscribe(mqtt_topic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      delay(1000);
    }
  }
}

void setup() {
  pinMode(RS485_1_RE_NEG, OUTPUT);
  pinMode(RS485_1_DE, OUTPUT);
  pinMode(RS485_2_RE_NEG, OUTPUT);
  pinMode(RS485_2_DE, OUTPUT);
  
  digitalWrite(RS485_1_RE_NEG, 0);
  digitalWrite(RS485_1_DE, 0);
  digitalWrite(RS485_2_RE_NEG, 0);
  digitalWrite(RS485_2_DE, 0);
  
  Serial.begin(9600);

  node1.begin(1, Serial2);
  node2.begin(1, Serial);

  node1.preTransmission(preTransmission);
  node1.postTransmission(postTransmission);

  node2.preTransmission(preTransmission);
  node2.postTransmission(postTransmission);

  pinMode(Relay_Pin, OUTPUT);
  digitalWrite(Relay_Pin, LOW);

  WiFi.begin(ssid, password);
  Serial.print("Menghubungkan...");

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.print("Berhasil terhubung, IP address: ");
  Serial.println(WiFi.localIP());

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  unsigned long waktuSekarang = millis();
  if (waktuSekarang - LoopingTime >= 1000)
  {
  digitalWrite(RS485_1_RE_NEG, 1);
  digitalWrite(RS485_1_DE, 1);
  result = node1.readInputRegisters(0x03E8, 1);
  data_Temp = node1.getResponseBuffer(0);
  digitalWrite(RS485_1_RE_NEG, 0);
  digitalWrite(RS485_1_DE, 0);

  if (result == node1.ku8MBSuccess) {
    Serial.print("Temp: ");
    Serial.println(data_Temp);
  } else {
    Serial.println("Failed to read register");
  }

  digitalWrite(RS485_2_RE_NEG, 1);
  digitalWrite(RS485_2_DE, 1);
  for (int i = 0; i < 3; i++) {
    result = node2.readInputRegisters(registerAddresses_EM4M[i], 1);
    if(i=0){
      data_Volt = node2.getResponseBuffer(0);
    }
    else if(i=1){
      data_Curr = node2.getResponseBuffer(0);
    }
    else if(i=2){
      data_PA = node2.getResponseBuffer(0);
    }
  }
  digitalWrite(RS485_2_RE_NEG, 0);
  digitalWrite(RS485_2_DE, 0);

  if (result == node2.ku8MBSuccess) {
      Serial.print("Voltage: ");
      Serial.println(data_Volt);
      Serial.print("Current: ");
      Serial.println(data_Curr);
      Serial.print("Power Active: ");
      Serial.println(data_PA);
  } else {
    Serial.println("Failed to read registers");
  }

  setpoint = suhu_normal * 102 / 100;
  if (data_Temp >= setpoint){
    digitalWrite(Relay_Pin, HIGH);
    fanStatus = "ON";
  }
  else if (data_Temp <= suhu_normal){
    digitalWrite(Relay_Pin, LOW);
    fanStatus = "OFF";
  }

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  StaticJsonDocument<200> jsonDoc;
  jsonDoc["status"] = "OK";
  jsonDoc["deviceID"] = "ramadhani-nurul-atina";
  JsonObject data = jsonDoc.createNestedObject("data");
  data["v"] = 221; //data_Volt
  data["i"] = 2.3; //data_Curr;
  data["pa"] = 508.3; //data_PA;
  data["temp"] = 27.4; //data_Temp;
  data["fan"] = "OFF"; //fanStatus;
  data["time"] = "2023-07-01 12:30:05"; //

  String payload;
  serializeJson(jsonDoc, payload);
  
  client.publish(mqtt_topic, payload.c_str());

  LoopingTime = waktuSekarang;
  }
}
