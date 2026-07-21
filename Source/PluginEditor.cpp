#include "PluginProcessor.h"
#include "PluginEditor.h"

AdvancedLooperAudioProcessorEditor::AdvancedLooperAudioProcessorEditor (AdvancedLooperAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (680, 420);

    // Funzione helper per configurare le manopole
    auto setupKnob = [this](juce::Slider& slider, juce::Label& label) {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 20);
        addAndMakeVisible (slider);
        label.attachToComponent (&slider, false);
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
    g.setFont (20.0f);
    g.drawText ("ADVANCED DUB LOOPER", getLocalBounds().removeFromTop (35), juce::Justification::centred, true);
}

void AdvancedLooperAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (15);
    area.removeFromTop (30); // Spazio per titolo

    // --- Prima Riga: Controlli Looper ---
    auto topRow = area.removeFromTop (160);
    int topWidth = topRow.getWidth() / 4;

    stateSlider.setBounds (topRow.removeFromLeft (topWidth).reduced (10, 0));
    loopLengthSlider.setBounds (topRow.removeFromLeft (topWidth).reduced (10, 0));

    // Step Div con pulsante REC AUTO sotto
    auto stepArea = topRow.removeFromLeft (topWidth).reduced (5, 0);
    stepReduceSlider.setBounds (stepArea.removeFromTop (120));
    recStepDivButton.setBounds (stepArea.removeFromTop (30));

    reverseButton.setBounds (topRow.removeFromLeft (topWidth).reduced (10, 40));

    // --- Seconda Riga: Controlli Dub Effects ---
    auto bottomRow = area.removeFromTop (160);
    int bottomWidth = bottomRow.getWidth() / 3;

    // Cutoff
    auto cutoffArea = bottomRow.removeFromLeft (bottomWidth).reduced (5, 0);
    cutoffSlider.setBounds (cutoffArea.removeFromTop (120));
    recCutoffButton.setBounds (cutoffArea.removeFromTop (30));

    // Delay Time
    auto delayTimeArea = bottomRow.removeFromLeft (bottomWidth).reduced (5, 0);
    delayTimeSlider.setBounds (delayTimeArea.removeFromTop (120));
    recDelayTimeButton.setBounds (delayTimeArea.removeFromTop (30));

    // Delay Feedback
    auto delayFBArea = bottomRow.removeFromLeft (bottomWidth).reduced (5, 0);
    delayFeedbackSlider.setBounds (delayFBArea.removeFromTop (120));
    recDelayFBButton.setBounds (delayFBArea.removeFromTop (30));
}
