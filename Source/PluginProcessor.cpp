/*
  ==============================================================================
    AudioLab Plugin Processor
    Converted from standalone GUI application to AudioProcessor plugin.

    Key changes vs. the original standalone app
    -------------------------------------------
    1.  AudioDeviceManager / AudioIODeviceCallback removed – the host now owns
        the audio device.  prepareToPlay / processBlock replace
        audioDeviceAboutToStart / audioDeviceIOCallbackWithContext.

    2.  ExternalAudioComponent removed – in a plugin the host always feeds us
        real input samples through processBlock, so we never need to decide
        whether to copy-through microphone input ourselves.

    3.  AudioPlayerComponent removed – loading and playing audio files from
        disk is not appropriate inside an audio-effect plugin.  The host
        supplies the audio.

    4.  OscillatorComponent and EffectComponent are kept intact and owned here.
        The editor only holds references to them for UI purposes.
  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AudioLabAudioProcessor::AudioLabAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                      #if ! JucePlugin_IsMidiEffect
                       #if ! JucePlugin_IsSynth
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                       #endif
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                      #endif
                        )
#endif
{
    // When the Effect's pitch-tracker locks onto / releases a note, drive
    // the synth's ADSR voices with it.
//    effect.onNoteOn  = [this] (int midiNoteNumber, float velocity)
//    {
//        synth.triggerNoteOn (midiNoteNumber, velocity);
//    };
//
//    effect.onNoteOff = [this] (int midiNoteNumber)
//    {
//        synth.triggerNoteOff (midiNoteNumber);
//    };
}

AudioLabAudioProcessor::~AudioLabAudioProcessor()
{
}

//==============================================================================
const juce::String AudioLabAudioProcessor::getName() const  { return JucePlugin_Name; }
bool AudioLabAudioProcessor::acceptsMidi()  const           { return true; }
bool AudioLabAudioProcessor::producesMidi() const           { return false; }
bool AudioLabAudioProcessor::isMidiEffect() const           { return false; }
double AudioLabAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int  AudioLabAudioProcessor::getNumPrograms()                              { return 1; }
int  AudioLabAudioProcessor::getCurrentProgram()                           { return 0; }
void AudioLabAudioProcessor::setCurrentProgram (int)                       {}
const juce::String AudioLabAudioProcessor::getProgramName (int)            { return {}; }
void AudioLabAudioProcessor::changeProgramName (int, const juce::String&)  {}

//==============================================================================
void AudioLabAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Mirror what AudioEngine::audioDeviceAboutToStart() used to do.
    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    spec.numChannels      = static_cast<juce::uint32> (getTotalNumOutputChannels());

    effect.prepare     (spec);
    synth.prepare      (spec);
}

void AudioLabAudioProcessor::releaseResources()
{
    // Mirror audioDeviceStopped().
    effect.reset();
    synth.reset();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool AudioLabAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

//==============================================================================
void AudioLabAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                           juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Clear any output channels that have no corresponding input.
    const int totalIn  = getTotalNumInputChannels();
    const int totalOut = getTotalNumOutputChannels();
    for (int i = totalIn; i < totalOut; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Let host-supplied MIDI (a DAW piano roll, a connected keyboard
    // controller, etc.) drive the synth alongside the effect's own
    // pitch-triggered notes and the on-screen keyboard.
    synth.pushIncomingMidi (midiMessages);

    // Wrap the AudioBuffer in the DSP block / context that our components expect.
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);

    // Mirror audioDeviceIOCallbackWithContext() – same order as before.
    synth.process(context);
    effect.process(context);

}

//==============================================================================
bool AudioLabAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* AudioLabAudioProcessor::createEditor()
{
    return new AudioLabAudioProcessorEditor (*this);
}

//==============================================================================
void AudioLabAudioProcessor::getStateInformation (juce::MemoryBlock& /*destData*/)
{
    // TODO: serialise oscillator / effect parameter state here.
}

void AudioLabAudioProcessor::setStateInformation (const void* /*data*/, int /*sizeInBytes*/)
{
    // TODO: restore oscillator / effect parameter state here.
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioLabAudioProcessor();
}
