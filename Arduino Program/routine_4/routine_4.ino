const byte numChars = 32;
char receivedChars[numChars];
char reqFunc[numChars] = {0};
int variable1, variable2, variable3;
boolean newData = false, runFunct = false, parsedData = true;

int EnablePin = 8;
int PWMPin = 11;
int PWMPin2 = 3;
int spd, n, pulseHeight, c, c2, caliSpd, i;
int homeRepeat = 3; // Set the number of times to attempot homeing routine before giving up
int speeda = 120;
int IRThresh = 100; // threshold value for home position
int speedb = 60;
int dlya, dlyb;
int IRsensor;
float speedaFloat, speedbFloat, dlyaFloat, dlybFloat;

void setup() {                
  pinMode(EnablePin, OUTPUT);     
  pinMode(PWMPin, OUTPUT);
  analogWrite(PWMPin, 0);
  pinMode(PWMPin2, OUTPUT);
  analogWrite(PWMPin2, 0);
  setPwmFrequency(PWMPin, 8);  // change Timer2 divisor to 8 gives 3.9kHz PWM freq
  Serial.begin(9600);
  digitalWrite(EnablePin, HIGH);
}

void loop() {
  
    recvWithStartEndMarkers();  // look for message via serial input
    parseData();                // split message into 3 parts: string (defines required function), variable1 and variable2
    //showNewData();

    if (runFunct == true && strcmp(reqFunc, "spin")  == 0){ //if ==0 they are the same
      spinUp();
      runFunct = false; // reset function variable
    } 

    if (runFunct == true && strcmp(reqFunc, "home")  == 0){ //if ==0 they are the same
      homing();
      runFunct = false; // reset function variable
    } 

    if (runFunct == true && strcmp(reqFunc, "calibration")  == 0){ //if ==0 they are the same
      calibration();
      runFunct = false; // reset function variable
    } 

  delay(10);

}

//--------- Functions -------//

// Reading serial data

// type in a string and 3 variables in the form <string_100_200_300>
void recvWithStartEndMarkers() {
    static boolean recvInProgress = false;
    static byte ndx = 0;
    char startMarker = '<';
    char endMarker = '>';
    char rc;
 
    while (Serial.available() > 0 && newData == false) {
        rc = Serial.read();

        if (recvInProgress == true) {
            if (rc != endMarker) {
                receivedChars[ndx] = rc;
                ndx++;
                if (ndx >= numChars) {
                    ndx = numChars - 1;
                }
            }
            else {
                receivedChars[ndx] = '\0'; // terminate the string
                recvInProgress = false;
                ndx = 0;
                newData = true;
                runFunct = true;
            }
        }

        else if (rc == startMarker) {
            recvInProgress = true;
        }
    }
}

// parsing serial data
void parseData() {
    if (newData == true) {
    
  char * strtokIndx; // this is used by strtok() as an index
  
  strtokIndx = strtok(receivedChars,"_");      // get the first part - the string
  strcpy(reqFunc, strtokIndx); // copy it to messageFromPC
  
  strtokIndx = strtok(NULL, "_"); // this continues where the previous call left off
  variable1 = atoi(strtokIndx);     // convert this part to an integer
  
  strtokIndx = strtok(NULL, "_"); // this continues where the previous call left off
  variable2 = atoi(strtokIndx);     // convert this part to an integer

  strtokIndx = strtok(NULL, "_"); // this continues where the previous call left off
  variable3 = atoi(strtokIndx);     // convert this part to an integer

    newData = false;
    parsedData = true;
    }
}

// showing serial data
void showNewData() {
    if (parsedData == true) {
           Serial.print("Function- ");
           Serial.println(reqFunc);
           Serial.print("Variable1- ");
           Serial.println(variable1);
           Serial.print("Variable2- ");
           Serial.println(variable2);
           Serial.print("Variable3- ");
           Serial.println(variable3);
        parsedData = false;
    }
}

//-----------start of functions to control centrifuge------------------------//

// Finding home position

