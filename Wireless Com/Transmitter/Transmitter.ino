/*
* Arduino Wireless Communication Tutorial
*     Example 1 - Transmitter Code
*                
* by Dejan Nedelkovski, www.HowToMechatronics.com
* 
* Library: TMRh20/RF24, https://github.com/tmrh20/RF24/
*/
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

int sensorPin = A0;    // select the input pin for the potentiometer
int inPin = 2;   // choose the input pin (for a pushbutton)

int sensorValue = 0;  // variable to store the value coming from the sensor
int val = 0;     // variable for reading the pin status

RF24 radio(9, 10); // CE, CSN
const byte address[6] = "00001";
void setup() {
  radio.begin();
  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_MIN);
  radio.stopListening();

  pinMode(inPin, INPUT);    // declare pushbutton as input
}
void loop() {
  //const char text[] =  "Hello World"; //String(sensorValue);
  //radio.write(&text, sizeof(text));
  
  // read the value from the sensor:
  sensorValue = analogRead(sensorPin);
  int PWM = sensorValue;

  val = digitalRead(inPin);  // read input value
    
  if (val == LOW) {         // check if the input Button is pushed
    PWM = 0;
    radio.write(&PWM, sizeof(PWM));
  } else {
    radio.write(&PWM, sizeof(PWM));
  }
  
  //delay(10);
}
