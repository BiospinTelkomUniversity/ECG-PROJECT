#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>

#define NAMA_AP "lab104"
#define PASSWD "enaksekali"

//delay setting
int periode = 10; //delay per 10 milidetik sampling rate=100Hz
int periode_mqtt = 10; //per 10 milidetik,mengirim data sebanyak data bufferECG
int periode_flag = 500; //per 500 milidetik,untuk mengecek apakah hentikan mengirim data atau tida


//electrode indicator
bool ElectrodePlug = false;
bool lastPlug = false;

//sampling rate setting
unsigned long time_now = 0; //sampling timer
unsigned long time_now2 = 0; //filter timer
unsigned long time_now3 = 0; //untuk send data ke broker mqtt
unsigned long time_now4 = 0; //subscribe topic mqtt


/*INITIALIZATION MQTT*/
const char* mqttUser = "biospin";
const char* mqttPassword = "biospin";
const char* mqtt_server = "raspberrypi";
const int mqttPort = 1883;
const char* hardwareTarget = "hardware1";
bool stateSend = false;

WiFiClient espclient;
PubSubClient mqttHardware(espclient);

void reconnect() {
  // Loop until we're reconnected
  while (!mqttHardware.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (mqttHardware.connect(clientId.c_str())) {
      mqttHardware.subscribe("stopFlag");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttHardware.state());
      Serial.println(" try again in 5 seconds");

      // Wait 5 seconds before retrying
      unsigned long interval_now = millis();
      while (millis() > interval_now + 5000);
    }
  }
}

void callbackSubs(String topic, byte* message, unsigned int length) {
  String messageTemp;
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }

  if (topic == "stopFlag") {

    //jika true maka berhenti mengirim data ke broker
    if (messageTemp == "true") {
      stateSend = true;
    }
    //jika false, tetap mengirim data ke broker
    else if (messageTemp == "false") {
      stateSend = false;
    }
  }
}


void publishECGData(int buffer) {

  mqttHardware.publish(hardwareTarget, String(buffer).c_str());
}

/*END*/


/* SIGNAL PROCESSING ECG */
float SignalNow = 0; //K+1
float SignalLast = 0; //K
bool isFuture = false; //maksud dari variable ini adalah state nya apakah sudah K+1 atau masih K, jika false jangan pindahkan data ke signalLast

float filteredSignal = 0;
bool qrsDone = false; // menandakan satu cycle sinyal QRS sudah selesai atau belum

float voltageValue = 0.0; //untuk ditampilkan di OLED


//setup ole
#define OLED_Address 0x3C

//custom clock speed, increase speed refresh rate
Adafruit_SSD1306 oled(128, 32, &Wire, -1, 800000, 800000);
int x = 0;
int yData = 0;
int yLastData = 0; // for differential purposes(to compare n data and n-1 data

//untuk plot kontinu
int lastX = 0;
int lastY = 0;
int lastTime = 0;


float differensial(float diffSignal) {
  return ((1 * diffSignal) / (2 * 100));
}

int getBPM(float input) {
  return int(60 / (float(input) / 1000));
}

float convertToVoltage(int analogValue) {
  return (analogValue * ( 1.5 / 1024));
}


/*Bandstop Filter
  Fc=50Hz
   Ordo = 3
*/

#define NZEROS_BSF 6
#define NPOLES_BSF 6
#define GAIN_BSF   1.000000023e+00

static float xvBSF[NZEROS_BSF + 1], yvBSF[NPOLES_BSF + 1];

