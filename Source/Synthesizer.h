//
//  Synthesizer.h
//  AudioLab
//
//  ADSR synth voice/sound + a processor-owned GUI component that follows
//  the same prepare()/process()/reset() pattern as OscillatorComponent and
//  EffectComponent, so it slots into the existing DSP chain.
//

#pragma once

#include <JuceHeader.h>
#include <juce_dsp/juce_dsp.h>

using namespace juce;
using namespace juce::dsp;

//==============================================================================
enum class WaveformType
{
    sine,
    saw,
    square,
    triangle
};

//==============================================================================
struct SynthSound : public juce::SynthesiserSound
{
    bool appliesToNote (int) override      { return true; }
    bool appliesToChannel (int) override    { return true; }
};

volatile static bool holdEnabled = false;

//==============================================================================
class SynthVoice : public juce::SynthesiserVoice
{
public:
    SynthVoice (std::atomic<float>* waveformTypeParam,
                std::atomic<float>* attackParam,
                std::atomic<float>* decayParam,
                std::atomic<float>* sustainParam,
                std::atomic<float>* releaseParam)
        : waveformType (waveformTypeParam),
          attack (attackParam), decay (decayParam),
          sustain (sustainParam), release (releaseParam)
    {
    }

    bool canPlaySound (juce::SynthesiserSound* sound) override
    {
        return dynamic_cast<SynthSound*> (sound) != nullptr;
    }

    void startNote (int midiNoteNumber, float velocity,
                     juce::SynthesiserSound*, int) override
    {
        currentAngle = 0.0;
        level = velocity * 0.15;

        auto cyclesPerSecond = juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber);
        auto cyclesPerSample = cyclesPerSecond / getSampleRate();
        angleDelta = cyclesPerSample * 2.0 * juce::MathConstants<double>::pi;

        updateADSRParams();
        adsr.noteOn();
    }

    void stopNote (float /*velocity*/, bool allowTailOff) override
    {
        if (!holdEnabled)
        {
            if (allowTailOff)
            {
                adsr.noteOff();
            }
            else
            {
                clearCurrentNote();
                angleDelta = 0.0;
                adsr.reset();
            }
        }

    }

    void pitchWheelMoved (int) override {}
    void controllerMoved (int, int) override {}

    void prepareToPlay (double sampleRate)
    {
        adsr.setSampleRate (sampleRate);
    }

    void renderNextBlock (juce::AudioSampleBuffer& outputBuffer,
                           int startSample, int numSamples) override
    {
        if (angleDelta == 0.0)
            return;

        updateADSRParams(); // pick up live slider changes each block

        auto type = static_cast<WaveformType> ((int) waveformType->load());

        while (--numSamples >= 0)
        {
            auto currentSample = (float) (getWaveformSample (type) * level);
            currentSample *= adsr.getNextSample();

            for (auto i = outputBuffer.getNumChannels(); --i >= 0;)
                outputBuffer.addSample (i, startSample, currentSample);

            currentAngle += angleDelta;
            if (currentAngle > juce::MathConstants<double>::twoPi)
                currentAngle -= juce::MathConstants<double>::twoPi;

            ++startSample;

            if (! adsr.isActive())
            {
                clearCurrentNote();
                angleDelta = 0.0;
                break;
            }
        }
    }

private:
    void updateADSRParams()
    {
        adsr.setParameters ({ attack->load(), decay->load(),
                               sustain->load(), release->load() });
    }

    double getWaveformSample (WaveformType type)
    {
        // currentAngle is 0..2pi; normalize to 0..1 phase for non-sine shapes
        auto phase = currentAngle / juce::MathConstants<double>::twoPi;

        switch (type)
        {
            case WaveformType::sine:
                return std::sin (currentAngle);

            case WaveformType::saw:
                return 2.0 * phase - 1.0; // ramps -1 to 1

            case WaveformType::square:
                return phase < 0.5 ? 1.0 : -1.0;

            case WaveformType::triangle:
                return 4.0 * std::abs (phase - 0.5) - 1.0;

            default:
                return 0.0;
        }
    }

    double currentAngle = 0.0, angleDelta = 0.0, level = 0.0;
    juce::ADSR adsr;

    // Shared pointers to UI-controlled values (all voices read the same settings)
    std::atomic<float>* waveformType;
    std::atomic<float>* attack;
    std::atomic<float>* decay;
    std::atomic<float>* sustain;
    std::atomic<float>* release;
};

