// Revision 1.01  
// Arduino 1.6.13

#include "Arduino.h"
#include <NewRemoteTransmitter.h>   // from: https://bitbucket.org/fuzzillogic/433mhzforarduino/src
#include "PinChangeInt.h"           // from: http://code.google.com/p/arduino-pinchangeint/
#include <avr/sleep.h>
#include "LowPower.h"		            // from: https://github.com/rocketscream/Low-Power


/*
 Brown-out must be disabled in bootloader.
 Change in boards.txt:  
 change: pro.menu.cpu.8MHzatmega328.bootloader.extended_fuses=0x05
 in: pro.menu.cpu.8MHzatmega328.bootloader.extended_fuses=0xFF
 reprogram bootloader. You can use an UNO board for reprogramming the Mini Pro

 Program Examples=> ArduinoISP in the UNO
 Connect pins:
 
 UNO    Mini PRO
 ===============
 5V     VCC
 GND    GND
 10     RESET
 11     11
 12     12
 13     13

Tools => Burn Bootloader
 
*/


// Fill in an unique random number:
#define TRANSMITTER_ADDRESS     130

#define DEBUG 1
#define WAIT_TIME_TRANSMITTER   100
#define WAIT_TIME_RFLINK        750
#define WAIT_TIME_NEXT          150
#define LOOP_TIME               50

#define ENCODER_PIN_A           2
#define ENCODER_PIN_B           5
#define ENCODER_PRESS_PIN       3
#define TX_POWER_PIN            9
#define TX_DATA_PIN             11
#define TX_PERIOD_DURATION      260
#define TX_RETRANSMIT           3
#define LED_PIN                 13


volatile int  EncoderTicksLeft=0;
volatile int  EncoderTicksRight=0;
volatile int  EncoderPressed=0;
int DimmerValue = 10;
int OldDimmerValue = 10;
int repeatcounter = WAIT_TIME_TRANSMITTER;


// Create a transmitter on address TRANSMITTER_ADDRESS, using digital pin TX_DATA_PIN to transmit, 
// with a period duration of TX_PERIOD_DURATION ms (260 ms default), repeating the transmitted
// code 2^TX_RETRANSMIT (2^3 =  8 times)
NewRemoteTransmitter transmitter(TRANSMITTER_ADDRESS, TX_DATA_PIN, TX_PERIOD_DURATION, TX_RETRANSMIT);