float BandStopFilter(float analogValue) {

  xvBSF[0] = xvBSF[1]; xvBSF[1] = xvBSF[2]; xvBSF[2] = xvBSF[3]; xvBSF[3] = xvBSF[4]; xvBSF[4] = xvBSF[5]; xvBSF[5] = xvBSF[6];
  xvBSF[6] = analogValue / GAIN_BSF;
  yvBSF[0] = yvBSF[1]; yvBSF[1] = yvBSF[2]; yvBSF[2] = yvBSF[3]; yvBSF[3] = yvBSF[4]; yvBSF[4] = yvBSF[5]; yvBSF[5] = yvBSF[6];
  yvBSF[6] =   (xvBSF[0] + xvBSF[6]) +   5.9881603706 * (xvBSF[1] + xvBSF[5]) +  14.9526882080 * (xvBSF[2] + xvBSF[4])
               +  19.9290556130 * xvBSF[3]
               + ( -1.0000000000 * yvBSF[0]) + ( -5.9881603706 * yvBSF[1])
               + (-14.9526882080 * yvBSF[2]) + (-19.9290556130 * yvBSF[3])
               + (-14.9526882080 * yvBSF[4]) + ( -5.9881603706 * yvBSF[5]);
  return yvBSF[6];
}


//BUTTERWORTH FILTER CONSTANT
// Fc1 =0.5Hz Fc2=35Hz
// jumlah ordo = 5
#define NZEROS_BPF 8
#define NPOLES_BPF 8
#define GAIN   3.807081066e+00

static float xv[NZEROS_BPF + 1], yv[NPOLES_BPF + 1];

float BandPassFilter(float analogValue) {
  xv[0] = xv[1]; xv[1] = xv[2]; xv[2] = xv[3]; xv[3] = xv[4]; xv[4] = xv[5]; xv[5] = xv[6]; xv[6] = xv[7]; xv[7] = xv[8];
  xv[8] = analogValue / GAIN;
  yv[0] = yv[1]; yv[1] = yv[2]; yv[2] = yv[3]; yv[3] = yv[4]; yv[4] = yv[5]; yv[5] = yv[6]; yv[6] = yv[7]; yv[7] = yv[8];
  yv[8] =   (xv[0] + xv[8]) - 4 * (xv[2] + xv[6]) + 6 * xv[4]
            + ( -0.0694168803 * yv[0]) + ( -0.1573294523 * yv[1])
            + (  0.1988344065 * yv[2]) + (  0.8672882976 * yv[3])
            + ( -0.5531263498 * yv[4]) + ( -0.7286735628 * yv[5])
            + ( -0.9163679567 * yv[6]) + (  2.3587872690 * yv[7]);
  return yv[8];
}
/*
   HIGHPASS FILTER
  Ordo =1
  Fc = 0.5 Hz

*/

#define NZEROS_HPF 2
#define NPOLES_HPF 2
#define GAIN_HPF   1.052651765e+00

static float xvHPF[NZEROS_HPF + 1], yvHPF[NPOLES_HPF + 1];

float HighPassFilter(int analogValue) {
  xvHPF[0] = xvHPF[1]; xvHPF[1] = xvHPF[2];
  xvHPF[2] = analogValue / GAIN_HPF;
  yvHPF[0] = yvHPF[1]; yvHPF[1] = yvHPF[2];
  yvHPF[2] =   (xvHPF[0] + xvHPF[2]) - 2 * xvHPF[1]
               + ( -0.9565436765 * yvHPF[0]) + (  1.9555782403 * yvHPF[1]);
  return yvHPF[2];
}


/*LPF
  Fc : 35
  Ordo :  6
*/
#define NZEROS_LPF 6
#define NPOLES_LPF 6
#define GAIN_LPF  6.768991272e+00

static float xvLPF[NZEROS_LPF + 1], yvLPF[NPOLES_LPF + 1];

