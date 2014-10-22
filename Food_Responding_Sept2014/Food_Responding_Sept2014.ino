/*
  laser and house light control program
  created 2013
  by Glenn Morton 4/20/2013
  
  Modified by Elizabeth G Guy 9/23/2014

  The circuit: (notice that all pins are organized by groups where inputs are first and outputs are last.)
   
*/


typedef struct OperantResponse
{  
  int inputPin;                   // pin to read inputs on the operant response device (nosepokes)
  boolean state;                   // State used to set the operant device
};

typedef struct Stimulus
{  
  int outputPin;                   // pin to turn the reinforcer on and off (Arduino to pellet dispenser and/or light)
  unsigned long OnTime;            // stores milliseconds of ON time for CS light or reward dispenser
  boolean state;                   // State used to set the reinforcer
  unsigned long startTime;         // Store start of stimulus
  unsigned long previousMillis;     // store last update time
};

const unsigned long DispenserOnTime = 50;         // set your  ON time here for pellet dispenser
const int RewardLightOnTime = 2000;       // set how long the CS associated with the pellet will be on
const int ReinforcerDelay = 1000;        // set how long in millis you want to delay reinforcer delivery when the CS is on
const int RandomRatio = 2;                   // set the random ratio value for your schedule of reinforcement

// pin information
const int ButtonPin = 2;     // Button for controlling the house light
const int HouseLight = 4;    // the house light- turn on at start of program and turn off for 2s during reinforcer presentations

const int Nosepoke_NonStim = 7;     // Button for reading the non-stimulated nosepoke hole
const int Nosepoke_Stim = 9;     // Button for reading the stimulated nosepoke hole

const int RewardLight = 12;     // Reward light for pellet dispenser

const int PelletDispenser = 11;  // Pellet dispenser pin

// Variables list:

boolean lightState = HIGH;                       // current state of the light

OperantResponse NonStim = {Nosepoke_NonStim, LOW};      // left operant stimulus information
OperantResponse Stim = {Nosepoke_Stim, LOW};      // right operant stimulus information

Stimulus rewardlight= {RewardLight, RewardLightOnTime, LOW, 0, 0};  // light information for light CS paired with reinforcer

Stimulus dispenser = {PelletDispenser, DispenserOnTime, LOW, 0, 0};  //pellet dispenser information

int masterClock = 0;
int seconds = 0;
int minutes = 0;
int hours = 0;
unsigned long lastChecked = 0;

boolean startLight = false;
int lightStartInSeconds = 0;

//Initialization: set all inputs as INPUT, outputs as OUTPUT and set the initial value of outputs
void setup() {
  Serial.begin(9600);
  
  randomSeed(analogRead(0)); // seed for the RR2 control of reward delivery

  pinMode(rewardlight.outputPin, OUTPUT);
  digitalWrite(rewardlight.outputPin, HIGH);
  
  pinMode(dispenser.outputPin, OUTPUT);
  digitalWrite(dispenser.outputPin, LOW);
  
  pinMode(NonStim.inputPin, INPUT);
  pinMode(Stim.inputPin, INPUT);
  

  pinMode(HouseLight, OUTPUT);
  digitalWrite(HouseLight, HIGH);
}

//Main loop. Evrything in here runs every frame
void loop() {
  // update the reinforcer stimulus
  rewardlight = UpdateRewardLight(rewardlight);
  
  //Update the pellet dispenser
  dispenser = UpdateDispenser(dispenser, rewardlight);
  // control the nosepokes
  NosepokeHandler();
  
  
  // control the house light
  HouseLighthandler();

  
  Clock();
}

// Clock function: updates global variables for tracking the amount of time this program has been running.
// parameters: none
// returns: none
void Clock()
{
  boolean printTime = false;
  unsigned long currentTime = millis();
  static int tenths = 100;
  
  if(lastChecked == 0)
  {
    lastChecked = currentTime;
  }
  if(currentTime - lastChecked >= tenths)
  {
    tenths += 100;
    Serial.println(".");
  }
  if(currentTime - lastChecked >= 1000)
  {    
    seconds++;
    tenths = 100;
    lastChecked += 1000;
    //printTime = true;
  }
  if(seconds >= 60)
  {
    minutes++;
    seconds -= 60;
  }
  if(minutes >= 60)
  {
    hours++;
    minutes -=60;
  }
  if(printTime)
  {
    Serial.print(hours);
    Serial.print(":");
    Serial.print(minutes);
    Serial.print(":");
    Serial.print(seconds);
    Serial.print("  millis:");
    Serial.println(currentTime-lastChecked);
 
    printTime = false;
  }
}
// UpdateRewardLight function: If the input pin is trying to turn the output pin on, have it turn on at the specified length of time; otherwise turn the light off.
// parameters: 
//   rewardlight: the light is either on or off
// returns: 
//   the updated state of the reinforcer stimulus
struct Stimulus UpdateRewardLight(struct Stimulus curLight) {
  unsigned long currentMillis = millis();
  
