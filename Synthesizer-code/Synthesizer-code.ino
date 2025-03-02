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
#include <Smooth.h>


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

// track the different oscillators and the current one
Oscil <2048, MOZZI_AUDIO_RATE> aSin(SIN2048_DATA);
Oscil <2048, MOZZI_AUDIO_RATE> aSaw(SAW2048_DATA);
Oscil <2048, MOZZI_AUDIO_RATE> aTri(TRIANGLE2048_DATA);
Oscil <2048, MOZZI_AUDIO_RATE> aSqu(SQUARE_NO_ALIAS_2048_DATA);
Oscil <2048, MOZZI_AUDIO_RATE> oscillators[4] = {aSin, aSaw, aTri, aSqu};
int waveFormCounter = 0;

Oscil <2048, MOZZI_AUDIO_RATE> vibratoLFO(SIN2048_DATA); //used for effects, the ones to implement are vibrato, tremolo, ring modulation, and distortion
Oscil <2048, MOZZI_AUDIO_RATE> modLFO(SIN2048_DATA);
int modCounter = 0;// 0 - nothing, 1 - Tremolo, 2 - Vibrato, 3 - Distortion, 4 - Ring modulation
int modStr = 0;

// adsr
ADSR <MOZZI_AUDIO_RATE, MOZZI_AUDIO_RATE> envelope; //ADSR envelope
EventDelay noteDelay;

StateVariable<NOTCH> LPFilter;
StateVariable<HIGHPASS> HPFilter;
StateVariable<BANDPASS> BPFilter;
StateVariable<NOTCH> NFilter;
int filterCounter = 0;
int resonanceControl = 0;
//Use .next() to choose a filter

// smooth functions
Smooth<float> smoothPitch(0.01f);

float baseFreq = 440.0; //base frequency for the oscillator
float lfoFreq = 440.0; //base frequency for LFO

uint8_t ADSRselection = 0; //0 - Attack, 1 - Decay, 2 - Sustain, 3 - Release
uint8_t filterMode = 0;  // 0 - Low Pass, 1 - High Pass, 2 - State Variable

// button timers
unsigned long ADSRLastTime = 0;
unsigned long waveLastTime = 0;
unsigned long filterLastTime = 0;
unsigned long modLastTime = 0;

void adsr() {
    if (noteDelay.ready()) {
        // choose envelope levels
        int adsrValues[4] = {100, 100, 100, 100};
        int attack = 100;
        int decay = 100;
        int sustain = 100;
        int releaseMs = 100;
        envelope.setADLevels(adsrValues[0], adsrValues[1]);

        // randomly choose one of the adsr parameters and set new value
        // generate random new adsr time parameter value in ms
        unsigned int newValue = (ADSRVal/1024.0)* 1500;
        Serial.print("New value:");
        Serial.println(newValue);
        Serial.print("ADSR: ");
        Serial.println(ADSRCycle);
        adsrValues[ADSRCycle] = newValue;

        envelope.setTimes(adsrValues[0],adsrValues[1], adsrValues[2], adsrValues[3]);
        envelope.noteOn(false);

        noteDelay.start(adsrValues[0] + adsrValues[1] + adsrValues[2] + adsrValues[3]);
    }
}

void pitch(){
  Serial.println(pitchControl);
  oscillators[waveFormCounter].setFreq((pitchControl / 1024.0f) * 5000.0f);
}

void filter(){
  resonanceControl = (20+(resonanceControl/1024.0f)*4096);
  Serial.println(filterCounter);
  switch (filterCounter) {
    case 0:
      LPFilter.setResonance(220);
      LPFilter.setCentreFreq(resonanceControl);
      break;
    case 1:
      HPFilter.setResonance(220);
      HPFilter.setCentreFreq(resonanceControl);
      break;
    case 2:
      BPFilter.setResonance(220);
      BPFilter.setCentreFreq(resonanceControl);
      break;
    case 3:
      NFilter.setResonance(220);
      NFilter.setCentreFreq(resonanceControl);
      break;
  }

}

void modulation()){
  modStr = (modStr/1023.0f);
  switch(modCounter){
    case 0:
      break;
    case 1:
      break;
    case 2:

      break;
    case 3:

      break;
    case 4:

      break;
  }
}

void toggleWave() {
  unsigned long curr = millis();
  if (digitalRead(buttonPin3) == HIGH && curr - waveLastTime >= 1000) {
    waveLastTime = curr;
    waveFormCounter++;
    if (waveFormCounter > 3) { waveFormCounter = 0; }
  }
}

void toggleFilter() {
  unsigned long curr = millis();
  if (digitalRead(buttonPin2) == HIGH && curr - filterLastTime >= 1000) {
    filterLastTime = curr;
    filterCounter++;
    if (filterCounter > 3) { filterCounter = 0; }
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
//TODO: Modulation - figure out different effects we want to put basically and it'll follow the same framework as ADSR switching kinda

void ModulationButton(){
  unsigned long curr = millis();
  if(digitalRead(buttonPin4) == HIGH && curr - modLastTime >= 1000){
    modLastTime = curr;
    modCounter++;
    Serial.println("button pressed");
  }
  if(modCounter > 4){
    modCounter = 0;
  }
}

void setup(){
  oscillators[waveFormCounter].setFreq(440);
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
  switch (filterCounter) {
    case 0:
  return MonoOutput::fromAlmostNBit(9, LPFilter.next(oscillators[waveFormCounter].next()));
    case 1:
  return MonoOutput::fromAlmostNBit(9, HPFilter.next(oscillators[waveFormCounter].next()));
    case 2:
  return MonoOutput::fromAlmostNBit(9, BPFilter.next(oscillators[waveFormCounter].next()));
    case 3:
  return MonoOutput::fromAlmostNBit(9, NFilter.next(oscillators[waveFormCounter].next()));
  }
}

void loop(){
  ADSRVal = analogRead(knobPin1);
  ADSRnum = digitalRead(buttonPin1);
  ADSRButton();


  pitchControl = analogRead(knobPin3);
  pitchControl = smoothPitch.next((float) pitchControl);
  pitch();
  toggleWave();
  
  resonanceControl = analogRead(knobPin2);
  filter();
  toggleFilter();

  modStr = analogRead(knobPin4);



  audioHook(); //DO NOT REMOVE, necessary for Mozzi library
}