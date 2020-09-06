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

//Control Structure
int state = IDLE; //0-idle,1-braking,2-forward,3-reverse, 4-coast
int command = C_NOCOMMAND; //0-no command, 1-brake, 2-forward, 3-reverse
int timeSince = 0; //time since last PWM write in millisecond

//Runtime values
int targetSpeed = 0; //target speed 0 - 255 from remote control
bool ramping = false;

void setup() {
  Serial.begin(9600);
  radio.begin();
  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_MIN);
  radio.startListening();

  pinMode(pwm_pin, OUTPUT);
  pinMode(hall_pin, INPUT);
  pinMode(brake_pin, OUTPUT);

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

  //Dev Prints
  printPWM();
  printState();
  printCommand();

  delay(10);
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
    break;
  case C_BRAKE: //brake
    // statements
    break;
  case C_FORWARD: //go forward
    // statements
    if (state == IDLE || state == ACCEL_F) { //if idle or going forward
      //ramping
      goForward(targetSpeed);
    }
    if (currentPWM < 255){
      return 2; //coast
    } else {
      return 4; // coast
    }
    
    break;
  case 3:
    // statements
    break;
  default: //4 - coast
    // statements
    break;
  }
}


void goForward(int targetSpeed) {
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
}

void printCommand(){
  Serial.print("command: ");
  Serial.print(command);
  Serial.print("\n");
}
