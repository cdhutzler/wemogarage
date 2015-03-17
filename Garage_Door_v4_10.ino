//Input Status pins
int WeMoStatus = 2; //Is the WeMo ON or OFF (true=HIGH=3.3V or false=LOW=0V)
int GarageOpen = 4; //Magnetic switch #1 is closed (ON) when door is open
int GarageClosed = 6; //Magnetic switch #2 is closed (ON) when door is closed

//Output pins
int WeMoToggle = 8;  //Briefly Close and then re-Open the Relay to tell the WeMo to change states
int GarageDoorToggle = 12; //Briefly Close and then re-Open the Relay to tell the Garage Door to change states
int DoorLED = 13; //If pin  is low, then arduino's built in LED is off and door is closed, otherwise pin goes high to signify door is open which lights the built in LED. 
int Buzzer = 9; //Alarm buzzer used to warn of a door about to open or close when triggered by an iphone/android or IFTTT or other Internet thing

//Status Variables
int WeMoLastStatus; //Last known status of the WeMo for comparison to see if an iPhone triggered it.
int DoorMoved; //Did the Garage Door move at all (to the opposite state or even briefly and then back).

//Variables to blink an LED on an interval of 300ms
long previousMillis = 0; //used to blink LEDs without using the DELAY function
long interval = 300; // interval at which to blink (milliseconds)

//Finite State Machine variable "door_state"
int door_state;
//Define some human readable values/states for the door
#define DoorClosed  0
#define DoorOpen   1
#define DoorInBetween 9

void setup() {
  //Tell the Arduino how the various I/O pins will be used (inputs or outputs)
  pinMode (WeMoStatus, INPUT);
  pinMode (GarageOpen, INPUT_PULLUP);
  pinMode (GarageClosed, INPUT_PULLUP);
  pinMode (WeMoToggle, OUTPUT);
  pinMode (GarageDoorToggle, OUTPUT);
  pinMode (DoorLED, OUTPUT);
  pinMode (Buzzer, OUTPUT);


  //Set initial state of Garage Door Pin to LOW (so darlington transistor is OFF and not affecting the garage door motor) and WeMo to HIGH (so Adruino output pin is not affecting the WeMo) 
  //FYI - you must pull WeMo to ground to trigger state change and then let it go back to HIGH afterwards.
  digitalWrite(GarageDoorToggle,LOW);
  digitalWrite(WeMoToggle,HIGH);
  digitalWrite(Buzzer,LOW);

  //Pause here for 2 mintes (120 seconds) to allow the WEMO to boot, find the wifi, and be online.
  delay(120000);
  //Find and set the initial door state to initialize the system and ensure door, wemo and door state are all set properly
  door_state = CurrentDoorStatus();
  CheckAndFixWeMoSync();
  //Set the WeMoLastStatus to current status so we can later check to see if the WeMo status changes
  WeMoLastStatus=digitalRead(WeMoStatus);

  //  Serial.begin(9600); //Set Serial communication back to computer on USB to monitor states for testing
}

void loop() {
  //Beginning of the Finite State Machine "door_state" SWITCH statement
  switch(door_state) {
  case DoorClosed:
    {
      //Did the WeMo status change? If so, move that door.
      if (WeMoStatusChanged()){
        moveDoor();
        WeMoLastStatus=digitalRead(WeMoStatus);
      }  
      //Did the Door change on its own (via a car remote, etc)? If so, resync the WeMo to the door state.
      if (DoorMoved){
        CheckAndFixWeMoSync();
        WeMoLastStatus=digitalRead(WeMoStatus);
        DoorMoved=false;
      }
      door_state=CurrentDoorStatus();
      delay(100);   // delay in between reads for stability   
    }
    break;
  case DoorOpen:
    {
      //Did the WeMo status change? If so, move that door.
      if (WeMoStatusChanged()){
        soundWarningBeeper();
        moveDoor();
        WeMoLastStatus=digitalRead(WeMoStatus);
      }  
      //Did the Door change on its own (via a car remote, etc)? If so, resync the WeMo to the door state.
      if (DoorMoved){
        CheckAndFixWeMoSync();
        WeMoLastStatus=digitalRead(WeMoStatus);
        DoorMoved=false;
      }
      door_state=CurrentDoorStatus(); // delay in between reads for stability
      delay(100);
    }
    break;
  case DoorInBetween:
    {
      //If the door moved, set the status variable to true so that we can resync the Wemo if needed once the garage door is stationary again
      DoorMoved=true;
      door_state=CurrentDoorStatus();
      delay(100); // delay in between reads for stability
    }
    break;
  }
}

