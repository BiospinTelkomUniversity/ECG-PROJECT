#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>


#include <ESP8266WiFi.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>


#define NAMA_AP ""
#define PASSWD ""


WiFiClient client;

int sensorValue = 0;
int filteredSignal = 0;

//Reserve for adjusting flow program
bool ElectrodePlug = false;
bool lastPlug = false;

int EMA_S_low = 0;          //initialization of EMA S
int EMA_S_high = 0;

//setup ole
#define OLED_Address 0x3C

//custom clock speed, increase speed refresh rate
Adafruit_SSD1306 oled(128, 32, &Wire, -1, 800000, 800000);
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
unsigned long time_now3 = 0; //time now untuk mqtt


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


  /*WIFI INITIALIZATION*/
  //  WiFi.begin(NAMA_AP, PASSWD);
  //  while (WiFi.status() != WL_CONNECTED) {
  //    delay(500);
  //  }





}

void loop() {

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
    //    if (millis() > time_now3 + periode_mqtt) {
    //      MQTT_connect();
    //      if (!dataSinyal.publish(yData )) {
    //        Serial.println("Failed");
    //      } else {
    //        Serial.println("OK");
    //      }
    //      time_now3 = millis();
    //    }
    if (millis() > time_now2 + periode) {
      time_now2 = millis();
      sensorValue = analogRead(A0);    //read the sensor value using ADC
      filteredSignal = BandStopFilter(sensorValue);
      filteredSignal = HighPassFilter(filteredSignal );
      filteredSignal = BandPassFilter(filteredSignal);
      filteredSignal = LowPassFilter(filteredSignal);

    }

    if (millis() > time_now + periode) {

      yData = 20 - (filteredSignal / 32);
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
