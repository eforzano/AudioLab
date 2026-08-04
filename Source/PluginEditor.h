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
    juce::Label oscillatorLabel, synthLabel, effectLabel;
        class TabPanel : public juce::Component
    {
    public:
        TabPanel (juce::Label& label, juce::Component& content)
            : labelRef (label), contentRef (content)
        {
            addAndMakeVisible (labelRef);
            addAndMakeVisible (contentRef);
        }

        void resized() override
        {
            auto bounds = getLocalBounds();
            labelRef.setBounds (bounds.removeFromTop (20));
            contentRef.setBounds (bounds);
        }

    private:
        juce::Label& labelRef;
        juce::Component& contentRef;
    };

    juce::TabbedComponent oscSynthTabs { juce::TabbedButtonBar::TabsAtTop };
    std::unique_ptr<TabPanel> oscTabPanel;
    std::unique_ptr<TabPanel> synthTabPanel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioLabAudioProcessorEditor)

};