//**********************************Subroutines*****************************************

//This routine is used to syncronize the WEMO state with the garage door - usually because the door changed state due to someone using a traditional button/car remote instead of the WEMO
int CheckAndFixWeMoSync(){
  if (CurrentDoorStatus()==1 && digitalRead(WeMoStatus)==LOW || (CurrentDoorStatus()==0 && digitalRead(WeMoStatus)==HIGH)){    
    digitalWrite(WeMoToggle, LOW);
    delay(500);
    digitalWrite(WeMoToggle, HIGH);
    delay(500); 
  }
}

//Used to see if the WEMO's status changed due to an internet device (e.g., iphone/android of IFTTT) caused it to change.
int WeMoStatusChanged(){
  if (digitalRead(WeMoStatus)!=WeMoLastStatus)
    return 1;
  else
    return 0;
}

//This routine checks the status of the door by reading the magnetic switches (and handles setting the LED on the Arduino to reflect the status - mostly for troubleshooting)
int CurrentDoorStatus () {
  //If the door is open, light the PIN13 LED
  if (digitalRead(GarageOpen)==LOW){
    digitalWrite(DoorLED, HIGH);
    return DoorOpen;
  }
  //If the door is closed, turn PIN13 LED OFF    
  if (digitalRead(GarageClosed)==LOW){
    digitalWrite(DoorLED, LOW);
    return DoorClosed;
  }
  //If the door is not open or closed, flash PIN13 LED    
  if (digitalRead(GarageClosed)==HIGH && digitalRead(GarageOpen)==HIGH)
  {
    unsigned long currentMillis = millis();
    if(currentMillis - previousMillis > interval) 
    {
      previousMillis = currentMillis;   
      if (digitalRead(DoorLED) == LOW)
        digitalWrite(DoorLED, HIGH);
      else
        digitalWrite(DoorLED, LOW);
    }
    return DoorInBetween;
  }
}

int soundWarningBeeper (){ 
  //Sound the alarm buzzer for 5 seconds to warn anyone in or around the garage that the door is about to move. 
  for (int i=0; i <= 5; i++){  
    digitalWrite(Buzzer, HIGH);
    delay (500);
    digitalWrite(Buzzer, LOW);
    delay (500);
  }    
}

//This routine moves the door. First it sounds the buzzer for 5 seconds, then it toggles the Darlington Transistor (or a relay) to make the door move. 
//Lastly it checks to be sure the door did move otherwise it tries one more time - some doors do not pay attention the first time for some reason. Chamberlin seems to have this issue.
int moveDoor (){ 
  digitalWrite(GarageDoorToggle, HIGH);
  delay(800);
  digitalWrite(GarageDoorToggle, LOW);
  //Give the door a chance to move (delay) so that the code does not think there is a sync issue and step on itself.
  delay(1000);

  //Did the door move? If not, toggle the garage motor relay one more time as the Garage door opener sometimes ignores the first attempt
  if (digitalRead(GarageClosed)==LOW || digitalRead(GarageOpen)==LOW)
  {
    digitalWrite(GarageDoorToggle, HIGH);
    delay(800);
    digitalWrite(GarageDoorToggle, LOW);
    //Give the door a chance to move (delay) so that the code does not think there is a sync issue and step on itself.
    delay(1000);
  }
}
