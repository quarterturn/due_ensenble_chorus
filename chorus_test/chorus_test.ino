// a string ensemble chorus effect like Roland RS-202 or Lowrey "Symphonic Strings"
// intended for the ElectroSmash Pedalshield and Arduino Due
// but you could adapt the code to pretty much any fast enough microcontroller with enough RAM

// Dual ADC/DAC version - be sure to place jumper on BIAS jumper

#import "lfo.h"

#define BUFFER_SIZE 2048
#define LED 3
#define FOOTSWITCH 7
#define TOGGLE 2
 
volatile int16_t in_ADC0, in_ADC1, out_DAC0, out_DAC1;  //variables for 2 ADCs values (ADC0, ADC1)
volatile int16_t POT0, POT1, POT2; //variables for 3 pots (ADC8, ADC9, ADC10)

// buffers
volatile int16_t delayBuffer[BUFFER_SIZE] = {0};

// track the input index
volatile int16_t inIndex = 0;
// track the output index
volatile int16_t outIndex1 = 1023; // stick it in the middle so there's room to shift it around
volatile int16_t outIndex2 = 1023;
volatile int16_t outIndex3 = 1023;
// track lfo index
volatile int16_t lfoIndex1 = 0;
volatile int16_t lfoIndex2 = 245;
volatile int16_t lfoIndex3 = 490;
// used to slow down reading the LFO wavetable
volatile int16_t interruptCount = 0;
// the offset value read from the wavetable
volatile int16_t offset1 = 0;
volatile int16_t offset2 = 0;
volatile int16_t offset3 = 0;
// the resulting input index plus the offset
volatile int16_t offsetIndex1 = 0;
volatile int16_t offsetIndex2 = 0;
volatile int16_t offsetIndex3 = 0;
// wavetable LFO speed
volatile uint16_t interruptsPerLFO = 100;

 
void setup()
{
  //turn on the timer clock in the power management controller
  pmc_set_writeprotect(false);
  pmc_enable_periph_clk(ID_TC4);
 
  //we want wavesel 01 with RC 
  TC_Configure(TC1,1, TC_CMR_WAVE | TC_CMR_WAVSEL_UP_RC | TC_CMR_TCCLKS_TIMER_CLOCK2);
  TC_SetRC(TC1, 1, 109); // sets <> 44.1 Khz interrupt rate
  TC_Start(TC1, 1);
 
  // enable timer interrupts on the timer
  TC1->TC_CHANNEL[1].TC_IER=TC_IER_CPCS;
  TC1->TC_CHANNEL[1].TC_IDR=~TC_IER_CPCS;
 
  //Enable the interrupt in the nested vector interrupt controller 
  //TC4_IRQn where 4 is the timer number * timer channels (3) + the channel number 
  //(=(1*3)+1) for timer1 channel1 
  NVIC_EnableIRQ(TC4_IRQn);
 
  //ADC Configuration
  ADC->ADC_MR |= 0x80;   // DAC in free running mode.
  ADC->ADC_CR=2;         // Starts ADC conversion.
  ADC->ADC_CHDR=0xFFFFFFFF; // disable all channels
  ADC->ADC_CHER=0x1CC0;  // Enable ADC channels 0,1,8,9 and 10

 
  //DAC Configuration
  analogWrite(DAC0,0);  // Enables DAC0
  //analogWrite(DAC1,0);  // Enables DAC0
 
  //pedalSHIELD pin configuration
  pinMode(LED, OUTPUT);  
  pinMode(FOOTSWITCH, INPUT);     
  pinMode(TOGGLE, INPUT);

  // begin test
//  Serial.begin(115200);
//  #define TIME_INTERRUPT
//  #ifdef TIME_INTERRUPT
//  uint32_t now = micros();
//  for(int i = 0; i < 10000; i++)
//    TC4_Handler();
//  now = micros() - now;
//  Serial.print("time = ");
//  // Round the result up
//  Serial.print((now+5000)/10000);
//  Serial.println(" us");
//  while(1);
//  #endif
// 
//  NVIC_EnableIRQ(TC4_IRQn);
  // end test
}
 
