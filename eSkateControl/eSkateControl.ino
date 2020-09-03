bool debug = true;
//Initializing Motor PWM Pin
int pwm_pin = 6;
int brake_pin = 7;
int feedback_pin = 8;
int state = 0; //0-idle,1-braking,2-forward,3-reverse, 4-coast
int command = 0; //0-no command, 1-brake, 2-forward, 3-reverse
int speed = 0; //Speed int RPM
int currentPWM = 0; //current PWM duty cycle
int timeSince = 0; //time since last PWM write in millisecond
int rampRate = 1; //0-255 per milisecond; initial ramp rate 1 per millisecond -> 0 - 100% in .255 sec
int initPWM = 25; //starting duty cycle
bool ramping = false;

void setup() {
  //Declaring PWM pin as output
  Serial.begin(9600);
  pinMode(pwm_pin, OUTPUT);
  pinMode(brake_pin, OUTPUT);
  pinMode(feedback_pin, OUTPUT);
}

void loop() {
  //Serial.print("hello!");
  //init timeSince as time since controller starts
  //Check input to  controller
  command = getCommand();
  //Get speed
  speed = getSpeed();
  //Execute current input
  state = execCommand(state,command);

  //Update I/Os
  analogWrite(pwm_pin, currentPWM);
  
  //Development
  delay(50);
  printPWM();
  printState();
}

int getSpeed() {
  //calculate speed from feedback_pin
  return 0;
}

int getCommand() {
  return 2; //return go forward
}

int execCommand(int state, int command){
  switch (command) {
  case 0: //no command
    // statements
    break;
  case 1: //brake
    // statements
    break;
  case 2: //go forward
    // statements
    if (state == 0 || state == 2) { //if idle or going forward
      //ramping
      goForward(getSpeed());
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


void goForward(int speed) {
  //Ramp PWM duty cycle
    if (ramping) {
      int now = millis();
      currentPWM = currentPWM + (now - timeSince)/10 * rampRate ;
      timeSince = now;
      if (debug){
        Serial.print("currentPWM: ");
        Serial.print(currentPWM);
        Serial.print("\n");
        Serial.print("now: ");
        Serial.print(now);
        Serial.print("\n");
        Serial.print("timeSince: ");
        Serial.print(timeSince);
        Serial.print("\n");
      }
      //analogWrite(pwm_pin, currentPWM);
      
      //when ramped to max Duty Cycle
      if (currentPWM >= 255) {
        ramping = false;
      }
      if (currentPWM > 255) {
        currentPWM = 255;
      }
    } else { //initial
      timeSince = millis();
      //currentPWM = initPWM; //init current PWM
      ramping = true;
      
      Serial.print("In here: ");
      Serial.print("\n");
    }
}

void printPWM(){
  Serial.print("currentPWM: ");
  Serial.print(currentPWM);
  Serial.print("\n");
}

void printState(){
  Serial.print("state: ");
  Serial.print(state);
  Serial.print("\n");
}
