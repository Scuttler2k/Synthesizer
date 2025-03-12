//KNOBS: Oscillator control(pitch), Filter(cutoff, resonance, low pass), ADSR, Modulation effects
//BUTTONS: Waveform selector, filter selector, reset envelop button, effect toggle/cycle
#include <Mozzi.h>
#include <Oscil.h>
#include <tables/sin2048_int8.h>
#include <tables/saw2048_int8.h>
#include <tables/triangle2048_int8.h>
#include <tables/square_no_alias_2048_int8.h> //these four tables are for the wave switching, figure out how to do a square wave
#include <tables/cos2048_int8.h>
#include <StateVariable.h> //figure out this one with filter switching
#include <ADSR.h>
#include <EventDelay.h>
#include <mozzi_rand.h>
#include <mozzi_midi.h>
#include <Smooth.h>

#include <Phasor.h>


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

Oscil <2048, MOZZI_AUDIO_RATE> tremoloLFO(SIN2048_DATA);
Oscil <2048, MOZZI_AUDIO_RATE> vibratoLFO(SIN2048_DATA); //used for effects, the ones to implement are vibrato, tremolo, ring modulation, and distortion
Oscil <2048, MOZZI_AUDIO_RATE> modLFO(SIN2048_DATA);
int modCounter = 0;// 0 - nothing, 1 - Tremolo, 2 - Vibrato, 3 - Distortion, 4 - Ring modulation
int modStr = 0;
// a line to interpolate control tremolo at audio rate
Line <unsigned int> aGain;
// Oscillators at different frequencies for ring modulation
Oscil <2048, MOZZI_AUDIO_RATE> ringOscillators[4] = {
  Oscil <2048, MOZZI_AUDIO_RATE> (COS2048_DATA),
  Oscil <2048, MOZZI_AUDIO_RATE> (SAW2048_DATA),
  Oscil <2048, MOZZI_AUDIO_RATE> (TRIANGLE2048_DATA),
  Oscil <2048, MOZZI_AUDIO_RATE> (SQUARE_NO_ALIAS_2048_DATA),
};

//Phasor<MOZZI_AUDIO_RATE> distortionPhasor;
Oscil <2048, MOZZI_AUDIO_RATE> distortionPhasor(SQUARE_NO_ALIAS_2048_DATA);

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
        adsrValues[ADSRCycle] = newValue;

        envelope.setTimes(adsrValues[0],adsrValues[1], adsrValues[2], adsrValues[3]);
        envelope.noteOn(false);

        noteDelay.start(adsrValues[0] + adsrValues[1] + adsrValues[2] + adsrValues[3]);
    }
}

void pitch(){
  oscillators[waveFormCounter].setFreq((pitchControl / 1024.0f) * 5000.0f);
}

void filter(){
  resonanceControl = ((resonanceControl/1024.0f)*3800);
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

void modulation() {
  float normalized = (modStr / 1024.0f);
  
  switch(modCounter){
    case 0:
      break;
    case 1: // tremolo
      // GENERAL EQUATION: D * signal + 1 where D is depth (decibels)
      // TODO: equivalent to doing (128 + signal) * 256. Now figure out if we want to control tremolo amplitude or frequency using the knob.
      unsigned int gain = (128.0f + tremoloLFO.next())*(normalized*256);
      aGain.set(gain, MOZZI_AUDIO_RATE / MOZZI_CONTROL_RATE);
      Serial.println("Tremolo");
      break;
    case 2: // vibrato
      //vibratoLFO.setFreq(normalized * 5);  // TODO: again play around with max freq 256 for vibrato modulation
      vibratoLFO.setFreq(15);  // TODO: again play around with max freq 256 for vibrato modulation
      Serial.println("Vibrato");
      break;
    case 3: // distortion
      distortionPhasor.setFreq(normalized * 10000.0f);  // TODO: play around with the max freq 256
      Serial.println("Distortion");
      break;
    case 4: // ring modulation
      // TODO: play with max freq 5000, the pitch modulation we currently have is also 5000
      ringOscillators[waveFormCounter].setFreq(normalized * 10000.0f);
      Serial.println("Ring");
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
  }
  if(modCounter > 4){
    modCounter = 0;
  }
}

void setup(){
  Serial.begin(4800);
  oscillators[waveFormCounter].setFreq(440);
  ADSRLastTime = millis();
  waveLastTime = millis();
  pinMode(buttonPin1, INPUT);
  pinMode(buttonPin3, INPUT);
  startMozzi(); //DO NOT REMOVE, necessary for Mozzi library
  tremoloLFO.setFreq(1.0f);
}

void updateControl(){
  adsr();
  modulation();
}

AudioOutput_t updateAudio(){
  envelope.update();

  int8_t oscil_sample = oscillators[waveFormCounter].next();

  // mod calculations
  int s = oscil_sample;
  switch (modCounter) {
    // ignore 0
    case 1:
      // WARN: might be some isses with fromAlmostNBit(9, ...), docs use 24
      s = (int32_t) s * aGain.next();
      break;
    case 2:
      //auto vibrato = 0.5 * toSFraction(vibratoLFO.next());

      /*float normalized = (modStr / 1024.0f);
      float depthFactor = normalized*0.1f;*/
      auto vibrato = UFix<0,8>(0.5) * toSFraction(vibratoLFO.next());

      s = (int32_t) oscillators[waveFormCounter].phMod(vibrato);
      break;
    case 3:
      s = distortionPhasor.next(); // TODO: maybe 2 phasors for more effect. Example uses 2  
      break;
    case 4:
      s = s * ringOscillators[waveFormCounter].next() * 2; // TODO: phase shift ?
      Serial.begin("ring");
      break;
  }

  int nextSample = 0;
  // filter calculations
switch (filterCounter) {
  case 0:
    nextSample = LPFilter.next(s);
    break;
  case 1:
    nextSample = HPFilter.next(s);
    break;
  case 2:
    nextSample = BPFilter.next(s);
    break;
  case 3:
    nextSample = NFilter.next(s);
    break;
}

  return MonoOutput::fromAlmostNBit(9, nextSample);
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
  ModulationButton();



  audioHook(); //DO NOT REMOVE, necessary for Mozzi library
}
