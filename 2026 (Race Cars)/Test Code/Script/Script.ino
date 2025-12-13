// Split Stick Drive Test Code for ScraphEEp 2026 (Race Cars)

#include <Bluepad32.h>

ControllerPtr myControllers[BP32_MAX_CONTROLLERS];

// **Motor and Pin Definitions (from ScraphEEp.ino)**
const int pwmPin1 = 18;  // Signal output pin for Left Motor (PWM pin)
const int pwmPin2 = 12;  // Signal output pin for Right Motor (PWM pin)
const int digPin1 = 17;  // Digital pin for Left Motor Direction A
const int digPin2 = 16;  // Digital pin for Left Motor Direction B
const int digPin3 = 14;  // Digital pin for Right Motor Direction A
const int digPin4 = 13;  // Digital pin for Right Motor Direction B
const int channel1 = 0;  // LEDC channel for PWM of Left Motor
const int channel2 = 1;  // LEDC channel for PWM of Right Motor

// **Control Variables**
// Keeping control enabled by default as per the simplification request
bool isControlEnabled = true; 

// The following functions are no longer needed but kept as stubs if they are called elsewhere.
// They will be empty or simplified.

// Forward declarations
void onConnectedController(ControllerPtr ctl);
void onDisconnectedController(ControllerPtr ctl);
void processGamepad(ControllerPtr gamepad);
void sendPWMSignal(int channel, int pulseWidth);

// The button check and password functions are removed or simplified to stubs
// to adhere to the strict "no button presses" and "no password" requirement.
void passwordCheck(ControllerPtr gamepad) { /* Removed */ }
bool checkLeftBumperPress(ControllerPtr gamepad) { return false; }
bool checkRightBumperPress(ControllerPtr gamepad) { return false; }
bool checkButtonPress(ControllerPtr gamepad, int buttonFlag) { return false; }
bool checkUpPress(ControllerPtr gamepad) { return false; }
bool checkLeftPress(ControllerPtr gamepad) { return false; }
bool checkRightPress(ControllerPtr gamepad) { return false; }


void setup() {
  Serial.begin(9600);

  // Print firmware and MAC address info
  Serial.println("Initializing Bluepad32...");
  Serial.print("Firmware version installed: ");
  Serial.println(BP32.firmwareVersion());

  const uint8_t* addr = BP32.localBdAddress();
  Serial.print("BD Address: ");
  for (int i = 0; i < 6; i++) {
    Serial.print(addr[i], HEX);
    if (i < 5)
      Serial.print(":");
    else
      Serial.println();
  }

  // Initialize Bluepad32
  BP32.setup(&onConnectedController, &onDisconnectedController);

  // Setup PWM for Motor 1 (Left)
  ledcSetup(channel1, 200, 16); // 200 Hz frequency, 16-bit resolution
  ledcAttachPin(pwmPin1, channel1); // Attach PWM channel to pin

  // Setup PWM for Motor 2 (Right)
  ledcSetup(channel2, 200, 16); // 200 Hz frequency, 16-bit resolution
  ledcAttachPin(pwmPin2, channel2); // Attach PWM channel to pin

  // Setup Digital Pins for H-Bridge Direction
  pinMode(digPin1, OUTPUT);
  pinMode(digPin2, OUTPUT);
  pinMode(digPin3, OUTPUT);
  pinMode(digPin4, OUTPUT);
  digitalWrite(digPin1, LOW);
  digitalWrite(digPin2, LOW);
  digitalWrite(digPin3, LOW);
  digitalWrite(digPin4, LOW); // Motors stopped initially

  // Initial motor signal (stop)
  sendPWMSignal(channel1, 0); // 0 pulse width (stop)
  sendPWMSignal(channel2, 0);
}

void onConnectedController(ControllerPtr ctl) {
  for (int i = 0; i < BP32_MAX_CONTROLLERS; i++) {
    if (myControllers[i] == nullptr) {
      Serial.print("CALLBACK: Controller connected, index=");
      Serial.println(i);
      myControllers[i] = ctl;
      isControlEnabled = true; // Control enabled on connect
      return;
    }
  }
  Serial.println("CALLBACK: No empty slot for new controller");
}