void loop()
{
  //Read the ADCs
  while((ADC->ADC_ISR & 0x1CC0)!=0x1CC0);// wait for ADC 0, 1, 8, 9, 10 conversion complete.
  in_ADC0=ADC->ADC_CDR[7];               // read data from ADC0
  in_ADC1=ADC->ADC_CDR[6];               // read data from ADC1  
  POT0=ADC->ADC_CDR[10];                 // read data from ADC8        
  POT1=ADC->ADC_CDR[11];                 // read data from ADC9   
  POT2=ADC->ADC_CDR[12];                 // read data from ADC10 

  interruptsPerLFO=map(POT0,0,4095,25,200);
}
 
//Interrupt at 44.1KHz rate (every 22.6us)
void TC4_Handler()
{
  //Clear status allowing the interrupt to be fired again.
  TC_GetStatus(TC1, 1);
 

  // update the input index
  inIndex++;
  // wrap around if at end of buffer
  if (inIndex > (BUFFER_SIZE - 1))
    inIndex = 0;

  // store the new ADC reading
  delayBuffer[inIndex] = in_ADC0;

  // update the output index
  outIndex1++;
  // wrap around if at end of buffer
  if (outIndex1 > (BUFFER_SIZE - 1))
    outIndex1 = 0;

  // update the output index
  outIndex2++;
  // wrap around if at end of buffer
  if (outIndex2 > (BUFFER_SIZE - 1))
    outIndex2 = 0;

  // update the output index
  outIndex3++;
  // wrap around if at end of buffer
  if (outIndex3 > (BUFFER_SIZE - 1))
    outIndex3 = 0;

  // slow down the wavetable by only getting the next value every 200 interrupts
  if (interruptCount > interruptsPerLFO)
  {  
    lfoIndex1++;
    // wrap around if at end of wavetable
    if (lfoIndex1 > (LFO_SIZE - 1))
    {
      lfoIndex1 = 0;
    }

    lfoIndex2++;
    // wrap around if at end of wavetable
    if (lfoIndex2 > (LFO_SIZE - 1))
    {
      lfoIndex2 = 0;
    }

    lfoIndex3++;
    // wrap around if at end of wavetable
    if (lfoIndex3 > (LFO_SIZE - 1))
    {
      lfoIndex3 = 0;
    }
      
    // reset the interrupt counter  
    interruptCount = 0;
  }
  else
  {
    interruptCount++;
  }
 
  // get the delay from the wavetable
  offset1 = pgm_read_word_near(LFO_TABLE + lfoIndex1);
  offset2 = pgm_read_word_near(LFO_TABLE + lfoIndex2);
  offset3 = pgm_read_word_near(LFO_TABLE + lfoIndex3);
  
  // add the delay to the buffer index to get the delay index
  offsetIndex1 = outIndex1 + offset1;
  offsetIndex2 = outIndex2 + offset2;
  offsetIndex3 = outIndex3 + offset3;
  
  // wrap the index if it goes past the end of the buffer
  if (offsetIndex1 > (BUFFER_SIZE - 1))
    offsetIndex1 = offsetIndex1 - BUFFER_SIZE;
  if (offsetIndex2 > (BUFFER_SIZE - 1))
    offsetIndex2 = offsetIndex2 - BUFFER_SIZE;
  if (offsetIndex3 > (BUFFER_SIZE - 1))
    offsetIndex3 = offsetIndex3 - BUFFER_SIZE;
    
  // wrap the index if it goes past the buffer the other way
  if (offsetIndex1 < 0)
    offsetIndex1 = BUFFER_SIZE + offsetIndex1;
  if (offsetIndex2 < 0)
    offsetIndex2 = BUFFER_SIZE + offsetIndex2;
  if (offsetIndex3 < 0)
    offsetIndex3 = BUFFER_SIZE + offsetIndex3;

  // output the delayed + current data to the DAC
  // divide by 4 since we're adding four copies
  out_DAC0 = 1023 + (delayBuffer[offsetIndex1] >> 2) + (delayBuffer[offsetIndex2] >> 2) + (delayBuffer[offsetIndex3] >> 2);
  
  //Add volume control based in POT2
  out_DAC0=map(out_DAC0,0,4095,1,POT2);
 
  //Write the DACs
  dacc_set_channel_selection(DACC_INTERFACE, 0);       //select DAC channel 0
  dacc_write_conversion_data(DACC_INTERFACE, out_DAC0);//write on DAC


}
