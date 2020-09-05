/*
* Arduino Wireless Communication Tutorial
*       Example 1 - Receiver Code
*                
* by Dejan Nedelkovski, www.HowToMechatronics.com
* 
* Library: TMRh20/RF24, https://github.com/tmrh20/RF24/
*/

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
RF24 radio(7, 8); // CE, CSN
const byte address[6] = "00001";

//Hall Motor Control
int pwm_pin = 2;
int hall_pin = 3;
long currentPWM = 0; //current PWM duty cycle

//ENCODER
//declaring the variables used for the encoder
int hallOn = false;
int encoder = 2;
float rps;
float rpm;
volatile byte pulses;
unsigned long timeold;
float s;

void setup() {
  Serial.begin(9600);
  radio.begin();
  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_MIN);
  radio.startListening();

  pinMode(pwm_pin, OUTPUT);
  pinMode(hall_pin, INPUT);

  attachInterrupt(digitalPinToInterrupt(hall_pin),counter,RISING);  //attaching the interrupt and declaring the variables, one of the interrupt pins on Nano is D2, and has to be declared as 0 here
    pulses=0;
    rps=0;
    rpm=0;
    timeold=0;
    s=0;    
}

void loop() {

  
  if (radio.available()) {
    /*char text[32] = "";
    radio.read(&text, sizeof(text));
    Serial.println(text);*/
    int PWM = 0;
    radio.read(&PWM,sizeof(PWM));
    //Serial.println(PWM);

    //Update I/Os
    currentPWM = PWM ;
    //Serial.println(currentPWM);
    analogWrite(pwm_pin, currentPWM);
  }

  hallOn = digitalRead(hall_pin);  // read input value
  //Serial.println(hallOn);

  if (pulses>=1) { //this part of the code calculates the rpm and rps in a way that every time the magnet passes by the sensor, the time between the pulses is measured and rpm and rps is calculated
    rpm = 60000.0/(millis()-timeold)*pulses;
    rps=rpm/60.0;
    timeold = millis();
    pulses=0;
  }
  Serial.println(rps);
  rps = 0;
} //loop ends

//this subprogram will run when interrupt is triggered
void counter()
{
  pulses++; //every time this subprogram runs, the number of pulses is increased by 1 
  s = s + 20.42; //and the variable s is increased by 20.42cm, which is the circumference of my wheel, s stands for path
  
}
