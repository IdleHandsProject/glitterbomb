/*
 * GLITTER BOMB FIRMWARE
 * 
 * 
 * See its use: https://youtu.be/xoxhDk-hwuo
 * See how it was made: https://youtu.be/IpMxOmUcfOI
 * 
 * ALLOWS CONTROL OF ANDROID PHONES THROUGH THE HEADPHONE JACK
 * LOW POWER MODE WAITING FOR MOVEMENT
 * WAITS FOR COVER TO RELEASE TO TRIGGER GLITTER CUP AND SMELL SPRAY
 * 
 * This firmware is Open Source CC-BY-SA-NC 4.0 
 * 
 */

#include <SparkFun_ADXL345.h>         // SparkFun ADXL345 Library
#include "LowPower.h"

ADXL345 adxl = ADXL345();             // USE FOR I2C COMMUNICATION

//#define ENDURANCE //Will test endurance of device, (15 seconds recording every 15 seconds)

//#define DEBUG //Doesn't detect charging

#define Serial SerialUSB

///Phone Button Trigger Pins
int pTrig1[4] = {10, 11, 2, 4};
int pGND[4] = {12, 13, 5, 3};

#define LED 0

#define MOTORG 9
#define MOTORS A5
#define BATT A4
#define ARMSW A2
#define ENABLE12V A1
#define ACCINT1 6
#define ACCINT2 7

int ARMED = 0;

int CHARGING = 0;
float voltage = 0;
int activityTimer = 0;
int isMoving = 2;          //If device detected movement = 1 otherwise 0 and sleeping.
int isRecording = 0;       //If the phone is recording = 1;
int wasCharging = 0;
int triggered = 0;        //If triggered = 1, glitter sequence has been set
int glitter = 0;          //If glitter = 1, glitter has been dispersed
//VARIABLES

int repeatEnd = 0;
int repeatTimes = 10;  //NUMBER OF TIMES TO REPEAT ENDING (SMELL AND VIDEO RECORD) UNTIL PERMANENT SLEEP

int glitterDelay = 1000;
int smellDelay = 5000;          //Time until smell spray is released.
int endRecordingDelay = 29000;  //How long the clips are at the end until phones die - milliseconds);

#define ARMINGTIME 30 // A DELAY AFTER THE COVER IS PLACED IN SECONDS, I would make this 30 or more. Won't arm device until switch is closed for that long.(seconds)
#define waitTime 6000 // How long cameras need to do the upload procedure before continuing(milliseconds)
#define restTime 15    // The time package is not moving to stop recording (Seconds)
#define afterChargeWait 30 //How long to wait before recording after charging(Seconds)
long recordingTime = 0;
long onTime = 0;
long offTime = 0;
long videoTime = 0;
long videoLimit = 29000;
long timeLimit = 1200000; //The amount of time allowed to record in milliseconds before sitting dormant;



