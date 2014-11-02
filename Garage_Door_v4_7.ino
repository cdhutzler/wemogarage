//Input Status pins
int WeMoStatus = 2; //Is the WeMo ON or OFF (true=HIGH=3.3V or false=LOW=0V)
int GarageOpen = 4; //Magnetic switch #1 is closed (ON) when door is open
int GarageClosed = 6; //Magnetic switch #2 is closed (ON) when door is closed

//Output pins
int WeMoToggle = 8;  //Briefly Close and then re-Open the Relay to tell the WeMo to change states
int GarageDoorToggle = 12; //Briefly Close and then re-Open the Relay to tell the Garage Door to change states
int DoorLED=13; //If pin  is low, then RED LED is lit and door is closed, otherwise pin goes high to signify door is open which lights a green LED. Or we flash Red/Green if door is in between. 

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

  //Set initial state of Garage Door Pin to LOW (so darlington transistor is OFF and not affecting the garage door motor) and WeMo to HIGH (so Adruino output pin is not affecting the WeMo) 
  //FYI - you must pull WeMo to ground to trigger state change and then let it go back to HIGH afterwards.
  digitalWrite(GarageDoorToggle,LOW);
  digitalWrite(WeMoToggle,HIGH);

  //Find and set the initial door state
  door_state = CurrentDoorStatus();

  //Set the WeMoLastStatus to current status so we can later check to see if the WeMo status changes
  WeMoLastStatus=digitalRead(WeMoStatus);

  //  Serial.begin(9600); //Set Serial communication back to computer on USB to monitor states for testing
}

void loop() {

  //Finite State Machine "door_state"

  //  Serial.println(door_state);

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
    }
    break;
  case DoorOpen:
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
    }
    break;
  case DoorInBetween:
    {
      //If the door moved, set the status variable to true so that we can resync the Wemo if needed once the garage door is stationary again
      DoorMoved=true;
      door_state=CurrentDoorStatus();
    }
    break;
  }
}

//**********************************Subroutines*****************************************

int CheckAndFixWeMoSync(){
  if (CurrentDoorStatus()==1 && digitalRead(WeMoStatus)==LOW || (CurrentDoorStatus()==0 && digitalRead(WeMoStatus)==HIGH)){    
    digitalWrite(WeMoToggle, LOW);
    delay(500);
    digitalWrite(WeMoToggle, HIGH);
    delay(500);
  }
}

int WeMoStatusChanged(){
  if (digitalRead(WeMoStatus)!=WeMoLastStatus)
    return 1;
  else
    return 0;
}

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
