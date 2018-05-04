#include <PinChangeInt.h>	// http://www.arduino.cc/playground/Main/PinChangeInt
#include <TimerOne.h>		// http://www.arduino.cc/playground/Code/Timer1

//Serial Display options
#define RAW_VALUES              0 //=1 overrides the parameter MATLAB and prints the raw sample values read by the Phototransistor to the serial console.
#define MATLAB                  1 //MATLAB=0 prints many statistics, MATLAB=1 prints only the delay.
#define CRT                     1 //=1 for measuring a CRT or Plasma, =0 for measuring an LCD, OLED or other monitor
#define WINDOW_LENGTH           5 //WINDOW_LENGTH samlpes are printed to the display in case an event is detected.
#define CRT_WINDOW             50 //The window length in samples, over which we maximize for CRT=1
#define TIME_BETWEEN          645 //Time between two measurements, in ms. In this instance chosen to ensure 1s from event to event

//Technical Options
#define THRESH_COUNT_SLOPES     2 //Minimum number of samples required to acknowledge a rising edge (this can always be overridden by one very high sample!)
#define THRESH_ACC_SLOPES      20 //Minimum sample value increase to acknowledge a rising edge
#define SAMPLING_RATE        1000 //Can be 1000,2000 or 8000 samples per second. 2000 proved as good tradeoff between measurement time and accuracy
#define NUM_SAMPLES          2000 //Do not change this
#define LED_START             NUM_SAMPLES/10
#define LED_END               NUM_SAMPLES - 200

// Global variables 
unsigned int val_A_in[NUM_SAMPLES]           = {0};
unsigned int storage[CRT_WINDOW]  	         = {0};
byte          count_pos_slopes               = {0};   // number of successive pos. slope samples
byte          acc_pos_slopes                 = {0};   // accumulated values of successive pos. slope samples
unsigned int  sample_counter                 = {0}; // Set this to zero if sampling shall start right away, to NUM_SAMPLES if you want to get the start signal via bluetooth
bool          sampling_finished              = LOW;
bool          flag_detected                  = LOW;
unsigned int  i_ledON;
unsigned int  i_ledOFF;
unsigned int buf;
short threshold                      = THRESH_ACC_SLOPES;
double e2edelay=0;

// Timestamps that will be measured with Timer1
unsigned int  t_ledTrig                      = {0};
unsigned int  t_photoDiodeTrig               = {0};
unsigned int  t_photoTransTrig               = {0};

//Timer settings and pin assignments
const unsigned int timer1_period             = 65535;  // microsec., http://playground.arduino.cc/code/timer1
const unsigned int lightEmitterLEDPin        = 13; // the number of the LED pin
const unsigned int phototrans_A_in_Pin       = 5;  // analog input pin
const unsigned int photoTransistorPin        = 3;  // digital input pin
const unsigned int randomseed_A_in_Pin       = 0;
int timeout                                  = 0;



void setup() {
  Serial.begin(115200);
  Serial1.begin(9600);
  Serial.println(); //warm up the serial port

  // initialize the lightEmitterLED pin as an output:
  pinMode(lightEmitterLEDPin, OUTPUT);  
  
  setup_msmt_timer1();
  setup_sampling_timer2(); 
  i_ledON  = LED_START;//random( 50, NUM_SAMPLES*0.1 );
  i_ledOFF = LED_END;//random( i_ledON+10, NUM_SAMPLES*0.9 ); 
}