//==============================================================================
// Processor-owned DSP + GUI component. Matches the pattern used by
// OscillatorComponent / EffectComponent: a juce::Component with
// prepare()/process()/reset() called from the processor's audio callbacks,
// and a Timer that reads its own sliders on the message thread.
class SynthesizerComponent final : public juce::Component,
                                    private juce::Timer
{
public:
    SynthesizerComponent()
    {
        waveformBox.addItemList ({ "Sine", "Saw", "Square", "Triangle" }, 1);
        waveformBox.setSelectedId (1);
        addAndMakeVisible (waveformBox);

        enableToggle.setToggleState (true, dontSendNotification);
        enableToggle.setClickingTogglesState (true);
        addAndMakeVisible (enableToggle);

        holdToggle.setToggleState (false, dontSendNotification);
        holdToggle.setClickingTogglesState (true);
        addAndMakeVisible (holdToggle);

        setupSlider (attackSlider,  attackLabel,  "Attack",  0.001, 2.0, 0.1);
        setupSlider (decaySlider,   decayLabel,   "Decay",   0.001, 2.0, 0.1);
        setupSlider (sustainSlider, sustainLabel, "Sustain", 0.0,   1.0, 0.8);
        setupSlider (releaseSlider, releaseLabel, "Release", 0.001, 3.0, 0.3);

        // On-screen keyboard for manual testing. Its notes are merged into the
        // same MidiBuffer as programmatic/host-triggered notes each block.
        keyboardComponent.setKeyWidth (24.0f); // was default (~16-18px) — bigger, easier to click
        keyboardComponent.setAvailableRange (36, 96); // C2–C7; keeps keys from getting squeezed too thin
        addAndMakeVisible (keyboardComponent);

        for (auto i = 0; i < 8; ++i)
            synth.addVoice (new SynthVoice (&waveformParam, &attackParam,
                                            &decayParam, &sustainParam, &releaseParam));
        synth.addSound (new SynthSound());

        startTimerHz (30);
    }
    ~SynthesizerComponent() override { stopTimer(); }

    // Called 30x/sec on the message thread — safe to read sliders here
    void timerCallback() override { updateParameters(); }

    void paint (juce::Graphics&) override {}

    void resized() override
    {
        auto area = getLocalBounds().reduced (4);

        auto topRow = area.removeFromTop (24);
        enableToggle.setBounds (topRow.removeFromLeft (80));
        holdToggle.setBounds (topRow.removeFromLeft (120));
        waveformBox.setBounds  (topRow.removeFromLeft (160).translated (8, 0));

        auto keyboardArea = area.removeFromBottom (90); // was 60 — taller keys
        keyboardComponent.setBounds (keyboardArea);

        area.removeFromTop (20); // room for the slider labels attached above them

        auto w = area.getWidth() / 4;
        attackSlider.setBounds  (area.removeFromLeft (w).reduced (4));
        decaySlider.setBounds   (area.removeFromLeft (w).reduced (4));
        sustainSlider.setBounds (area.removeFromLeft (w).reduced (4));
        releaseSlider.setBounds (area.removeFromLeft (w).reduced (4));
    }

    //==========================================================================
    // Audio DSP interface — mirrors OscillatorComponent / EffectComponent
    void prepare (const ProcessSpec& spec)
    {
        synth.setCurrentPlaybackSampleRate (spec.sampleRate);
        midiCollector.reset (spec.sampleRate);
        scratchBuffer.setSize ((int) spec.numChannels, (int) spec.maximumBlockSize);

        for (auto i = 0; i < synth.getNumVoices(); ++i)
            if (auto* v = dynamic_cast<SynthVoice*> (synth.getVoice (i)))
                v->prepareToPlay (spec.sampleRate);

        updateParameters();
    }

    void process (const ProcessContextReplacing<float>& context)
    {
        if (! synthEnabled)
            return;

        // Defensive: if prepare() was never called (or numChannels mismatch),
        // scratchBuffer will have 0 channels — bail out rather than deref a
        // bad channel pointer.
        if (scratchBuffer.getNumChannels() == 0)
            return;

        auto& outputBlock = context.getOutputBlock();
        auto  numSamples  = juce::jmin ((int) outputBlock.getNumSamples(), scratchBuffer.getNumSamples());
        auto  numChannels = (int) outputBlock.getNumChannels();

        // Gather MIDI from: programmatic/host triggers (queued via the
        // collector) and the on-screen keyboard, into one buffer for this block.
        juce::MidiBuffer midi;
        midiCollector.removeNextBlockOfMessages (midi, numSamples);
        keyboardState.processNextMidiBuffer (midi, 0, numSamples, true);

        // Render into a scratch buffer and ADD into the existing block — this
        // stage sits after the oscillator/effect and must not erase their output.
        scratchBuffer.clear();
        synth.renderNextBlock (scratchBuffer, midi, 0, numSamples);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* dest = outputBlock.getChannelPointer ((size_t) ch);
            auto* src  = scratchBuffer.getReadPointer (juce::jmin (ch, scratchBuffer.getNumChannels() - 1));

            for (int i = 0; i < numSamples; ++i)
                dest[i] += src[i];
        }
    }

    void reset()
    {
        for (auto i = 0; i < synth.getNumVoices(); ++i)
            synth.getVoice (i)->stopNote (0.0f, false);
    }
    void stop_all_midi()
    {
        for (auto i = 0; i < 128; ++i)
            triggerNoteOff (i);
    }

    void updateParameters()
    {
        waveformParam = (float) (waveformBox.getSelectedId() - 1);
        attackParam   = (float) attackSlider.getValue();
        decayParam    = (float) decaySlider.getValue();
        sustainParam  = (float) sustainSlider.getValue();
        releaseParam  = (float) releaseSlider.getValue();
        synthEnabled  = enableToggle.getToggleState();
        bool newholdEnabled = holdToggle.getToggleState();
        
        if ((holdEnabled == true) and (newholdEnabled == false))
        {
            stop_all_midi();
        }
        holdEnabled = newholdEnabled;

    }

    //==========================================================================
    // Public trigger API. Thread-safe — MidiMessageCollector::addMessageToQueue
    // is safe to call from the message thread, a Timer, or the audio thread
    // (e.g. from inside EffectComponent::process when it detects a pitch).
    void triggerNoteOn (int midiNoteNumber, float velocity)
    {
        auto msg = juce::MidiMessage::noteOn (1, midiNoteNumber, velocity);
        msg.setTimeStamp (juce::Time::getMillisecondCounterHiRes() * 0.001);
        midiCollector.addMessageToQueue (msg);
    }

    void triggerNoteOff (int midiNoteNumber)
    {
        auto msg = juce::MidiMessage::noteOff (1, midiNoteNumber);
        msg.setTimeStamp (juce::Time::getMillisecondCounterHiRes() * 0.001);
        midiCollector.addMessageToQueue (msg);
    }

    // Lets the processor forward host-supplied MIDI (DAW piano roll, a real
    // keyboard controller, etc.) into the same queue as programmatic triggers.
    void pushIncomingMidi (const juce::MidiBuffer& midi)
    {
        for (const auto metadata : midi)
            midiCollector.addMessageToQueue (metadata.getMessage());
    }

    bool synthEnabled = true;

