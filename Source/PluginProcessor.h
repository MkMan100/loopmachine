#pragma once

#include <JuceHeader.h>

class AdvancedLooperAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    AdvancedLooperAudioProcessor();
    ~AdvancedLooperAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Accesso alla APVTS per la GUI
    juce::AudioProcessorValueTreeState apvts;

private:
    //==============================================================================
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Loop Buffers
    juce::AudioBuffer<float> loopAudioBuffer;
    juce::MidiBuffer midiLoopBuffer;

    // Indici delle testine e metrica del loop
    int writePosition { 0 };
    int readPosition { 0 };
    int recordedLoopLength { 0 };
    int maxLoopSamples { 0 };
    double currentSampleRate { 44100.0 };

    // DSP Modules
    juce::dsp::StateVariableTPTFilter<float> cutoffFilter;
    juce::dsp::DelayLine<float> dubDelay { 192000 }; // Max ~2 sec a 96kHz
    
    // Buffer circolare per feedback del Dub Delay
    juce::AudioBuffer<float> delayFeedbackBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AdvancedLooperAudioProcessor)
};
