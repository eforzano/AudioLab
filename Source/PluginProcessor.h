/*
  ==============================================================================
    AudioLab Plugin Processor
    Converted from standalone GUI application to AudioProcessor plugin.

    The OscillatorComponent and EffectComponent are owned here (processor side)
    so their DSP state lives on the audio thread.  The editor only holds
    lightweight references to them for drawing the UI.
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Oscillator.h"
#include "Effect.h"
#include "Synthesizer.h"

//==============================================================================
class AudioLabAudioProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    AudioLabAudioProcessor();
    ~AudioLabAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    // DSP components – owned by the processor, referenced by the editor.
    // These must outlive the editor.
    EffectComponent     effect;
    SynthesizerComponent synth;

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioLabAudioProcessor)
};
