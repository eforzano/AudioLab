#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

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
    juce::Label oscillatorLabel, effectLabel, synthLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioLabAudioProcessorEditor)
};
