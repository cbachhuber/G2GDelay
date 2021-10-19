#include <TimerOne.h>		// http://www.arduino.cc/playground/Code/Timer1

/* PROGRAM OVERVIEW
  Conceptually, this program works as follows: parallel to the usual Arduino loop(), there runs
  an interrupt loop (ISR()) at a user defined frequency (SAMPLING_RATE). The interrupt loop manages
  turning on of the LED, records the resistance of the PT into the vector PT_voltages, and
  detects a sudden brightness increase (turning on LED visible at PT) at the PT. If this is
  detected, the interrupt loop sets flag_detected and records a time stamp. The main Arduino
  loop() always checks whether flag_detected has been set. If this is the case, the main loop
  computes the G2G delay and reports it over both the USB and the Bluetooth serial connections.
  Once that is done, all variables are reset and a new measurement is started.
*/

// Filtering Options
#define RAW_VALUES           0 // =1 prints the raw sample values read by the PT to the serial console.
#define CRT_WINDOW          50 // The window length in samples, over which we maximize for CRT=1
#define THRESH_COUNT_SLOPES  2 // Minimum number of samples required to acknowledge a rising edge
#define THRESH_ACC_SLOPES   20 // Minimum sample value increase to acknowledge a rising edge

//Technical Options
/* SAMPLING_RATE can be 1000,2000 or 8000 samples per second, defines system accuracy. Given a
  maximum number of samples (NUM_SAMPLES) also defines maximum G2G delay that can be measured.
  2000 proved to be a good tradeoff between maximum measurable G2G delay and accuracy */
#define SAMPLING_RATE    2000
#define NUM_SAMPLES      2000  // Number of recorded samples during one msmt process. Do not change.
#define TIME_BETWEEN      645  // Time between two measurements, in milliseconds. Do not change.

// Global variables
unsigned int  PT_voltages[NUM_SAMPLES] = {0};  // Vector of voltages read from the pin_PT
unsigned int  storage[CRT_WINDOW]  	   = {0};  // Vector of temporally stored samples for filtering
byte          count_pos_slopes         = {0};  // Number of successive positive slope samples
byte          acc_pos_slopes           = {0};  // Accumulated values of successive positive slope samples
/* Set sample_counter to zero if sampling shall start right away, to NUM_SAMPLES if you want to get the
  start signal via bluetooth */
unsigned int  sample_counter           = {0};
bool          sampling_finished        = LOW;  // Is set to true if sampling is finished
bool          flag_detected            = LOW;  // Is set to true if the PT detected that the LED was turned on.
unsigned int  i_ledON;                         // Sample at which the LED is turned on. Will be random.
unsigned int  i_ledOFF;                        // Sample at which the LED is turned off. Will be random.
short threshold          = THRESH_ACC_SLOPES;  // Set for now, can be changed from android software later

// Timestamps that will be measured with Timer1
unsigned int  t_ledTrig                = {0};  // When the LED was triggered
unsigned int  t_photoTransTrig         = {0};  // When a brightness increase was noted at the PT

//Timer settings and pin assignments
const unsigned int timer1_period     = 65535;  // timer period in microseconds, see http://playground.arduino.cc/code/timer1
const unsigned int pin_LED              = 13;  // the index of the LED pin, see circuit.pdf
const unsigned int pin_PT                = 5;  // PT analog input pin
const unsigned int pin_Randomseed        = 0;  // Random seed for the time between measurements
unsigned int randomSeedVal               = 0;  // Random value generated from multiple analog pin measurements

void setup() {
  Serial.begin(115200);  // USB connection to PC
  Serial.println();  // warm up the serial port

  Serial1.begin(9600);  // Bluetooth connection

  // initialize the lightEmitterLED pin as an output:
  pinMode(pin_LED, OUTPUT);

  // generate a random seed value by taking the LSB of 32 analogRead() measurements, see https://forum.arduino.cc/t/using-analogread-on-floating-pin-for-random-number-generator-seeding-generator/100936/3
  for (int i = 0; i < 32; i++)
  {
    randomSeedVal = (randomSeedVal << 1) + (1 & analogRead(pin_Randomseed));
  }
  // this seed should only need to be generated once as it should last a few days, see https://www.reddit.com/r/arduino/comments/6saycn/random_sequence_length/
  randomSeed(randomSeedVal);

  i_ledON = random( 50, NUM_SAMPLES * 0.1 );  // Setting constrained random start time of the LED
  i_ledOFF = random( NUM_SAMPLES * 0.85, NUM_SAMPLES * 0.95 ); // Setting constrained random end time of the LED

  setup_msmt_timer1();
  setup_sampling_timer2();
}

