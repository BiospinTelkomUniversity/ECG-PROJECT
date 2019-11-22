#include <FirebaseArduino.h>
#include <Firebase.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>

int sensorValue = 0;
int filteredSignal = 0;
//definisi koefisien alpha
float EMA_a_low = 0.5855;    //initialization of EMA alpha
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
int periode = 15; //delay per 15 milidetik
unsigned long time_now = 0;


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
  // initialize the serial communication:
  Serial.begin(115200);
  pinMode(D5, INPUT); // Setup for leads off detection LO +
  pinMode(D6, INPUT); // Setup for leads off detection LO -

  EMA_S_low = analogRead(A0);      //set EMA S for t=1
  EMA_S_high = analogRead(A0);
  oled.setCursor(0, 0);
  oled.clearDisplay();
  oled.setTextColor(WHITE);
  oled.display();
  delay(20);

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

    sensorValue = analogRead(A0);    //read the sensor value using ADC

    delay(20);
    filteredSignal = filterSignal(sensorValue);
    if (millis() > time_now + periode) {
      yData = 16 - (filteredSignal / 5);
      oled.writeLine(lastX, lastY, x, yData, WHITE);
      Serial.println(yData);

      lastY = yData;
      lastX = x;

      x++;
      time_now = millis();
      oled.display();
    }
    lastPlug = 1;
  }





}
