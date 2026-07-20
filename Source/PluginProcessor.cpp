#include "PluginProcessor.h"
#include "PluginEditor.h"

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

    // 0 = Empty, 1 = Recording, 2 = Playing, 3 = Overdub

    // --- 1. RECORDING ---
    if (currentState == 1) 
    {
        // Se siamo appena entrati in Recording, azzeriamo la posizione di scrittura
        if (recordedLoopLength == 0 && writePosition >= maxLoopSamples - numSamples)
            writePosition = 0;

        for (int channel = 0; channel < totalNumInputChannels; ++channel)
        {
            loopAudioBuffer.copyFrom (channel, writePosition, buffer, channel, 0, numSamples);
        }

        // Cattura eventi MIDI
        for (const auto metadata : midiMessages)
        {
            auto message = metadata.getMessage();
            int sampleOffset = metadata.samplePosition;
            midiLoopBuffer.addEvent (message, writePosition + sampleOffset);
        }

        writePosition += numSamples;
        recordedLoopLength = writePosition; // La lunghezza del loop cresce durante la registrazione
        readPosition = 0; // Prepara la testina di lettura
    }
    // --- 2. PLAYBACK & OVERDUB ---
    else if ((currentState == 2 || currentState == 3) && recordedLoopLength > 0) 
    {
        juce::AudioBuffer<float> outputBuffer;
        outputBuffer.setSize (totalNumOutputChannels, numSamples);
        outputBuffer.clear();

        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Assicuriamoci che readPosition sia sempre dentro i limiti del loop
            readPosition %= recordedLoopLength;

            // Calcolo indice di lettura (Forward vs Reverse)
            int actualReadPos = isReverse ? (recordedLoopLength - 1 - readPosition) : readPosition;

            for (int channel = 0; channel < totalNumInputChannels; ++channel)
            {
                float loopSample = loopAudioBuffer.getSample (channel, actualReadPos);

                // Se siamo in Overdub, sommiamo l'audio in ingresso al buffer di loop
                if (currentState == 3)
                {
                    float inputSample = buffer.getSample (channel, sample);
                    loopAudioBuffer.setSample (channel, actualReadPos, loopSample + inputSample);
                    loopSample += inputSample; // Ascoltiamo la somma
                }

                outputBuffer.setSample (channel, sample, loopSample);
            }

            readPosition++;
        }

        // Sostituiamo l'output con l'audio estratto dal loop
        for (int channel = 0; channel < totalNumOutputChannels; ++channel)
        {
            buffer.copyFrom (channel, 0, outputBuffer, channel, 0, numSamples);
        }

        // Playback MIDI
        juce::MidiBuffer outputMidi;
        for (const auto metadata : midiLoopBuffer)
        {
            int eventPos = metadata.samplePosition;
            // Verifica se l'evento ricade nella finestra di campioni attuale
            if (eventPos >= readPosition - numSamples && eventPos < readPosition)
            {
                int offset = eventPos - (readPosition - numSamples);
                if (offset >= 0 && offset < numSamples)
                    outputMidi.addEvent (metadata.getMessage(), offset);
            }
        }
        midiMessages.swapWith (outputMidi);
    }
    // --- 0. EMPTY / RESET ---
    else if (currentState == 0)
    {
        writePosition = 0;
        readPosition = 0;
        recordedLoopLength = 0;
        loopAudioBuffer.clear();
        midiLoopBuffer.clear();
    }

    // Passaggio attraverso il filtro Cutoff (DSP Block)
    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> context (block);
    cutoffFilter.process (context);
}

//==============================================================================
bool AdvancedLooperAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* AdvancedLooperAudioProcessor::createEditor() { return new AdvancedLooperAudioProcessorEditor (*this); }
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

bool AdvancedLooperAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
#else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
#endif
}

bool AdvancedLooperAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}
