#include "PluginProcessor.h"

//==============================================================================
AdvancedLooperAudioProcessor::AdvancedLooperAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
       apvts (*this, nullptr, "Parameters", createParameterLayout())
#endif
{
}

AdvancedLooperAudioProcessor::~AdvancedLooperAudioProcessor()
{
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout AdvancedLooperAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // 1. Stato Looper
    params.push_back (std::make_unique<juce::AudioParameterChoice> ("state", "State", 
        juce::StringArray { "Empty", "Recording", "Playing", "Overdub" }, 0));

    // 2. Step Reduction (es. 16 = normale, 8, 4, 2 step attivi)
    params.push_back (std::make_unique<juce::AudioParameterInt> ("stepReduce", "Step Reduction", 1, 16, 16));

    // 3. Lunghezza Sample in Beat/Bars (es. 1, 2, 4, 8, 16 beat)
    params.push_back (std::make_unique<juce::AudioParameterChoice> ("loopLength", "Loop Length (Beats)", 
        juce::StringArray { "1", "2", "4", "8", "16", "32" }, 2));

    // 4. Reverse Play
    params.push_back (std::make_unique<juce::AudioParameterBool> ("reverse", "Reverse Playback", false));

    // 5. Cutoff Filter (20Hz - 20kHz)
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("cutoff", "Cutoff Frequency", 
        juce::NormalisableRange<float> (20.0f, 20000.0f, 1.0f, 0.3f), 20000.0f));

    // 6. Dub Delay (Feedback & Time)
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("delayTime", "Dub Delay Time (ms)", 
        juce::NormalisableRange<float> (10.0f, 2000.0f, 1.0f, 0.5f), 375.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("delayFeedback", "Dub Delay Feedback", 
        juce::NormalisableRange<float> (0.0f, 0.95f, 0.01f), 0.5f));

    return { params.begin(), params.end() };
}

//==============================================================================
void AdvancedLooperAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Inizializzazione DSP e azzeramento buffer di loop (Audio + MIDI)
    currentSampleRate = sampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = juce::uint32 (samplesPerBlock);
    spec.numChannels = juce::uint32 (getTotalNumOutputChannels());

    // Inizializza filtro Cutoff
    cutoffFilter.prepare (spec);
    cutoffFilter.setType (juce::dsp::StateVariableTPTFilterType::lowpass);

    // Allocazione massima buffer loop (es. 60 secondi a 44.1/48kHz)
    maxLoopSamples = static_cast<int> (sampleRate * 60.0);
    loopAudioBuffer.setSize (getTotalNumOutputChannels(), maxLoopSamples);
    loopAudioBuffer.clear();

    midiLoopBuffer.clear();
    writePosition = 0;
    readPosition = 0;
}

void AdvancedLooperAudioProcessor::releaseResources()
{
}

void AdvancedLooperAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Aggiorna parametri DSP
    float cutoffHz = *apvts.getRawParameterValue ("cutoff");
    cutoffFilter.setCutoffFrequency (cutoffHz);

    bool isReverse = *apvts.getRawParameterValue ("reverse") > 0.5f;
    int currentState = static_cast<int> (*apvts.getRawParameterValue ("state"));

    int numSamples = buffer.getNumSamples();

    // -------------------------------------------------------------------------
    // LOGICA DI REGISTRAZIONE E PLAYBACK (AUDIO + MIDI)
    // -------------------------------------------------------------------------
    if (currentState == 1) // Recording
    {
        for (int channel = 0; channel < totalNumInputChannels; ++channel)
        {
            loopAudioBuffer.copyFrom (channel, writePosition, buffer, channel, 0, numSamples);
        }

        // Cattura eventi MIDI nel buffer di loop
        for (const auto metadata : midiMessages)
        {
            auto message = metadata.getMessage();
            int sampleOffset = metadata.samplePosition;
            midiLoopBuffer.addEvent (message, writePosition + sampleOffset);
        }

        writePosition += numSamples;
        recordedLoopLength = writePosition; // Imposta la lunghezza effettiva registrata
    }
    else if (currentState == 2 && recordedLoopLength > 0) // Playing
    {
        buffer.clear(); // Puliamo l'output attuale per riempirlo dal loop

        // Calcolo posizione di lettura (Reverse vs Forward)
        for (int sample = 0; sample < numSamples; ++sample)
        {
            int actualReadPos = isReverse ? (recordedLoopLength - 1 - readPosition) : readPosition;

            for (int channel = 0; channel < totalNumInputChannels; ++channel)
            {
                buffer.setSample (channel, sample, loopAudioBuffer.getSample (channel, actualReadPos));
            }

            readPosition++;
            if (readPosition >= recordedLoopLength)
                readPosition = 0;
        }

        // Playback MIDI
        juce::MidiBuffer outputMidi;
        for (const auto metadata : midiLoopBuffer)
        {
            int eventPos = metadata.samplePosition;
            if (eventPos >= readPosition && eventPos < readPosition + numSamples)
            {
                outputMidi.addEvent (metadata.getMessage(), eventPos - readPosition);
            }
        }
        midiMessages.swapWith (outputMidi);
    }

    // Passaggio attraverso il filtro Cutoff (DSP Block)
    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> context (block);
    cutoffFilter.process (context);
}

//==============================================================================
// Boilerplate di base di JUCE...
bool AdvancedLooperAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* AdvancedLooperAudioProcessor::createEditor() { return new juce::GenericAudioProcessorEditor (*this); }
const juce::String AdvancedLooperAudioProcessor::getName() const { return JucePlugin_Name; }
bool AdvancedLooperAudioProcessor::acceptsMidi() const { return true; }
bool AdvancedLooperAudioProcessor::producesMidi() const { return true; }
double AdvancedLooperAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int AdvancedLooperAudioProcessor::getNumPrograms() { return 1; }
int AdvancedLooperAudioProcessor::getCurrentProgram() { return 0; }
void AdvancedLooperAudioProcessor::setCurrentProgram (int index) {}
const juce::String AdvancedLooperAudioProcessor::getProgramName (int index) { return {}; }
void AdvancedLooperAudioProcessor::changeProgramName (int index, const juce::String& newName) {}
void AdvancedLooperAudioProcessor::getStateInformation (juce::MemoryBlock& destData) {}
void AdvancedLooperAudioProcessor::setStateInformation (const void* data, int sizeInBytes) {}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AdvancedLooperAudioProcessor();
}
