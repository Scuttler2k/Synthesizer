//KNOBS: Oscillator control(pitch), Filter(cutoff, resonance, low pass), ADSR, Modulation effects
//BUTTONS: Waveform selector, filter selector, reset envelop button, effect toggle/cycle
#include <Mozzi.h>
#include <Oscil.h>
#include <tables/sin2048_int8.h>
#include <tables/saw2048_int8.h>
//#include <tables/square2048_int8.h> //these three tables are for the wave switching, figure out how to do a square wave
#include <ADSR.h>
#include <LowPassFilter.h> // not needed possibly?
#include <StateVariable.h> //figure out this one with filter switching
//For ADSR control
const int knobPin1 = A0;
const int buttonPin1 = 2; //reset ADSR
//For filter control
const int knobPin2 = A1;
const int buttonPin2 = 3; //swap between filters
//for Oscillator control
const int knobPin3 = A2;
const int buttonPin3 = 4; // this button will be different as it will function as a wave form selector
//For modulation effects
const int knobPin4 = A3;
const int buttonPin4 = 5; //cycle through effects

Oscil <2048, MOZZI_AUDIO_RATE> aSin(SIN2048_DATA);//main oscillator
Oscil <2048, MOZZI_AUDIO_RATE> lfoOsc(SIN2048_DATA);//used for effects, the ones to implement are vibrato, tremolo, ring modulation, and distortion

ADSR <MOZZI_AUDIO_RATE, MOZZI_AUDIO_RATE> envelope;//ADSR envelope

float baseFreq = 440.0; //base frequency for the oscillator

uint8_t ADSRselection = 0; //0 - Attack, 1 - Decay, 2 - Sustain, 3 - Release
uint8_t filterMode = 0;  // 0 - Low Pass, 1 - High Pass, 2 - State Variable

void setup(){
  startMozzi(); //DO NOT REMOVE, necessary for Mozzi library

}

void updateControl(){

}

AudioOutput_t updateAudio(){

}

void loop(){
  audioHook(); //DO NOT REMOVE, necessary for Mozzi library
}