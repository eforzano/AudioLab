#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "NeonLookAndFeel.h"

class AudioLabAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit AudioLabAudioProcessorEditor (AudioLabAudioProcessor&);
    ~AudioLabAudioProcessorEditor() override;

    void paint  (juce::Graphics&) override;
    void resized() override;

private:
    AudioLabAudioProcessor& audioProcessor;

    // Must be declared before any child components that use it
    NeonLookAndFeel neonLAF;

    juce::Label oscillatorLabel, effectLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioLabAudioProcessorEditor)
};
