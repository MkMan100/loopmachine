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

    // 1. Stato Looper: 0 = Empty, 1 = Play, 2 = Overdub
    params.push_back (std::make_unique<juce::AudioParameterChoice> ("state", "State", 
        juce::StringArray { "Empty", "Playing", "Overdub" }, 0));

    // 2. Step Division (Estesa fino a 1/32)
    params.push_back (std::make_unique<juce::AudioParameterChoice> ("stepReduce", "Step Division", 
        juce::StringArray { "1/1 (Full)", "3/4", "1/2", "1/3", "1/4", "1/8", "1/16", "1/32" }, 0));

    // 3. Lunghezza Sample in Beat/Bars
    params.push_back (std::make_unique<juce::AudioParameterChoice> ("loopLength", "Loop Length (Beats)", 
        juce::StringArray { "1", "2", "4", "8", "16", "32" }, 2));

    // 4. Reverse Play
    params.push_back (std::make_unique<juce::AudioParameterBool> ("reverse", "Reverse Playback", false));

    // 5. Cutoff Filter
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("cutoff", "Cutoff Frequency", 
        juce::NormalisableRange<float> (20.0f, 20000.0f, 1.0f, 0.3f), 20000.0f));

    // 6. Dub Delay
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("delayTime", "Dub Delay Time (ms)", 
        juce::NormalisableRange<float> (10.0f, 2000.0f, 1.0f, 0.5f), 375.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("delayFeedback", "Dub Delay Feedback", 
        juce::NormalisableRange<float> (0.0f, 0.95f, 0.01f), 0.5f));

    // 7. Checkbox Abilitazione Registrazione Automazioni Effetti
    params.push_back (std::make_unique<juce::AudioParameterBool> ("recCutoff", "Rec Cutoff Auto", false));
    params.push_back (std::make_unique<juce::AudioParameterBool> ("recDelayTime", "Rec Delay Time Auto", false));
    params.push_back (std::make_unique<juce::AudioParameterBool> ("recDelayFB", "Rec Delay FB Auto", false));
    params.push_back (std::make_unique<juce::AudioParameterBool> ("recStepDiv", "Rec Step Div Auto", false));

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

    cutoffFilter.prepare (spec);
    cutoffFilter.setType (juce::dsp::StateVariableTPTFilterType::lowpass);

    dubDelay.prepare (spec);
    dubDelay.setMaximumDelayInSamples (static_cast<int> (sampleRate * 2.0));

    delayFeedbackBuffer.setSize (getTotalNumOutputChannels(), samplesPerBlock);
    delayFeedbackBuffer.clear();

    maxLoopSamples = static_cast<int> (sampleRate * 60.0);
    loopAudioBuffer.setSize (getTotalNumOutputChannels(), maxLoopSamples);
    loopAudioBuffer.clear();

    // Resize dei buffer di automazione per supportare la durata massima del loop
    cutoffAutomation.assign (maxLoopSamples, 20000.0f);
    delayTimeAutomation.assign (maxLoopSamples, 375.0f);
    delayFeedbackAutomation.assign (maxLoopSamples, 0.5f);
    stepReduceAutomation.assign (maxLoopSamples, 0);

    midiLoopBuffer.clear();
    writePosition = 0;
    readPosition = 0;
    recordedLoopLength = 0;
}

void AdvancedLooperAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Legge parametri manuali della GUI
    float manualCutoff        = *apvts.getRawParameterValue ("cutoff");
    float manualDelayTime     = *apvts.getRawParameterValue ("delayTime");
    float manualDelayFB       = *apvts.getRawParameterValue ("delayFeedback");
    int   manualStepReduce    = static_cast<int> (*apvts.getRawParameterValue ("stepReduce"));
    
    int   currentState        = static_cast<int> (*apvts.getRawParameterValue ("state"));
    bool  isReverse           = *apvts.getRawParameterValue ("reverse") > 0.5f;

    // Toggle per abilitazione automazione
    bool  recCutoffEnabled    = *apvts.getRawParameterValue ("recCutoff") > 0.5f;
    bool  recDelayTimeEnabled = *apvts.getRawParameterValue ("recDelayTime") > 0.5f;
    bool  recDelayFBEnabled   = *apvts.getRawParameterValue ("recDelayFB") > 0.5f;
    bool  recStepDivEnabled   = *apvts.getRawParameterValue ("recStepDiv") > 0.5f;

    int numSamples = buffer.getNumSamples();

    // Calcolo della durata target del loop basata sui Beat selezionati e BPM Host
    if (currentState == 2 && recordedLoopLength == 0) 
    {
        double bpm = 120.0;
        if (auto* playHead = getPlayHead())
        {
            if (auto pos = playHead->getPosition())
                if (pos->getBpm().hasValue())
                    bpm = *pos->getBpm();
        }

        int selectedBeatsIdx = static_cast<int> (*apvts.getRawParameterValue ("loopLength"));
        int beatsArray[] = { 1, 2, 4, 8, 16, 32 };
        int numBeats = beatsArray[selectedBeatsIdx];

        double secondsPerBeat = 60.0 / bpm;
        targetLoopLengthSamples = static_cast<int> (numBeats * secondsPerBeat * currentSampleRate);
        targetLoopLengthSamples = juce::jmin (targetLoopLengthSamples, maxLoopSamples);
    }

    // --- 0. EMPTY ---
    if (currentState == 0)
    {
        writePosition = 0;
        readPosition = 0;
        recordedLoopLength = 0;
        loopAudioBuffer.clear();
        midiLoopBuffer.clear();
    }
    // --- PLAYBACK & OVERDUB ---
    else if ((currentState == 1 || currentState == 2))
    {
        juce::AudioBuffer<float> outputBuffer;
        outputBuffer.setSize (totalNumOutputChannels, numSamples);
        outputBuffer.clear();

        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Se stiamo registrando per la prima volta
            if (recordedLoopLength == 0 && currentState == 2)
            {
                int currentPos = writePosition;

                for (int channel = 0; channel < totalNumInputChannels; ++channel)
                {
                    float inputSample = buffer.getSample (channel, sample);
                    loopAudioBuffer.setSample (channel, currentPos, inputSample);
                    outputBuffer.setSample (channel, sample, inputSample);
                }

                // Inizializza i valori di automazione durante il primo giro
                cutoffAutomation[currentPos]        = manualCutoff;
                delayTimeAutomation[currentPos]     = manualDelayTime;
                delayFeedbackAutomation[currentPos] = manualDelayFB;
                stepReduceAutomation[currentPos]    = manualStepReduce;

                writePosition++;

                if (writePosition >= targetLoopLengthSamples)
                {
                    recordedLoopLength = targetLoopLengthSamples;
                    readPosition = 0;
                    writePosition = 0;
                }
            }
            else if (recordedLoopLength > 0)
            {
                // Determina la Step Division attiva
                int activeStepReduce = manualStepReduce;
                if (!recStepDivEnabled)
                    activeStepReduce = stepReduceAutomation[readPosition % recordedLoopLength];

                // Moltiplicatore frazionario (fino a 1/32)
                float lengthMultiplier = 1.0f;
                switch (activeStepReduce)
                {
                    case 0: lengthMultiplier = 1.0f;    break; // 1/1
                    case 1: lengthMultiplier = 0.75f;   break; // 3/4
                    case 2: lengthMultiplier = 0.50f;   break; // 1/2
                    case 3: lengthMultiplier = 0.333f;  break; // 1/3
                    case 4: lengthMultiplier = 0.25f;   break; // 1/4
                    case 5: lengthMultiplier = 0.125f;  break; // 1/8
                    case 6: lengthMultiplier = 0.0625f; break; // 1/16
                    case 7: lengthMultiplier = 0.03125f;break; // 1/32
                    default: lengthMultiplier = 1.0f;   break;
                }

                int activeLoopLength = static_cast<int> (recordedLoopLength * lengthMultiplier);
                if (activeLoopLength < 1) activeLoopLength = recordedLoopLength;

                readPosition %= activeLoopLength;
                int actualReadPos = isReverse ? (activeLoopLength - 1 - readPosition) : readPosition;

                // Registrazione automazioni durante Overdub (Stato 2)
                if (currentState == 2)
                {
                    if (recCutoffEnabled)    cutoffAutomation[actualReadPos]        = manualCutoff;
                    if (recDelayTimeEnabled) delayTimeAutomation[actualReadPos]     = manualDelayTime;
                    if (recDelayFBEnabled)   delayFeedbackAutomation[actualReadPos] = manualDelayFB;
                    if (recStepDivEnabled)   stepReduceAutomation[actualReadPos]    = manualStepReduce;
                }

                // Applica automazione o valore manuale al DSP corrente
                manualCutoff    = recCutoffEnabled    ? manualCutoff    : cutoffAutomation[actualReadPos];
                manualDelayTime = recDelayTimeEnabled ? manualDelayTime : delayTimeAutomation[actualReadPos];
                manualDelayFB   = recDelayFBEnabled   ? manualDelayFB   : delayFeedbackAutomation[actualReadPos];

                for (int channel = 0; channel < totalNumInputChannels; ++channel)
                {
                    float loopSample = loopAudioBuffer.getSample (channel, actualReadPos);
                    
                    if (currentState == 2) // Overdub audio
                    {
                        float inputSample = buffer.getSample (channel, sample);
                        loopAudioBuffer.setSample (channel, actualReadPos, loopSample + inputSample);
                        loopSample += inputSample;
                    }

                    outputBuffer.setSample (channel, sample, loopSample);
                }

                readPosition++;
            }
        }

        if (recordedLoopLength > 0 || currentState == 2)
            for (int channel = 0; channel < totalNumOutputChannels; ++channel)
                buffer.copyFrom (channel, 0, outputBuffer, channel, 0, numSamples);
    }

    // --- ELABORAZIONE DSP APPLICATA AL BLOCCO ---
    cutoffFilter.setCutoffFrequency (manualCutoff);
    float delaySamples = (manualDelayTime / 1000.0f) * static_cast<float> (currentSampleRate);
    dubDelay.setDelay (delaySamples);

    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> context (block);
    cutoffFilter.process (context);

    for (int sample = 0; sample < numSamples; ++sample)
    {
        for (int channel = 0; channel < totalNumOutputChannels; ++channel)
        {
            float inputSample = buffer.getSample (channel, sample);
            float delayedSample = dubDelay.popSample (channel);

            buffer.setSample (channel, sample, inputSample + (delayedSample * 0.7f));

            float feedbackSample = inputSample + (delayedSample * manualDelayFB);
            feedbackSample = std::tanh (feedbackSample); 

            dubDelay.pushSample (channel, feedbackSample);
        }
    }
}

void AdvancedLooperAudioProcessor::releaseResources()
{
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
