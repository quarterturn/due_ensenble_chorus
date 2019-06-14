// Licensed under a Creative Commons Attribution 3.0 Unported License.
// Based on rcarduino.blogspot.com previous work.
// www.electrosmash.com/pedalshield
 
/*chorus_vibrato.ino creates a chorus guitar effect by delaying the signal and
modulating this delay with a triangular waveform.*/

#import "lfo.h"

#define BUFFER_SIZE 1000
#define LED 3
#define FOOTSWITCH 7
#define TOGGLE 2
 
int in_ADC0, in_ADC1, out_DAC0, out_DAC1;  //variables for 2 ADCs values (ADC0, ADC1)
int POT0, POT1, POT2; //variables for 3 pots (ADC8, ADC9, ADC10)

uint16_t delayBuffer[BUFFER_SIZE];
uint16_t bufferIndex = 0;
uint16_t delayIndex1 = 0;
uint16_t delayIndex2 = 0;
uint16_t delayIndex3 = 0;
// offset the lfo indexes by 120 degrees phase
uint16_t lfoIndex1 = 0;
uint16_t lfoIndex2 = 245;
uint16_t lfoIndex3 = 490;
uint16_t lastIndex = 0;
uint16_t bufferValue = 0;
uint16_t delayValue1 = 0;
uint16_t delayValue2 = 0;
uint16_t delayValue3 = 0;

// tracks interrupts
// used to periodically read the LFO wavetable
int interruptCount = 0;
 
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
  ADC->ADC_CHER=0x1CC0;  // Enable ADC channels 0,1,8,9 and 10  
 
  //DAC Configuration
  analogWrite(DAC0,0);  // Enables DAC0
  analogWrite(DAC1,0);  // Enables DAC0
 
  //pedalSHIELD pin configuration
  pinMode(LED, OUTPUT);  
  pinMode(FOOTSWITCH, INPUT);     
  pinMode(TOGGLE, INPUT);  
}
 
void loop()
{
  //Read the ADCs
  while((ADC->ADC_ISR & 0x1CC0)!=0x1CC0);// wait for ADC 0, 1, 8, 9, 10 conversion complete.
  in_ADC0=ADC->ADC_CDR[7];               // read data from ADC0
  //in_ADC1=ADC->ADC_CDR[6];               // read data from ADC1  
  //POT0=ADC->ADC_CDR[10];                 // read data from ADC8        
  //POT1=ADC->ADC_CDR[11];                 // read data from ADC9   
  //POT2=ADC->ADC_CDR[12];                 // read data from ADC10     
}
 
//Interrupt at 44.1KHz rate (every 22.6us)
void TC4_Handler()
{
  //Clear status allowing the interrupt to be fired again.
  TC_GetStatus(TC1, 1);
 
  //Store current readings  
  delayBuffer[bufferIndex] = in_ADC0;

  // update the buffer index
  bufferIndex++;
  // wrap around if at end of buffer
  if (bufferIndex > BUFFER_SIZE - 1)
    bufferIndex = 0;

  // slow down the wavetable by only getting the next value every ten interrupts
  if (interruptCount > 9)
  {
    lfoIndex1++;
    // wrap around if at end of wavetable
    if (lfoIndex1 > LFO_SIZE - 1)
      lfoIndex1 = 0;
      
    lfoIndex2++;
    if (lfoIndex2 > LFO_SIZE - 1)
      lfoIndex2 = 0;
      
    lfoIndex3++;
    if (lfoIndex3 > LFO_SIZE - 1)
      lfoIndex3 = 0;
      
    // reset the interrupt counter  
    interruptCount = 0;
  }
  else
  {
    interruptCount++;
  }
 
  //Adjust Delay Depth based in pot0 position.
  //POT0=map(POT0>>2,0,1024,1,25); //25 empirically chosen

  // get the delay from the wavetable
  delayValue1 = pgm_read_word_near(LFO_TABLE + lfoIndex1);
  delayValue2 = pgm_read_word_near(LFO_TABLE + lfoIndex2);
  delayValue3 = pgm_read_word_near(LFO_TABLE + lfoIndex3);

  // add the delay to the buffer index to get the delay index
  delayIndex1 = bufferIndex + delayValue1;
  delayIndex2 = bufferIndex + delayValue2;
  delayIndex3 = bufferIndex + delayValue3;
  
  // wrap the index if it goes past the end of the buffer
  if (delayIndex1 > BUFFER_SIZE - 1)
    delayIndex1 = delayIndex1 - (BUFFER_SIZE - 1);
  if (delayIndex2 > BUFFER_SIZE - 1)
    delayIndex2 = delayIndex2 - (BUFFER_SIZE - 1);
  if (delayIndex3 > BUFFER_SIZE - 1)
    delayIndex3 = delayIndex3 - (BUFFER_SIZE - 1);

  // wrap around the previous sample if needed
  if (bufferIndex == 0)
  {
    lastIndex = BUFFER_SIZE - 1;
  }
  else
  {
    lastIndex = bufferIndex - 1;
  }

  // delay the previous sample
  // mix it with what is already there
  // we will scale the output so the result does not overflow the DAC
  delayBuffer[delayIndex1] = (uint16_t)(delayBuffer[delayIndex1] + delayBuffer[lastIndex]) >> 1;
  delayBuffer[delayIndex1] = (uint16_t)(delayBuffer[delayIndex2] + delayBuffer[lastIndex]) >> 1;
  delayBuffer[delayIndex1] = (uint16_t)(delayBuffer[delayIndex3] + delayBuffer[lastIndex]) >> 1;

  // output the data
  out_DAC0 = delayBuffer[bufferIndex];
 
  //Add volume control based in POT2
  // out_DAC0=map(out_DAC0,0,4095,1,POT2);
 
  //Write the DACs
  dacc_set_channel_selection(DACC_INTERFACE, 0);       //select DAC channel 0
  dacc_write_conversion_data(DACC_INTERFACE, out_DAC0);//write on DAC
  //dacc_set_channel_selection(DACC_INTERFACE, 1);       //select DAC channel 1
  //dacc_write_conversion_data(DACC_INTERFACE, 0);       //write on DAC


}
