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

#define NUM_HARMONICS 6
#define NUM_ROTARY_KNOBS 6
#define NUM_MUSIC_NOTES 88

enum NOTES{ A, Bb, B, C, Db, D, Eb, E, F, Gb, G, Ab };

//==============================================================================
class EffectComponent final : public Component,
                              private juce::Timer
{
public:
    EffectComponent()
    : forwardFFT (fftOrder),
      window (fftSize, WindowingFunction<float>::hann)
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

        typeBox.addItemList ({ "Frequencies", "Major", "Minor", "Dominant", "Flute", "Violin" }, 1);
        typeBox.setSelectedId (1);
        addAndMakeVisible (typeBox);

        addAndMakeVisible (freqDisplay);

        startTimerHz (30);
    }

    ~EffectComponent() { stopTimer(); }

    //==========================================================================
    struct Frequency {
        float freq      = 0.0f;
        float note      = 0.0f;
        int   semitones = 0;
        float magnitude = 0.0f;
        juce::String note_name;
    };

    //==========================================================================
    // Inline frequency readout grid — paints directly, no child Labels needed
    class FreqDisplay : public juce::Component
    {
    public:
        void setData (const std::vector<Frequency>& data, int maxRows)
        {
            snapshot = data;
            rows     = maxRows;
            repaint();
        }

        void paint (juce::Graphics& g) override
        {
            // OLED palette — pure black pixel-off areas, electric blue emissive text
            // BG:          #000000  true OLED black
            // Border:      #003060  very dim blue structural line
            // Blue-bright: #00B4FF  primary neon blue (headers, freq values)
            // Blue-mid:    #0077CC  secondary / note names
            // Blue-dim:    #003A66  row tint, bar track
            // Blue-glow:   #00B4FF @ low alpha — simulated phosphor bloom
            // Cyan-accent: #00FFFF  magnitude bar fill
            // Dim-text:    #00334D  row numbers / inactive

            auto bounds = getLocalBounds().toFloat();
            const float w = bounds.getWidth();

            // True OLED black background
            g.setColour (juce::Colours::darkgrey);
            g.fillRect  (bounds);


            // Subtle inner glow line along top edge (phosphor bleed simulation)
            g.setColour (juce::Colour (0x2200B4FF));
            g.fillRect  (1.0f, 1.0f, w - 2.0f, 1.5f);

            const float rowH   = 18.0f;
            const float startY = 4.0f;
            const juce::Font monoFont (juce::FontOptions (
                juce::Font::getDefaultMonospacedFontName(), 10.0f, juce::Font::plain));
            g.setFont (monoFont);

            const float colX[] = { 4.0f,     w*0.10f, w*0.25f,  w*0.42f,  w*0.68f };
            const float colW[] = { w*0.08f,  w*0.30f, w*0.30f, w*0.24f,  w*0.30f };

            // --- Headers ---
            // Glow pass (wide, transparent — bloom effect)
            g.setColour (juce::Colour (0x33FFE000));
            g.drawFittedText ("SEMITONES",    (int)colX[0]-1, (int)startY, (int)colW[0]+2, (int)rowH, juce::Justification::centredLeft, 1);
            g.drawFittedText ("FREQ", (int)colX[1]-1, (int)startY, (int)colW[1]+2, (int)rowH, juce::Justification::centredLeft, 1);
            g.drawFittedText ("NOTE_FREQ", (int)colX[2]-1, (int)startY, (int)colW[1]+2, (int)rowH, juce::Justification::centredLeft, 1);
            g.drawFittedText ("NOTE", (int)colX[3]-1, (int)startY, (int)colW[2]+2, (int)rowH, juce::Justification::centredLeft, 1);
            g.drawFittedText ("MAG",  (int)colX[4]-1, (int)startY, (int)colW[3]+2, (int)rowH, juce::Justification::centredLeft, 1);
            // Crisp pass
            g.setColour (juce::Colour (0xFFFFE000));
            g.drawFittedText ("SEMITONES",    (int)colX[0], (int)startY, (int)colW[0], (int)rowH, juce::Justification::centredLeft, 1);
            g.drawFittedText ("FREQ", (int)colX[1], (int)startY, (int)colW[1], (int)rowH, juce::Justification::centredLeft, 1);
            g.drawFittedText ("NOTE_FREQ", (int)colX[2], (int)startY, (int)colW[1], (int)rowH, juce::Justification::centredLeft, 1);
            g.drawFittedText ("NOTE", (int)colX[3], (int)startY, (int)colW[2], (int)rowH, juce::Justification::centredLeft, 1);
            g.drawFittedText ("MAG",  (int)colX[4], (int)startY, (int)colW[3], (int)rowH, juce::Justification::centredLeft, 1);

            // Header underline — glowing blue line
            g.setColour (juce::Colour (0x44FFE000));
            g.fillRect (4.0f, startY + rowH,     w - 8.0f, 2.0f);
            g.setColour (juce::Colour (0xFFFFE000));
            g.fillRect (4.0f, startY + rowH,     w - 8.0f, 1.0f);

            // --- Data rows ---
            const int display = juce::jmin (rows, (int) snapshot.size());
            for (int i = 0; i < display; ++i)
            {
                const float ry = startY + rowH * (i + 1) + 2.0f;
                const auto& f  = snapshot[i];



                // Row index — dim, like inactive pixels
                g.setColour (juce::Colour (0xFFFFE000));
                g.drawFittedText (juce::String (f.semitones),
                                  (int)colX[0], (int)ry, (int)colW[0], (int)rowH,
                                  juce::Justification::centredLeft, 1);

                // Frequency — bright neon blue, double-drawn for glow
                g.setColour (juce::Colour (0x44FFE000));
                g.drawFittedText (juce::String (f.freq, 1) + " Hz",
                                  (int)colX[1]-1, (int)ry, (int)colW[1]+2, (int)rowH,
                                  juce::Justification::centredLeft, 1);
                g.setColour (juce::Colour (0xFFFFE000));
                g.drawFittedText (juce::String (f.freq, 1) + " Hz",
                                  (int)colX[1], (int)ry, (int)colW[1], (int)rowH,
                                  juce::Justification::centredLeft, 1);
                
                // Frequency — bright neon blue, double-drawn for glow
                g.setColour (juce::Colour (0x44FFE000));
                g.drawFittedText (juce::String (f.note, 1) + " Hz",
                                  (int)colX[2]-1, (int)ry, (int)colW[1]+2, (int)rowH,
                                  juce::Justification::centredLeft, 1);
                g.setColour (juce::Colour (0xFFFFE000));
                g.drawFittedText (juce::String (f.note, 1) + " Hz",
                                  (int)colX[2], (int)ry, (int)colW[1], (int)rowH,
                                  juce::Justification::centredLeft, 1);

                // Note name — slightly cooler blue-white
                g.setColour (juce::Colour (0x44FFE000));
                g.drawFittedText (f.note_name,
                                  (int)colX[3]-1, (int)ry, (int)colW[2]+2, (int)rowH,
                                  juce::Justification::centredLeft, 1);
                g.setColour (juce::Colour (0xFFFFE000));
                g.drawFittedText (f.note_name,
                                  (int)colX[3], (int)ry, (int)colW[2], (int)rowH,
                                  juce::Justification::centredLeft, 1);

                // Magnitude bar
                const float maxMag  = 200.0f;
                const float barW    = colW[4] - 36.0f;
                const float barFill = juce::jlimit (0.0f, barW, (f.magnitude / maxMag) * barW);
                const float barY    = ry + 5.0f;
                const float barH    = rowH - 11.0f;

                // Track — near-black with dim blue tint
                g.setColour (juce::Colour (0x44FFE000));
                g.fillRect (colX[4] + 30.0f, barY, barW, barH);

                // Fill — cyan glow bar (glow pass then crisp pass)
                g.setColour (juce::Colour (0x44FFE000));
                g.fillRect (colX[4] + 30.0f, barY - 1.0f, barFill, barH + 2.0f);
                g.setColour (juce::Colour (0xFF00FFFF));
                g.fillRect (colX[4] + 30.0f, barY, barFill, barH);

                // Numeric value — standard blue
                g.setColour (juce::Colour (0xFFFFE000));
                g.drawFittedText (juce::String (f.magnitude, 1),
                                  (int)(colX[4]), (int)ry, 34, (int)rowH,
                                  juce::Justification::centredLeft, 1);

                // Row separator — hairline dim blue
                g.setColour (juce::Colour (0x22004488));
                g.fillRect (4.0f, ry + rowH - 1.0f, w - 8.0f, 1.0f);
            }

            // Empty state — dim blue, like a standby display
            if (display == 0)
            {
                g.setColour (juce::Colour (0xFF00FFFF));
                g.drawFittedText ("-- NO SIGNAL --",
                                  getLocalBounds().reduced (4),
                                  juce::Justification::centred, 1);
            }

        }

    private:
        std::vector<Frequency> snapshot;
        int rows = 0;
    };

    //==========================================================================
    // Called on the message thread 30x/sec — safe to read UI state
    void timerCallback() override
    {
        updateParameters();

        std::vector<Frequency> snapshot;
        {
            juce::ScopedLock sl (freqBinLock);
            snapshot = freqBin;
        }

        // Push data to the display widget (message thread — safe to call repaint)
        freqDisplay.setData (snapshot, numFrequencies);
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

        grid.items.add (GridItem (typeBox)
            .withMargin ({1})
            .withColumn ({ GridItem::Span (3), {} })
            .withHeight (30)
            .withWidth (200)
            .withJustifySelf (GridItem::JustifySelf::center));

        grid.items.add (GridItem (freqDisplay)
            .withMargin ({4})
            .withColumn ({ GridItem::Span (3), {} }));

        grid.performLayout (getLocalBounds());
    }

    //==========================================================================
    // Called from prepareToPlay — audio thread not yet running
    void prepare (const ProcessSpec& spec)
    {
        sampleRate = (float) spec.sampleRate;

        for (int i = 0; i < NUM_MUSIC_NOTES; i++)
            notes[i] = note_freq_by_index (i);

        audioBufferMemory.allocate (spec.numChannels * spec.maximumBlockSize * sizeof (float), true);
        audioBuffer = AudioBlock<float> (audioBufferMemory, spec.numChannels, spec.maximumBlockSize);

        oscBufferMemory.allocate (spec.numChannels * spec.maximumBlockSize * sizeof (float), true);
        oscBuffer = AudioBlock<float> (oscBufferMemory, spec.numChannels, spec.maximumBlockSize);

        for (auto&& osc : oscillators)
            osc.prepare (spec);

        dryWetMixer.prepare ({ sampleRate,
                               (juce::uint32) spec.maximumBlockSize,
                               (juce::uint32) spec.numChannels });

        oscGain.prepare (spec);

        fifoIndex = 0;
        fifo.fill (0.0f);
        fftData.fill (0.0f);
        goodSamples = 0;
        play = false;
    }

    //==========================================================================
    // Called on the audio thread from processBlock
    void process (const ProcessContextReplacing<float>& context)
    {
        auto& inputBlock  = context.getInputBlock();
        auto& outputBlock = context.getOutputBlock();
        const auto numSamples  = outputBlock.getNumSamples();
        const auto numChannels = outputBlock.getNumChannels();

        // Pass audio through while feeding the FFT FIFO (channel 0 only)
        for (size_t ch = 0; ch < numChannels; ++ch)
        {
            auto* in  = inputBlock .getChannelPointer (ch);
            auto* out = outputBlock.getChannelPointer (ch);
            for (size_t i = 0; i < numSamples; ++i)
            {
                if (ch == 0) pushNextSampleIntoFifo (in[i]);
                out[i] = in[i];
            }
        }

        // Synthesise when we have enough stable pitch detections
        if (play)
        {
            dryWetMixer.pushDrySamples (inputBlock);
            audioBuffer.clear();

            int actualFreqs = (harmonics == 0)
                ? juce::jmin (numFrequencies, (int) freqBin.size())
                : numFrequencies;

            for (int i = 0; i < actualFreqs; i++)
            {
                oscBuffer.clear();
                ProcessContextReplacing<float> oscCtx (oscBuffer);
                oscillators[i].process (oscCtx);
                oscGain.setGainLinear (harmonic_table_gains[harmonics][i]);
                oscGain.process (oscCtx);
                audioBuffer.add (oscBuffer);
            }

            dryWetMixer.mixWetSamples (audioBuffer);
            outputBlock.copyFrom (audioBuffer);
        }
    }

    void reset() { goodSamples = 0; play = false; }

    //==========================================================================
    // Public DSP state (read by processor)
    float threshold    = 15.0f;
    int   numFrequencies = 1;
    int   minSamples   = 1;
    int   goodSamples  = 0;
    float freqMultiplier = 1.0f;
    float mix          = 0.5f;
    int   harmonics    = 0;
    float notes[NUM_MUSIC_NOTES];
    bool  play         = false;

    DryWetMixer<float> dryWetMixer;
    Gain<float>        oscGain;

    static constexpr auto fftOrder = 12;
    static constexpr auto fftSize  = 1 << fftOrder;

    HeapBlock<char>   audioBufferMemory, oscBufferMemory;
    AudioBlock<float> audioBuffer,       oscBuffer;

    FreqDisplay freqDisplay;

    Oscillator<float> oscillators[15] = {
        {[] (float x) { return std::sin (x); }},
        {[] (float x) { return std::sin (x); }},
        {[] (float x) { return std::sin (x); }},
        {[] (float x) { return std::sin (x); }},
        {[] (float x) { return std::sin (x); }},
        {[] (float x) { return std::sin (x); }},
        {[] (float x) { return std::sin (x); }},
        {[] (float x) { return std::sin (x); }},
        {[] (float x) { return std::sin (x); }},
        {[] (float x) { return std::sin (x); }},
        {[] (float x) { return std::sin (x); }},
        {[] (float x) { return std::sin (x); }},
        {[] (float x) { return std::sin (x); }},
        {[] (float x) { return std::sin (x); }},
        {[] (float x) { return std::sin (x); }},
    };

