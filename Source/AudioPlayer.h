//
//  AudioPlayer.h
//  AudioLab
//
//  Created by Ernest Forzano on 3/18/26.
//  Copyright © 2026 JUCE. All rights reserved.
//

#pragma once

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_utils/juce_audio_utils.h>
//#include "AudioThumbnailComponent.h"
#include "DSPDemos_Common.h"
using namespace dsp;


//==============================================================================
class AudioPlayerComponent final : public Component,
                                    private TimeSliceThread,
                                    private Value::Listener,
                                    private ChangeListener
{
public:
    
    //==============================================================================
    AudioPlayerComponent(AudioDeviceManager& adm)
        : TimeSliceThread("Audio File Reader Thread"),
            audioDeviceManager(adm),
            header(adm, formatManager, *this)
    {
        loopState.addListener (this);

        formatManager.registerBasicFormats();
        

        init();
        startThread();

        setOpaque (true);

        addAndMakeVisible (header);

        setSize (800, 250);
    }

    ~AudioPlayerComponent() override
    {
        signalThreadShouldExit();
        stop();
        //audioDeviceManager.removeAudioCallback (&audioSourcePlayer);
        waitForThreadToExit (10000);
    }
    
    void prepare (const ProcessSpec& spec)
    {
        audioBufferMemory.allocate (spec.numChannels * spec.maximumBlockSize * sizeof (float), true);
        audioBuffer = AudioBlock<float> (audioBufferMemory, spec.numChannels, spec.maximumBlockSize);
        
        currentSampleRate = spec.sampleRate;
        currentBlockSize  = spec.maximumBlockSize;

        transportSource = std::make_unique<AudioTransportSource>();
        transportSource->addChangeListener (this);
        transportSource->prepareToPlay ((int) spec.maximumBlockSize, spec.sampleRate);

        // Don't call setSource here — no file loaded yet
        getThumbnailComponent().setTransportSource (transportSource.get());
    }

    void process (const ProcessContextReplacing<float>& context)
    {
        if (transportSource == nullptr || !transportSource->isPlaying())
            return;

        // Get the underlying AudioBuffer from the AudioBlock
        auto& outputBlock = context.getOutputBlock();
        const int numSamples = (int) outputBlock.getNumSamples();
        const int numChannels = (int) outputBlock.getNumChannels();

        // Create a temporary AudioBuffer that points to the output block's memory
        AudioBuffer<float> buffer (numChannels, numSamples);

        // Fill it from the transport source
        AudioSourceChannelInfo channelInfo (&buffer, 0, numSamples);
        transportSource->getNextAudioBlock (channelInfo);

        // Copy the result into the output block
        for (int ch = 0; ch < numChannels; ++ch)
            outputBlock.getSingleChannelBlock (ch).copyFrom (AudioBlock<float> (buffer).getSingleChannelBlock (ch));
    }

    void reset() {
        
    }

    void updateParameters()
    {
        // read from param objects and apply to DSP
    }

    void paint (Graphics& g) override
    {
        g.setColour (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));
        g.fillRect (getLocalBounds());
    }

    void resized() override
    {
        auto r = getLocalBounds();

        header.setBounds (r.removeFromTop (120));

        r.removeFromTop (20);

        if (parametersComponent != nullptr)
            parametersComponent->setBounds (r.removeFromTop (parametersComponent->getHeightNeeded()).reduced (20, 0));
    }

    //==============================================================================
    bool loadURL (const URL& fileToPlay)
    {
        stop();

        //audioSourcePlayer.setSource (nullptr);
        getThumbnailComponent().setTransportSource (nullptr);
        transportSource.reset();
        readerSource.reset();

        auto source = makeInputSource (fileToPlay);

        if (source == nullptr)
            return false;

        auto stream = rawToUniquePtr (source->createInputStream());

        if (stream == nullptr)
            return false;

        reader = rawToUniquePtr (formatManager.createReaderFor (std::move (stream)));

        if (reader == nullptr)
            return false;

        readerSource.reset (new AudioFormatReaderSource (reader.get(), false));
        readerSource->setLooping (loopState.getValue());


        transportSource = std::make_unique<AudioTransportSource>();
        transportSource->addChangeListener (this);
        transportSource->prepareToPlay ((int) currentBlockSize, currentSampleRate);

        transportSource->setSource (readerSource.get(),
                                    roundToInt (currentSampleRate),
                                    this,
                                    reader->sampleRate);
        
        getThumbnailComponent().setTransportSource (transportSource.get());
        
    
    
        resized();

        return true;
    }

    void togglePlay()
    {
        if (playState.getValue())
            stop();
        else
            play();
    }

    void stop()
    {
        playState = false;

        if (transportSource.get() != nullptr)
        {
            transportSource->stop();
            transportSource->setPosition (0);
        }
    }

    void init()
    {
        if (transportSource.get() == nullptr)
        {
            transportSource.reset (new AudioTransportSource());
            transportSource->addChangeListener (this);

            if (readerSource != nullptr)
            {
                if (auto* device = audioDeviceManager.getCurrentAudioDevice())
                {
                    transportSource->setSource (readerSource.get(), roundToInt (device->getCurrentSampleRate()), this, reader->sampleRate);

                    getThumbnailComponent().setTransportSource (transportSource.get());
                }
            }
        }

        parametersComponent.reset();
    }

    void play()
    {
        if (readerSource == nullptr)
            return;

        if (transportSource->getCurrentPosition() >= transportSource->getLengthInSeconds()
            || transportSource->getCurrentPosition() < 0)
            transportSource->setPosition (0);

        transportSource->start();
        playState = true;
    }

    void setLooping (bool shouldLoop)
    {
        if (readerSource != nullptr)
            readerSource->setLooping (shouldLoop);
    }

    AudioThumbnailComponent& getThumbnailComponent()    { return header.thumbnailComp; }

