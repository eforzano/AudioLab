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

#define NUM_ROTARY_KNOBS 9

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
        grid.templateRows    = { 
            Grid::TrackInfo (30_px),        // row labels
            Grid::TrackInfo (Grid::Fr (1)),  // row sliders
            Grid::TrackInfo (30_px),         // row labels
            Grid::TrackInfo (Grid::Fr (1)),  // row sliders
            Grid::TrackInfo (30_px),         // row labels
            Grid::TrackInfo (Grid::Fr (1)),  // row sliders

            
        }; // freq display
        
        grid.templateColumns = { Grid::TrackInfo (Grid::Fr (1)),
            Grid::TrackInfo (Grid::Fr (1)),
            Grid::TrackInfo (Grid::Fr (1)),
        };

        for (int i = 0; i < 3; i++) grid.items.add (GridItem (rotarySliderLabels[i]).withMargin ({1}));
        for (int i = 0; i < 3; i++) grid.items.add (GridItem (rotarySliders[i])     .withMargin ({1}));
        
        for (int i = 3; i < 6; i++) grid.items.add (GridItem (rotarySliderLabels[i]).withMargin ({1}));
        for (int i = 3; i < 6; i++) grid.items.add (GridItem (rotarySliders[i])     .withMargin ({1}));
        
        for (int i = 6; i < 9; i++) grid.items.add (GridItem (rotarySliderLabels[i]).withMargin ({1}));
        for (int i = 6; i < 9; i++) grid.items.add (GridItem (rotarySliders[i])     .withMargin ({1}));

        
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
        return input;
    }
    
    float dry_wet (float dry, float wet)
    {
        return (mix * wet) + ((1.0f - mix) * dry);
    }
    
    //==========================================================================
    void process (const ProcessContextReplacing<float>& context)
    {
        auto& inputBlock  = context.getInputBlock();
        auto& outputBlock = context.getOutputBlock();
        const auto numSamples  = outputBlock.getNumSamples();
        const auto numChannels = outputBlock.getNumChannels();

        for (size_t i = 0; i < numSamples; ++i)
        {
            const float g = gain.getGainLinear(); // or gain.getNextValue() equivalent
            // advance smoothing once here, e.g. via a separate SmoothedValue you own,
            // or restructure to use gain.process() over the whole block instead.

            for (size_t ch = 0; ch < numChannels; ++ch)
            {
                auto* in  = inputBlock .getChannelPointer (ch);
                auto* out = outputBlock.getChannelPointer (ch);

                const float drySample = in[i];
                const float wetSample = dry_wet(drySample, effect(drySample));
                out[i] = wetSample * g * volume;
            }
        }
    }

    void reset() {  }

    //==========================================================================
    float mix = 0.5f;
    float volume = 1.0f;
    float gainValue = 1.0f;
    float ctrl1 = 0.5f;
    float ctrl2 = 0.5f;
    float ctrl3 = 0.5f;
    float ctrl4 = 0.5f;
    float ctrl5 = 0.5f;
    float ctrl6 = 0.5f;
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

        mix    = v[0];
        volume = v[1];
        gainValue = v[2];
        ctrl1  = v[3];
        ctrl2  = v[4];
        ctrl3  = v[5];
        ctrl4  = v[6];
        ctrl5  = v[7];
        ctrl6  = v[8];

        gain.setGainLinear(gainValue);
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
       { 0.0,    1.0,  0.001, mix },
        { 0.0,    1.0,  0.001, volume },
       { 1.0,    50.0,  0.001, gainValue },
        { 0.0,    1.0,  0.001, ctrl1 },
        { 0.0,    1.0,  0.001, ctrl2 },
        { 0.0,    1.0,  0.001, ctrl3 },
        { 0.0,    1.0,  0.001, ctrl4 },
       { 0.0,    1.0,  0.001, ctrl5 },
       { 0.0,    1.0,  0.001, ctrl6 }

    };
    float sampleRate = 44100.0f;

    // UI elements
    std::array<Slider,       NUM_ROTARY_KNOBS> rotarySliders;
    std::array<juce::Label,  NUM_ROTARY_KNOBS> rotarySliderLabels;
    std::array<juce::String, NUM_ROTARY_KNOBS> rotarySliderStrings = {
        "Dry/Wet", "Volume", "Gain", 
        "CTRL1", "CTRL2", "CTRL3",
        "CTRL4", "CTRL5", "CTRL6",
    };



    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EffectComponent)
};
