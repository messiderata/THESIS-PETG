#include <PID_v1.h>
#include <thermistor.h>  // Thermistor library for temperature sensing
#include <U8x8lib.h>     // Text-only display mode for SSD1309

// U8x8 object for OLED Display (SSD1309)
U8X8_SSD1309_128X64_NONAME0_HW_I2C u8x8(/* reset=*/U8X8_PIN_NONE);

// Thermistor and PID Control Parameters
#define initialTemp 220  // Default temperature
#define minTemp 220      // Minimum temperature
#define maxTemp 250      // Maximum temperature
#define tempStep 1       // Increment/decrement step for setpoint adjustment

const int temperaturePin = A2;  // Thermistor pin
const int pwmPin = 9;           // PWM output pin
const int stepX = 2;            // Stepper motor step pin
const int dirX = 4;             // Stepper motor direction pin
const int enPin = 5;            // Stepper motor enable pin
const int RotA = 6;             // Rotary encoder A pin
const int RotB = 7;             // Rotary encoder B pin
const int RotSwi = 8;           // Rotary encoder switch pin

thermistor therm1(temperaturePin, 0);  // Thermistor object
double setpoint = initialTemp;         // Desired temperature
double input, output;                  // PID variables
double Kp = 80, Ki = 12.3, Kd = 40;    // PID tuning constants
PID pid(&input, &output, &setpoint, Kp, Ki, Kd, DIRECT);

unsigned long lastTempUpdate = 0;      // Last temperature update time
const int tempUpdateInterval = 1000;   // Update temperature every second

unsigned long stepperPrevMicros = 0;   // Step timing variable
int stepperInterval = 3200;            // Stepper speed (microseconds per step)
bool stepState = LOW;                  // Current step state
bool isRunning = false;                // System running flag (starts stopped)
int lastRotState = HIGH;               // Rotary switch state tracking

bool inStepperMenu = false;            // Flag to track if we're in the stepper menu
unsigned long pressStartTime = 0;      // Track press duration for long press
const int longPressDuration = 3000;    // 3 seconds for long press

void setup() {
  // Serial communication for debugging
  Serial.begin(9600);

  // Pin configurations
  pinMode(pwmPin, OUTPUT);  // PWM pin for heater
  pinMode(stepX, OUTPUT);   // Stepper motor step pin
  pinMode(dirX, OUTPUT);    // Stepper motor direction pin
  pinMode(enPin, OUTPUT);   // Stepper motor enable pin
  pinMode(RotA, INPUT_PULLUP);
  pinMode(RotB, INPUT_PULLUP);
  pinMode(RotSwi, INPUT_PULLUP);

  digitalWrite(dirX, HIGH);
  // Initialize the OLED Display
  u8x8.begin();
  u8x8.setFont(u8x8_font_chroma48medium8_r);  // Small, memory-efficient font
  showIntroScreen();
  delay(2000);  // Show intro screen for 2 seconds

  // Initialize PID
  pid.SetMode(AUTOMATIC);
  pid.SetOutputLimits(0, 255);

  // Disable the stepper motor and PWM by default
  stopComponents();

  Serial.println(F("Setup complete"));
}

void loop() {
  // Handle rotary encoder input for adjusting the setpoint or stepper speed
  handleRotaryEncoder();

  // Handle rotary switch for starting/stopping the system and toggling modes
  handleRotarySwitch();

  if (isRunning) {
    unsigned long currentMillis = millis();

    // Update temperature and PID output periodically
    if (currentMillis - lastTempUpdate >= tempUpdateInterval) {
      lastTempUpdate = currentMillis;

      // Read temperature from the thermistor
      input = therm1.analog2temp();

      // Compute PID output
      pid.Compute();

      // Apply PWM output
      analogWrite(pwmPin, output);

      // Debug information
      Serial.print(F("Temperature: "));
      Serial.print(input);
      Serial.print(F(" \u00B0C\tPWM Output: "));
      Serial.println(output);

      // Update OLED display
      updateOled((int)input, (int)output, (int)setpoint);
    }

    // Check if the temperature is 220°C or above for the stepper motor to run
    if (input >= 220) {
      runMotor();  // Continuously run the stepper motor
    } else {
      stopStepper();  // Stop the stepper motor if below 220°C
    }
  } else {
    // Stop stepper motor and PWM when system is stopped
    stopComponents();
  }
}

