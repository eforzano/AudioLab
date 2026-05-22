#pragma once

#include "DemoUtilities.h"
#include "AudioDeviceManager.h"
#include "DSPDemos_Common.h"
using namespace dsp;

//==============================================================================
/**
    EffectsComponent with 6 Slider components so that the user can easily 
    sandbox new effects. 
*/

#define NUM_ROTARY_KNOBS 6
//==============================================================================
class EffectComponent final : public Component,
                                private juce::Timer
{
public:
    EffectComponent()
    :forwardFFT (fftOrder),
     window(fftSize, juce::dsp::WindowingFunction<float>::hann)
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


        for (uint8_t i =0; i<NUM_ROTARY_KNOBS; i++)
            setUpSlider (rotarySliders[i], Slider::Rotary, rotarySliderLabels[i], rotarySliderStrings[i],
                         default_values[i][0], default_values[i][1], default_values[i][2], default_values[i][3]);
        
        startTimerHz (30); // call timerCallback 30 times per second
    }

    ~EffectComponent()
    {
        stopTimer();
    }

    // Called 30x/sec on the message thread — safe to read Sliders here
    void timerCallback() override
    {
        updateParameters();
    }

    
    void paint (Graphics& g) override
    {
        
    }

    void resized() override
    {
        Grid grid;

        grid.templateRows = { Grid::TrackInfo (30_px),
                                Grid::TrackInfo (Grid::Fr (1)),
                                Grid::TrackInfo (30_px),
                                Grid::TrackInfo (Grid::Fr (1)), };

        grid.templateColumns = { Grid::TrackInfo (Grid::Fr (1)),
                                    Grid::TrackInfo (Grid::Fr (1)),
                                    Grid::TrackInfo (Grid::Fr (1)) };



        // Rotary labels and sliders
        grid.items.add (GridItem (rotarySliderLabels[0]).withMargin ({ 1 }));
        grid.items.add (GridItem (rotarySliderLabels[1]).withMargin ({ 1 }));
        grid.items.add (GridItem (rotarySliderLabels[2]).withMargin ({ 1 }));
        grid.items.add (GridItem (rotarySliders[0]).withMargin ({ 1 }));
        grid.items.add (GridItem (rotarySliders[1]).withMargin ({ 1 }));
        grid.items.add (GridItem (rotarySliders[2]).withMargin ({ 1 }));
        
        // Rotary labels and sliders
        grid.items.add (GridItem (rotarySliderLabels[3]).withMargin ({ 1 }));
        grid.items.add (GridItem (rotarySliderLabels[4]).withMargin ({ 1 }));
        grid.items.add (GridItem (rotarySliderLabels[5]).withMargin ({ 1 }));
        grid.items.add (GridItem (rotarySliders[3]).withMargin ({ 1}));
        grid.items.add (GridItem (rotarySliders[4]).withMargin ({ 1 }));
        grid.items.add (GridItem (rotarySliders[5]).withMargin ({ 1 }));
                
        grid.performLayout (getLocalBounds());
    }
    

    void prepare (const ProcessSpec& spec)
    {
        // initialize your effect here
    }

    void process (const ProcessContextReplacing<float>& context)
    {
        auto& inputBlock = context.getInputBlock();
        auto& outputBlock = context.getOutputBlock();
        auto numSamples = outputBlock.getNumSamples();
        auto numChannels = outputBlock.getNumChannels();
        jassert (inputBlock.getNumSamples() == numSamples);
        jassert (inputBlock.getNumChannels() == numChannels);
        for (size_t ch = 0; ch < numChannels; ++ch)
        {
            auto* input = inputBlock.getChannelPointer (ch);
            auto* output = outputBlock.getChannelPointer (ch);
            for (size_t i = 0; i < numSamples; ++i)
            {
                auto inputSample = input[i];
                pushNextSampleIntoFifo(inputSample);
                output[i] = inputSample;
            }
        }
        processFFT();
    }
    
    void processFFT()
    {
        if (nextFFTBlockReady)
        {
            // Window Data
            window.multiplyWithWindowingTable (fftData.data(), fftSize);
            // Then FFT
            forwardFFT.performFrequencyOnlyForwardTransform (fftData.data());
            // Skip 0 because thats 0Hz
            for (uint16_t i=1; i<fftSize/2; ++i)
            {
                binMagnitudes.push_back({ getFrequencyFromBin(i), fftData[i]});
            }
            
            std::sort(binMagnitudes.begin(), binMagnitudes.end(),
                [](const auto& a, const auto& b) { return a.second > b.second; });

            nextFFTBlockReady = false;
        }
    }

    void reset() {
        
    }
    
    uint16_t getFrequencyFromBin(uint16_t binIndex)
    {
        // Get SampleRate
        return (binIndex * 48000)/fftSize;
    }

    void pushNextSampleIntoFifo(float sample) noexcept
    {
        // if fifo contains enough data set a flag
        if (fifoIndex == fftSize)
        {

            //DBG("FFT Max Level " + juce::String(maxLevel));
            if (!nextFFTBlockReady)
            {
                std::fill(fftData.begin(), fftData.end(), 0.0f);
                std::copy(fifo.begin(), fifo.end(), fftData.begin());
                nextFFTBlockReady = true;
            }
            fifoIndex = 0;
        }
        fifo[(size_t) fifoIndex++] = sample;
    }

    void updateParameters()
    {
        // read from param objects and apply to DSP
        for (uint8_t i=0; i < NUM_ROTARY_KNOBS; i++)
        {
            values[i] = static_cast<float> (rotarySliders[i].getValue());
        }

    }

private:
    std::array<Slider, NUM_ROTARY_KNOBS> rotarySliders;
    std::array<juce::Label, NUM_ROTARY_KNOBS> rotarySliderLabels;
    std::array<juce::String, NUM_ROTARY_KNOBS> rotarySliderStrings = {"CTRL1","CTRL2","CTRL3","CTRL4","CTRL5","CTRL6"};
    float values[NUM_ROTARY_KNOBS];
    float default_values[NUM_ROTARY_KNOBS][4] = {
        {0.0, 1.0, 0.001, 0.5},
        {0.0, 1.0, 0.001, 0.5},
        {0.0, 1.0, 0.001, 0.5},
        {0.0, 1.0, 0.001, 0.5},
        {0.0, 1.0, 0.001, 0.5},
        {0.0, 1.0, 0.001, 0.5}
    };
    
    // Effect
    static constexpr auto fftOrder = 10;
    static constexpr auto fftSize = 1 << fftOrder;
    juce::dsp::FFT forwardFFT;
    std::array<float, fftSize> fifo;
    std::array<float, fftSize * 2> fftData;
    int fifoIndex;
    bool nextFFTBlockReady = false;
    std::vector<std::pair<uint16_t, float>> binMagnitudes;
    juce::dsp::WindowingFunction<float> window;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EffectComponent)
};
