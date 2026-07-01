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

    juce::Label oscillatorLabel, effectLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioLabAudioProcessorEditor)
};