// Rotary Encoder: Adjust the setpoint or stepper speed
void handleRotaryEncoder() {
  if (inStepperMenu) {
    // Adjust the stepper speed within the range (2900 to 3500 microseconds per step)
    static int lastStateA = HIGH;
    int currentStateA = digitalRead(RotA);
    int currentStateB = digitalRead(RotB);

    // Detect rising edge on RotA
    if (lastStateA == LOW && currentStateA == HIGH) {
      if (currentStateB == LOW && stepperInterval > 2900) {
        stepperInterval -= 100;  // Decrease stepper speed
      } else if (currentStateB == HIGH && stepperInterval < 3500) {
        stepperInterval += 100;  // Increase stepper speed
      }

      // Display stepper speed on OLED
      char speedMessage[20];
      snprintf(speedMessage, sizeof(speedMessage), "Speed: %d", stepperInterval);
      u8x8.clearDisplay();
      u8x8.drawString(0, 0, "Adjusting Speed");
      u8x8.drawString(0, 1, speedMessage);
      delay(300);

      Serial.print(F("Stepper speed adjusted to: "));
      Serial.println(stepperInterval);
    }
    lastStateA = currentStateA;
  } else {
    // Adjust the temperature setpoint when not in the stepper menu
    static int lastStateA = HIGH;
    int currentStateA = digitalRead(RotA);
    int currentStateB = digitalRead(RotB);

    // Detect rising edge on RotA for temperature adjustments
    if (lastStateA == LOW && currentStateA == HIGH) {
      if (currentStateB == LOW && setpoint < maxTemp) {
        setpoint += tempStep;  // Increase by tempStep
        if (setpoint > maxTemp) setpoint = maxTemp;  // Prevent overflow
      } else if (currentStateB == HIGH && setpoint > minTemp) {
        setpoint -= tempStep;  // Decrease by tempStep
        if (setpoint < minTemp) setpoint = minTemp;  // Prevent underflow
      }

      // Display adjustment message on OLED
      char message[20];
      snprintf(message, sizeof(message), "Setpoint: %d", (int)setpoint);
      u8x8.clearDisplay();
      u8x8.drawString(0, 0, "Adjusting Temp");
      u8x8.drawString(0, 1, message);
      delay(300);  // Delay to make the change more gradual (e.g., 300ms)

      Serial.print(F("Setpoint adjusted to: "));
      Serial.println(setpoint);
    }
    lastStateA = currentStateA;
  }
}

// Rotary Switch: Start/Stop the system and toggle between modes
void handleRotarySwitch() {
  int currentState = digitalRead(RotSwi);

  // Detect falling edge (button press)
  if (lastRotState == HIGH && currentState == LOW) {
    unsigned long pressDuration = millis() - pressStartTime;

    if (pressDuration >= longPressDuration) {
      // Long press: Toggle between stepper menu and normal operation
      inStepperMenu = !inStepperMenu;
      Serial.print(F("Stepper Menu: "));
      Serial.println(inStepperMenu ? F("Enabled") : F("Disabled"));
    } else {
      // Short press: Toggle system on/off
      isRunning = !isRunning;

      // Start/Stop the components based on the state
      if (isRunning) {
        startComponents();  // Start the system
      } else {
        stopComponents();   // Stop the system
      }

      Serial.print(F("System "));
      Serial.println(isRunning ? F("Started") : F("Stopped"));
    }

    // Reset press start time for next detection
    pressStartTime = millis();
  }

  lastRotState = currentState;
}

// Update the OLED display
void updateOled(int temperature, int pwm, int sp) {
  char tempBuffer[16];       // Buffer to store formatted temperature
  char pwmBuffer[16];        // Buffer to store formatted PWM value
  char setpointBuffer[16];   // Buffer to store formatted setpoint value

  snprintf(tempBuffer, sizeof(tempBuffer), "Temp: %d", temperature); 
  snprintf(pwmBuffer, sizeof(pwmBuffer), "PWM: %d", pwm);           
  snprintf(setpointBuffer, sizeof(setpointBuffer), "SP: %d", sp);    

  u8x8.clearDisplay();
  u8x8.drawString(0, 0, tempBuffer);
  u8x8.drawString(0, 1, pwmBuffer);
  u8x8.drawString(0, 2, setpointBuffer);
}

// Display an intro screen
void showIntroScreen() {
  u8x8.clearDisplay();
  u8x8.drawString(0, 0, "3D FILAMENT");
  u8x8.drawString(0, 1, "EXTRUSION");
  u8x8.drawString(0, 3, "Press Encoder");
  u8x8.drawString(0, 4, "To Start");
}

// Run the stepper motor
void runMotor() {
  unsigned long currentMicros = micros();

  if (currentMicros - stepperPrevMicros >= stepperInterval) {
    stepperPrevMicros = currentMicros;
    stepState = !stepState;  // Toggle step state
    digitalWrite(stepX, stepState);
  }
}

// Stop the stepper motor
void stopStepper() {
  digitalWrite(stepX, LOW);  // Set step signal to LOW
  stepState = LOW;
}

// Start PWM and Stepper Motor
void startComponents() {
  digitalWrite(enPin, LOW);
  analogWrite(pwmPin, output);
  Serial.println(F("Components started"));
}

// Stop PWM and Stepper Motor
void stopComponents() {
  analogWrite(pwmPin, 0);
  stopStepper();
  digitalWrite(enPin, HIGH);
  Serial.println(F("Components stopped"));
}
