#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>

const char* ssid = "barbar";
const char* password = "amengaja";

const char* mqttServerIP = "192.168.1.109";
const int mqttPort = 1883;

WiFiClient espClient;
PubSubClient client(espClient);



int pinled = 4;

//define var
String datasensor;

//define topic
char* Topic = "Test/datasensor"; // publish

void reconnect(){
  // MQTT Begin
  while(!client.connected()){
    Serial.println("Connecting to MQTT Server..");
    Serial.print("IP MQTT Server : "); Serial.println(mqttServerIP);

    bool hasConnection = client.connect("kans");
    if(hasConnection){
      Serial.println("Success connected to MQTT Broker");
    } else {
      Serial.print("Failed connected");
      Serial.println(client.state());
      digitalWrite(pinled,HIGH);
      delay(2000);
      Serial.println("Try to connect...");
    }
  }
  client.publish(Topic, "Reconnecting");
}

void callback(char* topic, byte* payload, unsigned int length){
  Serial.println("--------");
  Serial.println("Message Arrived");
  Serial.print("Topic :"); Serial.println(topic);
  Serial.print("Message : ");
  String pesan = "";
  for(int i=0; i < length; i++){
    Serial.print((char)payload[i]);

    pesan += (char)payload[i];

  }
     //if (topic == "esp/cmd") {
       if(pesan == "ON" ){
          Serial.println("LED ON.. Warning");
          digitalWrite(4, HIGH);
  
       } else if(pesan == "OFF"){
          Serial.println("LED OFF.. Safety");
          digitalWrite(4,LOW);
       }
       Serial.println("ESP/CMD topic arrived");
       Serial.println(pesan);
     //}
  Serial.print("Pesan masuk :");
  Serial.println(pesan);
  Serial.println("--------");
}

void setup() {
  Serial.begin(57600);
  pinMode(pinled, OUTPUT);
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi connected - ESP IP address: ");
  Serial.println(WiFi.localIP());

  client.setServer(mqttServerIP, mqttPort);
  client.setCallback(callback);
  
  delay(20);
}

char dataPublish[50];
void publishMQTT(char* topics, String data){
   data.toCharArray(dataPublish, data.length() + 1);
   client.publish(topics, dataPublish);
}

unsigned long startMillis;  //some global variables available anywhere in the program
unsigned long currentMillis;
const unsigned long period = 5000;  //the value is a number of milliseconds

unsigned long startMilliss;  //some global variables available anywhere in the program
unsigned long currentMilliss;
const unsigned long periods = 50000;  //the value is a number of milliseconds

int aa, bb = 85;

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  if(!client.loop()){
    client.connect("ESP8266Client");
  }

  aa = random(1880,2400);
  currentMillis = millis();  //get the current "time" (actually the number of milliseconds since the program started)
  if (currentMillis - startMillis >= period)  //test whether the period has elapsed
  {
    aa = random(4000,4990);
    startMillis = currentMillis;  //IMPORTANT to save the start time of the current LED state.
  }
  currentMilliss = millis();  //get the current "time" (actually the number of milliseconds since the program started)
  if (currentMilliss - startMilliss >= periods)  //test whether the period has elapsed
  {
    bb = random(85,89);
    startMilliss = currentMilliss;  //IMPORTANT to save the start time of the current LED state.
  }

  datasensor = String(aa)+","+String(bb);
  publishMQTT(Topic,datasensor);
    
  Serial.print(aa);
  Serial.print(", ");
  Serial.println(bb);
  delay(200);
}