  if (startLight){
      if (curLight.state == LOW){
      curLight.state = HIGH;
      curLight.startTime = currentMillis;
      Serial.print ("Reinforcer ");
      Serial.println("1");
      }
      else{       
      if (currentMillis- curLight.startTime > curLight.OnTime){
        curLight.state = LOW; //light turns off
        startLight =false;
        Serial.print ("Reinforcer ");
        Serial.println("0");
        }
      } 
  } 
    //update output with current state
    digitalWrite(rewardlight.outputPin, curLight.state);
    return curLight;
  }
 //Update Pellet Dispenser Function

//Update Pellet Dispenser Function
struct Stimulus UpdateDispenser(struct Stimulus curDispenser, struct Stimulus curLight) {
  unsigned long currentMillis = millis();
  
  Serial.print("startLight ");
  Serial.println(startLight);
  
  
  
  if (startLight){
      if (currentMillis - curLight.startTime >= ReinforcerDelay){
        if (curDispenser.state == LOW){
            curDispenser.state = HIGH;
            curDispenser.startTime = currentMillis;
        }            
        else if (currentMillis-curDispenser.startTime >= curDispenser.OnTime){
           curDispenser.state = LOW; 
        }
    } else {
        curDispenser.state= LOW;
    }
  }
  //update output with current state
  digitalWrite(dispenser.outputPin, curDispenser.state);
  return curDispenser;
}

void NosepokeHandler() {
  NosepokeHandlerStim();
  NosepokeHandlerNonStim();
}

// WheelHandler function: controls the events related to the wheel
// parameters: none
// returns: none
void NosepokeHandlerStim() {
  static int lastNosepokeState = LOW;               // keeps track of whether the wheel was pressed ON or released OFF
  //static int lastNosepokeTime = 0;
  static int NosepokesInTriggerWindow = 0;         // tracking variable for the turns that occured during the triggering window
  // unsigned long randNumber                     // variable to store the output of the random function
  int randNumber = 1;
 
  
  // get the current input from the wheel
  Stim.state = digitalRead(Stim.inputPin);
  
  // if wheel is not HIGH and was LOW the previous frame (i.e. rising edge detecting)
  if (lastNosepokeState == LOW && Stim.state == HIGH) {
    Serial.print("NosepokeStim  ");
    Serial.println(Stim.state);
    // turn the laser on if off
    if(!startLight) {
      
      if(NosepokesInTriggerWindow <= 10) { // initialize triggering window
        
        NosepokesInTriggerWindow++;  //update nosepoke counter occured during the triggering window
        startLight = true;          //turn on reinforcer
        Serial.print("Reinforcer ");
        Serial.println("1");
      }
      else{ //more than 10 nosepokes occured
          randNumber = random(RandomRatio);
          Serial.println(randNumber);
          if(randNumber =0){     
          // start the laser since enough turns occured during the triggering window
          startLight = true;
          Serial.print("Reinforcer ");
          Serial.println("1");
          }
      }
    }
    else{
            startLight= false;
          }
       }
  lastNosepokeState = Stim.state;         // keep the current state of the nosepoke apparatus for the next frame
  }
void NosepokeHandlerNonStim() {
  static int lastNosepokeState = LOW;      // keeps track of whether the wheel was pressed ON or released OFF
  
   // get the current input from the wheel
  Stim.state = digitalRead(Stim.inputPin);
  
  // if wheel is not HIGH and was LOW the previous frame (i.e. rising edge detecting)
  if (lastNosepokeState == LOW && Stim.state == HIGH) {
    Serial.print("NosepokeNonStim  ");
    Serial.println("1");
  }
  else{
    Serial.print("NosepokeNonStim  ");
    Serial.println("0");
  }
 lastNosepokeState = Stim.state;         // keep the current state of the nosepoke apparatus for the next frame
}

//Controlling the Houselight:  On unless the reinforcer is on (then off)
void HouseLighthandler (){
  if (startLight == false){
     digitalWrite (HouseLight, HIGH);
  }
  else {
      digitalWrite (HouseLight, LOW);
  }
}

