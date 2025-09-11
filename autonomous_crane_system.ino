/* variables to be entered before code upload */
const int load = 200;                    // load weight (grams)
const float calibration_factor = 492.5;  // loadcell calibration must be calculated and entered manually
const float drumCircumference = 0.3;     // drum circumference (meters)
unsigned long pulseWidth_func; //CH7
unsigned long pulseWidth_man;  //CH6
unsigned long ch7_2;

#include "HX711.h"  // loadcell library

const int encoderPinA = 2;     // interrupt 0
const int encoderPinB = 3;     // interrupt 1

const int loadcell_SCK = 6;    // loadcell sck
const int loadcell_DOUT = 7;   // loadcell dout

const int R_EN = 8;    // motor driver R_EN
const int L_EN = 9;    // motor driver L_EN
const int RPWM = 10;   // motor driver RPWM
const int LPWM = 11;   // motor driver LPWM

volatile unsigned long ch6_start, ch6_width;
volatile unsigned long ch7_start, ch7_width;

HX711 scale;

int manualMode = 0; // manual mode state (1 = up, -1 = down)

volatile bool stopFlag = false;  // Status control flag (for exiting functions)

volatile long int encoderPosition = 0;
volatile long int lastEncoded = 0;

// PID parameters (must be tuned experimentally) current values are approximate
float Kp = 0.7;
float Ki = 0.05;
float Kd = 0.2;

long lastError = 0; // Initial values for PID, do not change these
long integral = 0;

const int pwmMin = 50;   // Minimum PWM for motor rotation
const int pwmMax = 255;  // Maximum PWM

const int tolerance = 100; // Encoder tolerance, ±100 ticks acceptable

void setup() {
  Serial.begin(9600);

  pinMode(encoderPinA, INPUT);
  pinMode(encoderPinB, INPUT);
  pinMode(RPWM, OUTPUT);
  pinMode(LPWM, OUTPUT);
  pinMode(R_EN, OUTPUT);
  pinMode(L_EN, OUTPUT);
  pinMode(20, INPUT);   // pwm read
  pinMode(21, INPUT);   // pwm read

  digitalWrite(R_EN, HIGH);  // activate motor driver
  digitalWrite(L_EN, HIGH);

  analogWrite(RPWM, 0);
  analogWrite(LPWM, 0);

  scale.begin(loadcell_DOUT, loadcell_SCK);
  scale.set_scale(calibration_factor);  // Calibration factor fixed
  scale.tare();                         // Accept current weight as zero

  // Enable internal pull-ups if necessary (only if pull-up resistors not connected):
  // digitalWrite(encoderPinA, HIGH);
  // digitalWrite(encoderPinB, HIGH);

  // Attach interrupts to both A and B pins
  attachInterrupt(digitalPinToInterrupt(encoderPinA), updateEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(encoderPinB), updateEncoder, CHANGE);

  attachInterrupt(digitalPinToInterrupt(20), isrCH6, CHANGE);   // interrupt pwm0
  attachInterrupt(digitalPinToInterrupt(21), isrCH7, CHANGE);   // interrupt pwm1
}

