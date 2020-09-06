#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
RF24 radio(7, 8); // CE, CSN
const byte address[6] = "00001";

#define PI 3.1415926535897932384626433832795

//Control State Constants
#define IDLE 0 //When speed is == 0 and PWM Duty Cycle is 0
#define BRAKING 1
#define ACCEL_F 2
#define ACCEL_R 3
#define COAST 4 //When speed is > 0 and PWM Duty Cycle is 0
#define CRUISE 5 //When PWM is maintaining Current Speed > 0

//Command Constants
#define C_NOCOMMAND 0
#define C_BRAKE 1
#define C_FORWARD 2
#define C_BACKWARD 3

bool debug = true; //Serial print toggle

//Hall Motor Control
int pwm_pin = 2;
int hall_pin = 3;
int brake_pin = 4;
int reverse_pin = 5;
long currentPWM = 0; //current PWM duty cycle

//ENCODER
//declaring the variables used for the encoder
float rps; //rev per sec
float rpm; //rev per min
float mph; // miles per hour
volatile byte pulses;
unsigned long timeold; //Encoder speed calc time memory
float wheelSize = 70; //Diameter of wheel for Speed Calc

//Motor Control Runtime
int initPWM = 25; //starting duty cycle with 255 as 100% Duty Cycle
int DEADZONE = 20; //minimum command threshold for PWM 0 - 255
float rampRate = 0.1; //0-255 per milisecond; initial ramp rate 1 per millisecond -> 0 - 100% in .255 sec
float brakeRate = 0.03;

//Control Structure
int state = IDLE; //0-idle,1-braking,2-forward,3-reverse, 4-coast
int command = C_NOCOMMAND; //0-no command, 1-brake, 2-forward, 3-reverse
int timeSince = 0; //time since last PWM write in millisecond
int breakTimer = 0; //brake to reverse timer

//Runtime values
int targetSpeed = 0; //target speed 0 - 255 from remote control
bool ramping = false;
bool reversing = false;

void setup() {
  Serial.begin(9600);
  radio.begin();
  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_MIN);
  radio.startListening();

  pinMode(pwm_pin, OUTPUT);
  pinMode(hall_pin, INPUT);
  pinMode(brake_pin, OUTPUT);
  pinMode(reverse_pin, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(hall_pin),counter,RISING);  //attaching the interrupt and declaring the variables, one of the interrupt pins on Nano is D2, and has to be declared as 0 here
    pulses=0;
    rps=0;
    rpm=0;
    timeold=0;
}

void loop() {

  if (radio.available()) {
    /*char text[32] = "";
    radio.read(&text, sizeof(text));
    Serial.println(text);*/
    int PWM = 0;
    radio.read(&PWM,sizeof(PWM));
    command = getCommand(PWM);

    //Update I/Os
    targetSpeed = PWM; //Write to global scope
    if (reversing) {
      targetSpeed = -PWM;
    }

    printTargetSpeed();
    
  }

  //Execute command regardless of remote control
  state = execCommand(state,command);
  
  if ((millis()-timeold) > 100) { //this part of the code calculates the rpm and rps in a way that every time the magnet passes by the sensor, the time between the pulses is measured and rpm and rps is calculated
    rpm = 60000.0/(millis()-timeold)*pulses / 10; //10 rising edges per revolution
    rps=rpm/60.0;
    mph=rps*60*60*PI*wheelSize*1e-6*0.621371;
    timeold = millis();
    pulses=0;
  }
  if (rps > 0) {
    /*Serial.print("Duty Cycle: ");
    Serial.println(currentPWM/15);
    Serial.print("MPH: ");
    Serial.println(mph);*/
  }
  rps = 0;
  
  //Update I/Os
  analogWrite(pwm_pin, currentPWM);
  if (state == ACCEL_R) {
    analogWrite(reverse_pin, 255);
  } else {
    analogWrite(reverse_pin, 0);
  }
  if (state == BRAKING) {
    analogWrite(brake_pin, 255);
  } else {
    analogWrite(brake_pin, 0);
  }

  //Dev Prints
  printPWM();
  printState();
  printCommand();

  //delay(500);
} //loop ends

