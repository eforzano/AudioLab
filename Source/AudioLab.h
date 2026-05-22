/*
  ==============================================================================

   This file is part of the JUCE framework examples.
   Copyright (c) Raw Material Software Limited

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   to use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
   REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
   AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
   INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
   LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
   OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
   PERFORMANCE OF THIS SOFTWARE.

  ==============================================================================
*/

/*******************************************************************************
 The block below describes the properties of this PIP. A PIP is a short snippet
 of code that can be read by the Projucer and used to generate a JUCE project.

 BEGIN_JUCE_PIP_METADATA

 name:             AccessibilityDemo
 version:          1.0.0
 vendor:           JUCE
 website:          http://juce.com
 description:      Simple UI demonstrating accessibility features on supported
                   platforms.

 dependencies:     juce_core, juce_data_structures, juce_events, juce_graphics,
                   juce_gui_basics, juce_gui_extra
 exporters:        xcode_mac, vs2022, vs2026, androidstudio, xcode_iphone

 moduleFlags:      JUCE_STRICT_REFCOUNTEDPOINTER=1
                   JUCE_PUSH_NOTIFICATIONS=1

 type:             Component
 mainClass:        AccessibilityDemo

 useLocalCopy:     1

 END_JUCE_PIP_METADATA

*******************************************************************************/

#pragma once
#include <string>

#include "DemoUtilities.h"
#include "AudioDeviceManager.h"
#include "Effect.h"
#include "Oscillator.h"
#include "AudioPlayer.h"
#include "DSPDemos_Common.h"
using namespace dsp;
using namespace std;

//==============================================================================
/**
    A simple holder component with some content, a title and an info tooltip
    containing a brief description.

    This component sets its accessibility title and help text properties and
    also acts as a focus container for its children.
*/

class ContentComponent final : public Component
{
public:
    ContentComponent (const String& title, const String& info, Component& contentToDisplay)
         : titleLabel ({}, title),
           content (contentToDisplay)
    {
        addAndMakeVisible (titleLabel);
        addAndMakeVisible (infoIcon);

        setTitle (title);
        setDescription (info);
        setFocusContainerType (FocusContainerType::focusContainer);

        infoIcon.setTooltip (info);
        infoIcon.setHelpText (info);

        addAndMakeVisible (content);
    }

    void paint (Graphics& g) override
    {
        g.setColour (Colours::black);
        g.drawRoundedRectangle (getLocalBounds().reduced (2).toFloat(), 5.0f, 3.0f);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (5);

        auto topArea = bounds.removeFromTop (30);
        infoIcon.setBounds (topArea.removeFromLeft (30).reduced (5));
        titleLabel.setBounds (topArea.reduced (5));

        content.setBounds (bounds);
    }

private:
    //==============================================================================
    struct InfoIcon final : public Component,
                            public SettableTooltipClient
    {
        InfoIcon()
        {
            constexpr uint8 infoPathData[] =
            {
              110,109,0,0,122,67,0,0,0,0,98,79,35,224,66,0,0,0,0,0,0,0,0,79,35,224,66,0,0,0,0,0,0,122,67,98,0,0,
              0,0,44,247,193,67,79,35,224,66,0,0,250,67,0,0,122,67,0,0,250,67,98,44,247,193,67,0,0,250,67,0,0,
              250,67,44,247,193,67,0,0,250,67,0,0,122,67,98,0,0,250,67,79,35,224,66,44,247,193,67,0,0,0,0,0,0,122,
              67,0,0,0,0,99,109,114,79,101,67,79,35,224,66,108,71,88,135,67,79,35,224,66,108,71,88,135,67,132,229,
              28,67,108,116,79,101,67,132,229,28,67,108,116,79,101,67,79,35,224,66,99,109,79,35,149,67,106,132,
              190,67,108,98,185,123,67,106,132,190,67,98,150,123,106,67,106,132,190,67,176,220,97,67,168,17,187,
              67,176,220,97,67,18,150,177,67,108,176,220,97,67,248,52,108,67,98,176,220,97,67,212,8,103,67,238,
              105,94,67,18,150,99,67,204,61,89,67,18,150,99,67,108,98,185,73,67,18,150,99,67,108,98,185,73,67,88,
              238,59,67,108,160,70,120,67,88,238,59,67,98,54,194,132,67,88,238,59,67,169,17,137,67,60,141,68,67,
              169,17,137,67,8,203,85,67,108,169,17,137,67,26,97,166,67,98,169,17,137,67,43,247,168,67,10,203,138,
              67,141,176,170,67,27,97,141,67,141,176,170,67,108,80,35,149,67,141,176,170,67,108,80,35,149,67,106,
              132,190,67,99,101,0,0
            };

            infoPath.loadPathFromData (infoPathData, numElementsInArray (infoPathData));

            setTitle ("Info");
        }

        void paint (Graphics& g) override
        {
            auto bounds = getLocalBounds().toFloat().reduced (2);

            g.setColour (Colours::white);
            g.fillPath (infoPath, RectanglePlacement (RectanglePlacement::centred)
                                    .getTransformToFit (infoPath.getBounds(), bounds));
        }

        Path infoPath;
    };

