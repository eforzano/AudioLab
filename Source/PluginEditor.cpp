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
    synthLabel.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    addAndMakeVisible (synthLabel);


    effectLabel.setText ("Effect", juce::dontSendNotification);
    effectLabel.setJustificationType (juce::Justification::centredLeft);
    effectLabel.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    addAndMakeVisible (effectLabel);


    // --- DSP UI components (owned by the processor) ---
    addAndMakeVisible (audioProcessor.synth);
    addAndMakeVisible (audioProcessor.effect);


    // A reasonable default size; the user can resize freely.
    setResizable (true, true);
    setSize (700, 760);
}

AudioLabAudioProcessorEditor::~AudioLabAudioProcessorEditor()
{
    // Remove the processor-owned components so they don't paint after we die.
    removeChildComponent (&audioProcessor.synth);
    removeChildComponent (&audioProcessor.effect);
}

//==============================================================================
void AudioLabAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    auto bounds = getLocalBounds().reduced (4);
    const int synthHeight = (bounds.getHeight() * 9) / 20; // ~45% for synth

    g.setColour (juce::Colours::grey);
    g.drawHorizontalLine (bounds.getY() + synthHeight + 4, 0.0f, static_cast<float> (getWidth()));
}

void AudioLabAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced (4);
    const int labelH = 20;

    auto synthArea = bounds.removeFromTop ((bounds.getHeight() * 9) / 20); // ~45%
    synthLabel.setBounds (synthArea.removeFromTop (labelH));
    audioProcessor.synth.setBounds (synthArea);

    bounds.removeFromTop (8);

    effectLabel.setBounds (bounds.removeFromTop (labelH));
    audioProcessor.effect.setBounds (bounds);
}