// Final Code for ScraphEEp 2026 (Race Cars)

#include <Bluepad32.h>

ControllerPtr myControllers[BP32_MAX_CONTROLLERS];

const int pwmPin1 = 18;  // Signal output pin for Motor 1 (PWM pin)
const int pwmPin2 = 12; // Signal output pin for Motor 2 (PWM pin)
const int digPin1 = 17; // Digital pin for Motor control (e.g., direction)
const int digPin2 = 16; // Digital pin for Motor control (e.g., direction)
const int digPin3 = 14; // Digital pin for Motor control (e.g., enable)
const int digPin4 = 13; // Digital pin for Motor control (e.g., enable)
const int channel1 = 0; // LEDC channel for PWM of Motor 1
const int channel2 = 1; // LEDC channel for PWM of Motor 2

// --- TUNING PARAMETER (For Split Stick Mode only) ---
const float turnSensitivity = 1.5; 

// --- DRIVE MODES ---
enum DriveMode {
  MODE_SPLIT_STICK,
  MODE_TANK
};

// Default to Split Stick
DriveMode currentMode = MODE_SPLIT_STICK;

void setup() {
  Serial.begin(9600);
  Serial.println("Initializing Bluepad32...");
  Serial.print("Firmware version installed: ");
  Serial.println(BP32.firmwareVersion());

  // Setup Bluepad32
  BP32.setup(&onConnectedController, &onDisconnectedController);

  // Setup PWM Channels
  ledcSetup(channel1, 200, 16); 
  ledcAttachPin(pwmPin1, channel1); 
  ledcSetup(channel2, 200, 16); 
  ledcAttachPin(pwmPin2, channel2); 

  // Setup Digital Pins
  pinMode(digPin1, OUTPUT);
  pinMode(digPin2, OUTPUT);
  pinMode(digPin3, OUTPUT);
  pinMode(digPin4, OUTPUT);
  
  // Stop Motors Initially
  digitalWrite(digPin1, LOW);
  digitalWrite(digPin2, LOW);
  digitalWrite(digPin3, LOW);
  digitalWrite(digPin4, LOW); 
  sendPWMSignal(channel1, 0); 
  sendPWMSignal(channel2, 0);
}

void onConnectedController(ControllerPtr ctl) {
  for (int i = 0; i < BP32_MAX_CONTROLLERS; i++) {
    if (myControllers[i] == nullptr) {
      Serial.print("CALLBACK: Controller connected, index=");
      Serial.println(i);
      myControllers[i] = ctl;
      return;
    }
  }
}

void onDisconnectedController(ControllerPtr ctl) {
  for (int i = 0; i < BP32_MAX_CONTROLLERS; i++) {
    if (myControllers[i] == ctl) {
      Serial.print("CALLBACK: Controller disconnected, index=");
      Serial.println(i);
      sendPWMSignal(channel1, 0);
      sendPWMSignal(channel2, 0);
      myControllers[i] = nullptr;
      return;
    }
  }
}

// --- HELPER: WRITE MOTOR PINS ---
// This ensures consistent direction logic for both modes
void setMotorState(int motorId, int speed) {
  int dirPinA, dirPinB, channel;
  
  if (motorId == 1) { // Left Motor
    dirPinA = digPin1; dirPinB = digPin2; channel = channel1;
  } else { // Right Motor
    dirPinA = digPin3; dirPinB = digPin4; channel = channel2;
  }

  int pulseWidth = abs(speed) * 9; 

  if (speed > 10) { // FORWARD
    // Motor 1 Logic: LOW/HIGH
    if(motorId == 1) { digitalWrite(dirPinA, LOW); digitalWrite(dirPinB, HIGH); }
    // Motor 2 Logic: LOW/HIGH (Matches the fix from previous code)
    else             { digitalWrite(dirPinA, LOW); digitalWrite(dirPinB, HIGH); }
    
  } else if (speed < -10) { // REVERSE
    // Motor 1 Logic: HIGH/LOW
    if(motorId == 1) { digitalWrite(dirPinA, HIGH); digitalWrite(dirPinB, LOW); }
    // Motor 2 Logic: HIGH/LOW
    else             { digitalWrite(dirPinA, HIGH); digitalWrite(dirPinB, LOW); }

  } else { // STOP
    digitalWrite(dirPinA, LOW);
    digitalWrite(dirPinB, LOW);
    pulseWidth = 0;
  }

  sendPWMSignal(channel, pulseWidth);
}

void processGamepad(ControllerPtr gamepad) {
  if(gamepad && gamepad->isConnected()) { 
    
    // --- 1. CHECK FOR MODE CHANGE ---
    if (gamepad->dpad() & 0x01) { // DPAD UP
      if (currentMode != MODE_TANK) {
        currentMode = MODE_TANK;
        Serial.println("MODE SWITCHED: TANK CONTROLS");
      }
    }
    if (gamepad->dpad() & 0x02) { // DPAD DOWN
      if (currentMode != MODE_SPLIT_STICK) {
        currentMode = MODE_SPLIT_STICK;
        Serial.println("MODE SWITCHED: SPLIT STICK");
      }
    }

    // --- 2. EXECUTE DRIVING LOGIC ---
    
    int leftSpeed = 0;
    int rightSpeed = 0;

    if (currentMode == MODE_SPLIT_STICK) {
      // ===========================
      // MODE: SPLIT STICK (ARCADE)
      // ===========================
      int throttle = -gamepad->axisY(); 
      int turn = gamepad->axisRX() * turnSensitivity;

      // Mixing
      leftSpeed = throttle + turn;
      rightSpeed = throttle - turn;

      // Overflow / Skim Logic
      int maxVal = 512;
      if (leftSpeed > maxVal) { rightSpeed -= (leftSpeed - maxVal); leftSpeed = maxVal; }
      else if (leftSpeed < -maxVal) { rightSpeed -= (leftSpeed + maxVal); leftSpeed = -maxVal; }

      if (rightSpeed > maxVal) { leftSpeed -= (rightSpeed - maxVal); rightSpeed = maxVal; }
      else if (rightSpeed < -maxVal) { leftSpeed -= (rightSpeed + maxVal); rightSpeed = -maxVal; }
    } 
    else {
      // ===========================
      // MODE: TANK CONTROLS
      // ===========================
      // Left Stick Y controls Left Motor
      leftSpeed = -gamepad->axisY();
      // Right Stick Y controls Right Motor
      rightSpeed = -gamepad->axisRY();
    }

    // Constrain Final Output
    leftSpeed = constrain(leftSpeed, -512, 512);
    rightSpeed = constrain(rightSpeed, -512, 512);

    // Write to motors using helper function
    setMotorState(1, leftSpeed);
    setMotorState(2, rightSpeed);
  } 
}

void loop() {
  BP32.update(); 
  for (int i = 0; i < BP32_MAX_CONTROLLERS; i++) {
    if (myControllers[i]) { 
      processGamepad(myControllers[i]);
    }
  }
}

void sendPWMSignal(int channel, int pulseWidth) {
  static int lastPulseWidth[2] = {-1, -1}; 
  if (pulseWidth != lastPulseWidth[channel]) {
    int dutyCycle = (pulseWidth * 65536L) / 5000L;
    ledcWrite(channel, dutyCycle);
    lastPulseWidth[channel] = pulseWidth; 
  }
}