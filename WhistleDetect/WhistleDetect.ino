#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>

// GUItool: begin automatically generated code
AudioInputI2S            i2s1;           //xy=143,290
AudioAnalyzeFFT1024      fft;      //xy=292,307
AudioConnection          patchCord1(i2s1, 0, fft, 0);
AudioControlSGTL5000     sgtl5000;     //xy=294,222
// GUItool: end automatically generated code


void setup()
{
  
  AudioMemory(12);
  sgtl5000.enable();
  sgtl5000.inputSelect(AUDIO_INPUT_MIC);
  sgtl5000.lineInLevel(15);
  sgtl5000.micGain(60);
  
  pinMode(A7, OUTPUT);
  pinMode(A6, OUTPUT);
  analogWrite(A7, 0);
  analogWrite(A6, 0);
  
  //digitalWrite(13,LOW);
  
  //Serial.begin(115200);
  //while (!Serial) ;
  delay(300);
  
}

char symbol='\0';
char lastSymbol='\0';
double lastSymbolTime=0;
void loop()
{
  
  int peakLocation = getPeakLocation();
  
  if(floor(peakLocation/10)==2)
  {
    symbol='L';
  }
  else if(floor(peakLocation/10)==3)
  {
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
      //digitalWrite(13,(symbol=='H')?HIGH:LOW);
      //Serial.println((symbol=='H')?"ON":"OFF");
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
  
  if(peak>0.1)
  {
    //Serial.println(peakLocation);  
  }
  return peakLocation;
}