void setup() {

  for (int i = 0; i < 4; i++) {
    pinMode(pTrig1[i], INPUT_PULLUP);
    pinMode(pGND[i], INPUT_PULLUP);
  }
  pinMode(ARMSW, INPUT_PULLUP);
  pinMode(ENABLE12V, OUTPUT);     //Set the 12V booster to low power.
  digitalWrite(ENABLE12V, LOW);

  pinMode(LED, OUTPUT);

  SerialUSB.begin(115200);                 // Start the Serial Terminal
  //while (!SerialUSB);
  SerialUSB.println("KGB Firmware");
  SerialUSB.print("Battery Voltage: ");
  int battVolt = analogRead(BATT);
  SerialUSB.println(battVolt);

  timeLimit = 10000;  //Video Total Time Limit Initially is 10 seconds.
   
  ////////////////////////////////////
  // WAITS FOR CHARGER TO BE CONNECTED AND REMOVED BEFORE PROCEEDING
//  checkCharging();
//  while(CHARGING == 0){
//    checkCharging();
//    digitalWrite(LED, HIGH);
//    delay(200);
//    digitalWrite(LED, LOW);
//    delay(50);
//  }
//  while(CHARGING == 1){
//    checkCharging();
//    digitalWrite(LED, HIGH);
//    delay(100);
//    digitalWrite(LED, LOW);
//    delay(10);
//  }

  
  // WAITS FOR CASE SWITCH BEFORE ALLOWING PROGRAM TO CONTINUE
  // LID MUST BE IN PLACE FOR <ARMING TIME> SECONDS BEFORE ARMING. IF LID IS REMOVED, TIME IS RESET.
  
  
  pinMode(ACCINT1, INPUT);
  pinMode(ACCINT2, INPUT);

  //ACCELEROMETER SETTINGS

  adxl.powerOn();                     // Power on the ADXL345

  adxl.setRangeSetting(16);           // Give the range settings
  // Accepted values are 2g, 4g, 8g or 16g
  // Higher Values = Wider Measurement Range
  // Lower Values = Greater Sensitivity

  adxl.setActivityXYZ(1, 1, 1);       // Set to activate movement detection in the axes "adxl.setActivityXYZ(X, Y, Z);" (1 == ON, 0 == OFF)
  adxl.setActivityThreshold(18);      // 62.5mg per increment   // Set activity   // Inactivity thresholds (0-255)

  adxl.setInactivityXYZ(1, 1, 1);     // Set to detect inactivity in all the axes "adxl.setInactivityXYZ(X, Y, Z);" (1 == ON, 0 == OFF)
  adxl.setInactivityThreshold(75);    // 62.5mg per increment   // Set inactivity // Inactivity thresholds (0-255)
  adxl.setTimeInactivity(5);         // How many seconds of no activity is inactive?

  // Setting all interupts to take place on INT1 pin
  adxl.InactivityINT(0);
  adxl.ActivityINT(1);
  adxl.FreeFallINT(0);
  adxl.doubleTapINT(0);
  adxl.singleTapINT(0);

  adxl.setInterruptLevelBit(1);
  adxl.setImportantInterruptMapping(1, 1, 1, 1, 1);     // Sets "adxl.setEveryInterruptMapping(single tap, double tap, free fall, activity, inactivity);"
  // Accepts only 1 or 2 values for pins INT1 and INT2. This chooses the pin on the ADXL345 to use for Interrupts.
  // This library may have a problem using INT2 pin. Default to INT1 pin.

  SerialUSB.println("Waiting for ARM Switch(Cover Placed)");
  ARMED = digitalRead(ARMSW);
  while (ARMED == 0) {
    ARMED = digitalRead(ARMSW);
  }
  int x = 0;
  while (x < ARMINGTIME) {
    ARMED = digitalRead(ARMSW);
    while (ARMED == 0) {
      ARMED = digitalRead(ARMSW);
      x = 0;
    }
    digitalWrite(LED, HIGH);
    delay(500);
    digitalWrite(LED, LOW);
    delay(500);
    x++;
    SerialUSB.print(ARMINGTIME - x);
    SerialUSB.println("...");
  }
  SerialUSB.println("KGB IS ARMED");
  SerialUSB.println();
  //pinMode(LED, INPUT);



  adxl.setLowPower(1);
  adxl.set_bw(ADXL345_BW_6_25);
  byte clearint = adxl.getInterruptSource();
  attachInterrupt(ACCINT1, ADXL_ISR, LOW);   // Attach Interrupt
  attachInterrupt(ARMSW, TRIGGER, LOW);
  SerialUSB.println("Entering standby mode.");
  SerialUSB.println("Apply low signal to wake the processor.");
  SerialUSB.println("Zzzz...");
  clearint = adxl.getInterruptSource();
  SerialUSB.flush();
  LowPower.standby();
}


void loop() {

  //////BEFORE PACKAGE IS OPENED//////////////

  while (triggered == 0) {
    checkCharging();
    while (CHARGING == 1) {
      if (wasCharging == 0) {
        SerialUSB.println("Charging.");
      }
      timeLimit = 1200000;
      recordingTime = 0;
      wasCharging = 1;
      checkCharging();
      phoneSTOP();
    }
    if (wasCharging == 1) {     // Wait a set amount of time (afterChargeWait in seconds) before resuming the motion detection.
      for (int x = afterChargeWait; x > 0; x--) {
        digitalWrite(LED, HIGH);
        delay(200);
        digitalWrite(LED, LOW);
        delay(800);
      }
      wasCharging = 0;
      isMoving = 0;
    }

    if (isMoving == 1) {
      if (recordingTime < timeLimit) {
        if (isRecording == 0) {
          phoneSTART();
          SerialUSB.println("Innactivity in... ");
          while (activityTimer > 0) {
            SerialUSB.println(activityTimer);
            delay(1000);
            activityTimer--;
            videoTime = millis() - onTime;
            SerialUSB.println(videoTime);
            if (videoTime > videoLimit) {
              activityTimer = 0;
              videoTime = 0;
              isMoving = 0;
              phoneSTOP();
            }
            isMoving = 0;
          }
        }
      }
    }
    if (triggered == 0) {
      if (isMoving == 0) {
        if (isRecording == 1) {
          phoneSTOP();
        }
        SerialUSB.println("Entering standby mode.");
        SerialUSB.println("Apply low signal to wake the processor.");
        SerialUSB.println("Zzzz...");
        byte clearint = adxl.getInterruptSource();
        SerialUSB.flush();
        LowPower.standby();  //GOING TO SLEEP
      }
    }
  }
  /////////AFTER PACKAGE IS OPENED///////////
  while (triggered == 1) {

    SerialUSB.println("Glitter!");
    digitalWrite(LED, HIGH);
    delay(300);
    digitalWrite(LED, LOW);
    delay(300);
    if (isRecording == 0) {
      phoneSTART();
    }
    else {
      digitalWrite(LED, HIGH);
    }
    if (isRecording == 1) {
      if (glitter == 0) {
        delay(glitterDelay);
        GLITTER();
      }
      delay(smellDelay);
      SMELL();
      delay(endRecordingDelay);
    }

    if (isRecording == 1) {
      phoneSTOP();
    }
    repeatEnd++;
    if (repeatEnd > repeatTimes) {
      for(int x = 0; x<10; x++){
      digitalWrite(LED, HIGH);
      delay(200);
      digitalWrite(LED, LOW);
      delay(200);
      }
      detachInterrupt(ACCINT1);
      SerialUSB.println("going Dormant.");
      SerialUSB.println("Bye...");
      SerialUSB.flush();
      LowPower.standby();
      while (1);
    }
  }
}

