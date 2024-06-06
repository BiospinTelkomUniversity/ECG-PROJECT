#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>


#define FIREBASE_HOST "pengmas-ekg.firebaseio.com"
#define FIREBASE_AUTH "4mkZwqFND27ZoFf7DUlvCAppYXm1eOR1OCq7pwth"

#define NAMA_AP ""
#define PASSWD ""


//MQTT Setup
const char* mqttServer = "m11.cloudmqtt.com";
const int mqttPort = 12948;
const char* mqttUser = "YourMqttUser";
const char* mqttPassword = "YourMqttUserPassword";

//WiFiClient espClient;
//PubSubClient client(espClient);

int sensorValue = 0;
int filteredSignal = 0;
//definisi koefisien alpha
float EMA_a_low = 0.6555;    //initialization of EMA alpha
float EMA_a_high = 0.8202;

//Reserve for adjusting flow program
bool ElectrodePlug = false;
bool lastPlug = false;

int EMA_S_low = 0;          //initialization of EMA S
int EMA_S_high = 0;

//setup oled
#define OLED_Address 0x3C

//custom clock speed, increase speed refresh rate
Adafruit_SSD1306 oled(128, 64, &Wire, -1, 800000, 800000);
int x = 0;
int yData = 0;

//untuk plot kontinu
int lastX = 0;
int lastY = 0;
int lastTime = 0;

//delay setting
int periode = 10; //delay per 10 milidetik sampling rate=100Hz
unsigned long time_now = 0;
unsigned long time_now2 = 0;

int filterSignal(int analogValue) {

  //EXPONENTIAL MOVING AVERAGE
  EMA_S_low = (EMA_a_low * sensorValue) + ((1 - EMA_a_low) * EMA_S_low);
  EMA_S_high = (EMA_a_high * sensorValue) + ((1 - EMA_a_high) * EMA_S_high);

  int filteredSignal = EMA_S_high - EMA_S_low;      //find the band-pass

  return filteredSignal;
}

bool isAttach() {
  if ((digitalRead(D5) == 1 && digitalRead(D6) == 1)) {
    return false;
  } else {
    return true;
  }
}

void setup() {


  oled.begin(SSD1306_SWITCHCAPVCC, OLED_Address);
  oled.setCursor(0, 0);
  oled.clearDisplay();
  oled.setTextColor(WHITE);
  oled.display();
  delay(20);


  /*WIFI INITIALIZATION*/
//  WiFi.begin(NAMA_AP, PASSWD);
//  while (WiFi.status() != WL_CONNECTED) {
//    delay(500);
//  }

  /*MQTT INITIALIZATION*/
//  while (!client.connected()) {
//    Serial.println("Connecting to MQTT...");
//
//    if (client.connect("ESP8266Client", mqttUser, mqttPassword )) {
//
//      Serial.println("connected");
//
//    } else {
//
//      Serial.print("failed with state ");
//      Serial.print(client.state());
//      delay(2000);
//
//    }
//  }

  // initialize the serial communication:
  Serial.begin(115200);
  pinMode(D5, INPUT); // Setup for leads off detection LO +
  pinMode(D6, INPUT); // Setup for leads off detection LO -

  EMA_S_low = analogRead(A0);      //set EMA S for t=1
  EMA_S_high = analogRead(A0);

}

void loop() {

  if (isAttach() == 0) {
    oled.setRotation(2);
    oled.clearDisplay();
    oled.setTextColor(WHITE);
    oled.setCursor(30, 20);
    oled.println("ELEKTRODA");
    oled.setCursor(15, 35);
    oled.println("TIDAK TERPASANG");
    oled.display();
    lastPlug = 0;

  } else if (isAttach() == 1) {


    oled.setRotation(0);

    // kasus tengah jalan keputus
    if (isAttach() == 1 && lastPlug == 0) {
      oled.clearDisplay();
      x = 0;
      lastX = 0;

    }
    //kondisi overflow, perlu di reset lagi
    else if (x > 128) {

      oled.clearDisplay();
      x = 0;
      lastX = 0;
    }
    if (millis() > time_now2 + periode) {
      time_now2 = millis();
      sensorValue = analogRead(A0);    //read the sensor value using ADC
      filteredSignal = filterSignal(sensorValue);
    }

    if (millis() > time_now + periode) {
      yData = 16 - (filteredSignal / 6);
      oled.writeLine(lastX, lastY, x, yData, WHITE);

      lastY = yData;
      lastX = x;

      x++;
      time_now = millis();
      oled.display();

    }
    lastPlug = 1;
  }





}