private:
    //==============================================================================
    class AudioPlayerHeader final : public Component,
                                    private ChangeListener,
                                    private Value::Listener
    {
    public:
        AudioPlayerHeader (AudioDeviceManager& adm,
                        AudioFormatManager& afm,
                        AudioPlayerComponent& afr)
            : thumbnailComp (adm, afm),
            audioFileReader (afr)
        {
            setOpaque (true);

            addAndMakeVisible (loadButton);
            addAndMakeVisible (playButton);
            addAndMakeVisible (loopButton);

            playButton.setColour (TextButton::buttonColourId, Colour (0xff79ed7f));
            playButton.setColour (TextButton::textColourOffId, Colours::black);

            loadButton.setColour (TextButton::buttonColourId, Colour (0xff797fed));
            loadButton.setColour (TextButton::textColourOffId, Colours::black);

            loadButton.onClick = [this] { openFile(); };
            playButton.onClick = [this] { audioFileReader.togglePlay(); };

            addAndMakeVisible (thumbnailComp);
            thumbnailComp.addChangeListener (this);

            audioFileReader.playState.addListener (this);
            loopButton.getToggleStateValue().referTo (audioFileReader.loopState);
        }

        ~AudioPlayerHeader() override
        {
            audioFileReader.playState.removeListener (this);
        }

        void paint (Graphics& g) override
        {
            g.setColour (getLookAndFeel().findColour (ResizableWindow::backgroundColourId).darker());
            g.fillRect (getLocalBounds());
        }

        void resized() override
        {
            auto bounds = getLocalBounds();

            auto buttonBounds = bounds.removeFromLeft (jmin (250, bounds.getWidth() / 4));
            auto loopBounds = buttonBounds.removeFromBottom (30);

            loadButton.setBounds (buttonBounds.removeFromTop (buttonBounds.getHeight() / 2));
            playButton.setBounds (buttonBounds);

            loopButton.setSize (0, 25);
            loopButton.changeWidthToFitText();
            loopButton.setCentrePosition (loopBounds.getCentre());

            thumbnailComp.setBounds (bounds);
        }

        AudioThumbnailComponent thumbnailComp;

    private:
        //==============================================================================
        void openFile()
        {
            audioFileReader.stop();

            if (fileChooser != nullptr)
                return;

            if (! RuntimePermissions::isGranted (RuntimePermissions::readExternalStorage))
            {
                SafePointer<AudioPlayerHeader> safeThis (this);
                RuntimePermissions::request (RuntimePermissions::readExternalStorage,
                                            [safeThis] (bool granted) mutable
                                            {
                                                if (safeThis != nullptr && granted)
                                                    safeThis->openFile();
                                            });
                return;
            }

            fileChooser.reset (new FileChooser ("Select an audio file...", File(), "*.wav;*.mp3;*.aif"));

            fileChooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles,
                                    [this] (const FileChooser& fc) mutable
                                    {
                                        if (fc.getURLResults().size() > 0)
                                        {
                                            const auto u = fc.getURLResult();

                                            if (! audioFileReader.loadURL (u))
                                            {
                                                auto options = MessageBoxOptions().withIconType (MessageBoxIconType::WarningIcon)
                                                                                    .withTitle ("Error loading file")
                                                                                    .withMessage ("Unable to load audio file")
                                                                                    .withButton ("OK");
                                                messageBox = NativeMessageBox::showScopedAsync (options, nullptr);
                                            }
                                            else
                                            {
                                                thumbnailComp.setCurrentURL (u);
                                            }
                                        }

                                        fileChooser = nullptr;
                                    }, nullptr);
        }

        void changeListenerCallback (ChangeBroadcaster*) override
        {
            if (audioFileReader.playState.getValue())
                audioFileReader.stop();

            audioFileReader.loadURL (thumbnailComp.getCurrentURL());
        }

        void valueChanged (Value& v) override
        {
            playButton.setButtonText (v.getValue() ? "Stop" : "Play");
            playButton.setColour (TextButton::buttonColourId, v.getValue() ? Colour (0xffed797f) : Colour (0xff79ed7f));
        }

        //==============================================================================
        TextButton loadButton { "Load File..." }, playButton { "Play" };
        ToggleButton loopButton { "Loop File" };

        AudioPlayerComponent& audioFileReader;
        std::unique_ptr<FileChooser> fileChooser;
        ScopedMessageBox messageBox;
    };

    //==============================================================================
    void valueChanged (Value& v) override
    {
        if (readerSource != nullptr)
            readerSource->setLooping (v.getValue());
    }

    void changeListenerCallback (ChangeBroadcaster*) override
    {
        if (playState.getValue() && ! transportSource->isPlaying())
            stop();
    }

    //==============================================================================

    AudioDeviceManager& audioDeviceManager;
    AudioFormatManager formatManager;
    Value playState { var (false) };
    Value loopState { var (false) };

    double currentSampleRate = 44100.0;
    uint32 currentBlockSize = 512;
    uint32 currentNumChannels = 2;

    std::unique_ptr<AudioFormatReader> reader;
    std::unique_ptr<AudioFormatReaderSource> readerSource;
    std::unique_ptr<AudioTransportSource> transportSource;

    AudioSourcePlayer audioSourcePlayer;
    AudioPlayerHeader header;

    AudioBuffer<float> fileReadBuffer;
    HeapBlock<char> audioBufferMemory;
    AudioBlock<float> audioBuffer;

    std::unique_ptr<DemoParametersComponent> parametersComponent;
};


