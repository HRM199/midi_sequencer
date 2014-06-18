#include "Sequence_Data.h"
#include <Wire.h>


// user defined variables ////////////////////////////////

int step_delay = 500;
int display_brightness = 20 ; // 0-255


//// Main variables used ///////////////////////////////////

int i,sequence_number,sequence_step,step_value,tens,ones,data_step,fader_1,fader_2,fader_step_1,fader_step_2;
int count = 1;
boolean start_delay,counting_delay,next_step,next_sequence,last_sequence,wait_for_high,wait_for_high_2,trigger_sequence;
unsigned long start_of_delay,debounce,debounce_2,delay_1,delay_2;

int fade_up_1 = true,fade_up_2 = true;


//// MIDI Variables ////////////////////////////////////

byte midi_start = 0xfa;
byte midi_stop = 0xfc;
byte midi_clock = 0xf8;
byte data;
int baudrate = 31250;
int midi_mode,midi_trigger;

//// Pin Mapping & PWM ////////////////////////////////

int pinmap[8] = {6,2,9,11,7,5,10,12}; // New pinmap reflects timers
//int pinmap[8] = {2,11,9,3,4,12,10,5}; // Mega Pinmap
//int pinmap[8] = {2,5,9,11,8,6,10,3}; // Uno Pinmap
int fading_pin_1 = 3;
int fading_pin_2 = 8;

int next_button = 14;
int last_button = 15;

void setup() {
  
TCCR4B = TCCR4B & 0b11111000 | 0x05;
TCCR3B = TCCR3B & 0b11111000 | 0x03;
TCCR2B = TCCR2B & 0b11111000 | 0x03;
TCCR1B = TCCR1B & 0b11111000 | 0x03;


///////////////////////////////////////////////////////
  
Serial.begin(baudrate);

Wire.begin();
write_to_display(0,0);
write_to_display(2,0);

  Wire.beginTransmission(0x31);
    Wire.write(1);      
    Wire.write(display_brightness);    
  Wire.endTransmission();

pinMode(13,OUTPUT);
digitalWrite(13,0);

pinMode(next_button,INPUT);
digitalWrite(next_button,1);

pinMode(last_button,INPUT);
digitalWrite(last_button,1);

digitalWrite(A0,0);
 
////////// Setup sequence pins output ////////////////

for (i=0;i<8;i++){ pinMode(pinmap[i],OUTPUT);  }

// PW frequency of the output pins

 TCCR1B = TCCR1B & 0b11111000 | 0x03; // Pin 12 & 11
 TCCR2B = TCCR2B & 0b11111000 | 0x06; // Pin 10 & 9
 
//////////////////////////////////////////////////////
  
}


