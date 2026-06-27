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

//==============================================================================
/**
    EffectsComponent with 6 Slider components so that the user can easily
    sandbox new effects.
*/

#define NUM_HARMONICS 6
#define NUM_ROTARY_KNOBS 6
#define NUM_MUSIC_NOTES 88

enum NOTES{
    A,
    Bb,
    B,
    C,
    Db,
    D,
    Eb,
    E,
    F,
    Gb,
    G,
    Ab,
};

//==============================================================================
class EffectComponent final : public Component,
                                private juce::Timer
{
public:
    EffectComponent()
    :forwardFFT (fftOrder),
     window(fftSize, WindowingFunction<float>::hann)
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
        
                                 // Dropdowns
        typeBox.addItemList ({ "None", "Major", "Minor", "Dominant", "Flute", "Violin"}, 1); // Type
        typeBox.setSelectedId (1);
        addAndMakeVisible (typeBox);

        
        startTimerHz (100); // call timerCallback 30 times per second
        
    }

    ~EffectComponent()
    {
        stopTimer();
        myFFTThread.signalThreadShouldExit();
        myFFTThread.waitForThreadToExit(4000);
    }

    struct Frequency{
        float freq = 0.0;
        float note = 0.0;
        int semitones = 0;
        float magnitude = 0.0;
        float phase = 0.0;
        juce::String note_name;
    };
    
    // Called 30x/sec on the message thread — safe to read Sliders here
    void timerCallback() override
    {
        updateParameters();
        juce::ScopedLock sl(freqBinLock);

        int actualnumFrequencies = juce::jmin(numFrequencies, (int)freqBin.size());
    
        for (uint8_t i = 0; i < actualnumFrequencies; i++)
        {
            
            DBG("[" + juce::String(i) + "] "
                + "Freq: "  + juce::String(freqBin[i].freq, 4) + " Hz  "  // 1 decimal place
                + "Mag: "   + juce::String(freqBin[i].magnitude,  4)
                + " Semitones: "   + juce::String(freqBin[i].semitones)
                + " Note:  " + freqBin[i].note_name);
            
        }
        
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
            Grid::TrackInfo (Grid::Fr (1)),
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

        grid.items.add (GridItem (typeBox).withMargin ({ 1 }));
                
        grid.performLayout (getLocalBounds());
        
    }
    
    class fftThread: public juce::Thread
    {
        public:
            fftThread(EffectComponent& owner): Thread("FFT Thread"), owner(owner){}
            
            void run() override
            {
                while (!threadShouldExit())
                {
                    if (owner.nextFFTBlockReady)
                    {

                        // Window Data
                        owner.window.multiplyWithWindowingTable (owner.fftData.data(), owner.fftSize);
                        // Then FFT
                        owner.forwardFFT.performRealOnlyForwardTransform (owner.fftData.data());
                    

                        auto* cdata = reinterpret_cast<std::complex<float>*>(owner.fftData.data());
                        std::vector<Frequency> newfreqBin;

                        for (int i = 1; i < fftSize / 2; ++i)
                        {

                            float center_magnitude = std::abs(cdata[i]);

                            //float phase = std::arg(cdata[i]);

                            // Threshold to skip noise
                            if (center_magnitude < owner.threshold)
                                continue;
                            
                            float left_magnitude = std::abs(cdata[i-1]);
                            float right_magnitude = std::abs(cdata[i+1]);
 
                            
                            if ((center_magnitude > left_magnitude) && (center_magnitude > right_magnitude))
                            {
                                DBG(" [left]");
                                Frequency leftFreq;
                                owner.getFrequencyFromBin(&leftFreq, left_magnitude, i-1);
                              
                                
                                DBG(" [center] ");
                                Frequency centerFreq;
                                owner.getFrequencyFromBin(&centerFreq, center_magnitude, i);

                                DBG(" [right] ");
                                Frequency rightFreq;
                                owner.getFrequencyFromBin(&rightFreq, right_magnitude, i+1);
                                
                                DBG("\n");
                                
                                
                                if (centerFreq.freq < 80)
                                    continue;
                                newfreqBin.push_back({ centerFreq });
                            }

                                  

                        }


 

                        if (!newfreqBin.empty())
                        {
            
                            if (owner.goodSamples < owner.minSamples)
                                owner.goodSamples++;
                            
                        }
                        else
                        {
                            
                            if (owner.goodSamples >= 1)
                                owner.goodSamples--;
                        }
                        
                        // Check if we should play
                        if ( owner.goodSamples >= owner.minSamples)
                        {
                            owner.play = true;
                            if (!newfreqBin.empty())
                            {
                                
                                if (owner.harmonics == 0)
                                {
                                    for (uint8_t i=0; i < owner.numFrequencies; i++)
                                    {
                                        owner.oscillators[i].setFrequency(newfreqBin[i].note * owner.freqMultiplier) ;
                                    }
                                }
                                else
                                {
                                    float baseFreq = newfreqBin[0].note;
                                    for (uint8_t i=0; i < 10; i++)
                                    {
                                        owner.oscillators[i].setFrequency( owner.harmonic_table[owner.harmonics][i] * baseFreq * owner.freqMultiplier);
                                        //owner.harmonic_gain_table[i] =

                                    }
                                }
    
                            };
                        }
                        else
                        {
                            owner.play = false;
                        }
                        juce::ScopedLock sl(owner.freqBinLock);
                        owner.freqBin = std::move(newfreqBin);
             
                        owner.nextFFTBlockReady = false;
               
                            
                    }
                    //wait(1);
                }
            }
        

        private:
            EffectComponent& owner;

    };
    

    void prepare (const ProcessSpec& spec)
    {
        for (int i = 0; i < NUM_MUSIC_NOTES; i++)
        {

            notes[i] = note_freq_by_index(i);
            //DBG("Index: " + juce::String(i) + "Note: "  + juce::String(notes[i], 4) + " Hz  ");

        }
        audioBufferMemory.allocate (spec.numChannels * spec.maximumBlockSize * sizeof (float), true);
        audioBuffer = AudioBlock<float> (audioBufferMemory, spec.numChannels, spec.maximumBlockSize);
        
        oscBufferMemory.allocate (spec.numChannels * spec.maximumBlockSize * sizeof (float), true);
        oscBuffer = AudioBlock<float> (oscBufferMemory, spec.numChannels, spec.maximumBlockSize);
        
        for (auto&& oscillator : oscillators)
        {
            oscillator.prepare (spec);
        }

        
        sampleRate = (float)spec.sampleRate;
        if (!myFFTThread.isThreadRunning())
            myFFTThread.startThread();


        dryWetMixer.prepare({ sampleRate, (juce::uint32) audioBuffer.getNumSamples(), (juce::uint32) audioBuffer.getNumChannels() });


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
        
        if (play)
        {
            dryWetMixer.pushDrySamples(inputBlock);
            audioBuffer.clear ();
            oscBuffer.clear();

            ProcessContextReplacing<float> wetContext(audioBuffer);
            ProcessContextReplacing<float> oscContext (oscBuffer);
            
            int actualnumFrequencies = 0;
            if (harmonics == 0)
            {
                actualnumFrequencies = juce::jmin(numFrequencies, (int)freqBin.size());
            }
            else
            {
                actualnumFrequencies = numFrequencies;
            }
                
            for (uint8_t i=0; i < actualnumFrequencies; i++)
            {
                oscBuffer.clear();
                oscillators[i].process(oscContext);
                oscGain.setGainLinear(harmonic_table_gains[harmonics][i]);
                oscGain.process(oscContext);
                
                audioBuffer.add(oscBuffer);
            }
            dryWetMixer.mixWetSamples(audioBuffer);
            outputBlock.copyFrom (audioBuffer);
            
        }
   
    }
    

    void reset()
    {
        
    }
    
    float note_freq_by_index(int n)
    {
        return 440 * std::pow(2, (n-49)/12.0f);
    }
    
    int get_semitones(float freq)
    {
        return 12.0f*std::log2(freq/27.5);
    }




    juce::String get_note_name(int semitones)
    {
        int note = semitones % 12;
        int octave = std::round(semitones/12);

        juce::String note_name;

        switch(note)
        {
            case (A):
                note_name = "A";
                break;
            case (Bb):
                note_name = "A#/Bb";
                break;
            case (B):
                note_name = "B";
                break;
            case (C):
                note_name = "C";
                break;
            case (Db):
                note_name = "C#/Db";
                break;
            case (D):
                note_name = "D";
                break;
            case (Eb):
                note_name = "D#/Eb";
                break;
            case (E):
                note_name = "E";
                break;
            case (F):
                note_name = "F";
                break;
            case (Gb):
                note_name = "F#/Gb";
                break;
            case (G):
                note_name = "G";
                break;
            case (Ab):
                note_name = "G#/Ab";
                break;
            default:
                note_name = "";
                break;
    }

        return note_name + juce::String(octave);
    }
    
    void getFrequencyFromBin(Frequency *newFreq, float magnitude, int binIndex)
    {
        // Get SampleRate
        newFreq->magnitude = magnitude;
        newFreq->freq = (((binIndex+1) * sampleRate) / static_cast<float>(fftSize));
        newFreq->semitones = get_semitones(newFreq->freq);
        newFreq->note = notes[newFreq->semitones];
        newFreq->note_name = get_note_name(newFreq->semitones);
        
        DBG("[" + juce::String(binIndex) + "] "
        + "Freq: "  + juce::String(newFreq->freq, 4) + " Hz  "  // 1 decimal place
        + "Mag: "   + juce::String(newFreq->magnitude,  4)
        + " Semitones: "   + juce::String(newFreq->semitones)
        + " Note:  " + newFreq->note_name);
        
    }




    void pushNextSampleIntoFifo(float sample) noexcept
    {
        // if fifo contains enough data set a flag
        if (fifoIndex == fftSize)
        {

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
    
    float convertFreqMultiplier(float value)
    {
        if (value >= 1.0)
        {
            return value +1.0;
        }
        if (value == 0)
        {
            return 1.0;
        }
        if (value < 0)
        {
            return  (1/(abs(value) *2));
        }
        return 1.0;
    }

    void updateParameters()
    {
        // read from param objects and apply to DSP
        for (uint8_t i=0; i < NUM_ROTARY_KNOBS; i++)
        {
            values[i] = static_cast<float> (rotarySliders[i].getValue());
        }
        
        threshold = values[0];
        minSamples = int(values[1]);
        numFrequencies = int(values[2]);
        mix = values[3];
        freqMultiplier = convertFreqMultiplier(values[4]);

        harmonics = jmin(NUM_HARMONICS, typeBox.getSelectedItemIndex());
        dryWetMixer.setWetMixProportion(mix);
    }
    
    
    //TODO: Add other synths
    //==============================================================================
    Oscillator<float> oscillators[15] =
    {
        // No Approximation
        {[] (float x) { return std::sin (x); }},                   // sine
        {[] (float x) { return std::sin (x); }},                   // sine
        {[] (float x) { return std::sin (x); }},                   // sine
        {[] (float x) { return std::sin (x); }},                   // sine
        {[] (float x) { return std::sin (x); }},                   // sine
        {[] (float x) { return std::sin (x); }},                   // sine
        {[] (float x) { return std::sin (x); }},                   // sine
        {[] (float x) { return std::sin (x); }},                   // sine
        {[] (float x) { return std::sin (x); }},                   // sine
        {[] (float x) { return std::sin (x); }},                   // sine
        {[] (float x) { return std::sin (x); }},                   // sine
        {[] (float x) { return std::sin (x); }},                   // sine
        {[] (float x) { return std::sin (x); }},                   // sine
        {[] (float x) { return std::sin (x); }},                   // sine
        {[] (float x) { return std::sin (x); }},                   // sine
    };
    


    
public:
    
    // Effect
    float threshold = 15.0;
    int numFrequencies = 1;
    int minSamples = 1;
    int goodSamples = 0;
    float freqMultiplier = 1.0;
    float mix = 0.5;
    int harmonics = 0;
    float notes[NUM_MUSIC_NOTES];
    bool play = false;
    DryWetMixer<float> dryWetMixer;
    Gain<float> oscGain;
    
    // FFT Variables
    static constexpr auto fftOrder = 12;
    static constexpr auto fftSize = 1 << fftOrder;
    static constexpr auto hopSize = 2048;
    FFT forwardFFT;
    std::array<float, fftSize> fifo;
    std::array<float, fftSize * 2> fftData;
    int fifoIndex = 0;
    float sampleRate = 44100;
    std::atomic<bool> nextFFTBlockReady = {false};
    std::vector<Frequency> freqBin;
    //std::vector<std::pair<float, float>> freqBin;
    WindowingFunction<float> window;
    juce::CriticalSection freqBinLock;
    fftThread myFFTThread { *this };

        
    // Audio Buffers
    HeapBlock<char> audioBufferMemory;
    AudioBlock<float> audioBuffer;
    HeapBlock<char> oscBufferMemory;
    AudioBlock<float> oscBuffer;
    

private:
    // UI Elements
    std::array<Slider, NUM_ROTARY_KNOBS> rotarySliders;
    std::array<juce::Label, NUM_ROTARY_KNOBS> rotarySliderLabels;
    std::array<juce::String, NUM_ROTARY_KNOBS> rotarySliderStrings = {"Volume Cutoff","Min Samples","Num Frequencies",
        "Dry/Wet", "Freq Multiplier","CTRL6"};
    float values[NUM_ROTARY_KNOBS];
    ComboBox typeBox;
    float default_values[NUM_ROTARY_KNOBS][4] = {
        {0.0, 100.0, 0.001, threshold},
        {0.0, 10.0, 1.0, 1.0},
        {0.0, 100.0, 1.0, 1.0},
        {0.0, 1.0, 0.001, 0.0},
        {-10.0, 10.0, 1.0, 0.0},
        {0.0, 10.0, 1.0, 0.0}
    };
    

    float harmonic_table[NUM_HARMONICS][10] = {
        
        {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10},
        {1.0, 5.0/4.0, 3.0/2.0, 15.0/8.0, (9.0/8.0)*2.0, (4.0/3.0), 7.0, 8.0, 9.0, 10},
        //1     3        5       b7        9
        {1.0, 6.0/5.0, 3.0/2.0, 9.0/5.0, (9.0/8.0)*2.0, (4.0/3.0), 7.0, 8.0, 9.0, 10},
        {1.0, 5.0/4.0, 3.0/2.0, 9.0/5.0, (9.0/8.0)*2.0, (4.0/3.0), 7.0, 8.0, 9.0, 10},
        // FLUTE
        {1.0, 2.0, (3.0/2.0), (4.0/3.0), (5.0/4.0), (6.0/5.0), (7.0/6.0), 8.0/7.0, 1.0, 1.0},
        // Violin
        {1.0, 2.0, (3.0/2.0), (4.0/3.0), (5.0/4.0), (6.0/5.0), (7.0/6.0), 8.0/7.0, 1.0, 1.0},
    };
    
    float harmonic_table_gains[NUM_HARMONICS][10] = {
        
        {1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0},
        {1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0},
        {1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0},
        {1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0},
        // FLUTE
        {1.0, 1.0, 0.1, 0.2, 0.26, 0.01, 0.01, 0.01, 1.0, 1.0},
        // VIOLIN
        {1.0, 0.6, 0.6, 0.7, 0.45, 0.2, 0.45, 0.1, 1.0, 1.0},

    };
    
    
    
    

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EffectComponent)
};
