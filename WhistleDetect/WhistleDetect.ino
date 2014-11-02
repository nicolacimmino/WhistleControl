// WhistleDetect implements a detector for whistled patterns.
//
//  Copyright (C) 2014 Nicola Cimmino
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//   This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see http://www.gnu.org/licenses/.
//
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>

AudioInputI2S            i2s_in;
AudioAnalyzeFFT1024      fft;
AudioConnection          patchCord1(i2s_in, 0, fft, 0);
AudioPlaySdWav           playWav1;
AudioOutputI2S           i2s_out;
AudioConnection          patchCord2(playWav1, 0, i2s_out, 0);
AudioConnection          patchCord3(playWav1, 1, i2s_out, 1);
AudioControlSGTL5000     sgtl5000;

// This is the amount of peaks detected, in frequency domain, from
// the original melody sample.
#define KEY_LEN 200

// This is the amount of times the user should repeat the original
// whistled melody.
#define VERIFICATION_COUNT 2

// Peaks in frequency domain from the original melody.
int key[KEY_LEN];

// Estimated during the verificatin phase keeps into account
//  the ability of the user to repeat the melody consistently.
int expectedError = 0;

// How main bins a detected peak can be off from the expected
// and still be considered valid.
#define DETECT_ERROR_HYSTERESIS 5

void setup()
{
  AudioMemory(12);
  sgtl5000.enable();
  sgtl5000.inputSelect(AUDIO_INPUT_MIC);
  sgtl5000.lineInLevel(15);  
  sgtl5000.volume(0.5);
  
  // For testing we have for now a LED between A7 and A6
  pinMode(A7, OUTPUT);
  pinMode(A6, OUTPUT);
  analogWrite(A7, 0);
  analogWrite(A6, 0);

  // Init SD Card
  SPI.setMOSI(7);
  SPI.setSCK(14);
  if (!(SD.begin(10))) 
  {
    // We cannot continue without the SD
    while (1) 
    {
      Serial.println("Unable to access the SD card");
      delay(500);
    }
  }
  
  // Do the training once at startup and estimate the
  // expected max error as 10% more of the average
  // error detected in training.
  expectedError = train()*1.1;  
}

void loop()
{
  playAudio("Listen.wav");
  while(true)
  {
    // Detect the melody and report success if the 
    //  error was less than the maximum.
    int error = detectMelody();
    
    if(error < expectedError)
    {
      analogWrite(A6, HIGH);
      playAudio("Correct.wav");
      playAudio("Listen.wav"); 
      analogWrite(A6, LOW);
    }
    else
    {
      playAudio("Wrong.wav");  
    }
    
    // Not strictly necessary but when the user gets it wrong
    // he might get the "wrong, try again" prompt while he is
    // still whistling, if he doesn't stop immediately this
    // might trigger the next call to "detectMelody" wich might
    // turn out to be rather annoying.
    delay(1000);
  }
}

/*
 * Here we sample the incoming sound and estimate how much it maches
 * with the original one. Sampling, FFT and peak detection is done
 * as in the learning phase. Cumulative error is then calculated after
 * after passing it through a non linear function to allow for a small
 * error of few FFT bins. 
 */
int detectMelody()
{
  while(getPeakLocation()<0);

  int cumulativeError=0;
  for(int ix=0;ix<KEY_LEN;ix++)
  {
    int peakLocation = getPeakLocation();
    int error = abs(key[ix]-peakLocation);
    cumulativeError += (error>DETECT_ERROR_HYSTERESIS)?error:0;
    delay(15);
  } 
  
  return cumulativeError;
}

int getPeakLocation()
{
  while(!fft.available());
  
  // Peak detector with threshold at 0.1
  // Detects the FFT bin with the highest
  // energy above threshold.
  float peak=0.1;
  int peakLocation=-1;
  for(int ix=5; ix<512;ix++)
  {
    if(fft.read(ix)>peak)
    {
      peak=fft.read(ix);
      peakLocation=ix;
    }  
  }  

  return peakLocation;
}

int train()
{
  playAudio("LeStart.wav");
  while(getPeakLocation()<0);
  
  for(int ix=0;ix<KEY_LEN;ix++)
  {
    key[ix]=getPeakLocation();
    delay(15);
  } 
  
  int averageError = 0;
  for(int ix=0;ix<VERIFICATION_COUNT;ix++)
  {
    delay(1000);
    playAudio("LeVerify.wav");
    int error = detectMelody();
    if(error>3000)
    {
      playAudio("LeVeFail.wav"); 
      ix--;
      continue; 
    }
    else
    {
      playAudio("ok.wav"); 
    }
    averageError += error; 
  }
  averageError=averageError/VERIFICATION_COUNT;
  return averageError;
}

void playAudio(const char *filename)
{
  playWav1.play(filename);

  // Wait before testing if we are playing
  // as it will return false until file is read.
  delay(5);

  // Wait until the file is finished to play.
  while (playWav1.isPlaying());
 
}
