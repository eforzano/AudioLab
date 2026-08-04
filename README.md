# AudioLab
AudioLab is a JUCE development platform for sandboxing your effect.
It can be a standalone or VST, and be built for Linux, MacOS, or Windows. 

The signal flow is:
- Synthesizer -> Oscillator -> Effect


## Synthesizer
The synthesizer is an ADSR synth has the following waveforms: 
- sine
- square
- saw
- triangle

It can take in MIDI from your computer keyboard or an external. 


## Oscillator 
The oscillator has the following waveforms: 
- sine
- square
- saw
- triangle
- white noise
- pink noise
- brown noise

## Effect
The effect you are sandboxing with knob controls:
- drywet, volume, gain,
- ctrl1, ctrl2, ctrl3,
- ctrl4, ctrl5, ctrl6



