
# due_ensenble_chorus
a string-machine ensemble chorus emulation using an Arduino Due and PedalShield audio interface

Uses a pre-made wavetable which is read out to modulate the delay times and approximate the Roland RS-202 chorus effect.

The left pot on the Pedalshield controls the LFO speed; 12 o'clock position is about right. The right pot acts as an output volume control. The center pot is not used. The left-hand switch should be in the "ip" position so the original signal is not mixed into the output. Finally, leave the offset jumper off, since my code doesn't gang the ADC/DACs.

You can try the dual ADC/DAC version, but I think it's a lot more noisy. I think the Due's ADCs are picking up a lot of noise and using both doubles it. If you use the dual version, place the jumper on "BIAS".
