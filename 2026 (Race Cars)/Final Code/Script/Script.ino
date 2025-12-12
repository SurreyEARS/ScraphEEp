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

  // Setup PWM for Motor 1
  ledcSetup(channel1, 200, 16);   // 200 Hz frequency, 16-bit resolution
  ledcAttachPin(pwmPin1, channel1); // Attach PWM channel to pin

  // Setup PWM for Motor 2
  ledcSetup(channel2, 200, 16);   // 200 Hz frequency, 16-bit resolution
  ledcAttachPin(pwmPin2, channel2); // Attach PWM channel to pin

  pinMode(digPin1, OUTPUT);
  pinMode(digPin2, OUTPUT);
  pinMode(digPin3, OUTPUT);
  pinMode(digPin4, OUTPUT);
  digitalWrite(digPin1, LOW);
  digitalWrite(digPin2, LOW);
  digitalWrite(digPin3, LOW);
  digitalWrite(digPin4, LOW);
}

void onConnectedController(ControllerPtr ctl) {
  for (int i = 0; i < 1; i++) {
    if (myControllers[i] == nullptr) {
      Serial.print("CALLBACK: Controller connected, index=");
      Serial.println(i);
      myControllers[i] = ctl;
      return;
    }
  }
  Serial.println("CALLBACK: No empty slot for new controller");
}

void onDisconnectedController(ControllerPtr ctl) {
  for (int i = 0; i < 1; i++) {
    if (myControllers[i] == ctl) {
      Serial.print("CALLBACK: Controller disconnected, index=");
      Serial.println(i);
      myControllers[i] = nullptr;
      return;
    }
  }
  Serial.println("CALLBACK: Disconnected controller not found in myControllers");
}

void processGamepad(ControllerPtr gamepad) {
  // Read joystick axes for motor control
    int axisY1 = gamepad->axisY();  // Control Motor 1
    int axisY2 = gamepad->axisRY(); // Control Motor 2

    // Calculate pulse width for Motor 1
    int pulseWidth1 = 0; // Default to neutral
    if (axisY1 < 0) {
      pulseWidth1 = abs(axisY1) * 9; // Forward
      digitalWrite(digPin1, LOW);
      digitalWrite(digPin2, HIGH);
    } else if (axisY1 > 0) {
      pulseWidth1 = axisY1 * 9;     // Reverse
      digitalWrite(digPin1, HIGH);
      digitalWrite(digPin2, LOW);
    }

    // Calculate pulse width for Motor 2
    int pulseWidth2 = 0; // Default to neutral
    if (axisY2 < 0) {
      pulseWidth2 = abs(axisY2) * 9; // Forward
      digitalWrite(digPin3, HIGH);
      digitalWrite(digPin4, LOW);
    } else if (axisY2 > 0) {
      pulseWidth2 = axisY2 * 9;     // Reverse
      digitalWrite(digPin3, LOW);
      digitalWrite(digPin4, HIGH);
    }

    // Send PWM signals
    sendPWMSignal(channel1, pulseWidth1);
    sendPWMSignal(channel2, pulseWidth2);
}

void loop() {
  BP32.update(); // Update controller states

  for (int i = 0; i < BP32_MAX_CONTROLLERS; i++) {
    ControllerPtr myController = myControllers[i];

    if (myController && myController->isConnected()) {
      processGamepad(myController);
    }
  }
}

// Function to convert microseconds to duty cycle and send PWM signal
void sendPWMSignal(int channel, int pulseWidth) {
  static int lastPulseWidth[2] = {-1, -1}; // Keep track of the last pulse width sent for both motors

  // Only send the signal if the pulse width has changed
  if (pulseWidth != lastPulseWidth[channel]) {
    int dutyCycle = (pulseWidth * 65536L) / 5000L;
    ledcWrite(channel, dutyCycle);

    // Debug output
    Serial.print("Motor ");
    Serial.print(channel + 1);
    Serial.print(": Pulse width: ");
    Serial.print(pulseWidth);
    Serial.print(" us, Duty cycle: ");
    Serial.println(dutyCycle);

    lastPulseWidth[channel] = pulseWidth; // Update the last sent pulse width
  }
}