    //==============================================================================
    Label titleLabel;
    InfoIcon infoIcon;
    Component& content;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ContentComponent)
};

//==============================================================================
/**
    The top-level component containing the accessible JUCE widget examples.

    Most JUCE UI elements have built-in accessibility support and will be
    visible and controllable by accessibility clients. There are a few examples
    of some widgets in this demo such as Sliders, Buttons and a TreeView.
*/
class AudioLabComponent final : public Component
{
public:
    AudioLabComponent(AudioDeviceManager& adm)
        : audioFileComponent(adm)
    {
        setTitle ("AudioLab");
        setDescription ("A platform to develop your audio effects using JUCE.");
        setFocusContainerType (FocusContainerType::focusContainer);

        addAndMakeVisible (buttons);
        addAndMakeVisible (sliders);
        addAndMakeVisible (effect);
        

    }

    void resized() override
    {
        Grid grid;

        grid.templateRows = { Grid::TrackInfo (Grid::Fr (1)), Grid::TrackInfo (Grid::Fr (2)) };
        grid.templateColumns = { Grid::TrackInfo (Grid::Fr (1)), Grid::TrackInfo (Grid::Fr (1)) };

        grid.items = { GridItem (buttons).withMargin ({ 2 }),
                       GridItem (sliders).withMargin ({ 2 }),
                       GridItem (effect).withMargin ({ 2 }).withColumn ({ GridItem::Span (2), {} }) };

        grid.performLayout (getLocalBounds());
    }
    
    //==============================================================================
    class AudioEngine : public juce::AudioIODeviceCallback
    {
    public:
        AudioEngine (AudioDeviceManager& adm,
                     AudioPlayerComponent& af,
                     OscillatorComponent& osc,
                     EffectComponent& fx)
            : deviceManager (adm), audioFile (af), oscillator (osc), effect (fx)
        {
            deviceManager.addAudioCallback (this);  // register on the SHARED manager
        }

        ~AudioEngine() override
        {
            deviceManager.removeAudioCallback (this);  // clean up
        }