void homing() {

  // here variable1 is used as the pulse magnitude, limits: 100 - 150, typically 125 or 135 are used
  if (variable1>=100 && variable1<=150 && variable2==0 && variable3==0){
    pulseHeight = variable1;
    c = 0; // set counter
    c2 = 0;
    Serial.println("Finding home");
    IRsensor = analogRead(A0);
    while(IRsensor <= IRThresh){
      analogWrite(PWMPin, pulseHeight);
      delay(5);
      analogWrite(PWMPin, 0);
      delay(500);
      IRsensor = analogRead(A0);
      c = c + 1;
  
      // nudge contingency adjustments
      
      if (c == 40){                     // if we have done 4 rotations of nudgeing (without finding home)
        pulseHeight = pulseHeight + 10;  // increase pulseheight
      }
  
      if (c == 60){                     // if we have done 6 rotations of nudgeing (without finding home)
        pulseHeight = pulseHeight - 20; // decrease pulseheight (5 below set value)
      }
  
      if (c == 80){                     // if we have done 8 rotations of nudgeing (without finding home)
        if (c2 < homeRepeat){
        analogWrite(PWMPin, 55);        // rotate at slow speed for 3 s
        delay(3000);
        c = 0;                          // reset counter to repeat everything
        c2 = c2 + 1;                    // increment counter2
        pulseHeight = variable1;        // reset to set value
        }
        else if (IRsensor <= IRThresh) {
          Serial.println("Cannot find home");
          IRsensor = 200;
        }
      }
    }
      if (c2 <= homeRepeat || IRThresh > IRThresh){
      Serial.println("At home position");
      }
    // Reset chip
    digitalWrite(EnablePin, LOW);
    delay(500);
    digitalWrite(EnablePin, HIGH);
  }
  else{ // variable out of allowed range
    Serial.println("Check variables");
  }
}

// Spin function
void spinUp() {

  // variable1 - ramp accelleration, units - g/s, limits: 200 - 1500 g/s
  // variable2 - top speed, units - g           , limits: 180 - 1870 g
  // variable3 - hold time, units - s           , limits: 1 - 1000   s

  // First check inputs are within limits
  if (variable1>=200  && variable1<=1500 && variable2>=180  && variable2<=1870 && variable3>=1  && variable3<=1000){ //within limits

    // convert variable2 (top speed, units of g) to voltage 'speeda' (0-255 units)
    // This is based on measured rpm-V calibration curve
    speedaFloat = round((0.000000012 * pow(variable2, 3)) - (0.00005 * pow(variable2, 2)) + (0.1276 * variable2) + 28.766);
    speeda = (int) speedaFloat;
  
    // convert variable1 (ramp rate, units of g/s) to delay time (units of ms). 
    dlyaFloat = round((((float)variable2 / (float)variable1) * 1000) / speeda); // calculate delay time in ms
    dlya = (int) dlyaFloat;
    
    // Ramp up to speed
    Serial.println("Ramping up");
    analogWrite(PWMPin2, 0);
    for(spd = 1; spd <= speeda; spd += 1){
      analogWrite(PWMPin, spd);
      delay(dlya);
    }
  
    // Hold top speed
    analogWrite(PWMPin, speeda);
    Serial.println("At top speed");
    delay(variable3 * 1000);
  
    // Ramp down
    Serial.println("Ramping down");
    for(spd = speeda; spd >= 1; spd -= 1){
      analogWrite(PWMPin, spd);
      delay(dlya);
    }
    analogWrite(PWMPin, 0); // set speed to zero
  
    // Reset chip
    digitalWrite(EnablePin, LOW);
    delay(500);
    digitalWrite(EnablePin, HIGH);
  
    // send function complete message
    Serial.println("Spin complete");

  }
  else { // input variable were outside limits
    Serial.println("Check variables");
  }
}

// Calibration function
void calibration() {

  for(i = 10; i >= 1; i--){ // Countdown
    Serial.print(i);
    Serial.print(", ");
    delay(1000);
  }
  Serial.println();

  for(caliSpd = 50; caliSpd <= 170; caliSpd += 10){ // Move through range fo speeds for calibration
    Serial.print(caliSpd);
    Serial.print(", ");
    analogWrite(PWMPin, caliSpd);
    delay(5000);
  }

  Serial.println();
  analogWrite(PWMPin, 0);
  Serial.print("Calibration sequence complete");

  digitalWrite(EnablePin, LOW); // Toggle enable to reset the power chips
  delay(500);
  digitalWrite(EnablePin, HIGH);
  delay(500);

}
  
//-----------function that came with motor control board------------------------//

void setPwmFrequency(int pin, int divisor) {
  byte mode;
  if(pin == 5 || pin == 6 || pin == 9 || pin == 10) { // Timer0 or Timer1
    switch(divisor) {
      case 1: mode = 0x01; break;
      case 8: mode = 0x02; break;
      case 64: mode = 0x03; break;
      case 256: mode = 0x04; break;
      case 1024: mode = 0x05; break;
      default: return;
    }
    if(pin == 5 || pin == 6) { 
      TCCR0B = TCCR0B & 0b11111000 | mode; // Timer0
    } else {
      TCCR1B = TCCR1B & 0b11111000 | mode; // Timer1
    }
  } else if(pin == 3 || pin == 11) {
    switch(divisor) {
      case 1: mode = 0x01; break;
      case 8: mode = 0x02; break;
      case 32: mode = 0x03; break;
      case 64: mode = 0x04; break;
      case 128: mode = 0x05; break;
      case 256: mode = 0x06; break;
      case 1024: mode = 0x7; break;
      default: return;
    }
    TCCR2B = TCCR2B & 0b11111000 | mode; // Timer2
  }
}
