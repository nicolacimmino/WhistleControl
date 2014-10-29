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

AudioInputI2S            i2s1;
AudioAnalyzeFFT1024      fft;
AudioConnection          patchCord1(i2s1, 0, fft, 0);
AudioControlSGTL5000     sgtl5000;

void setup()
{
  AudioMemory(12);
  sgtl5000.enable();
  sgtl5000.inputSelect(AUDIO_INPUT_MIC);
  sgtl5000.lineInLevel(15);
  sgtl5000.micGain(60);
  
  // For testing we have for now a LED between A7 and A6
  pinMode(A7, OUTPUT);
  pinMode(A6, OUTPUT);
  analogWrite(A7, 0);
  analogWrite(A6, 0);
}

char symbol='\0';
char lastSymbol='\0';
double lastSymbolTime=0;

void loop()
{
  
  int peakLocation = getPeakLocation();
  
  if(abs(peakLocation-20)<3)
  {
    Serial.println(peakLocation);
    symbol='L';
  }
  else if(abs(peakLocation-31)<3)
  {
    Serial.println(peakLocation);
    symbol='H'; 
  }
  else
  {
     symbol='\0'; 
  }
  
  if(symbol!=lastSymbol && symbol!='\0')
  {
    lastSymbolTime=millis();
    if(lastSymbol!='\0')
    {
      analogWrite(A6, (symbol=='H')?255:0);
      lastSymbol='\0';
      delay(500);
    }
    else
    {
      lastSymbol=symbol; 
    } 
  }
  
  if(millis()-lastSymbolTime>500)
  {
    lastSymbol='\0'; 
  }
  
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
