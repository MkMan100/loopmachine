#include "PluginProcessor.h"
#include "PluginEditor.h"

AdvancedLooperAudioProcessorEditor::AdvancedLooperAudioProcessorEditor (AdvancedLooperAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (680, 430); // Finestra più alta per dare spazio alle due righe

    // Funzione helper per configurare le manopole
    auto setupKnob = [this](juce::Slider& slider, juce::Label& label) {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 16);
        addAndMakeVisible (slider);
        
        label.attachToComponent (&slider, false); // Etichetta agganciata sopra
        label.setJustificationType (juce::Justification::centred);
    };

    // Configurazione Manopole
    setupKnob (stateSlider, stateLabel);
    setupKnob (loopLengthSlider, loopLengthLabel);
    setupKnob (stepReduceSlider, stepReduceLabel);
    setupKnob (cutoffSlider, cutoffLabel);
    setupKnob (delayTimeSlider, delayTimeLabel);
    setupKnob (delayFeedbackSlider, delayFeedbackLabel);

    // Configurazione Pulsanti Toggle
    addAndMakeVisible (reverseButton);
    addAndMakeVisible (recCutoffButton);
    addAndMakeVisible (recDelayTimeButton);
    addAndMakeVisible (recDelayFBButton);
    addAndMakeVisible (recStepDivButton);

    // Styling dei pulsanti REC AUTO
    auto setupRecButton = [](juce::ToggleButton& btn) {
        btn.setColour (juce::ToggleButton::tickColourId, juce::Colours::red);
        btn.setColour (juce::ToggleButton::textColourId, juce::Colours::lightgrey);
    };

    setupRecButton (recCutoffButton);
    setupRecButton (recDelayTimeButton);
    setupRecButton (recDelayFBButton);
    setupRecButton (recStepDivButton);

    // Collega gli Attachments all'APVTS
    stateAttachment         = std::make_unique<SliderAttachment> (audioProcessor.apvts, "state", stateSlider);
    loopLengthAttachment    = std::make_unique<SliderAttachment> (audioProcessor.apvts, "loopLength", loopLengthSlider);
    stepReduceAttachment    = std::make_unique<SliderAttachment> (audioProcessor.apvts, "stepReduce", stepReduceSlider);
    reverseAttachment       = std::make_unique<ButtonAttachment> (audioProcessor.apvts, "reverse", reverseButton);
    cutoffAttachment        = std::make_unique<SliderAttachment> (audioProcessor.apvts, "cutoff", cutoffSlider);
    delayTimeAttachment     = std::make_unique<SliderAttachment> (audioProcessor.apvts, "delayTime", delayTimeSlider);
    delayFeedbackAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "delayFeedback", delayFeedbackSlider);

    // Attachments per i Toggle di Automazione
    recCutoffAttachment    = std::make_unique<ButtonAttachment> (audioProcessor.apvts, "recCutoff", recCutoffButton);
    recDelayTimeAttachment = std::make_unique<ButtonAttachment> (audioProcessor.apvts, "recDelayTime", recDelayTimeButton);
    recDelayFBAttachment   = std::make_unique<ButtonAttachment> (audioProcessor.apvts, "recDelayFB", recDelayFBButton);
    recStepDivAttachment   = std::make_unique<ButtonAttachment> (audioProcessor.apvts, "recStepDiv", recStepDivButton);
}

AdvancedLooperAudioProcessorEditor::~AdvancedLooperAudioProcessorEditor() {}

void AdvancedLooperAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1a1a1d));

    g.setColour (juce::Colours::cyan);
    g.setFont (18.0f);
    g.drawText ("ADVANCED DUB LOOPER", getLocalBounds().removeFromTop (30), juce::Justification::centred, true);
}

void AdvancedLooperAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (15);
    area.removeFromTop (25); // Spazio per il titolo

    // --- PRIMA RIGA: Controlli Looper ---
    auto topRow = area.removeFromTop (150);
    int topWidth = topRow.getWidth() / 4;

    // Mode & Beats
    auto stateArea = topRow.removeFromLeft (topWidth).reduced (10, 0);
    stateArea.removeFromTop (18); // Spazio dedicato all'etichetta in alto
    stateSlider.setBounds (stateArea);

    auto loopArea = topRow.removeFromLeft (topWidth).reduced (10, 0);
    loopArea.removeFromTop (18);
    loopLengthSlider.setBounds (loopArea);

    // Step Div con pulsante REC AUTO sotto
    auto stepArea = topRow.removeFromLeft (topWidth).reduced (10, 0);
    stepArea.removeFromTop (18);
    stepReduceSlider.setBounds (stepArea.removeFromTop (100));
    recStepDivButton.setBounds (stepArea.removeFromTop (24));

    // Reverse Button
    reverseButton.setBounds (topRow.removeFromLeft (topWidth).reduced (10, 45));

    // --- SPAZIATORE VERTICALE ---
    area.removeFromTop (20); // Separa nettamente la prima riga dalla seconda

    // --- SECONDA RIGA: Controlli Dub Effects ---
    auto bottomRow = area.removeFromTop (160);
    int bottomWidth = bottomRow.getWidth() / 3;

    // Cutoff
    auto cutoffArea = bottomRow.removeFromLeft (bottomWidth).reduced (12, 0);
    cutoffArea.removeFromTop (18);
    cutoffSlider.setBounds (cutoffArea.removeFromTop (105));
    recCutoffButton.setBounds (cutoffArea.removeFromTop (24));

    // Delay Time
    auto delayTimeArea = bottomRow.removeFromLeft (bottomWidth).reduced (12, 0);
    delayTimeArea.removeFromTop (18);
    delayTimeSlider.setBounds (delayTimeArea.removeFromTop (105));
    recDelayTimeButton.setBounds (delayTimeArea.removeFromTop (24));

    // Delay Feedback
    auto delayFBArea = bottomRow.removeFromLeft (bottomWidth).reduced (12, 0);
    delayFBArea.removeFromTop (18);
    delayFeedbackSlider.setBounds (delayFBArea.removeFromTop (105));
    recDelayFBButton.setBounds (delayFBArea.removeFromTop (24));
}