// This interrupt is called at the frequency defined in SAMPLING_RATE
ISR(TIMER2_COMPA_vect) {
  if (sample_counter == 0)
  {
    PT_voltages[sample_counter] = 255;
    sample_counter++;
  }
  else if (sample_counter < NUM_SAMPLES)
  {
    if (sample_counter == i_ledON)
    { // Turning on LED, recording time
      digitalWrite(pin_LED, HIGH);
      t_ledTrig = Timer1.read();
    }
    else if (sample_counter == i_ledOFF)
    { // Turning off LED
      digitalWrite(pin_LED, LOW);
    }

    if (sample_counter <= CRT_WINDOW) // Just build the storage
    {
      storage[sample_counter] = analogRead(pin_PT);
    }
    else  // shift the entire storage by one, put the newest value to the end
    {
      for (int i = 0; i < CRT_WINDOW; i++)
      {
        storage[i] = storage[i + 1];
      }
      storage[CRT_WINDOW] = analogRead(pin_PT);
    }

    // Extract the maximum over the last few values
    unsigned int buf = 0;
    for (int i = 0; i <= min(CRT_WINDOW, sample_counter); i++)
    {
      buf = max(buf, storage[i]);
    }
    // Write the maximum back to the samples
    PT_voltages[sample_counter] = buf;

    // This if clause catches the positive edge of the phototransistor response to the LED
    if (sample_counter >= 2)
    {
      short current_slope  = (PT_voltages[sample_counter] - PT_voltages[sample_counter - 1]);
      short previous_slope = (PT_voltages[sample_counter - 1] - PT_voltages[sample_counter - 2]);

      if (current_slope > 0  && previous_slope > 0)
      { // Accumulate slopes
        count_pos_slopes++;
        acc_pos_slopes += current_slope;
      }

      if (current_slope <= 0)
      {
        count_pos_slopes = 0;
        acc_pos_slopes   = 0;
      }

      // If the number of positive slopes and their accumulated value are greater than
      // these respective thresholds, we say that we have found the positive edge of the phototransistor response
      // OR if the rise from one sample to the other is very high, higher than the 'accumulated' threshold
      if ( ( ( count_pos_slopes >= THRESH_COUNT_SLOPES &&  acc_pos_slopes > threshold ) || current_slope > threshold)
           && !flag_detected && sample_counter >= i_ledON )
      {
        count_pos_slopes = 0;
        acc_pos_slopes   = 0;

        t_photoTransTrig = Timer1.read();
        flag_detected = HIGH;
      }
    }

    sample_counter++;

    // disable timer2 compare interrupt once the last sample of this frame is read
    if ( sample_counter == (NUM_SAMPLES - 1) )
    {
      TIMSK2 &= ~(1 << OCIE2A);
      sampling_finished = HIGH;
    }
  }
}

void loop() {
  if (sampling_finished)
  {
    // For Printing the raw values to Matlab
    if (RAW_VALUES)
    {
      for (unsigned int i = 0; i < NUM_SAMPLES; i++)
      {
        Serial.println( PT_voltages[i] );
      }
    }
    else
    {
      if (flag_detected)
      {
        float g2gDelay;
        if (t_photoTransTrig > t_ledTrig)
        { // Computing G2G delay. Multiplying with sample duration, subtracting system inherent delays from calibration
          g2gDelay = (t_photoTransTrig - t_ledTrig) * 0.008 - 0.255;
        }
        else
        { // In this case, a roll-over has taken place
          g2gDelay = (65535 + t_photoTransTrig - t_ledTrig) * 0.008 - 0.255;
        }

        // Printing G2G delay to USB serial conneciton
        Serial.println(g2gDelay);
        // Printing G2G delay to Bluetooth serial conneciton
        Serial1.println(" ");
        Serial1.println(g2gDelay);
      }
      flag_detected = LOW;
    }

    delay(random(0.9 * TIME_BETWEEN, 1.1 * TIME_BETWEEN));

    sample_counter = 0;
    sampling_finished = LOW;
    TIMSK2 |= (1 << OCIE2A);

    // Creating the random LED triggers
    i_ledON  = random( 50, NUM_SAMPLES * 0.1 );
    i_ledOFF = random( NUM_SAMPLES * 0.85, NUM_SAMPLES * 0.95 );
  }

  // Turning off and on via the bluetooth device
  byte buf;
  int insize;
  if ((insize = Serial1.available()) > 0)
  {
    for (int i = 0; i < insize; i++) {
      buf = char(Serial1.read());

      //Serial.println(insize);
      if (buf == 49) // Start message (1 in ASCCII)
      {
        sample_counter = 0;
        Serial.println("Started  ");
      }
      else // Quit/pause message (0 in ASCII):
      {
        if (buf == 48)
        {
          sample_counter = NUM_SAMPLES;
          Serial.println("Stopped.");
          delay(100); // To prevent the interrupt starting right away
        }
        else // Other signals: for now only threshold changes
        {
          if (buf == 117) // u in ASCII
          {
            // Raise threshold
            threshold++;
            Serial.println(threshold);
          }
          else
          {
            if (buf == 100) // d in ASCII
            {
              // Lower threshold
              threshold--;
              Serial.println(threshold);
            }
            else
            {
              if (buf == 114) // r in ASCII
              {
                // Reset threshold
                threshold = THRESH_ACC_SLOPES;
                Serial.println("Threshold reset.");
              }
              else
              {
                // Further signals
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
  TCCR1B |= (1 << CS11) | (1 << CS10); // Assigning a prescale of 64: http://www.instructables.com/id/Arduino-Timer-Interrupts/step1/Prescalers-and-the-Compare-Match-Register/
}


void setup_sampling_timer2()
{
  // set timer2 interrupt at 8/2/1/0.25kHz
  TCCR2A = 0; // set entire TCCR2A register to 0
  TCCR2B = 0; // same for TCCR2B
  TCNT2  = 0; // initialize counter value to 0

  switch (SAMPLING_RATE)  // 250, 1000, 2000 or 8000 sampling rate in Hz
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
