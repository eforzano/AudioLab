//
//  Oscillator.hpp
//  AudioLab
//
//  Created by Ernest Forzano on 3/18/26.
//  Copyright © 2026 JUCE. All rights reserved.
//

#pragma once

#include "DemoUtilities.h"
#include "AudioDeviceManager.h"
#include "DSPDemos_Common.h"
#include "float.h"
#include "math.h"
using namespace dsp;

class PinkNoise
{
public:
    float process()
    {
        float white = whiteNoise();

        b0 = 0.99886f * b0 + white * 0.0555179f;
        b1 = 0.99332f * b1 + white * 0.0750759f;
        b2 = 0.96900f * b2 + white * 0.1538520f;

        return (b0 + b1 + b2 + white * 0.5362f) * 0.11f;
    }

private:
    float whiteNoise() { return (rand() / (float)RAND_MAX) * 2.0f - 1.0f; }
    float b0 = 0, b1 = 0, b2 = 0;
};

class BrownNoise
{
public:
    float process(float aggression)
    {
        float white = whiteNoise();
        runningSum += white * aggression;
        runningSum = juce::jlimit(-1.0f, 1.0f, runningSum);
        return runningSum;
        
    }

private:
    float whiteNoise(){ return (rand() /(float)RAND_MAX) * 2.0f - 1.0f;}
    float runningSum = 0.0f;
};


//==============================================================================
class OscillatorComponent final : public Component,
                                  private juce::Timer
{
public:
    OscillatorComponent()
    {
        auto setUpSlider = [this] (Slider& slider, Slider::SliderStyle style,
                                   double start, double end, double interval, double initial)
        {
            slider.setSliderStyle (style);
            slider.setRange (start, end, interval);
            slider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 80, 20);
            slider.setValue (initial);
            addAndMakeVisible (slider);
        };
        
        // Toggle
        enableToggle.setToggleState (false, dontSendNotification);
        enableToggle.setClickingTogglesState(true);
        //enableToggle.onClick = [this]() {this.toggleState(); };
        addAndMakeVisible(enableToggle);
        // Dropdowns
        typeBox.addItemList ({ "sine", "saw", "square", "white noise", "pink noise", "brown noise"}, 1); // Type
        typeBox.setSelectedId (1);
        addAndMakeVisible (typeBox);
        //Sliders
        setUpSlider (freqSlider, Slider::LinearHorizontal, 0.0, 20000.0, 0.4, 440); //"Frequency", "Hz"
        setUpSlider (gainSlider, Slider::LinearHorizontal, -100.0, 20.0, 3.0, -20); //"Gain", "dB"
        
        
        // call timerCallback 30 times per second
        startTimerHz (30);
    }

    ~OscillatorComponent()
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
        //juce::Colours::red.withAlpha (0.2f);
    }

    void resized() override
    {
        Grid grid;

        grid.templateRows = { Grid::TrackInfo (Grid::Fr (1)),
            Grid::TrackInfo (Grid::Fr (1)),
            Grid::TrackInfo (Grid::Fr (1)),
            Grid::TrackInfo (Grid::Fr (1)),
            Grid::TrackInfo (Grid::Fr (1)) };

        grid.templateColumns = { Grid::TrackInfo (Grid::Fr (1))};

    
        grid.items = { GridItem (enableToggle).withMargin ({ 1 }),
            GridItem (typeBox).withMargin ({ 1 }),
            GridItem (freqSlider).withMargin ({ 1 }).withColumn ({ GridItem::Span (1), {} }),
            GridItem (gainSlider).withMargin ({ 1 })};

        grid.performLayout (getLocalBounds());
    }

    // Audio DSP Componenents
    void prepare (const ProcessSpec& spec)
    {
        audioBufferMemory.allocate (spec.numChannels * spec.maximumBlockSize * sizeof (float), true);
        audioBuffer = AudioBlock<float> (audioBufferMemory, spec.numChannels, spec.maximumBlockSize);


        gain.setGainDecibels (-6.0f);

        for (auto&& oscillator : oscillators)
        {
            oscillator.setFrequency (440.f);
            oscillator.prepare (spec);
        }

        updateParameters();

        audioBuffer = AudioBlock<float> (audioBufferMemory, spec.numChannels, spec.maximumBlockSize);
    }

    void process (const ProcessContextReplacing<float>& context)
    {
        if (oscEnabled)
        {
            audioBuffer.copyFrom (context.getInputBlock());
            audioBuffer.multiplyBy (static_cast<float> (fileMix));

            oscillators[currentOscillatorIdx].process (context);
            context.getOutputBlock().multiplyBy (static_cast<float> (1.0 - fileMix));

            context.getOutputBlock().add (audioBuffer);

            gain.process (context);
            
        }

    }

    void reset()
    {
        oscillators[currentOscillatorIdx].reset();
    }

    void updateParameters()
    {
        //typeParam.getCurrentSelectedID() - 1
        currentOscillatorIdx = jmin (numElementsInArray (oscillators),
                                        (typeBox.getSelectedItemIndex()) );

        auto freq = static_cast<float> (freqSlider.getValue());

        for (auto&& oscillator : oscillators)
            oscillator.setFrequency (freq);

        gain.setGainDecibels (static_cast<float> (gainSlider.getValue()));

        oscEnabled = enableToggle.getToggleState();
        //fileMix = mixParam.getCurrentValue();
    }
    
    float whiteNoise(){
        return (rand() / (float)RAND_MAX) * 2.0f - 1.0f;
    }
        
    
    //==============================================================================
    Oscillator<float> oscillators[6] =
    {
        // No Approximation
        {[] (float x) { return std::sin (x); }},                   // sine
        {[] (float x) { return x / MathConstants<float>::pi; }},   // saw
        {[] (float x) { return x < 0.0f ? -1.0f : 1.0f; }},        // square
        {[this] (float x) { return whiteNoise();}},                // white
        {[this] (float x) { return pink.process();}},              // pink
        {[this] (float x) { return brown.process(aggression);}},   // brown


    };

    // Audio files
    bool oscEnabled;
    HeapBlock<char> audioBufferMemory;
    AudioBlock<float> audioBuffer;
    double fileMix = 0.0f;
    float aggression = 0.5f;
    Random rng;
    PinkNoise pink;
    BrownNoise brown;

private:

    int currentOscillatorIdx = 0;
    Gain<float> gain;

    // GUI elements
    Slider freqSlider, gainSlider;
    ComboBox typeBox;
    ToggleButton enableToggle { "Enable" };

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OscillatorComponent)
};
    