        void audioDeviceAboutToStart (AudioIODevice* device) override
        {
            double sampleRate = device->getCurrentSampleRate();
            int blockSize     = device->getCurrentBufferSizeSamples();
            ProcessSpec spec { sampleRate, (uint32) blockSize, 2 };
            audioFile.prepare (spec);
            oscillator.prepare (spec);
            effect.prepare (spec);
        }
        
       
        void audioDeviceIOCallbackWithContext (const float* const* inputChannelData,
                                              int numInputChannels,
                                              float* const* outputChannelData,
                                              int numOutputChannels,
                                              int numSamples,
                                              const AudioIODeviceCallbackContext&) override
        {

            AudioBuffer<float> buffer (outputChannelData, numOutputChannels, numSamples);

            for (int ch = 0; ch < numOutputChannels; ++ch)
            {
                auto* in  = (ch < numInputChannels) ? inputChannelData[ch] : nullptr;
                auto* out = outputChannelData[ch];

                if (in != nullptr)
                    FloatVectorOperations::copy (out, in, numSamples);
                else
                    FloatVectorOperations::clear (out, numSamples);
            }
            
            AudioBlock<float> block (buffer);
            ProcessContextReplacing<float> context (block);
            // TODO: Add boolean check
//            if (audioFile.enabled())
//                audioFile.process (context);
            if (oscillator.oscEnabled)
                oscillator.process (context);
            effect.process (context);
        }

        void audioDeviceStopped() override
        {
            oscillator.reset();
            effect.reset();
        }

    private:
        AudioDeviceManager& deviceManager;
        AudioPlayerComponent& audioFile;
        OscillatorComponent&  oscillator;
        EffectComponent&      effect;
    };
    //==============================================================================
    Label descriptionLabel { {}, "Audio" };

    AudioPlayerComponent audioFileComponent;
    OscillatorComponent oscillatorComponent;
    EffectComponent effectComponent;

    ContentComponent buttons { "AudioClip",
                               "Choose an audio clip.",
                               audioFileComponent };
    ContentComponent sliders { "Oscillator",
                               "Enable the oscillator.",
                               oscillatorComponent };
    ContentComponent effect { "Effect",
                                "Test bed for your effect.",
                                effectComponent };

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioLabComponent)
};

struct NameAndRole
{
    const char* name;
    AccessibilityRole role;
};

constexpr NameAndRole accessibilityRoles[]
{
    { "Ignored",       AccessibilityRole::ignored },
    { "Unspecified",   AccessibilityRole::unspecified },
    { "Button",        AccessibilityRole::button },
    { "Toggle",        AccessibilityRole::toggleButton },
    { "ComboBox",      AccessibilityRole::comboBox },
    { "Slider",        AccessibilityRole::slider },
    { "Static Text",   AccessibilityRole::staticText },
    { "Editable Text", AccessibilityRole::editableText },
    { "Image",         AccessibilityRole::image },
    { "Group",         AccessibilityRole::group },
    { "Window",        AccessibilityRole::window }
};

//==============================================================================
/**
    The main demo content component.

    This just contains a TabbedComponent with a tab for each of the top-level demos.
*/
class AudioLab final : public Component
{
public:
    AudioLab()
    {
        setTitle ("AudioLab");
        setDescription ("A demo of JUCE's accessibility features.");
        setFocusContainerType (FocusContainerType::focusContainer);

        tabs.setTitle ("Demo tabs");

        const auto tabColour = getLookAndFeel().findColour (ResizableWindow::backgroundColourId).darker (0.1f);

        tabs.addTab ("AudioLab",                    tabColour, &audioLabComponent, false);
        tabs.addTab ("Audio Settings",              tabColour, &audioManagerContentComponent,    false);
        addAndMakeVisible (tabs);

        setSize (800, 600);
    }

    void paint (Graphics& g) override
    {
        g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));
    }

    void resized() override
    {
        tabs.setBounds (getLocalBounds());
    }
    
    

private:
    TooltipWindow tooltipWindow { nullptr, 100 };

    TabbedComponent tabs { TabbedButtonBar::Orientation::TabsAtTop };

    AudioManagerContentComponent audioManagerContentComponent;
    
    AudioLabComponent audioLabComponent { audioManagerContentComponent.deviceManager };
    AudioLabComponent::AudioEngine audioEngine {
                                            audioManagerContentComponent.deviceManager,
                                            audioLabComponent.audioFileComponent,
                                            audioLabComponent.oscillatorComponent,
                                            audioLabComponent.effectComponent };

    

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioLab)
};

