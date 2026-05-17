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

//==============================================================================
class EffectAnalyzer final : public Component,
                                private juce::Timer
{
public:
    EffectAnalyzer()
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


        // for (uint8_t i =0; i<NUM_ROTARY_KNOBS; i++)
        //     setUpSlider (rotarySliders[i], Slider::Rotary, rotarySliderLabels[i], rotarySliderStrings[i],  0.0, 1.0, 0.001, 0.5);
        
        // startTimerHz (30); // call timerCallback 30 times per second
    }

    ~EffectAnalyzer()
    {
        stopTimer();
    }

    // Called 30x/sec on the message thread — safe to read Sliders here
    void timerCallback() override
    {
        updateParameters();
    }


    void impulseResponse()
    {
        // generate an impulse
        buffer[0] = 1.0f;
        for (int i = 1; i < bufferSize; i++)
            buffer[i] = 0.0f;

        // feed it through your effect and record the output
        // the output IS the impulse response
    }

    void dryWetIsolated()
    {
        float dry = input;
        float wet = effect.process(input);
        float isolated = wet - dry;  // what the effect added
    }
    
    void paint (Graphics& g) override
    {
        
    }

    void resized() override
    {

    }

    void prepare (const ProcessSpec& spec)
    {
        // initialize your effect here
    }

    void process (const ProcessContextReplacing<float>& context)
    {
        // process audio here (currently a no-op pass-through)
    }

    void reset() {
        
    }

    void updateParameters()
    {
        // read from param objects and apply to DSP
    }

private:
    // std::array<Slider, NUM_ROTARY_KNOBS> rotarySliders;
    // std::array<juce::Label, NUM_ROTARY_KNOBS> rotarySliderLabels;
    // std::array<juce::String, NUM_ROTARY_KNOBS> rotarySliderStrings = {"CTRL1","CTRL2","CTRL3","CTRL4","CTRL5","CTRL6"};

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EffectAnalyzer)
};