void setup() {
  if (DEBUG == 1) {
    Serial.begin(9600);
    Serial.write("Starting serial debug\n");
  }

  unsigned int resetvalue = MCUSR;
  MCUSR = 0;

if (DEBUG == 1) {
  if (resetvalue & 0x01)
    Serial.write("Power-on Reset\n");
  if (resetvalue & 0x02)
    Serial.write("External Reset\n");
  if (resetvalue & 0x04)
    Serial.write("Brownout Reset\n");
  if (resetvalue & 0x08)
    Serial.write("Watchdog Reset\n");
}    

  
  
  pinMode(ENCODER_PIN_A, INPUT_PULLUP);        // Sets Encoder pin A as input
  pinMode(ENCODER_PIN_B, INPUT_PULLUP);        // Sets Encoder pin  B as input
  pinMode(ENCODER_PRESS_PIN, INPUT_PULLUP);    // Sets Encoder press pin as input
  pinMode(TX_POWER_PIN, OUTPUT);               // Sets Transmitter power pin as output
  pinMode(LED_PIN, OUTPUT);                    // Sets LED pin as output
  digitalWrite(TX_POWER_PIN, LOW);             // Do not power Transmitter
  digitalWrite(LED_PIN, LOW);                   // LED Off
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_A), ExtInterrupt, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_PRESS_PIN), PressInterrupt, FALLING);
  PCintPort::attachInterrupt(ENCODER_PIN_B, &PinChangeInterrupt, CHANGE);
  
}
int number = 1;
void loop() {
  delay(LOOP_TIME);
  if (DEBUG == 1) {
    Serial.print(".");
  }
  
  
  
  if (EncoderPressed > 0) {
      if (DEBUG) {
         Serial.println("Sending light off");
      }
    digitalWrite(TX_POWER_PIN, HIGH);      // Power up transmitter
    digitalWrite(LED_PIN, HIGH);           // LED On
    delay(WAIT_TIME_TRANSMITTER);
    transmitter.sendUnit(1, false);
    DimmerValue = 0;
    digitalWrite(TX_POWER_PIN, LOW);          // Power off transmitter
    digitalWrite(LED_PIN, LOW);               // LED Off
    EncoderPressed = 0;
    delay(WAIT_TIME_NEXT);
    EncoderTicksRight = 0;
    EncoderTicksLeft = 0;
  }
  
  // increase light
  while (EncoderTicksRight > 0) {
    if (DimmerValue < 16) {
      DimmerValue += 1;
    }
    else {
      OldDimmerValue -=1;                    // send command again
    }
    EncoderTicksRight -= 1;
    
  }
  while (EncoderTicksLeft > 0) {
    if (DimmerValue > 0) {
      DimmerValue -= 1;
    }
    else {
        OldDimmerValue +=1;                   // send command again
    }
    EncoderTicksLeft -= 1;
  }
  
  if (DimmerValue != OldDimmerValue) {
    if (DimmerValue > 0 ) {
      digitalWrite(TX_POWER_PIN, HIGH);       // Power up transmitter
      digitalWrite(LED_PIN, HIGH);            // LED On
      delay(WAIT_TIME_TRANSMITTER);
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
      digitalWrite(TX_POWER_PIN, HIGH);        // Power up transmitter
      digitalWrite(LED_PIN, HIGH);             // LED On
      delay(WAIT_TIME_TRANSMITTER);
      transmitter.sendUnit(1, false);
    }
    OldDimmerValue = DimmerValue;
    digitalWrite(TX_POWER_PIN, LOW);          // Power off transmitter
    digitalWrite(LED_PIN, LOW);               // LED Off
    delay(WAIT_TIME_NEXT);                    // wait before sending new value.
  }

    
  repeatcounter = WAIT_TIME_RFLINK;
  
  // Send command again to RFlink (need long pause) and go to sleep when noting is happening
  while ( (OldDimmerValue == DimmerValue) &&
       (EncoderPressed == 0)&&
       (EncoderTicksRight == 0)&&
       (EncoderTicksLeft == 0)&&
       (--repeatcounter > 0)
       ) {

    delay(1);  // wait before sending value again
  }
  
  if (repeatcounter == 0){                   // send last command again
    if (DimmerValue > 0 ) {
      digitalWrite(TX_POWER_PIN, HIGH);      // Power up transmitter
      digitalWrite(LED_PIN, HIGH);           // LED On
      delay(WAIT_TIME_TRANSMITTER);
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
      digitalWrite(TX_POWER_PIN, HIGH);          // Power up transmitter
      digitalWrite(LED_PIN, HIGH);               // LED On
      delay(WAIT_TIME_TRANSMITTER);
      transmitter.sendUnit(1, false);
    }
    digitalWrite(TX_POWER_PIN, LOW);             // Power off transmitter
    digitalWrite(LED_PIN, LOW);                  // LED Off
  
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    cli();
    sleep_enable();
    sei();
    
    LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
    sleep_cpu();

    // ******  SLEEP *************
    
    /* wake up here */
    sleep_disable();
    
  }
}



// Encoder interrupt routines. 
void ExtInterrupt() {
 int state = digitalRead(ENCODER_PIN_B) & 0x01;
  state += (digitalRead(ENCODER_PIN_A) << 1) & 0x02;
  if (state == 3)
    EncoderTicksRight += 1;
}


void PinChangeInterrupt() {
  int state = digitalRead(ENCODER_PIN_B) & 0x01;
  state += (digitalRead(ENCODER_PIN_A) << 1) & 0x02;
  if (state == 3)
    EncoderTicksLeft += 1;
}


void PressInterrupt(){
  delay(100);
  if (digitalRead(ENCODER_PRESS_PIN) == 0) {
    EncoderPressed += 1;
  }
  
}






