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
    oscillatorLabel.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    addAndMakeVisible (oscillatorLabel);

    effectLabel.setText ("Effect", juce::dontSendNotification);
    effectLabel.setJustificationType (juce::Justification::centredLeft);
    effectLabel.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    addAndMakeVisible (effectLabel);

    synthLabel.setText ("Synth", juce::dontSendNotification);
    synthLabel.setJustificationType (juce::Justification::centredLeft);
    synthLabel.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    addAndMakeVisible (synthLabel);

    // --- DSP UI components (owned by the processor) ---
    addAndMakeVisible (audioProcessor.oscillator);
    addAndMakeVisible (audioProcessor.effect);
    addAndMakeVisible (audioProcessor.synth);

    // A reasonable default size; the user can resize freely.
    setResizable (true, true);
    setSize (700, 760);
}

AudioLabAudioProcessorEditor::~AudioLabAudioProcessorEditor()
{
    // Remove the processor-owned components so they don't paint after we die.
    removeChildComponent (&audioProcessor.oscillator);
    removeChildComponent (&audioProcessor.effect);
    removeChildComponent (&audioProcessor.synth);
}

//==============================================================================
void AudioLabAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    // Dividing lines between Oscillator / Effect / Synth panels
    auto bounds = getLocalBounds();
    const int oscHeight    = bounds.getHeight() / 5;       // ~20%
    const int effectHeight = (bounds.getHeight() * 2) / 5; // ~40%

    g.setColour (juce::Colours::grey);
    g.drawHorizontalLine (oscHeight + 20, 0.0f, static_cast<float> (bounds.getWidth()));
    g.drawHorizontalLine (oscHeight + effectHeight + 28, 0.0f, static_cast<float> (bounds.getWidth()));
}

void AudioLabAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced (4);

    const int labelH = 20;

    // ---- Oscillator panel (~20% height) ----
    auto oscArea = bounds.removeFromTop (bounds.getHeight() / 5);
    oscillatorLabel.setBounds (oscArea.removeFromTop (labelH));
    audioProcessor.oscillator.setBounds (oscArea);

    bounds.removeFromTop (8); // gap / divider

    // ---- Effect panel (~50% of remaining height) ----
    auto fxArea = bounds.removeFromTop ((bounds.getHeight() * 5) / 8);
    effectLabel.setBounds (fxArea.removeFromTop (labelH));
    audioProcessor.effect.setBounds (fxArea);

    bounds.removeFromTop (8); // gap / divider

    // ---- Synth panel (remaining space) ----
    auto synthArea = bounds;
    synthLabel.setBounds (synthArea.removeFromTop (labelH));
    audioProcessor.synth.setBounds (synthArea);
}