// analog in signal sampling routine
ISR(TIMER2_COMPA_vect){ //timer2 interrupt 
  
  if ( sample_counter == 0 )
  {      
      val_A_in[sample_counter] = 255;
      sample_counter++;
  }
  else if( sample_counter < NUM_SAMPLES )
  {             
    
    if( sample_counter == i_ledON )
    {
      digitalWrite(lightEmitterLEDPin, HIGH);
      t_ledTrig = Timer1.read();
    }
    else if ( sample_counter == i_ledOFF )
    {
      digitalWrite(lightEmitterLEDPin, LOW);
    }
    
    if(!CRT)
    {
      val_A_in[sample_counter] = analogRead( phototrans_A_in_Pin );
    }
    else
    {
      //Load current value via analogRead into the small storage structre
      if(sample_counter<=CRT_WINDOW) //Just build the storage
      {
        storage[sample_counter] = analogRead(phototrans_A_in_Pin);
      }
      else//shift the entire storage by one, put the newest value to the end
      {
        for(int i=0; i<CRT_WINDOW;i++) //Not efficient, let's see if this works
         {
	    storage[i]=storage[i+1];
         }
         storage[CRT_WINDOW] = analogRead(phototrans_A_in_Pin);
      }
      
      //Extract the maximum over the last few values
     buf=0;
     for(int i=0; i<=min(CRT_WINDOW,sample_counter);i++) 
     {
	buf=max(buf,storage[i]);
     }
      //Write it to the samples
       val_A_in[sample_counter] = buf;
    }
    
    // This if loop catches the positive edge of the phototransistor response to the LED
    if( sample_counter >=  2)
    {
      short current_slope  = (val_A_in[sample_counter] - val_A_in[sample_counter-1]);
      short previous_slope = (val_A_in[sample_counter-1] - val_A_in[sample_counter-2]);
      
      if( current_slope > 0  && previous_slope > 0 ) 
      {
          count_pos_slopes++; 
          acc_pos_slopes += current_slope;
          
      }
      
      if( current_slope <= 0 )
      {
        count_pos_slopes = 0;
        acc_pos_slopes   = 0;
      }      
      
      // If the number of positive slopes and their accumulated value are greater than
      // these respective thresholds, we say that we have found the positive edge of the phototransistor response
      // OR if the rise from one sample to the other is very high, higher than the 'accumulated' threshold
      if( ( ( count_pos_slopes >= THRESH_COUNT_SLOPES &&  acc_pos_slopes > threshold ) || current_slope > threshold) && timeout == 0 && sample_counter>=LED_START)
      {
        count_pos_slopes = 0;
        acc_pos_slopes   = 0;
        
        t_photoTransTrig=Timer1.read();
        timeout=NUM_SAMPLES; 
        flag_detected = HIGH;
        
        
      }else{
        if(timeout>0){timeout--;}
        else{timeout==0;}  
      }   
    }
    
    sample_counter++;
    
    // disable timer2 compare interrupt, once the last sample of this frame is read.
    if( sample_counter == (NUM_SAMPLES-1) )
    {       
       TIMSK2 &= ~(1 << OCIE2A); 
       sampling_finished = HIGH;
    }
  }
}

void loop() {
  if( sampling_finished )
  {
    //Serial1.println("1");
    //For Printing the raw values to Matlab
    if(RAW_VALUES)
    {
      for (unsigned int i = 0; i < NUM_SAMPLES; i++)
      {
        Serial.println( val_A_in[i] );
      }    
    }
    else
    {
    if(flag_detected)
        {
        if(t_photoTransTrig > t_ledTrig)
        {
          if(!MATLAB)
           { 
            Serial.print("Window Values: ");
            for(int i=0;i<WINDOW_LENGTH;i++)
            {Serial.print(val_A_in[sample_counter-WINDOW_LENGTH+i]);Serial.print(" ");};
            Serial.print("End-to-End delay: ");
	    //In the following line, 0.255 is the delay inherent to the delay measurement system. Needs to be subtracted.
            Serial.print( (t_photoTransTrig - t_ledTrig)*0.008 - 0.255);
            Serial.println("ms");
            Serial.print(t_photoTransTrig);
            Serial.print(", ");
            Serial.println(t_ledTrig);
           }else{
//            e2edelay =     (t_photoTransTrig - t_ledTrig)*0.008 - 0.255;
//            if(e2edelay < 10){Serial.print("0");}
//            if(e2edelay < 100){Serial.print("0");}
            Serial.println( (t_photoTransTrig - t_ledTrig)*0.008 - 0.255 );
            Serial1.println(" ");
            Serial1.println( (t_photoTransTrig - t_ledTrig)*0.008 - 0.255 );
           }
        }
        else //In this case, a roll-over has happened
        {
          if(!MATLAB)
            {
              Serial.print("Window Values: ");
              for(int i=0;i<=WINDOW_LENGTH;i++)
              {Serial.print(val_A_in[sample_counter-WINDOW_LENGTH+i]);Serial.print(" ");};
              Serial.print("End-to-End delay: ");
              Serial.print( (t_photoTransTrig + 65535 - t_ledTrig)*0.008 -0.255);
              Serial.println("ms");
              Serial.print("Roll Over! Values: ");
              Serial.print(t_photoTransTrig);
              Serial.print(", ");
              Serial.println(t_ledTrig);
            }else{
              e2edelay =    (t_photoTransTrig + 65535 - t_ledTrig)*0.008 - 0.255;
//            if(e2edelay < 10){Serial.print("0");}
//            if(e2edelay < 100){Serial.print("0");}
            Serial.println( (t_photoTransTrig + 65535 - t_ledTrig)*0.008 - 0.255 );
            Serial1.println(" ");
            Serial1.println( (t_photoTransTrig + 65535 - t_ledTrig)*0.008 - 0.255);
            }
          }
        }
        flag_detected = LOW;
    }
    
//    delay(TIME_BETWEEN);
    delay(random(0.9*TIME_BETWEEN,1.1*TIME_BETWEEN));
    
    sample_counter = 0;
    timeout=0;
    sampling_finished = LOW;
    // enable timer compare interrupt
    TIMSK2 |= (1 << OCIE2A);
    
    // Creating the random LED triggers
    randomSeed( analogRead(randomseed_A_in_Pin) );
    i_ledON  = LED_START;//random( 50, NUM_SAMPLES*0.1 );
    i_ledOFF = LED_END;//random( NUM_SAMPLES*0.85, NUM_SAMPLES*0.95 ); 
    
  }
  
  //Turning off and on via the bluetooth device
  byte buf;
  int insize;
  if((insize=Serial1.available())>0)
  {
    for (int i=0; i<insize; i++){
     buf = char(Serial1.read());
    
    //Serial.println(insize);
    if(buf == 49) //Start message (1 in ASCCII)
    {
      sample_counter = 0;    
      Serial.println("Started  ");
    }
    else //Quit/pause message (0 in ASCII):
    {
      if(buf==48)
      {
        sample_counter = NUM_SAMPLES;
        Serial.println("Stopped.");
        delay(100); //To prevent the interrupt starting right away
      }
      else //Other signals: for now only threshold changes
      {
        if(buf == 117) //u in ASCII
        {
          //Raise threshold
          threshold++;
          Serial.println(threshold);
        }
        else
        {
          if(buf == 100) //d in ASCII
          {
           //Lower threshold 
           threshold--;
           Serial.println(threshold);
          }
          else
          {
            if(buf == 114) //r in ASCII
            {
              //Reset threshold
              threshold = THRESH_ACC_SLOPES;
              Serial.println("Threshold reset.");
            }
            else
            {
              //Further signals
            }
          }
        }
      }
    }
    }
  }
}


