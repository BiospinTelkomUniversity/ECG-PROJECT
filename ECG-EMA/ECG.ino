int sensorValue = 0;

//definisi koefisien alpha

// menggunakan filter pasif
//float EMA_a_low = 0.34;    //initialization of EMA alpha
//float EMA_a_high = 0.55;

// tanpa filter pasif
float EMA_a_low = 0.3455;    //initialization of EMA alpha
float EMA_a_high = 0.8282;


int EMA_S_low = 0;          //initialization of EMA S
int EMA_S_high = 0;

int highpass = 0;
int bandpass = 0;

void setup() {
  // initialize the serial communication:
  Serial.begin(115200);
  pinMode(D4, INPUT); // Setup for leads off detection LO +
  pinMode(D3, INPUT); // Setup for leads off detection LO -

  EMA_S_low = analogRead(A0);      //set EMA S for t=1
  EMA_S_high = analogRead(A0);

}

void loop() {

  if ((digitalRead(D4) == 1) || (digitalRead(D3) == 1)) {
    Serial.println('!');
  }
  else {
    sensorValue = analogRead(A0);    //read the sensor value using ADC
    EMA_S_low = (EMA_a_low * sensorValue) + ((1 - EMA_a_low) * EMA_S_low); //run the EMA
    EMA_S_high = (EMA_a_high * sensorValue) + ((1 - EMA_a_high) * EMA_S_high);

    highpass = sensorValue - EMA_S_low;     //find the high-pass as before (for comparison)
    bandpass = EMA_S_high - EMA_S_low;      //find the band-pass

    //    Serial.print(highpass);
    //    Serial.print(" ");
    Serial.println(bandpass);
  }
  //Wait for a bit to keep serial data from saturating
  //  delay(10);
  delay(10);
}
