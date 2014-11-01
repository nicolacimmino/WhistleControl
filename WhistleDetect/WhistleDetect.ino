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

AudioPlaySdWav           playWav1;       //xy=154,78
AudioOutputI2S           i2s_out;           //xy=334,89
AudioConnection          patchCord2(playWav1, 0, i2s_out, 0);
AudioConnection          patchCord3(playWav1, 1, i2s_out, 1);

AudioControlSGTL5000     sgtl5000;

#define KEY_LEN 200
#define VERIFICATION_COUNT 2

int key[KEY_LEN];

int expectedError = 0;

void setup()
{
  AudioMemory(12);
  sgtl5000.enable();
  sgtl5000.inputSelect(AUDIO_INPUT_MIC);
  sgtl5000.lineInLevel(15);
  //sgtl5000.micGain(60);
  
  sgtl5000.volume(0.5);
  
  // For testing we have for now a LED between A7 and A6
  pinMode(A7, OUTPUT);
  pinMode(A6, OUTPUT);
  analogWrite(A7, 0);
  analogWrite(A6, 0);

  //while(!Serial);
  delay(300);

 SPI.setMOSI(7);
  SPI.setSCK(14);
  if (!(SD.begin(10))) {
    // stop here, but print a message repetitively
    while (1) {
      Serial.println("Unable to access the SD card");
      delay(500);
    }
  }
  
  expectedError = train()*1.1;  
}

char symbol='\0';
char lastSymbol='\0';
double lastSymbolTime=0;

void loop()
{
  playAudio("Listen.wav");
  while(true)
  {
    int error = detectMelody();
    Serial.println((error < expectedError)?"OK":"FAIL");
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
    delay(1000);
  }
}

int detectMelody()
{
  Serial.println("Start whistling.");
  while(getPeakLocation()<0);
  Serial.println("Listening....");
 
  int cumulativeError=0;
  for(int ix=0;ix<KEY_LEN;ix++)
  {
    int peakLocation = getPeakLocation();
    int error = abs(key[ix]-peakLocation);
    cumulativeError += (error>5)?error:0;
    //Serial.print(peakLocation);
    //Serial.print(" ");
    delay(15);
  } 
  Serial.println(cumulativeError); 
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
  Serial.println("Start whistling.");
  while(getPeakLocation()<0);
  Serial.println("Learning....");
 
  for(int ix=0;ix<KEY_LEN;ix++)
  {
    key[ix]=getPeakLocation();
    //Serial.print(key[ix]);
    //Serial.print(" ");
    delay(15);
  } 
  
  int averageError = 0;
  for(int ix=0;ix<VERIFICATION_COUNT;ix++)
  {
    delay(1000);
    playAudio("LeVerify.wav");
    Serial.println("Repeat to verify...");
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
  //Serial.println("");
  return averageError;
}

void playAudio(const char *filename)
{
  playWav1.play(filename);

  // Wait before testing if we are playing
  // as it will return false until file
  // is read.
  delay(5);

  // Wait until the file is finished to play.
  while (playWav1.isPlaying());
 
}
