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
    oscillatorLabel.setText ("Oscillator", juce::dontSendNotification);
    oscillatorLabel.setJustificationType (juce::Justification::centredLeft);
    oscillatorLabel.setFont (juce::FontOptions (18.0f, juce::Font::bold));
    addAndMakeVisible (oscillatorLabel);

    synthLabel.setText ("Synth", juce::dontSendNotification);
    synthLabel.setJustificationType (juce::Justification::centredLeft);
    synthLabel.setFont (juce::FontOptions (18.0f, juce::Font::bold));
    addAndMakeVisible (synthLabel);

    effectLabel.setText ("Effect", juce::dontSendNotification);
    effectLabel.setJustificationType (juce::Justification::centredLeft);
    effectLabel.setFont (juce::FontOptions (18.0f, juce::Font::bold));
    addAndMakeVisible (effectLabel);

    // --- DSP UI components (owned by the processor) ---
    addAndMakeVisible (audioProcessor.oscillator);
    addAndMakeVisible (audioProcessor.synth);
    addAndMakeVisible (audioProcessor.effect);

    setResizable (true, true);
    setSize (700, 760);
}

AudioLabAudioProcessorEditor::~AudioLabAudioProcessorEditor()
{
    removeChildComponent (&audioProcessor.oscillator);
    removeChildComponent (&audioProcessor.synth);
    removeChildComponent (&audioProcessor.effect);
}

//==============================================================================
void AudioLabAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    auto bounds = getLocalBounds().reduced (4);

    // Same proportions as resized() — keep these in sync
    const int oscHeight   = (bounds.getHeight() * 1) / 5;  // ~20%

    // Compute actual pixel boundaries the same way resized() does
    auto afterOsc = bounds;
    afterOsc.removeFromTop (oscHeight);
    const int synthPixelHeight = (afterOsc.getHeight() * 11) / 20;

    g.setColour (juce::Colours::grey);
    g.drawHorizontalLine (bounds.getY() + oscHeight + 4, 0.0f, static_cast<float> (getWidth()));
    g.drawHorizontalLine (bounds.getY() + oscHeight + 8 + synthPixelHeight + 4, 0.0f, static_cast<float> (getWidth()));
}

void AudioLabAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced (4);
    const int labelH = 20;

    // ---- Oscillator panel (TOP, ~20%) ----
    auto oscArea = bounds.removeFromTop (bounds.getHeight() / 5);
    oscillatorLabel.setBounds (oscArea.removeFromTop (labelH));
    audioProcessor.oscillator.setBounds (oscArea);

    bounds.removeFromTop (8); // gap / divider

    // ---- Synth panel (MIDDLE, ~55% of what's left) ----
    auto synthArea = bounds.removeFromTop ((bounds.getHeight() * 11) / 20);
    synthLabel.setBounds (synthArea.removeFromTop (labelH));
    audioProcessor.synth.setBounds (synthArea);

    bounds.removeFromTop (8); // gap / divider

    // ---- Effect panel (BOTTOM, remaining space) ----
    effectLabel.setBounds (bounds.removeFromTop (labelH));
    audioProcessor.effect.setBounds (bounds);
}
