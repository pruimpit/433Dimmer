#include "Arduino.h"
#include <NewRemoteTransmitter.h>   // from: https://bitbucket.org/fuzzillogic/433mhzforarduino/src
#include "PinChangeInt.h"           // from: http://code.google.com/p/arduino-pinchangeint/
#include <avr/sleep.h>
#include "LowPower.h"		            // from: https://github.com/rocketscream/Low-Power


// Create a transmitter on address 128, using digital pin 11 to transmit, 
// with a period duration of 260ms (default), repeating the transmitted
// code 2^3=8 times.
NewRemoteTransmitter transmitter(128, 11, 260, 3);

#define DEBUG 0

#define EncoderPinA 2
#define EncoderPinB 5
#define EncoderPress 3
#define TxPower 9
#define LEDPin  13


volatile int  EncoderTicksLeft=0;
volatile int  EncoderTicksRight=0;
volatile int  EncoderPressed=0;
int DimmerValue = 10;
int OldDimmerValue = 10;

void setup() {
  if (DEBUG == 1) {
    Serial.begin(9600);
    Serial.write("Starting serial debug");
  }
  pinMode(EncoderPinA, INPUT_PULLUP);        // Sets Encoder pin A as input
  pinMode(EncoderPinB, INPUT_PULLUP);        // Sets Encoder pin  B as input
  pinMode(EncoderPress, INPUT_PULLUP);       // Sets Encoder press pin as input
  pinMode(TxPower, OUTPUT);                  // Sets Transmitter power pin as output
  pinMode(LEDPin, OUTPUT);                   // Sets LED pin as output
  digitalWrite(TxPower, LOW);                // Do not power Transmitter
  digitalWrite(LEDPin, LOW);                 // LED Off
  attachInterrupt(digitalPinToInterrupt(EncoderPinA), ExtInterrupt, CHANGE);
  attachInterrupt(digitalPinToInterrupt(EncoderPress), PressInterrupt, FALLING);
  PCintPort::attachInterrupt(EncoderPinB, &PinChangeInterrupt, CHANGE);
  
}
int number = 1;
void loop() {
  delay(100);
  if (DEBUG == 1) {
    Serial.print(".");
  }
  if (EncoderPressed > 0) {
    if  (DimmerValue > 0) {	

      if (DEBUG) {
         Serial.println("Sending light off");
      }
      digitalWrite(TxPower, HIGH);          // Power up transmitter
      digitalWrite(LEDPin, HIGH);           // LED On
      delay(200);
      transmitter.sendUnit(1, false);
      DimmerValue = 0;
      OldDimmerValue = 0;
    }
    else {
      if (DEBUG) {
         Serial.println("Sending light on");
      }
      digitalWrite(TxPower, HIGH);          // Power up transmitter
      digitalWrite(LEDPin, HIGH);           // LED On
      delay(200);
      DimmerValue = 15;
      OldDimmerValue = 15;
      transmitter.sendDim(1, DimmerValue-1);
      Serial.print(DimmerValue-1);
   }
   digitalWrite(TxPower, LOW);          // Power off transmitter
   digitalWrite(LEDPin, LOW);           // LED Off
   EncoderPressed = 0;
   delay(750);
   EncoderTicksRight = 0;
   EncoderTicksLeft = 0;
  }
  
  // increase light
  while (EncoderTicksRight > 0) {
    if (DimmerValue < 16) {
      DimmerValue += 1;
    }
    EncoderTicksRight -= 1;
  }
  while (EncoderTicksLeft > 0) {
    if (DimmerValue > 0) {
      DimmerValue -= 1;
    }
    EncoderTicksLeft -= 1;
  }
  
  if (DimmerValue != OldDimmerValue) {
    if (DimmerValue > 0 ) {
      digitalWrite(TxPower, HIGH);          // Power up transmitter
      digitalWrite(LEDPin, HIGH);                 // LED On
      delay(200);
      transmitter.sendDim(1, DimmerValue-1);
      if (DEBUG == 1) {
        Serial.print("Sending Dim: ");
        Serial.print(DimmerValue-1);
        Serial.print("\n");
      }
    }
    else {
      if (DEBUG == 1) {
        Serial.println("Sending light off");
      }
      transmitter.sendUnit(1, false);
    }
    OldDimmerValue = DimmerValue;
    digitalWrite(TxPower, LOW);          // Power off transmitter
    digitalWrite(LEDPin, LOW);           // LED Off
    delay(750); // wait 500 ms before sending new value.
  }
  // Go to sleep when noting is happening
  if ( (OldDimmerValue = DimmerValue) &&
       (digitalRead(EncoderPinA) == 1)&&
       (digitalRead(EncoderPinB) == 1)&&
       (digitalRead(EncoderPress) == 1)&&
       (EncoderTicksRight == 0)&&
       (EncoderTicksLeft == 0)) {

    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    cli();
    sleep_enable();
    sei();
    
    LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
    sleep_cpu();

    // ******  SLEEP *************
    
    /* wake up here */
    sleep_disable();
    //pinMode(TxPower, OUTPUT);                  // Sets Transmitter power pin as output
    //pinMode(LEDPin, OUTPUT);                   // Sets LED pin as output
  }
}

void ExtInterrupt() {
 int state = digitalRead(EncoderPinB) & 0x01;
  state += (digitalRead(EncoderPinA) << 1) & 0x02;
  if (state == 3)
    EncoderTicksRight += 1;
}


void PinChangeInterrupt() {
  int state = digitalRead(EncoderPinB) & 0x01;
  state += (digitalRead(EncoderPinA) << 1) & 0x02;
  if (state == 3)
    EncoderTicksLeft += 1;
}


void PressInterrupt(){
  delay(100);
  if (digitalRead(EncoderPress) == 0) {
    EncoderPressed += 1;
  }
  
}






