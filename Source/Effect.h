#pragma once

#include <JuceHeader.h>
#include <juce_dsp/juce_dsp.h>
#include <cfloat>
#include <cmath>
#include <complex>
#include <vector>
#include <array>
#include <atomic>

using namespace juce;
using namespace juce::dsp;
using namespace std;

#define NUM_ROTARY_KNOBS 6

//==============================================================================
class EffectComponent final : public Component,
                              private juce::Timer
{
public:
    EffectComponent()
    {
        auto setUpSlider = [this] (Slider& slider, Slider::SliderStyle style,
                                   Label& label, const juce::String& name,
                                   double start, double end, double interval, double initial)
        {
            slider.setSliderStyle (style);
            slider.setRange (start, end, interval);
            slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);
            label.setText (name, juce::dontSendNotification);
            label.setJustificationType (juce::Justification::centred);
            addAndMakeVisible (label);
            slider.setValue (initial);
            addAndMakeVisible (slider);
        };

        for (uint8_t i = 0; i < NUM_ROTARY_KNOBS; i++)
            setUpSlider (rotarySliders[i], Slider::Rotary, rotarySliderLabels[i],
                         rotarySliderStrings[i],
                         default_values[i][0], default_values[i][1],
                         default_values[i][2], default_values[i][3]);

        startTimerHz (30);
    }

    ~EffectComponent() { stopTimer(); }


    //==========================================================================
    // Called on the message thread 30x/sec — safe to read UI state
    void timerCallback() override
    {
        updateParameters();
    }

    void paint (Graphics& g) override
    {
    }

    void resized () override
    {
        Grid grid;
        grid.templateRows    = { Grid::TrackInfo (30_px),        // row labels
                                 Grid::TrackInfo (Grid::Fr (1)),  // row sliders
                                 Grid::TrackInfo (30_px),         // row labels
                                 Grid::TrackInfo (Grid::Fr (1)),  // row sliders
                                 Grid::TrackInfo (30_px),         // typeBox row
                                 Grid::TrackInfo (Grid::Fr (1))}; // freq display
        grid.templateColumns = { Grid::TrackInfo (Grid::Fr (1)),
                                 Grid::TrackInfo (Grid::Fr (1)),
                                 Grid::TrackInfo (Grid::Fr (1)) };

        for (int i = 0; i < 3; i++) grid.items.add (GridItem (rotarySliderLabels[i]).withMargin ({1}));
        for (int i = 0; i < 3; i++) grid.items.add (GridItem (rotarySliders[i])     .withMargin ({1}));
        for (int i = 3; i < 6; i++) grid.items.add (GridItem (rotarySliderLabels[i]).withMargin ({1}));
        for (int i = 3; i < 6; i++) grid.items.add (GridItem (rotarySliders[i])     .withMargin ({1}));

        grid.performLayout (getLocalBounds());
    }

    //==========================================================================
    // Called from prepareToPlay — audio thread not yet running
    void prepare (const ProcessSpec& spec)
    {
        sampleRate = (float) spec.sampleRate;


        inputBufferMemory.allocate (spec.numChannels * spec.maximumBlockSize * sizeof (float), true);
        inputBuffer = AudioBlock<float> (inputBufferMemory, spec.numChannels, spec.maximumBlockSize);

        outputBufferMemory.allocate (spec.numChannels * spec.maximumBlockSize * sizeof (float), true);
        outputBuffer = AudioBlock<float> (outputBufferMemory, spec.numChannels, spec.maximumBlockSize);


        dryWetMixer.prepare ({ sampleRate,
                               (juce::uint32) spec.maximumBlockSize,
                               (juce::uint32) spec.numChannels });

    }

    float effect(float input)
    {
        return input * volume;
    }
    
    float dry_wet (float dry, float wet)
    {
        return (mix * wet) + ((1.0f - mix) * dry);
    }
    
    //==========================================================================
    // Called on the audio thread from processBlock
    void process (const ProcessContextReplacing<float>& context)
    {
        auto& inputBlock  = context.getInputBlock();
        auto& outputBlock = context.getOutputBlock();
        const auto numSamples  = outputBlock.getNumSamples();
        const auto numChannels = outputBlock.getNumChannels();

        for (size_t ch = 0; ch < numChannels; ++ch)
        {
            auto* in  = inputBlock .getChannelPointer (ch);
            auto* out = outputBlock.getChannelPointer (ch);

            for (size_t i = 0; i < numSamples; ++i)
            {
                const float drySample = in[i];
                const float wetSample = effect (drySample);

                out[i] = dry_wet (drySample, wetSample);
            }
        }
    }

    void reset() {  }

    //==========================================================================

    float volume         = 0.5f;
    float mix          = 0.5f;
    DryWetMixer<float> dryWetMixer;
    Gain<float>        gain;
    HeapBlock<char>   inputBufferMemory, outputBufferMemory;
    AudioBlock<float> inputBuffer,   outputBuffer;


private:
    float note_freq_by_index (int n) {
        return 440.0f * std::pow (2.0f, (n - 49) / 12.0f);
    }
    int get_semitones (float freq) {
        return (int) (12.0f * std::log2 (freq / 27.5f));
    }

    juce::String get_note_name (int semitones)
    {
        static const char* names[] = {"A", "A#/Bb", "B","C","C#/Db","D","D#/Eb","E","F","F#/Gb","G", "G#/Ab"};
        int octave = (int) std::round (semitones / 12.0f);
        return juce::String (names[((semitones % 12) + 12) % 12]) + juce::String (octave);
    }

    void updateParameters()
    {
        float v[NUM_ROTARY_KNOBS];
        for (int i = 0; i < NUM_ROTARY_KNOBS; i++)
            v[i] = (float) rotarySliders[i].getValue();

        volume   = v[0];
        mix      = v[3];
        dryWetMixer.setWetMixProportion (mix);
    }

    float convertFreqMultiplier (float value)
    {
        if (value >= 1.0f) return value + 1.0f;
        if (value == 0.0f) return 1.0f;
        if (value <  0.0f) return 1.0f / (std::abs (value) * 2.0f);
        return 1.0f;
    }

   float default_values[NUM_ROTARY_KNOBS][4] = {
        { 0.0,    1.0,  0.001, 0.5 },
        { 0.0,    1.0,  0.001, 0.5 },
        { 0.0,    1.0,  0.001, 0.5 },
        { 0.0,    1.0,  0.001, 0.5 },
        { 0.0,    1.0,  0.001, 0.5 },
        { 0.0,    1.0,  0.001, 0.5 }
    };
    float sampleRate = 44100.0f;

    // UI elements
    std::array<Slider,       NUM_ROTARY_KNOBS> rotarySliders;
    std::array<juce::Label,  NUM_ROTARY_KNOBS> rotarySliderLabels;
    std::array<juce::String, NUM_ROTARY_KNOBS> rotarySliderStrings = {
        "Gain", "CTRL1", "CTRL2",
        "Dry/Wet", "CTRL4", "CTRL5"
    };
    float values[NUM_ROTARY_KNOBS] {};



    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EffectComponent)
};