/********************* ISR *********************/

void ADXL_ISR() {

  // getInterruptSource clears all triggered actions after returning value
  // Do not call again until you need to recheck for triggered actions
  byte interrupts = adxl.getInterruptSource();

  // Activity
  if (adxl.triggered(interrupts, ADXL345_ACTIVITY)) {
    SerialUSB.println("*** ACTIVITY ***");
    isMoving = 1;
    activityTimer = restTime;
  }
}

void checkCharging() {
  int voltsense = analogRead(BATT);
  voltage = voltsense * (5.0 / 1023.0);
  if (voltage > 4.3) {
    CHARGING = 1;
  }
  else {
    CHARGING = 0;
  }
#ifdef DEBUG++
  CHARGING = 0;
#endif
}



void phonesStartStop() {
  for (int i = 0; i < 4; i++) {
    SerialUSB.print("Pressing Media Button on Phone: ");
    SerialUSB.println(i);
    phoneTrigger1(i);
  }
}

void phoneSTART() {
  if (isRecording == 0) {
    onTime = millis();
    digitalWrite(LED, HIGH);
    SerialUSB.println("Starting Phones");
    phonesStartStop();
    isRecording = 1;
  }
}

void phoneSTOP() {

  if (isRecording == 1) {
    offTime = millis();
    digitalWrite(LED, LOW);
    SerialUSB.println("Stopping Phones");
    phonesStartStop();
    isRecording = 0;
    recordingTime = recordingTime + (offTime - onTime);
    SerialUSB.print("Total Recording Time: ");
    SerialUSB.println(recordingTime);
    delay(waitTime);

  }


}

///Selects the Phone with 'phoneNum' and presses media button///
void phoneTrigger1(int phoneNum) {
  pinMode(pTrig1[phoneNum], OUTPUT);
  digitalWrite(pTrig1[phoneNum], LOW);
  pinMode(pGND[phoneNum], OUTPUT);
  digitalWrite(pGND[phoneNum], LOW);
  delay(100);
  pinMode(pTrig1[phoneNum], INPUT_PULLUP);
  pinMode(pGND[phoneNum], INPUT_PULLUP);
}



//GLITTER TIME
void TRIGGER() {
//  Add a delay for the trigger
//  int trigCheck = digitalRead(ARMSW);
//  int x = 0;
//  while(trigCheck == 0){
//    trigCheck = digitalRead(ARMSW);
//    x++;
//  }
//  if (x>50000){
  activityTimer  = 0;
  SerialUSB.println("GLITTER!");
  triggered = 1;
  digitalWrite(ENABLE12V, HIGH);
  detachInterrupt(ARMSW);
//  }
}

void GLITTER() {
  for (int x = 0; x < 254; x++) {
    analogWrite(MOTORG, x);
    delay(1);
  }
  pinMode(MOTORG, OUTPUT);
  SerialUSB.println("Glitter Motor On");
  digitalWrite(MOTORG, HIGH);
  glitter = 1;
  delay(1500);
  SerialUSB.println("Glitter Motor Off");
  digitalWrite(MOTORG, LOW);
}

void SMELL() {
  //digitalWrite(ENABLE12V, LOW);
  //delay(1000);
  //digitalWrite(ENABLE12V, HIGH);
  //delay(1000);
  pinMode(MOTORS, OUTPUT);
  SerialUSB.println("Smell Motor On");
  digitalWrite(MOTORS, HIGH);
  delay(2000);
  SerialUSB.println("Smell Motor Off");
  digitalWrite(MOTORS, LOW);
}
