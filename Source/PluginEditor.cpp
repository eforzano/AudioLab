/*
  ==============================================================================
    AudioLab Plugin Editor

    Components are owned by the processor and are merely *added* to the
    editor as children here.  They will be *removed* in the destructor so that
    they don't try to paint after the editor is deleted.
  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AudioLabAudioProcessorEditor::AudioLabAudioProcessorEditor (AudioLabAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // --- Labels ---
    synthLabel.setText ("Synth", juce::dontSendNotification);
    synthLabel.setJustificationType (juce::Justification::centredLeft);
    synthLabel.setFont (juce::FontOptions (18.0f, juce::Font::bold));
    
    oscillatorLabel.setText ("Oscillator", juce::dontSendNotification);
    oscillatorLabel.setJustificationType (juce::Justification::centredLeft);
    oscillatorLabel.setFont (juce::FontOptions (18.0f, juce::Font::bold));


    effectLabel.setText ("Effect", juce::dontSendNotification);
    effectLabel.setJustificationType (juce::Justification::centredLeft);
    effectLabel.setFont (juce::FontOptions (18.0f, juce::Font::bold));
    addAndMakeVisible (effectLabel);

    // --- Oscillator / Synth tabs ---
    synthTabPanel = std::make_unique<TabPanel> (synthLabel, audioProcessor.synth);
    oscTabPanel   = std::make_unique<TabPanel> (oscillatorLabel, audioProcessor.oscillator);

    oscSynthTabs.addTab ("Synth",      juce::Colours::transparentBlack, synthTabPanel.get(), false);
    oscSynthTabs.addTab ("Oscillator", juce::Colours::transparentBlack, oscTabPanel.get(), false);
    addAndMakeVisible (oscSynthTabs);

    // --- DSP UI components (owned by the processor) ---
    addAndMakeVisible (audioProcessor.effect);

    setResizable (true, true);
    setSize (700, 760);
}

AudioLabAudioProcessorEditor::~AudioLabAudioProcessorEditor()
{
    // Detach processor-owned components from the tab panels before we're destroyed
    if (oscTabPanel != nullptr)
        oscTabPanel->removeChildComponent (&audioProcessor.oscillator);

    if (synthTabPanel != nullptr)
        synthTabPanel->removeChildComponent (&audioProcessor.synth);

    removeChildComponent (&audioProcessor.effect);
}

//==============================================================================
void AudioLabAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    auto bounds = getLocalBounds().reduced (4);
    const int tabAreaHeight = (bounds.getHeight() * 3) / 7; // keep in sync with resized()

    g.setColour (juce::Colours::grey);
    g.drawHorizontalLine (bounds.getY() + tabAreaHeight + 4, 0.0f, static_cast<float> (getWidth()));
}

void AudioLabAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced (4);
    const int labelH = 20;

    // ---- Oscillator/Synth tabs (TOP, ~60%) ----
    auto tabArea = bounds.removeFromTop ((bounds.getHeight() * 3) / 7);
    oscSynthTabs.setBounds (tabArea);

    bounds.removeFromTop (8); // gap / divider

    // ---- Effect panel (BOTTOM, remaining space) ----
    effectLabel.setBounds (bounds.removeFromTop (labelH));
    audioProcessor.effect.setBounds (bounds);
}
