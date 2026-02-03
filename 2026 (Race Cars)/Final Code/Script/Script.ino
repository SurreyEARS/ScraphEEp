// Final Code for ScraphEEp 2026 (Race Cars) - Dynamic Configuration
// Triangle/Cross = Flip Throttle
// Square/Circle = Flip Steering

#include <Bluepad32.h>

ControllerPtr myControllers[BP32_MAX_CONTROLLERS];

const int pwmPin1 = 18;  // Signal output pin for Motor 1 (PWM pin)
const int pwmPin2 = 12;  // Signal output pin for Motor 2 (PWM pin)
const int digPin1 = 17;  // Digital pin for Motor control
const int digPin2 = 16;  // Digital pin for Motor control
const int digPin3 = 14;  // Digital pin for Motor control
const int digPin4 = 13;  // Digital pin for Motor control
const int channel1 = 0;  // LEDC channel for PWM of Motor 1
const int channel2 = 1;  // LEDC channel for PWM of Motor 2

// --- HARDWARE DIRECTION CONFIGURATION ---
// Set these to true ONLY if your wheels spin backwards when the controller is in "Normal" mode
const bool HARDWARE_INVERT_MOTOR_1 = false; 
const bool HARDWARE_INVERT_MOTOR_2 = false; 

// --- TUNING PARAMETER ---
const float turnSensitivity = 1.5; 

// --- DRIVE MODES ---
enum DriveMode {
  MODE_SPLIT_STICK,
  MODE_TANK
};

DriveMode currentMode = MODE_SPLIT_STICK;

// --- DYNAMIC CONFIGURATION VARIABLES ---
int throttleMultiplier = 1; // 1 = Normal, -1 = Reversed
int steeringMultiplier = 1; // 1 = Normal, -1 = Reversed

// Forward Declarations
void onConnectedController(ControllerPtr ctl);
void onDisconnectedController(ControllerPtr ctl);
void sendPWMSignal(int channel, int pulseWidth); 