void loop() {
  stopFlag = false; // when functions exit, it becomes "true". so reset it

  if (manualMode == 1 && encoderPosition <= 6000) { // if in manual mode and encoder reaches 6000, stop automatically to prevent accidents
    motorStop();
    manualMode = 0;
    Serial.println("Motor upward limit exceeded, stopped.");
  }

  if (Serial.available()) {
    String data = Serial.readStringUntil('\n');
    data.trim();
    if (data == "STOP") { // if STOP is read from serial monitor, stop (works in both manual and automatic functions)
      stopFlag = true;
      manualMode = 0;
      motorStop();
      Serial.println("Operation stopped.");
    }
    else if (data.startsWith("Load_Take")) { // detect load take function
      int parameter = data.substring(8).toInt(); // example input Load_Take 30
      Load_Take(parameter);
    }
    else if (data.startsWith("Load_Release")) { // detect load release function
      int parameter = data.substring(12).toInt(); // example input Load_Release 30
      Load_Release(parameter);
    }
    else if (data.startsWith("Manual")) { // detect manual function
      char dirChar = data.charAt(7); // example input Manual U or Manual D
      int dir = (dirChar == 'U') ? 1 : ((dirChar == 'D') ? -1 : 0);
      manualControl(dir);
    }
  }

  unsigned long ch6 = readStableValue(&ch6_width); // instead of stopping interrupt, check differently
  unsigned long ch7 = readStableValue(&ch7_width); // too many interrupt stops in loop may harm encoder value
  ch7_2 = ch7;

  Serial.print("CH6: "); Serial.print(ch6);
  Serial.print("  CH7: "); Serial.println(ch7);
  delay(100);

  if (ch7 > 1050 && ch7 < 1220) { // FUNCTIONAL STOP
    stopFlag = true;
    motorStop();
    manualMode = 0;
    Serial.println("Stop command given by remote control");
  }
  else if (ch7 > 1220 && ch7 < 1400) { // FUNCTIONAL LOAD TAKE 2m
    Serial.println("Load take2 command given by remote control");
    Load_Take(2);
  }
  else if (ch7 > 1400 && ch7 < 1700) { // FUNCTIONAL LOAD RELEASE 2m
    Serial.println("Load release2 command given by remote control");
    Load_Release(2);
  }

  if (ch6 > 1000 && ch6 < 1300) {  // MANUAL UP
    Serial.println("Manual up command given by remote control");
    manualControl(1);
  }
  else if (ch6 > 1500 && ch6 < 1800) { // MANUAL DOWN
    Serial.println("Manual down command given by remote control");
    manualControl(-1);
  }
}

void motorUp(int pwm) {
  pwm = constrain(pwm, pwmMin, pwmMax);
  analogWrite(RPWM, 0);  // motor up
  analogWrite(LPWM, pwm);
}

void motorDown(int pwm) {
  pwm = constrain(pwm, pwmMin, pwmMax);
  analogWrite(RPWM, pwm);  // motor down
  analogWrite(LPWM, 0);
}

void motorStop() {
  analogWrite(RPWM, 0);  // motor stop
  analogWrite(LPWM, 0);
}

// Manual motor control function
void manualControl(int direction) {
  if (direction == 1) {
    analogWrite(RPWM, 0);
    analogWrite(LPWM, 255);
  }
  else if (direction == -1) {
    analogWrite(RPWM, 255);
    analogWrite(LPWM, 0);
  }
  else {
    motorStop();
  }
}

bool isLoadPresent() { // load detection
  return (scale.get_units(10) >= load * 0.8);
}

void moveToTargetPID(long int target) {
  integral = 0;
  lastError = 0;

  while (true) {

    if (checkControl()) { // with checkControl function, STOP command is checked
      return;
    }

    long error = target - encoderPosition;

    if (abs(error) <= tolerance) {
      motorStop();
      Serial.println("Target reached.");
      break;
    }

    integral += error;
    long derivative = error - lastError;
    lastError = error;

    float output = Kp * error + Ki * integral + Kd * derivative;

    if (output > pwmMax) output = pwmMax;
    if (output < -pwmMax) output = -pwmMax;

    if (abs(output) < pwmMin) {  // if pwm is too low, motor may not work, set to pwmMin
      if (output > 0) output = pwmMin;
      else output = -pwmMin;
    }

    if (output > 0) {
      motorDown((int)output);
    } else {
      motorUp((int)(-output));
    }

    waitAndCheck(10);
  }
}

void waitAndCheck(unsigned long duration) { // instead of delay, millis used, allows STOP check during wait
  unsigned long startTime = millis();
  while (millis() - startTime < duration) {
    if (checkControl()) { // STOP command check
      return;
    }
  }
}

bool checkControl() {  // checks if STOP command entered
  if (Serial.available()) {
    String data = Serial.readStringUntil('\n');
    data.trim();
    if (data == "STOP") {
      stopFlag = true;   // if STOP entered, flag becomes true
      motorStop();       // stop motor
      manualMode = 0;    // reset manual mode flag
      Serial.println("Operation stopped.");
    }
  }
  unsigned long ch7_current = readStableValue(&ch7_width); // get current ch7 value

  if (ch7_current > 1050 && ch7_current < 1220) { // FUNCTIONAL STOP
    stopFlag = true;
    motorStop();
    manualMode = 0;
    Serial.println("Stop command given by remote control");
  }

  return stopFlag;
}

