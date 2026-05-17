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
            setUpSlider (rotarySliders[i], Slider::Rotary, rotarySliderLabels[i], rotarySliderStrings[i],  0.0, 1.0, 0.001, 0.5);
        
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
                output[i] = inputSample;
            }
        }
    }

    void reset() {
        
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

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EffectComponent)
};