void setup_msmt_timer1()
{

  Timer1.initialize( timer1_period ); // Initializing the timer with the correct period
  TCCR1B |=(1 << CS11) | (1 << CS10); // Assigning a prescale of 64: http://www.instructables.com/id/Arduino-Timer-Interrupts/step1/Prescalers-and-the-Compare-Match-Register/

}


void setup_sampling_timer2()
{
  //set timer2 interrupt at 8/2/1/0.25kHz
  TCCR2A = 0;// set entire TCCR2A register to 0
  TCCR2B = 0;// same for TCCR2B
  TCNT2  = 0;//initialize counter value to 0

  switch ( SAMPLING_RATE)  // 250, 1000, 2000 or 8000 sampling rate in Hz
  {
    case 8000: 
    {
      // set compare match register for 8khz increments
      OCR2A = 249;// = (16*10^6) / (8000*8) - 1 (must be <256)
          
      // turn on CTC mode
      TCCR2A |= (1 << WGM21);
    
      // 8 prescaler
      TCCR2B |= (1 << CS21);   

      break;      
      
    }
    
    case 2000:
    {
      // set compare match register for 2khz increments
      OCR2A = 249;// = (16*10^6) / (2000*32) - 1 (must be <256)
    
      // turn on CTC mode
      TCCR2A |= (1 << WGM21);
      
      // 32 prescaler
      TCCR2B |= (1 << CS21) | (1 << CS20);   
      
      break;
    }
    
    case 1000:
    {
      // set compare match register for 1khz increments
      OCR2A = 249;// = (16*10^6) / (1000*64) - 1 (must be <256)
    
      // turn on CTC mode
      TCCR2A |= (1 << WGM21);
      
      // 32 prescaler
      TCCR2B |= (1 << CS22);   
      
      break;
    }
    
    case 250:
    {
      // set compare match register for 250Hz increments
      OCR2A = 249;// = (16*10^6) / (250*256) - 1 (must be <256)
    
      // turn on CTC mode
      TCCR2A |= (1 << WGM21);
      
      // 32 prescaler
      TCCR2B |= (1 << CS22) | (1 << CS21);   
      
      break;
    }
  }
  
  // enable timer compare interrupt
  TIMSK2 |= (1 << OCIE2A);  
}
