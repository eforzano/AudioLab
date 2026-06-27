/*
  ==============================================================================
    AudioLab Plugin Editor
    Converted from standalone GUI application to AudioProcessorEditor.

    Layout
    ------
    ┌─────────────────────────────────────────────────────┐
    │  Oscillator (label + OscillatorComponent)           │  ~30 % height
    ├─────────────────────────────────────────────────────┤
    │  Effect     (label + EffectComponent)               │  ~70 % height
    └─────────────────────────────────────────────────────┘

    Both components are owned by the processor and are merely *added* to the
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
    oscillatorLabel.setJustificationType (juce::Justification::centredLeft);
    oscillatorLabel.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    addAndMakeVisible (oscillatorLabel);

    effectLabel.setJustificationType (juce::Justification::centredLeft);
    effectLabel.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    addAndMakeVisible (effectLabel);

    // --- DSP UI components (owned by the processor) ---
    addAndMakeVisible (audioProcessor.oscillator);
    addAndMakeVisible (audioProcessor.effect);

    // A reasonable default size; the user can resize freely.
    setResizable (true, true);
    setSize (700, 520);
}

AudioLabAudioProcessorEditor::~AudioLabAudioProcessorEditor()
{
    // Remove the processor-owned components so they don't paint after we die.
    removeChildComponent (&audioProcessor.oscillator);
    removeChildComponent (&audioProcessor.effect);
}

//==============================================================================
void AudioLabAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    // Dividing line between Oscillator and Effect panels
    auto bounds = getLocalBounds();
    const int oscHeight = bounds.getHeight() / 3;
    g.setColour (juce::Colours::grey);
    g.drawHorizontalLine (oscHeight + 20, 0.0f, static_cast<float> (bounds.getWidth()));
}

void AudioLabAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced (4);

    // ---- Oscillator panel (top third) ----
    const int labelH  = 20;
    const int oscH    = bounds.getHeight() / 3;

    auto oscArea = bounds.removeFromTop (oscH);
    oscillatorLabel.setBounds (oscArea.removeFromTop (labelH));
    audioProcessor.oscillator.setBounds (oscArea);

    // ---- Gap / divider ----
    bounds.removeFromTop (8);

    // ---- Effect panel (remaining space) ----
    auto fxArea = bounds;
    effectLabel.setBounds (fxArea.removeFromTop (labelH));
    audioProcessor.effect.setBounds (fxArea);
}
