
#include <Wire.h>
#include <Adafruit_SSD1306.h>
int sensorValue = 0;

//definisi koefisien alpha
float EMA_a_low = 0.3255;    //initialization of EMA alpha
float EMA_a_high = 0.8202;

//auto clean conditional for oled
bool ElectrodePlug = false;
int counterPlug = 0;

int EMA_S_low = 0;          //initialization of EMA S
int EMA_S_high = 0;

//setup oled
#define OLED_Address 0x3C
//mengganti clock I2C default dari 100Khz menjadi 800Khz, hal ini diperlukan agar pada proses sampling dari ADC tidak ada mengalami 'aliasing'
Adafruit_SSD1306 oled(128, 64, &Wire, -1, 800000, 800000);
int x = 0;
int yData = 0;

//untuk plot kontinu
int lastX = 0;
int lastY = 0;
int lastTime = 0;

//delay setting
int periode = 2; //delay per 30 milidetik
unsigned long time_now = 0;


int filterSignal(int analogValue) {

  //EXPONENTIAL MOVING AVERAGE
  EMA_S_low = (EMA_a_low * sensorValue) + ((1 - EMA_a_low) * EMA_S_low);
  EMA_S_high = (EMA_a_high * sensorValue) + ((1 - EMA_a_high) * EMA_S_high);

  int filteredSignal = EMA_S_high - EMA_S_low;      //find the band-pass

  return filteredSignal;
}

void setup() {
  oled.begin(SSD1306_SWITCHCAPVCC, OLED_Address);
  // initialize the serial communication:
  Serial.begin(115200);
  pinMode(D4, INPUT); // Setup for leads off detection LO +
  pinMode(D3, INPUT); // Setup for leads off detection LO -

  EMA_S_low = analogRead(A0);      //set EMA S for t=1
  EMA_S_high = analogRead(A0);

  oled.clearDisplay();
  oled.setTextColor(WHITE);
  oled.display();

}

void loop() {



  if ((digitalRead(D4) == 1) || (digitalRead(D3) == 1)) {
    Serial.println('!');
    oled.clearDisplay();
    oled.setTextColor(WHITE);
    oled.setCursor(30, 0);
    oled.println("ELEKTRODA");
    oled.println("TIDAK TERPASANG");

    ElectrodePlug = false;
  }
  else {

    ElectrodePlug = true;

    if (ElectrodePlug == 1) {
      counterPlug++;
    }
    if (ElectrodePlug == 1 && counterPlug > 5) {
      if (x > 127) {

        oled.clearDisplay();
        x = 0;
        lastX = 0;


      }


      sensorValue = analogRead(A0);    //read the sensor value using ADC
      int filteredSignal = filterSignal(sensorValue);
      Serial.println(filteredSignal);
      yData = 32 - (filteredSignal / 16);





    }
  }
  delay(10);

  if (millis() > time_now + periode) {
    oled.writeLine(lastX, lastY, x, yData, WHITE);


    lastY = yData;
    lastX = x;

    x++;
    oled.display();
    time_now = millis();
  }


}