void onDisconnectedController(ControllerPtr ctl) {
  for (int i = 0; i < BP32_MAX_CONTROLLERS; i++) {
    if (myControllers[i] == ctl) {
      Serial.print("CALLBACK: Controller disconnected, index=");
      Serial.println(i);
      // Stop motors
      sendPWMSignal(channel1, 0);
      sendPWMSignal(channel2, 0);
      myControllers[i] = nullptr;
      isControlEnabled = false; // Disable control
      return;
    }
  }
  Serial.println("CALLBACK: Disconnected controller not found in myControllers");
}


// --- THE UPDATED processGamepad FUNCTION ---
void processGamepad(ControllerPtr gamepad) {
  // isControlEnabled check is kept simplified as there is no password check
  // and we assume control is always active when connected.
  if(gamepad && gamepad->isConnected()) { 
    // Read left stick Y for throttle, right stick X for turning
    int throttle = -gamepad->axisY();  // Invert so up is forward
    int turn = gamepad->axisRX();

    // Removed: Gear scaling (scale = 1.0)
    float scale = 1.0; 

    // Mix throttle and turn to calculate motor speeds (Split Stick Drive)
    int leftSpeed = throttle + turn;
    int rightSpeed = throttle - turn;

    // Clamp the speeds to acceptable range (-512 to 512 for raw axis)
    leftSpeed = constrain(leftSpeed, -512, 512);
    rightSpeed = constrain(rightSpeed, -512, 512);
    
    // Apply scale (still 1.0, but kept for completeness of the calculation chain)
    leftSpeed = (int)(leftSpeed * scale);
    rightSpeed = (int)(rightSpeed * scale);

    // Removed: DPAD gear control

    // Removed: Servo control

    // H-bridge motor control logic (adapted from ScraphEEp.ino)

    // Left Motor (Motor 1) Direction and Speed
    // Scaling factor '9' is from the original ScraphEEp code's pulse width calculation.
    int pulseWidth1 = abs(leftSpeed) * 9; 
    if (leftSpeed < 0) { // Forward
      digitalWrite(digPin1, LOW);
      digitalWrite(digPin2, HIGH);
    } else if (leftSpeed > 0) { // Reverse
      digitalWrite(digPin1, HIGH);
      digitalWrite(digPin2, LOW);
    } else { // Stop
      digitalWrite(digPin1, LOW);
      digitalWrite(digPin2, LOW);
      pulseWidth1 = 0;
    }
    sendPWMSignal(channel1, pulseWidth1);

    // Right Motor (Motor 2) Direction and Speed
    int pulseWidth2 = abs(rightSpeed) * 9; 
    if (rightSpeed < 0) { // Forward
      digitalWrite(digPin3, HIGH);
      digitalWrite(digPin4, LOW);
    } else if (rightSpeed > 0) { // Reverse
      digitalWrite(digPin3, LOW);
      digitalWrite(digPin4, HIGH);
    } else { // Stop
      digitalWrite(digPin3, LOW);
      digitalWrite(digPin4, LOW);
      pulseWidth2 = 0;
    }
    sendPWMSignal(channel2, pulseWidth2);

  } 
  // Removed: else { passwordCheck(gamepad); }
}

void loop() {
  BP32.update(); // Update controller states

  for (int i = 0; i < BP32_MAX_CONTROLLERS; i++) {
    ControllerPtr myController = myControllers[i];
    if (myController) { // Check if slot is occupied
      processGamepad(myController);
    }
  }
}

// Function to convert pulse width (speed) to duty cycle and send PWM signal
void sendPWMSignal(int channel, int pulseWidth) {
  static int lastPulseWidth[2] = {-1, -1}; 

  // Only send the signal if the pulse width has changed
  if (pulseWidth != lastPulseWidth[channel]) {
    int dutyCycle = (pulseWidth * 65536L) / 5000L;
    ledcWrite(channel, dutyCycle);

    // Debug output
    Serial.print("Motor ");
    Serial.print(channel + 1);
    Serial.print(": Speed/Raw Pulse: ");
    Serial.print(pulseWidth);
    Serial.print(", Duty cycle: ");
    Serial.println(dutyCycle);

    lastPulseWidth[channel] = pulseWidth; // Update the last sent pulse width
  }
}
