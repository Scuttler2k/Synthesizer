//KNOBS: Oscillator control(pitch), Filter(cutoff, resonance, low pass), ADSR, Modulation effects
//BUTTONS: Waveform selector, filter selector, reset envelop button, effect toggle/cycle
#include <Mozzi.h>
#include <Oscil.h>
#include <tables/sin2048_int8.h>
#include <tables/saw2048_int8.h>
#include <tables/triangle2048_int8.h>
#include <tables/square_no_alias_2048_int8.h> //these four tables are for the wave switching, figure out how to do a square wave
#include <StateVariable.h> //figure out this one with filter switching
#include <ADSR.h>
#include <EventDelay.h>
#include <mozzi_rand.h>
#include <mozzi_midi.h>


//For ADSR control
const int knobPin1 = A0;
const int buttonPin1 = 5; //swap between ADSR
int ADSRVal = 0;
int ADSRnum = 0;
int ADSRCycle = 0;
//For filter control
const int knobPin2 = A1;
const int buttonPin2 = 4; //swap between filters
//for Oscillator control
const int knobPin3 = A2;
const int buttonPin3 = 3; // this button will be different as it will function as a wave form selector
int pitchControl = 0;
//For modulation effects
const int knobPin4 = A3;
const int buttonPin4 = 2; //cycle through effects

Oscil <2048, MOZZI_AUDIO_RATE> aSin(SIN2048_DATA);//main oscillator
Oscil <2048, MOZZI_AUDIO_RATE> aSaw(SAW2048_DATA);//main oscillator
Oscil <2048, MOZZI_AUDIO_RATE> aTri(TRIANGLE2048_DATA);//main oscillator
Oscil <2048, MOZZI_AUDIO_RATE> aSqu(SQUARE_NO_ALIAS_2048_DATA);//main oscillator
Oscil <2048, MOZZI_AUDIO_RATE> *currentWave = &aSin;
int waveFormCounter = 0;

Oscil <2048, MOZZI_AUDIO_RATE> lfoOsc(SIN2048_DATA);//used for effects, the ones to implement are vibrato, tremolo, ring modulation, and distortion

// adsr
ADSR <MOZZI_AUDIO_RATE, MOZZI_AUDIO_RATE> envelope;//ADSR envelope
EventDelay noteDelay;

StateVariable<LOWPASS> LPFilter;
StateVariable<HIGHPASS> HPFilter;
StateVariable<BANDPASS> BPFilter;
StateVariable<NOTCH> NFilter;
//Use .next() to choose a filter


float baseFreq = 440.0; //base frequency for the oscillator
float lfoFreq = 440.0; //base frequency for LFO

uint8_t ADSRselection = 0; //0 - Attack, 1 - Decay, 2 - Sustain, 3 - Release
uint8_t filterMode = 0;  // 0 - Low Pass, 1 - High Pass, 2 - State Variable

// button timers
unsigned long ADSRLastTime = 0;
unsigned long waveLastTime = 0;

//TODO: ADSR - add a method for a case switch for the buttons and figure out how to code potentiometers
void adsr() {
    if (noteDelay.ready()) {
        // choose envelope levels
        int attack = 100;
        int decay = 100;
        int sustain = 100;
        int releaseMs = 100;
        envelope.setADLevels(attack, decay);

        // generate random new adsr time parameter value in ms
        unsigned int newValue = (ADSRVal/1024.0)* 1500;
        Serial.print("New value:");
        Serial.println(newValue);
        Serial.print("ADSR: ");
        Serial.println(ADSRCycle);
        // randomly choose one of the adsr parameters and set new value
        switch (ADSRCycle) {
            case 0:
            attack = newValue;
            break;
            case 1:
            decay = newValue;
            break;
            case 2:
            sustain = newValue;
            break;
            case 3:
            releaseMs= newValue;
            break;
        }
        envelope.setTimes(attack, decay, sustain, releaseMs);
        envelope.noteOn(false);

        noteDelay.start(attack + decay + sustain + releaseMs);
    }
}

void pitch(){
  Serial.println(pitchControl);
  currentWave->setFreq((pitchControl / 1024.0f) * 5000.0f);
}

void toggleWave() {
  unsigned long curr = millis();
  if (digitalRead(buttonPin3) == HIGH && curr - waveLastTime >= 1000) {
    waveLastTime = curr;
    waveFormCounter++;
    if (waveFormCounter > 3) { waveFormCounter = 0; }
    switch(waveFormCounter) {
      case 0:
      currentWave = &aSin;
      Serial.println("Sine wave");
      break;
      case 1:
      currentWave = &aSaw;
      Serial.println("Saw wave");
      break;
      case 2:
      currentWave = &aTri;
      Serial.println("Triangle Wave");
      break;
      case 3:
      currentWave = &aSqu;
      Serial.println("Square Wave");
      break;
    }
  }
}

void ADSRButton(){
  unsigned long curr = millis();
  if(digitalRead(buttonPin1) == HIGH && curr - ADSRLastTime >= 1000){
    ADSRLastTime = curr;
    ADSRCycle++;
    Serial.println("button pressed");
  }
  if(ADSRCycle >= 4){
    ADSRCycle = 0;
  }
}
//TODO: Filters - figure out if StateVariableFilter allows you to swap between filters then do the same as ADSR switching but swap filters and their effect
//TODO: Pitch - just figure out potentiometers and update the frequency
//TODO: Wave selector - figure out what each wave table does and then connect it with the button to change between different waves
//TODO: Modulation - figure out different effects we want to put basically and it'll follow the same framework as ADSR switching kinda

void setup(){
  currentWave->setFreq(440);
  ADSRLastTime = millis();
  waveLastTime = millis();
  pinMode(buttonPin1, INPUT);
  pinMode(buttonPin3, INPUT);
  startMozzi(); //DO NOT REMOVE, necessary for Mozzi library
}

void updateControl(){
  adsr();
}

AudioOutput_t updateAudio(){
  envelope.update();
  return MonoOutput::from8Bit(currentWave->next());
}

void loop(){
  ADSRVal = analogRead(knobPin1);
  ADSRnum = digitalRead(buttonPin1);
  ADSRButton();


  pitchControl = analogRead(knobPin3);
  pitch();
  toggleWave();
  audioHook(); //DO NOT REMOVE, necessary for Mozzi library
}