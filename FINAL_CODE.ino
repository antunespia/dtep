#define REMOTEXY__DEBUGLOG    
#define REMOTEXY_MODE__WIFI_CLOUD

#include <AccelStepper.h>
#include <WiFi.h>
#include <RemoteXY.h>

// RemoteXY Configuration

//#define REMOTEXY_WIFI_SSID "DNA-WIFI-24F8"
//#define REMOTEXY_WIFI_PASSWORD "87100912219"
#define REMOTEXY_WIFI_SSID "iPhoneRede"
#define REMOTEXY_WIFI_PASSWORD "antunespia"
#define REMOTEXY_CLOUD_SERVER "cloud.remotexy.com"
#define REMOTEXY_CLOUD_PORT 6376
#define REMOTEXY_CLOUD_TOKEN "df89b210cc7cf0187043b751246267b2"

#pragma pack(push, 1)  
uint8_t RemoteXY_CONF[] =   // 70 bytes
  { 255,3,0,0,0,63,0,19,0,0,0,0,33,1,106,200,1,1,3,0,
  1,35,44,36,36,0,31,17,79,80,69,78,32,0,1,35,101,36,36,0,
  17,31,67,76,79,83,69,0,2,31,152,44,22,0,33,31,31,33,65,85,
  84,79,0,77,65,78,85,65,76,0 };

struct {
  uint8_t button_01; // =1 if button pressed, else =0
  uint8_t button_02; // =1 if button pressed, else =0
  uint8_t switch_01; // =1 if switch ON and =0 if OFF
  uint8_t connect_flag;  // =1 if wire connected, else =0
} RemoteXY;
#pragma pack(pop)

#define STEP_PIN 18  
#define DIR_PIN 19   
#define ENABLE_PIN 21 
#define PHOTO_PIN 4  // Analog pin for photoresistor


AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);

// Timing variables
unsigned long startTime = 0;
unsigned long interval = 2800; // 2800 milliseconds
bool isMoving = false; // Tracks if the motor is currently moving
bool isOpen = false; // True if curtains are open, false if closed
uint8_t lastButtonPressed = 0; // 0 = none, 1 = OPEN, 2 = CLOSE

// Photoresistor threshold
const int photoThresholdOpen = 3000;  // Adjust based on light level for opening
const int photoThresholdClose = 1000; // Adjust based on light level for closing

void setup() {
  RemoteXY_Init(); 

  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(ENABLE_PIN, OUTPUT);
  pinMode(PHOTO_PIN, INPUT);

  digitalWrite(ENABLE_PIN, LOW); // Enable the motor driver
  stepper.setMaxSpeed(1000); // Set max speed
  stepper.setAcceleration(500); // Set acceleration
}

void loop() {
  RemoteXY_Handler();

  if (RemoteXY.switch_01 == 1) { // Automatic mode

    // Synchronize state when switching to automatic mode
    if (lastButtonPressed == 1) {
      isOpen = true; // Curtains are open
    } 
    else if (lastButtonPressed == 2) {
      isOpen = false; // Curtains are closed
    }

    // Automatic control with photoresistor
    int photoValue = analogRead(PHOTO_PIN); // Read photoresistor value

    if (!isMoving && photoValue > photoThresholdOpen && !isOpen) {
      // Open curtain if light intensity exceeds threshold and curtains are not open
      startTime = millis();
      digitalWrite(ENABLE_PIN, LOW); // Enable the motor driver
      stepper.setSpeed(400); // Set speed for opening
      isMoving = true;
      lastButtonPressed = 1; // Mark last action as open
      isOpen = true; // Update state to open
    } 
    else if (!isMoving && photoValue < photoThresholdClose && isOpen) {
      // Close curtain if light intensity drops below threshold and curtains are open
      startTime = millis();
      digitalWrite(ENABLE_PIN, LOW); // Enable the motor driver
      stepper.setSpeed(-400); // Set speed for closing
      isMoving = true;
      lastButtonPressed = 2; // Mark last action as close
      isOpen = false; // Update state to closed
    }

    if (isMoving && millis() - startTime >= interval) {
      stepper.setSpeed(0); // Stop the motor
      digitalWrite(ENABLE_PIN, HIGH); // Disable the motor driver to prevent noise
      isMoving = false;
    }

    if (isMoving) {
      stepper.runSpeed();
    }
  } 
  else if (RemoteXY.switch_01 == 0) { // Manual mode

    // Handle the OPEN button
    if (!isMoving && RemoteXY.button_01 == 1 && lastButtonPressed != 1) {
      startTime = millis();
      digitalWrite(ENABLE_PIN, LOW); // Enable the motor driver
      stepper.setSpeed(400); // Set speed for opening
      isMoving = true;
      lastButtonPressed = 1; // Update the last button pressed
      isOpen = true; // Update state to open
    } 
    // Handle the CLOSE button
    else if (!isMoving && RemoteXY.button_02 == 1 && lastButtonPressed != 2) {
      startTime = millis();
      digitalWrite(ENABLE_PIN, LOW); // Enable the motor driver
      stepper.setSpeed(-400); // Set speed for closing
      isMoving = true;
      lastButtonPressed = 2; // Update the last button pressed
      isOpen = false; // Update state to closed
    }

    // Check if the motor has been moving for the specified interval
    if (isMoving && millis() - startTime >= interval) {
      stepper.setSpeed(0); // Stop the motor
      digitalWrite(ENABLE_PIN, HIGH); // Disable the motor driver to prevent noise
      isMoving = false;
      lastButtonPressed = 0; // Reset last button after movement ends
    }

    // Keep the motor running while moving
    if (isMoving) {
      stepper.runSpeed();
    }
  }
}