void setup() {
  Serial.begin(9600);
  Serial.println("Initializing Bluepad32...");
  Serial.print("Firmware version installed: ");
  Serial.println(BP32.firmwareVersion());

  //Bluetooth MAC Address
  const uint8_t* addr = BP32.localBdAddress();
  Serial.print("BD Address: ");
  for (int i = 0; i < 6; i++) {
    Serial.print(addr[i], HEX);
    if (i < 5)
      Serial.print(":");
    else
      Serial.println();
  }

  BP32.setup(&onConnectedController, &onDisconnectedController);

  // Setup PWM
  ledcSetup(channel1, 200, 16); 
  ledcAttachPin(pwmPin1, channel1); 
  ledcSetup(channel2, 200, 16); 
  ledcAttachPin(pwmPin2, channel2); 

  // Setup Digital Pins
  pinMode(digPin1, OUTPUT);
  pinMode(digPin2, OUTPUT);
  pinMode(digPin3, OUTPUT);
  pinMode(digPin4, OUTPUT);
  
  // Stop Motors
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
void setMotorState(int motorId, int speed) {
  int dirPinA, dirPinB, channel;
  bool invertThisMotor = false;
  
  if (motorId == 1) { // Left Motor
    dirPinA = digPin1; dirPinB = digPin2; channel = channel1;
    invertThisMotor = HARDWARE_INVERT_MOTOR_1;
  } else { // Right Motor
    dirPinA = digPin3; dirPinB = digPin4; channel = channel2;
    invertThisMotor = HARDWARE_INVERT_MOTOR_2;
  }

  int pulseWidth = abs(speed) * 9; 

  if (speed > 10) { // FORWARD
    if (!invertThisMotor) {
       digitalWrite(dirPinA, LOW); digitalWrite(dirPinB, HIGH); 
    } else {
       digitalWrite(dirPinA, HIGH); digitalWrite(dirPinB, LOW);
    }
    
  } else if (speed < -10) { // REVERSE
    if (!invertThisMotor) {
       digitalWrite(dirPinA, HIGH); digitalWrite(dirPinB, LOW);
    } else {
       digitalWrite(dirPinA, LOW); digitalWrite(dirPinB, HIGH);
    }

  } else { // STOP
    digitalWrite(dirPinA, LOW);
    digitalWrite(dirPinB, LOW);
    pulseWidth = 0;
  }

  sendPWMSignal(channel, pulseWidth);
}

void processGamepad(ControllerPtr gamepad) {
  if(gamepad && gamepad->isConnected()) { 
    
    // --- 1. CONFIGURATION BUTTONS ---
    // Triangle (Y) -> Swap Forward/Back
    if (gamepad->y()) { 
      if(throttleMultiplier != -1) Serial.println("CFG: Throttle REVERSED");
      throttleMultiplier = -1; 
    }
    // Cross (A) -> Reset Forward/Back
    if (gamepad->a()) { 
      if(throttleMultiplier != 1) Serial.println("CFG: Throttle NORMAL");
      throttleMultiplier = 1; 
    }
    // Square (X) -> Swap Left/Right
    if (gamepad->x()) { 
      if(steeringMultiplier != -1) Serial.println("CFG: Steering REVERSED");
      steeringMultiplier = -1; 
    }
    // Circle (B) -> Reset Left/Right
    if (gamepad->b()) { 
      if(steeringMultiplier != 1) Serial.println("CFG: Steering NORMAL");
      steeringMultiplier = 1; 
    }

    // --- 2. CHECK FOR MODE CHANGE ---
    if (gamepad->dpad() & 0x01) { // DPAD UP
      if (currentMode != MODE_TANK) {
        currentMode = MODE_TANK;
        Serial.println("MODE: TANK CONTROLS");
      }
    }
    if (gamepad->dpad() & 0x02) { // DPAD DOWN
      if (currentMode != MODE_SPLIT_STICK) {
        currentMode = MODE_SPLIT_STICK;
        Serial.println("MODE: SPLIT STICK");
      }
    }

    // --- 3. EXECUTE DRIVING LOGIC ---
    int leftSpeed = 0;
    int rightSpeed = 0;

    if (currentMode == MODE_SPLIT_STICK) {
      // ===========================
      // MODE: SPLIT STICK (ARCADE)
      // ===========================
      
      // Apply Throttle Multiplier (1 or -1)
      int throttle = -gamepad->axisY() * throttleMultiplier; 
      
      // Apply Steering Multiplier (1 or -1)
      int turn = gamepad->axisRX() * turnSensitivity * steeringMultiplier;

      // Standard Mixing
      leftSpeed = throttle + turn;
      rightSpeed = throttle - turn;
    } 
    else {
      // ===========================
      // MODE: TANK CONTROLS
      // ===========================
      
      // Get Raw Stick Values and apply Throttle Multiplier
      int stickLeft = -gamepad->axisY() * throttleMultiplier;
      int stickRight = -gamepad->axisRY() * throttleMultiplier;

      // Apply Steering Multiplier
      // In Tank mode, "Inverting Steering" implies swapping the sticks
      if (steeringMultiplier == -1) {
         leftSpeed = stickRight;
         rightSpeed = stickLeft;
      } else {
         leftSpeed = stickLeft;
         rightSpeed = stickRight;
      }
    }

    // --- 4. OUTPUT ---
    
    // Overflow / Skim Logic
    int maxVal = 512;
    if (currentMode == MODE_SPLIT_STICK) {
      if (leftSpeed > maxVal) { rightSpeed -= (leftSpeed - maxVal); leftSpeed = maxVal; }
      else if (leftSpeed < -maxVal) { rightSpeed -= (leftSpeed + maxVal); leftSpeed = -maxVal; }
      if (rightSpeed > maxVal) { leftSpeed -= (rightSpeed - maxVal); rightSpeed = maxVal; }
      else if (rightSpeed < -maxVal) { leftSpeed -= (rightSpeed + maxVal); rightSpeed = -maxVal; }
    }

    // Constrain Final Output
    leftSpeed = constrain(leftSpeed, -512, 512);
    rightSpeed = constrain(rightSpeed, -512, 512);

    // Write to motors
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
