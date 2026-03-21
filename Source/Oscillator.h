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
using namespace dsp;


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
            typeBox.addItemList ({ "sine", "saw", "square" }, 1); // Type
            typeBox.setSelectedId (1);
            addAndMakeVisible (typeBox);
            accuracyBox.addItemList ({ "No Approximation", "Use Wavetable"}, 1); //Accuracy
            accuracyBox.setSelectedId (1);
            addAndMakeVisible (accuracyBox);
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
                           GridItem (accuracyBox).withMargin ({ 1 }),
                           GridItem (freqSlider).withMargin ({ 1 }).withColumn ({ GridItem::Span (1), {} }),
                           GridItem (gainSlider).withMargin ({ 1 }) };

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
                                        3 * (accuracyBox.getSelectedItemIndex()) + (typeBox.getSelectedItemIndex()) );

            auto freq = static_cast<float> (freqSlider.getValue());

            for (auto&& oscillator : oscillators)
                oscillator.setFrequency (freq);

            gain.setGainDecibels (static_cast<float> (gainSlider.getValue()));

            oscEnabled = enableToggle.getToggleState();
            //fileMix = mixParam.getCurrentValue();
        }

        //==============================================================================
        Oscillator<float> oscillators[6] =
        {
            // No Approximation
            {[] (float x) { return std::sin (x); }},                   // sine
            {[] (float x) { return x / MathConstants<float>::pi; }},   // saw
            {[] (float x) { return x < 0.0f ? -1.0f : 1.0f; }},        // square

            // Approximated by a wave-table
            {[] (float x) { return std::sin (x); }, 100},                 // sine
            {[] (float x) { return x / MathConstants<float>::pi; }, 100}, // saw
            {[] (float x) { return x < 0.0f ? -1.0f : 1.0f; }, 100}       // square
        };

        // Audio files
        HeapBlock<char> audioBufferMemory;
        AudioBlock<float> audioBuffer;
        double fileMix = 0.0;


    private:
        bool oscEnabled;
        int currentOscillatorIdx = 0;
        Gain<float> gain;
        
        // GUI elements
        Slider freqSlider, gainSlider;
        ComboBox typeBox, accuracyBox;
        ToggleButton enableToggle { "Enable" };
 
        //==============================================================================
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OscillatorComponent)
    };
    