private:
    //==========================================================================
    // Run inside pushNextSampleIntoFifo — called on the audio thread.
    // When the FIFO is full, run the FFT immediately and update pitch state.
    void runFFT()
    {
        // Copy FIFO → fftData and zero the second half
        std::fill  (fftData.begin(), fftData.end(), 0.0f);
        std::copy  (fifo.begin(),    fifo.end(),    fftData.begin());

        window.multiplyWithWindowingTable (fftData.data(), fftSize);
        forwardFFT.performRealOnlyForwardTransform (fftData.data());

        auto* cdata = reinterpret_cast<std::complex<float>*> (fftData.data());
        std::vector<Frequency> newBin;

        for (int i = 1; i < fftSize / 2 - 1; ++i)
        {
            float mag  = std::abs (cdata[i]);
            if (mag < threshold) continue;

            float magL = std::abs (cdata[i - 1]);
            float magR = std::abs (cdata[i + 1]);

            if (mag > magL && mag > magR)
            {
                Frequency f;
                f.magnitude = mag;
                f.freq      = ((i + 1) * sampleRate) / static_cast<float> (fftSize);
                if (f.freq < 80.0f) continue;
                f.semitones = get_semitones (f.freq);
//                int octave = std::round(f.semitones / 12);
//                int note = (((f.semitones % 12) + 12) % 12);
                int note = std::round(f.semitones );
                f.note      = notes[juce::jlimit (0, NUM_MUSIC_NOTES, f.semitones+1)]; //-1
                f.note_name = get_note_name (f.semitones);
                newBin.push_back (f);
            }
        }

        // Update good-sample counter
        if (!newBin.empty()) { if (goodSamples < minSamples) ++goodSamples; }
        else                 { if (goodSamples > 0)          --goodSamples; }

        if (goodSamples >= minSamples)
        {
            play = true;
            if (!newBin.empty())
            {
                if (harmonics == 0)
                {
                    for (int i = 0; i < juce::jmin (numFrequencies, (int) newBin.size()); i++)
                        oscillators[i].setFrequency (newBin[i].note * freqMultiplier);
                }
                else
                {
                    float base = newBin[0].note;
                    for (int i = 0; i < 10; i++)
                        oscillators[i].setFrequency (harmonic_table[harmonics][i] * base * freqMultiplier);
                }
            }
        }
        else { play = false; }

        // Publish results for the timer/debug read
        {
            juce::ScopedLock sl (freqBinLock);
            freqBin = std::move (newBin);
        }
    }

    void pushNextSampleIntoFifo (float sample) noexcept
    {
        fifo[(size_t) fifoIndex++] = sample;
        if (fifoIndex == fftSize)
        {
            fifoIndex = 0;
            runFFT();   // do analysis immediately on the audio thread
        }
    }

    //==========================================================================
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

        threshold      = v[0];
        minSamples     = (int) v[1];
        numFrequencies = (int) v[2];
        mix            = v[3];
        freqMultiplier = convertFreqMultiplier (v[4]);
        harmonics      = jmin (NUM_HARMONICS, typeBox.getSelectedItemIndex());
        dryWetMixer.setWetMixProportion (mix);
    }

    float convertFreqMultiplier (float value)
    {
        if (value >= 1.0f) return value + 1.0f;
        if (value == 0.0f) return 1.0f;
        if (value <  0.0f) return 1.0f / (std::abs (value) * 2.0f);
        return 1.0f;
    }

    //==========================================================================
    // FFT internals — only touched on the audio thread
    FFT forwardFFT;
    WindowingFunction<float> window;
    std::array<float, fftSize>     fifo    {};
    std::array<float, fftSize * 2> fftData {};
    int   fifoIndex = 0;
    float sampleRate = 44100.0f;

    // freqBin is written on the audio thread and read on the timer thread
    // — protect with a lock
    std::vector<Frequency> freqBin;
    juce::CriticalSection  freqBinLock;

    // UI elements
    std::array<Slider,       NUM_ROTARY_KNOBS> rotarySliders;
    std::array<juce::Label,  NUM_ROTARY_KNOBS> rotarySliderLabels;
    std::array<juce::String, NUM_ROTARY_KNOBS> rotarySliderStrings = {
        "Volume Cutoff", "Min Samples", "Num Frequencies",
        "Dry/Wet", "Freq Multiplier", "CTRL6"
    };
    float values[NUM_ROTARY_KNOBS] {};
    ComboBox typeBox;

    float default_values[NUM_ROTARY_KNOBS][4] = {
        { 0.0,   100.0,  0.001, threshold },
        { 0.0,    10.0,  1.0,   1.0 },
        { 0.0,   100.0,  1.0,   1.0 },
        { 0.0,     1.0,  0.001, 0.0 },
        { -10.0,  10.0,  1.0,   0.0 },
        { 0.0,    10.0,  1.0,   0.0 }
    };

    float harmonic_table[NUM_HARMONICS][10] = {
        { 1.0, 2.0,        3.0,       4.0,       5.0,         6.0,       7.0, 8.0, 9.0, 10.0 },
        { 1.0, 5.0/4.0,   3.0/2.0,  15.0/8.0,  (9.0/8.0)*2, 4.0/3.0,   7.0, 8.0, 9.0, 10.0 },
        { 1.0, 6.0/5.0,   3.0/2.0,   9.0/5.0,  (9.0/8.0)*2, 4.0/3.0,   7.0, 8.0, 9.0, 10.0 },
        { 1.0, 5.0/4.0,   3.0/2.0,   9.0/5.0,  (9.0/8.0)*2, 4.0/3.0,   7.0, 8.0, 9.0, 10.0 },
        { 1.0, 2.0,       3.0/2.0,   4.0/3.0,   5.0/4.0,    6.0/5.0,  7.0/6.0, 8.0/7.0, 1.0, 1.0 },
        { 1.0, 2.0,       3.0/2.0,   4.0/3.0,   5.0/4.0,    6.0/5.0,  7.0/6.0, 8.0/7.0, 1.0, 1.0 },
    };

    float harmonic_table_gains[NUM_HARMONICS][10] = {
        { 1.0, 1.0, 1.0,  1.0,  1.0,  1.0,  1.0,  1.0, 1.0, 1.0 },
        { 1.0, 1.0, 1.0,  1.0,  1.0,  1.0,  1.0,  1.0, 1.0, 1.0 },
        { 1.0, 1.0, 1.0,  1.0,  1.0,  1.0,  1.0,  1.0, 1.0, 1.0 },
        { 1.0, 1.0, 1.0,  1.0,  1.0,  1.0,  1.0,  1.0, 1.0, 1.0 },
        { 1.0, 1.0, 0.1,  0.2,  0.26, 0.01, 0.01, 0.01, 1.0, 1.0 },
        { 1.0, 0.6, 0.6,  0.7,  0.45, 0.2,  0.45, 0.1,  1.0, 1.0 },
    };

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EffectComponent)
};