private:
    void setupSlider (juce::Slider& slider, juce::Label& label,
                       const juce::String& name, double min, double max, double defaultVal)
    {
        slider.setSliderStyle (juce::Slider::LinearVertical);
        slider.setRange (min, max, 0.001);
        slider.setValue (defaultVal);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
        addAndMakeVisible (slider);

        label.setText (name, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        label.attachToComponent (&slider, false);
        addAndMakeVisible (label);
    }

    juce::Synthesiser synth;
    juce::MidiKeyboardState keyboardState;
    juce::MidiKeyboardComponent keyboardComponent { keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard };
    juce::MidiMessageCollector midiCollector;
    juce::AudioBuffer<float> scratchBuffer;

    juce::ComboBox waveformBox;
    juce::ToggleButton enableToggle { "Enable" },  holdToggle { "Hold" };
    juce::Slider attackSlider, decaySlider, sustainSlider, releaseSlider;
    juce::Label  attackLabel, decayLabel, sustainLabel, releaseLabel;

    // Shared, thread-safe params read by all voices in the audio thread
    std::atomic<float> waveformParam { 0.0f };
    std::atomic<float> attackParam   { 0.1f };
    std::atomic<float> decayParam    { 0.1f };
    std::atomic<float> sustainParam  { 0.8f };
    std::atomic<float> releaseParam  { 0.3f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SynthesizerComponent)
};