unsigned long readStableValue(volatile unsigned long* var) {
  unsigned long val1, val2;
  do {
    val1 = *var;
    val2 = *var;
  } while (val1 != val2);
  return val1;
}

// Function processing encoder data
void updateEncoder() {
  int MSB = digitalRead(encoderPinA);  // Channel A
  int LSB = digitalRead(encoderPinB);  // Channel B

  int encoded = (MSB << 1) | LSB;  // create 2-bit number
  int sum = (lastEncoded << 2) | encoded;

  // Gray code decoding: direction determination
  if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011)
    encoderPosition++;
  else if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000)
    encoderPosition--;

  lastEncoded = encoded;
}

void isrCH6() {
  if (digitalRead(20)) {
    ch6_start = micros();
  } else {
    ch6_width = micros() - ch6_start;
  }
}

void isrCH7() {
  if (digitalRead(21)) {
    ch7_start = micros();
  } else {
    ch7_width = micros() - ch7_start;
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Load_Take(int targetHeight) {
  long int numTurns = targetHeight / drumCircumference;    // calculate number of turns to rotate. Drum circumference approx. 30cm
  long int encoderCurrent = encoderPosition;               // get current encoder value
  long int encoderTarget = encoderCurrent + 4000 * numTurns;

  moveToTargetPID(encoderTarget); // move down to target
  waitAndCheck(20000);            // use waitAndCheck instead of delay

  bool flag = true;
  while (flag) {

    if(checkControl()){ // STOP check at start of loop
      return;
    }

    moveToTargetPID((long)(encoderTarget * 0.7)); // move back 30% for load check

    if (isLoadPresent()) {  // check if load is on
      flag = 0;             // exit control loop if load present
      break;
    }
    else {
      moveToTargetPID(encoderTarget); // if no load, go to target again and continue loop
      waitAndCheck(20000);
    }
  }

  if(checkControl()){
    return;
  }

  moveToTargetPID((long)(encoderTarget * 0.5)); // check load at 50% of upward distance
  waitAndCheck(500);

  if(!isLoadPresent()){ // if no load, retry function
    Load_Take(targetHeight);
    return; // exit current function
  }

  moveToTargetPID((long)(encoderTarget * 0.25)); // second load check at 25% of upward distance
  waitAndCheck(500);

  if(!isLoadPresent()){ // if no load, retry function
    Load_Take(targetHeight);
    return; // exit current function
  }

  moveToTargetPID(4000); // lift fully
  waitAndCheck(1000);
  Serial.println("LOAD_TAKE_COMPLETE");
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Load_Release(int targetHeight) {

  long int numTurns = targetHeight / drumCircumference;   // calculate number of turns to rotate
  long int encoderCurrent = encoderPosition;             // get current encoder value
  long int encoderTarget = encoderCurrent + 4000 * numTurns;

  while (encoderPosition <= encoderTarget + 4000 && isLoadPresent()) {  // lower motor until target height reached or load dropped
    if(checkControl()){
      return;
    }

    Serial.print("Motor Down: ");
    Serial.println(encoderPosition);
    motorDown(pwmMax);
  }
  motorStop();
  waitAndCheck(1000);

  if(encoderPosition >= encoderTarget){  // check reason for motor stop: load dropped or target reached?
    moveToTargetPID((long)(encoderTarget * 0.8)); // if stopped at encoderTarget+4000 region, loop continues until load released
    waitAndCheck(1000);

    while(isLoadPresent()){

      if(checkControl()){
        return;
      }

      moveToTargetPID((long)(encoderTarget + 4000));
      waitAndCheck(1000);

      moveToTargetPID((long)(encoderTarget * 0.8));
      waitAndCheck(2000);
    }
  }

  if(checkControl()){
    return;
  }

  moveToTargetPID(1000);                      // if load released or dropped before target, winch rewinds
  waitAndCheck(1000);
  Serial.println("LOAD_RELEASE_COMPLETE");
}
