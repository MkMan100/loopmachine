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
    currentSampleRate = sampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = juce::uint32 (samplesPerBlock);
    spec.numChannels = juce::uint32 (getTotalNumOutputChannels());

    // Inizializza filtro Cutoff
    cutoffFilter.prepare (spec);
    cutoffFilter.setType (juce::dsp::StateVariableTPTFilterType::lowpass);

    // Inizializza Dub Delay
    dubDelay.prepare (spec);
    dubDelay.setMaximumDelayInSamples (static_cast<int> (sampleRate * 2.0)); // Max 2 secondi

    // Inizializza buffer interno di feedback per il delay
    delayFeedbackBuffer.setSize (getTotalNumOutputChannels(), samplesPerBlock);
    delayFeedbackBuffer.clear();

    // Allocazione massima buffer loop (60 secondi)
    maxLoopSamples = static_cast<int> (sampleRate * 60.0);
    loopAudioBuffer.setSize (getTotalNumOutputChannels(), maxLoopSamples);
    loopAudioBuffer.clear();

    midiLoopBuffer.clear();
    writePosition = 0;
    readPosition = 0;
}

void AdvancedLooperAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Legge i parametri APVTS
    float cutoffHz       = *apvts.getRawParameterValue ("cutoff");
    bool isReverse       = *apvts.getRawParameterValue ("reverse") > 0.5f;
    int currentState     = static_cast<int> (*apvts.getRawParameterValue ("state"));
    int stepReduceVal    = static_cast<int> (*apvts.getRawParameterValue ("stepReduce")); // 1 (normale) a 16 (molto quantizzato)
    float delayTimeMs    = *apvts.getRawParameterValue ("delayTime");
    float delayFeedback  = *apvts.getRawParameterValue ("delayFeedback");

    // Aggiorna Filtro
    cutoffFilter.setCutoffFrequency (cutoffHz);

    // Aggiorna Tempo Delay in campioni
    float delaySamples = (delayTimeMs / 1000.0f) * static_cast<float> (currentSampleRate);
    dubDelay.setDelay (delaySamples);

    int numSamples = buffer.getNumSamples();

    // --- 1. RECORDING ---
    if (currentState == 1) 
    {
        if (recordedLoopLength == 0 && writePosition >= maxLoopSamples - numSamples)
            writePosition = 0;

        for (int channel = 0; channel < totalNumInputChannels; ++channel)
            loopAudioBuffer.copyFrom (channel, writePosition, buffer, channel, 0, numSamples);

        for (const auto metadata : midiMessages)
            midiLoopBuffer.addEvent (metadata.getMessage(), writePosition + metadata.samplePosition);

        writePosition += numSamples;
        recordedLoopLength = writePosition;
        readPosition = 0;
    }
    // --- 2. PLAYBACK & OVERDUB ---
    else if ((currentState == 2 || currentState == 3) && recordedLoopLength > 0) 
    {
        juce::AudioBuffer<float> outputBuffer;
        outputBuffer.setSize (totalNumOutputChannels, numSamples);
        outputBuffer.clear();

        // Calcolo della dimensione dello step per Step Reduction (sample-hold / stutter effect non distruttivo)
        int stepSizeSamples = 1;
        if (stepReduceVal < 16)
        {
            // Mappa il valore di stepReduce in una dimensione di blocco audio quantizzata
            int divisionFactor = 17 - stepReduceVal; // da 1 (micro) a 16 (macro)
            stepSizeSamples = juce::jmax (1, static_cast<int> (currentSampleRate / (divisionFactor * 4)));
        }

        for (int sample = 0; sample < numSamples; ++sample)
        {
            readPosition %= recordedLoopLength;

            // Logica Step Reduction: quantizza la posizione di lettura a blocchi rigidi
            int quantizedReadPos = (readPosition / stepSizeSamples) * stepSizeSamples;
            quantizedReadPos %= recordedLoopLength;

            // Indice finale tenendo conto di Reverse
            int actualReadPos = isReverse ? (recordedLoopLength - 1 - quantizedReadPos) : quantizedReadPos;

            for (int channel = 0; channel < totalNumInputChannels; ++channel)
            {
                float loopSample = loopAudioBuffer.getSample (channel, actualReadPos);

                if (currentState == 3) // Overdub
                {
                    float inputSample = buffer.getSample (channel, sample);
                    loopAudioBuffer.setSample (channel, actualReadPos, loopSample + inputSample);
                    loopSample += inputSample;
                }

                outputBuffer.setSample (channel, sample, loopSample);
            }

            readPosition++;
        }

        for (int channel = 0; channel < totalNumOutputChannels; ++channel)
            buffer.copyFrom (channel, 0, outputBuffer, channel, 0, numSamples);

        // Playback MIDI
        juce::MidiBuffer outputMidi;
        for (const auto metadata : midiLoopBuffer)
        {
            int eventPos = metadata.samplePosition;
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

    // --- ELABORAZIONE DSP (Filtro Cutoff + Dub Delay) ---
    
    // 1. Applica Filtro Cutoff al segnale principale
    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> context (block);
    cutoffFilter.process (context);

    // 2. Applica Dub Delay con Saturazione di Feedback
    for (int sample = 0; sample < numSamples; ++sample)
    {
        for (int channel = 0; channel < totalNumOutputChannels; ++channel)
        {
            float inputSample = buffer.getSample (channel, sample);
            float delayedSample = dubDelay.popSample (channel);

            // Mix audio: segnale originale + segnale ritardato
            buffer.setSample (channel, sample, inputSample + (delayedSample * 0.7f));

            // Feedback con lieve saturazione stile tape dub (std::tanh)
            float feedbackSample = inputSample + (delayedSample * delayFeedback);
            feedbackSample = std::tanh (feedbackSample); // Saturazione analogica non lineare

            dubDelay.pushSample (channel, feedbackSample);
        }
    }
}
void AdvancedLooperAudioProcessor::releaseResources()
{
    // Quando la riproduzione si ferma, liberiamo le risorse dei moduli DSP
    cutoffFilter.reset();
    dubDelay.reset();
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
