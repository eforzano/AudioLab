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
#define NUM_BUFF_SAMPLES  44100

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

        startTimerHz (60);
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
        /*
        const uint32_t maxBitnessValue = 0x7F;//0x7FFF;

        //crushiness = powf(crushiness, 2);

        float theBitness = ctrl1 * maxBitnessValue;
        int32_t truncatedSamp = input * theBitness;
        float crushedSamp = (float)truncatedSamp / theBitness;
        
        return crushedSamp * volume;
        */

        /*
        //For initializing buffer on first call
        static bool firstRun = 1;
        //For checking if knob has been turned enough to trigger settings change
        static float prevKnobSetting = 0;
        //Buffer indexer
        static uint32_t readIndex = 0;
        //Index in the buffer where read/write starts
        static const uint32_t startIndex = 0;
        //Index in buffer where last read/write occurs and loops back to startIndex.
        static uint32_t stopIndex = NUM_BUFF_SAMPLES - 1;
        */

        float delayKnob = ctrl1;
        float depthKnob = ctrl2;
        float levelKnob = ctrl3;

        //Check if knob adjustment hits threshold
        if ((abs(delayKnob - prevKnobSetting) >= 0.002) || firstRun)
        {
            //First call buffer setup
            if (firstRun)
            {
                //Reset buffer
                memset(delayBuffer, 0, NUM_BUFF_SAMPLES * sizeof(float));
                firstRun = 0;
            }

            //Set stop index to x number of samples ahead of start index based on knob setpoint
            //Subtract 1 from total number of buffer samples to ensure stop does not equal start at both delayknob = 0 and = 1.
            uint32_t prevStopIndex = stopIndex;
            stopIndex = (uint32_t)(delayKnob * (NUM_BUFF_SAMPLES - 1));
            DBG("Stop Index: " + juce::String(stopIndex));

            //Set minimum delay buffer size to 3 samples.
            //if(stopIndex - startIndex + 1 < 3) stopIndex = (startIndex + 2) % NUM_BUFF_SAMPLES;

            /*
            if (stopIndex > prevStopIndex)
            {
                float blendFactor = 1.0f / (stopIndex - prevStopIndex + 1);

                for (uint32_t i = 1; i <= stopIndex - prevStopIndex; i++)
                {
                    delayBuffer[prevStopIndex + i] = (delayBuffer[prevStopIndex] * (1 - (blendFactor * i))) + (delayBuffer[startIndex] * (blendFactor * i));
                }

                //Alternate blend method where it divides the previous stop index sample by two (towards zero) 
                //and blends it with the same thing using the start index sample from the opposite index direction
                /*
                float tempVal = delayBuffer[prevStopIndex];
                for(uint32_t i = 1; i <= stopIndex - prevStopIndex; i++)
                {
                    tempVal /= 2;
                    delayBuffer[prevStopIndex + i] = tempVal;
                }

                tempVal = delayBuffer[startIndex];
                for(uint32_t i = stopIndex - prevStopIndex; i >= 1 ; i--)
                {
                    tempVal /= 2;
                    delayBuffer[prevStopIndex + i] += tempVal;
                }
                */

        /*
        }
        else
        {
            //Stop index may have moved to lower than read index, so constrain i back in bounds by setting it to start index.
            if (readIndex > stopIndex) readIndex = startIndex;

            float blendedSample = (delayBuffer[startIndex] + delayBuffer[stopIndex]) / 2;
            delayBuffer[startIndex] = delayBuffer[stopIndex] = blendedSample;
        }
        */

            //Record the knob setting for checking threshold on later loops
            prevKnobSetting = delayKnob;
        }

        //TODO: Try 0.99f + 0.01f for the mixing equations, or find a more optimum one.

        //Modulate input with sample from delay buffer
        //The greater the level knob, the more the buffer sample is weighted for playback.  
        //Current max weight is 0.50 and min is 0.05 for buffer.
        //float output = (input * ((1 - levelKnob) * 0.45f + 0.50f)) + (delayBuffer[readIndex] * (levelKnob * 0.45f + 0.05f));
        float output = ((input * 0.5f) + (delayBuffer[readIndex] * 0.5f));

        //Store current input in delay buffer.  
        //The greater the depth knob, the more previous samples are weighted in the buffer.
        delayBuffer[readIndex] = (input * ((1 - depthKnob) * 0.90f + 0.05f)) + (delayBuffer[readIndex] * (depthKnob * 0.90f + 0.05f));

        //Increment i
        readIndex = (readIndex + 1) % (stopIndex + 1);

        return output;
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
                float drySample = in[i];
                float wetSample = effect (drySample);

                out[i] = dry_wet (drySample, wetSample);
            }
        }
    }

    void reset() {  }

    //==========================================================================

    float volume         = 0.5f;
    float mix          = 0.5f;
    float ctrl1 = 0.5f;
    float ctrl2 = 0.5f;
    float ctrl3 = 0.5f;
    float ctrl4 = 0.5f;
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
        /*
        float values[NUM_ROTARY_KNOBS];
        for (int i = 0; i < NUM_ROTARY_KNOBS; i++)
            values[i] = (float) rotarySliders[i].getValue();
            */

        volume   = rotarySliders[0].getValue();
        mix      = rotarySliders[3].getValue();
        ctrl1 = rotarySliders[1].getValue();
        ctrl2 = rotarySliders[2].getValue();
        ctrl3 = rotarySliders[4].getValue();
        ctrl4 = rotarySliders[5].getValue();
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
        { 0.0,    1.0,  0.001, volume },
        { 0.0,    1.0,  0.001, ctrl1 },
        { 0.0,    1.0,  0.001, ctrl2 },
        { 0.0,    1.0,  0.001, mix },
        { 0.0,    1.0,  0.001, ctrl3 },
        { 0.0,    1.0,  0.001, ctrl4 }
    };
    float sampleRate = 44100.0f;

    // UI elements
    std::array<Slider,       NUM_ROTARY_KNOBS> rotarySliders;
    std::array<juce::Label,  NUM_ROTARY_KNOBS> rotarySliderLabels;
    std::array<juce::String, NUM_ROTARY_KNOBS> rotarySliderStrings = {
        "Gain", "CTRL1", "CTRL2",
        "Dry/Wet", "CTRL3", "CTRL4"
    };
    // float values[NUM_ROTARY_KNOBS]{};
    float delayBuffer[NUM_BUFF_SAMPLES];

    //For initializing buffer on first call
    bool firstRun = 1;
    //For checking if knob has been turned enough to trigger settings change
    float prevKnobSetting = 0;
    //Buffer indexer
    uint32_t readIndex = 0;
    //Index in the buffer where read/write starts
    const uint32_t startIndex = 0;
    //Index in buffer where last read/write occurs and loops back to startIndex.
    uint32_t stopIndex = NUM_BUFF_SAMPLES - 1;



    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EffectComponent)
};
