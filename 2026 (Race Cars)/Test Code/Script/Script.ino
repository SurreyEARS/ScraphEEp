// Split Stick Drive Test Code for ScraphEEp 2026 (Race Cars)

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

// --- TUNING PARAMETER ---
// Increase this to make turning faster/sharper. 
// 1.0 = Default, 1.5 = Aggressive, 2.0 = Very twitchy
const float turnSensitivity = 1.5; 

void setup() {
  Serial.begin(9600);
  Serial.println("Initializing Bluepad32...");
  Serial.print("Firmware version installed: ");
  Serial.println(BP32.firmwareVersion());

  const uint8_t* addr = BP32.localBdAddress();
  Serial.print("BD Address: ");
  for (int i = 0; i < 6; i++) {
    Serial.print(addr[i], HEX);
    if (i < 5) Serial.print(":");
    else Serial.println();
  }

  BP32.setup(&onConnectedController, &onDisconnectedController);

  ledcSetup(channel1, 200, 16); 
  ledcAttachPin(pwmPin1, channel1); 
  ledcSetup(channel2, 200, 16); 
  ledcAttachPin(pwmPin2, channel2); 

  pinMode(digPin1, OUTPUT);
  pinMode(digPin2, OUTPUT);
  pinMode(digPin3, OUTPUT);
  pinMode(digPin4, OUTPUT);
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

void processGamepad(ControllerPtr gamepad) {
  if(gamepad && gamepad->isConnected()) { 
    
    // --- INPUTS ---
    int throttle = -gamepad->axisY(); 
    
    // Apply Sensitivity Boost to the turn
    int turn = gamepad->axisRX() * turnSensitivity;

    // --- MIXING ---
    int leftSpeed = throttle + turn;
    int rightSpeed = throttle - turn;

    // --- OVERFLOW HANDLING (The "Skim" Logic) ---
    // If we exceed 512, subtract the excess from the OTHER motor.
    // This ensures that if we turn hard, we force a spin.
    int maxVal = 512;

    if (leftSpeed > maxVal) {
      rightSpeed -= (leftSpeed - maxVal);
      leftSpeed = maxVal;
    } else if (leftSpeed < -maxVal) {
      rightSpeed -= (leftSpeed + maxVal);
      leftSpeed = -maxVal;
    }

    if (rightSpeed > maxVal) {
      leftSpeed -= (rightSpeed - maxVal);
      rightSpeed = maxVal;
    } else if (rightSpeed < -maxVal) {
      leftSpeed -= (rightSpeed + maxVal);
      rightSpeed = -maxVal;
    }

    // Final safety constrain
    leftSpeed = constrain(leftSpeed, -512, 512);
    rightSpeed = constrain(rightSpeed, -512, 512);
    
    // --- MOTOR CONTROL LOGIC ---

    // LEFT MOTOR (Motor 1)
    int pulseWidth1 = abs(leftSpeed) * 9; 
    
    if (leftSpeed > 10) { // Forward
      digitalWrite(digPin1, LOW);
      digitalWrite(digPin2, HIGH);
    } else if (leftSpeed < -10) { // Reverse
      digitalWrite(digPin1, HIGH);
      digitalWrite(digPin2, LOW);
    } else { // Stop
      digitalWrite(digPin1, LOW);
      digitalWrite(digPin2, LOW);
      pulseWidth1 = 0;
    }
    sendPWMSignal(channel1, pulseWidth1);

    // RIGHT MOTOR (Motor 2)
    int pulseWidth2 = abs(rightSpeed) * 9; 

    // Using the FIXED pin logic from previous step
    if (rightSpeed > 10) { // Forward
      digitalWrite(digPin3, LOW); 
      digitalWrite(digPin4, HIGH);
    } else if (rightSpeed < -10) { // Reverse
      digitalWrite(digPin3, HIGH);
      digitalWrite(digPin4, LOW); 
    } else { // Stop
      digitalWrite(digPin3, LOW);
      digitalWrite(digPin4, LOW);
      pulseWidth2 = 0;
    }
    sendPWMSignal(channel2, pulseWidth2);
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