void loop() {

/*

When the boolean 'trigger_sequence' is set to high either by MIDI clock overflow or audio on the analog pin the
next sequence loop is started. Using millis() ( so the time is not relative to a clock signal but real time )
it counts through 8 steps, on the eighth.


*/


if(trigger_sequence)
  {
    if(!next_step)
      {
        
        if(!counting_delay)
          {
            
           dot(3,1);
           
           for(data_step=0;data_step<8;data_step++)
            {
              step_value = map(Sequence[sequence_number][sequence_step][data_step],0,9,0,255);
              analogWrite(pinmap[data_step],step_value);
              start_of_delay = millis();
              counting_delay = true;
              write_to_display(0,sequence_number + 1);
              write_to_display(2,sequence_step + 1);
            }
            
            dot(3,0);
            
           }
           
           if(millis() - start_of_delay >= step_delay){next_step = true; counting_delay = false; }
      }
      
      else
      {
        next_step = false;
        if (sequence_step >= 7) {sequence_step = 0; trigger_sequence = false; }
        else { sequence_step++; }
        ;
      }
      
  }
     
//// code for the faders //////////////////////////////////////////////   
     
     if(fade_up_1 && millis() - delay_1 >= fader_1_sequence[fader_step_1]) 
       { 
       analogWrite(fading_pin_1,fader_1); 
       fader_1++; 
       delay_1 = millis();
       if(fader_1 == 255 ) { fader_step_1++; }
       }
  
       
     else if(!fade_up_1 && millis() - delay_1 >= fader_1_sequence[fader_step_1]) 
       { 
       analogWrite(fading_pin_1, fader_1); 
       fader_1--; 
       delay_1 = millis();
       if(fader_1 == 0 ) { fader_step_1++; }
       }  
     
     if(fader_1 == 255 ) {fade_up_1 = false; }
     if(fader_1 == 0) {fade_up_1 = true; }
     if(fader_step_1 >= sizeof(fader_1_sequence) / 2){ fader_step_1 = 0;}
     
     
      if(fade_up_2 && millis() - delay_2 >= fader_2_sequence[fader_step_2]) 
       { 
       analogWrite(fading_pin_2,fader_2); 
       fader_2++; 
       delay_2 = millis();
       if(fader_2 == 255 ) { fader_step_2++; }
       }
  
       
     else if(!fade_up_2 && millis() - delay_2 >= fader_2_sequence[fader_step_2]) 
       { 
       analogWrite(fading_pin_2, fader_2); 
       fader_2--; 
       delay_2 = millis();
       if(fader_2 == 0 ) { fader_step_2++; }
       }  
     
     if(fader_2 == 255 ) {fade_up_2 = false; }
     if(fader_2 == 0) {fade_up_2 = true; }
     if(fader_step_2 >= sizeof(fader_2_sequence) / 2){ fader_step_2 = 0;}
     
//////////////////////////////////////////////////////////////////////////    
      
      midi_clock_trigger();
      
///////////////// Audio Trigger /////////////////////////////////////////    

      if(analogRead(A0) > 10 && !wait_for_high )
      { 
        trigger_sequence=true; 
        sequence_step = 0; 
        counting_delay = false; 
        wait_for_high = true; 
        debounce = 0;
        dot(0,1);
          if(next_sequence){next_sequence=false; sequence_number++;}
          if(last_sequence){last_sequence=false; sequence_number--;}
       }
      else if (analogRead(A0) <= 10 && debounce >= 10000) {wait_for_high = false; dot(0,0);}
      else { debounce++; }
      
///////////////// Next and Last buttons /////////////////////////////////////////      
      
      if(!digitalRead(next_button) && !next_sequence){ next_sequence = true; }
      if(!digitalRead(last_button) && !last_sequence){ last_sequence = true; }
      
      if(sequence_number >= 15){ sequence_number = 0; }
      if(sequence_number < 0){ sequence_number = 0; }
      digitalWrite(13,next_sequence);
      
     // delayMicroseconds(100);
     
  }
    


void midi_clock_trigger(){
  
if(Serial.available() > 0) {
  
  data = Serial.read();
  
  if(data == midi_start) {midi_mode = 1;}
  
  else if(data == midi_stop) {midi_mode = 0;}
  
  else if(data == midi_clock && midi_mode ) {
  
  if(count == 97){count = 1; }
  if(count == 96){trigger_sequence = true;         
     sequence_step = 0; 
        counting_delay = false; }

  
  count++;

   }
  
  }  
}


///// 7 segment display functions ////////////////

void write_to_display(int digit_no, int number)
{
  
  tens = number/10;
  ones = number-tens*10;


  Wire.beginTransmission(0x31);
  
    Wire.write(5);      
    Wire.write(digit_no + 1);        
    Wire.write(ones); 

    Wire.write(5);      
    Wire.write(digit_no);        
    Wire.write(tens);      
  
  Wire.endTransmission();
  
}

void clear_display()
{
  Wire.beginTransmission(0x31);
  
    Wire.write(3);               
  
  Wire.endTransmission();
  
}


void dot(int dot_no, int on_off)
{
  Wire.beginTransmission(0x31);
  
    Wire.write(6);      
    Wire.write(dot_no);        
    Wire.write(on_off);               
  
  Wire.endTransmission();
  
}