int getCommand(int PWM) {
  if (PWM < DEADZONE && PWM > -DEADZONE) {
    return C_NOCOMMAND;
  } else if (PWM > DEADZONE) {
    return C_FORWARD;
  } else if (PWM < -DEADZONE) {
    return C_BRAKE;
  }
}

int execCommand(int state, int command){
  switch (command) {
  case C_NOCOMMAND: //no command
    // statements
    currentPWM = 0;
    targetSpeed = 0;
    reversing = false;
    ramping = false;
    breakTimer = 0;
    //////////////////////////////
    //Add code to tell if coasting or idle
    return IDLE;
    break;
  case C_BRAKE: //brake
    Serial.print("breakTimer");
    Serial.println(breakTimer);
    if ((millis() - breakTimer) > 20000 && breakTimer != 0){
      brakeReverse(targetSpeed);
      return ACCEL_R;
    }
    //statements
    currentPWM = 0;
    targetSpeed = 0;
    if (breakTimer == 0){
      breakTimer = millis(); //Aggregate time
    } 
    return BRAKING;
    break;
  case C_FORWARD: //go forward
    // statements
    if (state == IDLE || state == ACCEL_F) { //if idle or going forward
      //ramping
      goForward(targetSpeed);
    }
    return ACCEL_F;    
    break;
  case C_BACKWARD:
    // statements
    breakTimer = 0;
    brakeReverse(targetSpeed);
    return ACCEL_R;
    break;
  default: //4 - coast
    // statements
    break;
  }
}


void goForward(int targetSpeed) {
  reversing = false;
  //Ramp PWM duty cycle
    if (ramping) {
      int now = millis();
      currentPWM = (int) currentPWM + (now - timeSince) * rampRate ;
      timeSince = now;
      //when ramped to max Duty Cycle
      if (currentPWM >= targetSpeed) {
        currentPWM = targetSpeed; //Stop the ramping
        ramping = false;
      }
    } else { //initial
      timeSince = millis();
      ramping = true;
      Serial.println("In here first ramp");
    }
}

void brakeReverse(int targetBrake) {
  //This brake function reverses the motor at increasing duty cycle until brake is released
  //targetSpeed = targetBrake;
  reversing = true;
  //Ramp PWM duty cycle
    if (ramping) {
      int now = millis();
      currentPWM = (int) currentPWM + (now - timeSince) * brakeRate ;
      timeSince = now;
      
      //when ramped to max Duty Cycle
      if (currentPWM >= targetSpeed) {
        Serial.print(targetSpeed);
        currentPWM = targetSpeed; //Stop the ramping
        ramping = false;
      }
    } else { //initial
      timeSince = millis();
      ramping = true;
      Serial.println("In here first brake ramp");
    }
}

//this subprogram will run when interrupt is triggered
void counter()
{
  pulses++; //every time this subprogram runs, the number of pulses is increased by 1
}

//Dev Functions
void printPWM(){
  Serial.print("currentPWM: ");
  Serial.print(currentPWM);
  Serial.print("\n");
}

void printState(){
  Serial.print("state: ");
  Serial.print(state);
  Serial.print("\n");
  Serial.print("ramping: ");
  Serial.println(ramping);
  Serial.print("reversing: ");
  Serial.println(reversing);
  Serial.print("braking: ");
  if (state == BRAKING) {
    Serial.println(true);
  } else {
    Serial.println(false);
  }
}

void printCommand(){
  Serial.print("command: ");
  Serial.print(command);
  Serial.print("\n");
}

void printTargetSpeed(){
  Serial.print("targetSpeed: ");
  Serial.println(targetSpeed);
}