int LowPassFilter(float analogValue) {
  xvLPF[0] = xvLPF[1]; xvLPF[1] = xvLPF[2]; xvLPF[2] = xvLPF[3]; xvLPF[3] = xvLPF[4]; xvLPF[4] = xvLPF[5]; xvLPF[5] = xvLPF[6];
  xvLPF[6] = analogValue / GAIN;
  yv[0] = yvLPF[1]; yvLPF[1] = yvLPF[2]; yvLPF[2] = yvLPF[3]; yvLPF[3] = yvLPF[4]; yvLPF[4] = yvLPF[5]; yvLPF[5] = yvLPF[6];
  yvLPF[6] =   (xvLPF[0] + xvLPF[6]) + 6 * (xvLPF[1] + xvLPF[5]) + 15 * (xvLPF[2] + xvLPF[4])
               + 20 * xvLPF[3]
               + ( -0.0218315740 * yvLPF[0]) + ( -0.2098654504 * yvLPF[1])
               + ( -0.8779238976 * yvLPF[2]) + ( -2.0551314368 * yvLPF[3])
               + ( -2.9104065679 * yvLPF[4]) + ( -2.3797210446 * yvLPF[5]);
  return yvLPF[6];
}

/*END CODE SIGNAL PROCESSING*/

bool isAttach() {
  if ((digitalRead(D5) == 1 && digitalRead(D6) == 1)) {
    return false;
  } else {
    return true;
  }
}



void setup() {

  // initialize the serial communication:
  Serial.begin(115200);
  pinMode(D5, INPUT); // Setup for leads off detection LO +
  pinMode(D6, INPUT); // Setup for leads off detection LO -

  oled.begin(SSD1306_SWITCHCAPVCC, OLED_Address);
  oled.setCursor(0, 0);
  oled.clearDisplay();
  oled.setTextColor(WHITE);
  oled.display();
  delay(20);

  oled.println(hardwareTarget);
  oled.println("Connecting Wifi...");
  oled.display();

  /*WIFI INITIALIZATION*/
  WiFi.begin(NAMA_AP, PASSWD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  oled.setCursor(0, 0);
  oled.clearDisplay();
  oled.setTextColor(WHITE);

  oled.println(hardwareTarget);
  oled.println("Connecting MQTT...");
  oled.display();
  delay(20);

  mqttHardware.setServer(mqtt_server, mqttPort);
  mqttHardware.setCallback(callbackSubs);

  oled.setCursor(0, 0);
  oled.clearDisplay();
  oled.setTextColor(WHITE);

  oled.setCursor(11, 10);
  oled.println("MOHON PASANG");
  oled.setCursor(20, 19);
  oled.println("ELEKTRODA!");
  oled.display();
  delay(20);
  while (isAttach() != 1) {
    delay(500);
  }
}

void loop() {

  //    if (!mqttHardware.connected()) {
  //      reconnect();
  //    }
  //    mqttHardware.loop();


  if (isAttach() == 0) {
    oled.setRotation(0);
    oled.clearDisplay();
    oled.setTextColor(WHITE);
    oled.setCursor(29, 10);
    oled.println("ELEKTRODA");
    oled.setCursor(15, 20);
    oled.println("TIDAK TERPASANG!");
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
      int sensorValue = analogRead(A0);    //read the sensor value using ADC

      filteredSignal = BandStopFilter(sensorValue);
      filteredSignal = HighPassFilter(filteredSignal );
      filteredSignal = BandPassFilter(filteredSignal);
      filteredSignal = LowPassFilter(filteredSignal);


    }
    if (millis() > time_now3 + periode_mqtt && stateSend == 1) {
      time_now3 = millis();
      publishECGData(filteredSignal);


    }

    if (millis() > time_now + periode) {

      //scaling data to OLED
      yData = 20 - (filteredSignal / 32);
      // do differentiation
      /**
        TO DO :
        1. Bikin proses adaptive threshold
        2. lakukan deteksi interval r
        3. hitung bpm
      */
      if (filteredSignal != yLastData) {
        //        String logdat = "Before : " + String(filteredSignal) + "\t" + "After : " + String(yLastData);
        float signalDiff = differensial( filteredSignal - yLastData);
        yLastData = filteredSignal;
        Serial.println(signalDiff);


      }


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
