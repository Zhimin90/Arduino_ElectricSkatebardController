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

int pwm_pin = 2;
long currentPWM = 0; //current PWM duty cycle

void setup() {
  Serial.begin(9600);
  radio.begin();
  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_MIN);
  radio.startListening();

  pinMode(pwm_pin, OUTPUT);
}

void loop() {
  
  if (radio.available()) {
    /*char text[32] = "";
    radio.read(&text, sizeof(text));
    Serial.println(text);*/
    int PWM = 0;
    radio.read(&PWM,sizeof(PWM));
    Serial.println(PWM);

    //Update I/Os
    currentPWM = PWM / 4 ;
    Serial.println(currentPWM);
    analogWrite(pwm_pin, currentPWM);
  }
}
