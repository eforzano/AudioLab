/*
  ==============================================================================
    AudioLab Plugin Editor
    Converted from standalone GUI application to AudioProcessorEditor.

    The editor holds references (not ownership) to the OscillatorComponent and
    EffectComponent that live in the processor.  It lays them out as child
    components directly – no AudioEngine, no DeviceManager tabs, no file player.
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
class AudioLabAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit AudioLabAudioProcessorEditor (AudioLabAudioProcessor&);
    ~AudioLabAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    AudioLabAudioProcessor& audioProcessor;

    // Labels for each panel
    juce::Label oscillatorLabel { {}, "Oscillator" };
    juce::Label effectLabel     { {}, "Effect"     };

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioLabAudioProcessorEditor)
};
