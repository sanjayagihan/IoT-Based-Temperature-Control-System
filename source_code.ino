#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

// motor controller - l298n
// https://microcontrollerslab.com/l298n-dc-motor-driver-module-esp8266-nodemcu/

// Update these with values suitable for your network.
const char* ssid = "Redmi Note 9S";
const char* password = "sanjayagihan";
const char* mqtt_server = "test.mosquitto.org";

// for temperature output
int outputpin= A0;
float celsius;
float millivolts;
int analogValue;

// for SCADA temperature input
int scada_temperature = 100;

// for motor controller
int ENA = D7;
int IN1 = D1;
int IN2 = D2;

// NTPClient - to get date time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

// Fan switch
char fanSwitch = 'f';

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];


void setup_wifi() {
  delay(100);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  // Set ESP8266 to station mode and connect to the specified WiFi network
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  // Wait until the WiFi connection is established
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Connection successful message
  Serial.println("\nWifi connection successful");
}


void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] : ");
  //----------------------------------------------------
  if (payload[0] == 't') {
    // Switch on the fan
    // digitalWrite(D1, HIGH);
    Serial.println("Fan switched ON");
    Serial.print("Obtained value: ");
    Serial.println((char)payload[0]);
    fanSwitch = 't';
  } else if (payload[0] == 'f') {
    // Switch off the fan
    // digitalWrite(D1, LOW);
    Serial.println("Fan switched OFF");
    Serial.print("Obtained value: ");
    Serial.println((char)payload[0]);
    fanSwitch = 'f';
  } else {
      scada_temperature = atoi((char *)payload);
      Serial.println(scada_temperature);
  }
}


void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("client01")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      // client.publish("UoP_CO_326_E18_Gr12_LM35", "Connected...");
      // ... and resubscribe
      client.subscribe("UoP_CO_326_E18_Gr12_LM35_Threshold");
      client.subscribe("UoP_CO_326_E18_Gr12_Fan_Switch");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


void setupMotor(){
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT); 
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
}

void setup() {
  Serial.begin(115200);
  setupMotor();
  setup_wifi();
  // Set MQTT broker and port
  client.setServer(mqtt_server, 1883);
  // Set callback function for incoming MQTT messages
  client.setCallback(callback);
  // Initialize a NTPClient to get time
  timeClient.begin();
  timeClient.setTimeOffset(0);
}

// temperature_output -> UoP_CO_326_E18_Gr12_LM35
// time_data -> UoP_CO_326_E18_Gr12_Time

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  // Maintain MQTT connection and handle incoming messages
  client.loop();
  unsigned long now = millis();
  // Publish a message every 2 seconds
  if (now - lastMsg > 2000) {
  lastMsg = now;
  analogValue = analogRead(outputpin);
  millivolts = (analogValue/1024.0) * 3300; //3300 is the voltage provided by NodeMCU
  celsius = millivolts/10;
  // getting time
  timeClient.update();
  time_t epochTime = timeClient.getEpochTime();
  Serial.print("Epoch Time: ");
  Serial.println(epochTime);
  // Create a message with a value that increments each time
  snprintf(msg, MSG_BUFFER_SIZE, "%f", celsius);
  Serial.print("Publish message: ");
  Serial.println(msg);
  // Publish the message to the topic "UoP_CO_326_E18_Gr12_LM35"
  char* timeData = ctime(&epochTime);
  client.publish("UoP_CO_326_E18_Gr12_Time",timeData);
  client.publish("UoP_CO_326_E18_Gr12_LM35_Output", msg);
  }
  if(scada_temperature<celsius && fanSwitch=='t'){
    startFan();
  }else{
    stopFan();
  }
}


void startFan(){
  analogWrite(ENA, 255);

  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
}


void stopFan(){
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
}
