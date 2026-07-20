#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AdvancedLooperAudioProcessorEditor::AdvancedLooperAudioProcessorEditor (AdvancedLooperAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Dimensioni finestra GUI
    setSize (600, 400);

    // Configurazione ComboBox: State (Empty, Record, Play, Overdub)
    stateComboBox.addItemList (juce::StringArray { "Empty", "Recording", "Playing", "Overdub" }, 1);
    addAndMakeVisible (stateComboBox);
    stateLabel.attachToComponent (&stateComboBox, false);
    stateAttachment = std::make_unique<ComboBoxAttachment> (audioProcessor.apvts, "state", stateComboBox);

    // Configurazione ComboBox: Loop Length
    loopLengthComboBox.addItemList (juce::StringArray { "1", "2", "4", "8", "16", "32" }, 1);
    addAndMakeVisible (loopLengthComboBox);
    loopLengthLabel.attachToComponent (&loopLengthComboBox, false);
    loopLengthAttachment = std::make_unique<ComboBoxAttachment> (audioProcessor.apvts, "loopLength", loopLengthComboBox);

    // Configurazione Slider: Step Reduction
    stepReduceSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    stepReduceSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 50, 20);
    addAndMakeVisible (stepReduceSlider);
    stepReduceLabel.attachToComponent (&stepReduceSlider, false);
    stepReduceAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "stepReduce", stepReduceSlider);

    // Configurazione Toggle: Reverse
    addAndMakeVisible (reverseButton);
    reverseAttachment = std::make_unique<ButtonAttachment> (audioProcessor.apvts, "reverse", reverseButton);

    // Configurazione Slider: Cutoff Filter (Manopola Rotativa)
    cutoffSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    cutoffSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible (cutoffSlider);
    cutoffLabel.attachToComponent (&cutoffSlider, false);
    cutoffAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "cutoff", cutoffSlider);

    // Configurazione Slider: Dub Delay Time
    delayTimeSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    delayTimeSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible (delayTimeSlider);
    delayTimeLabel.attachToComponent (&delayTimeSlider, false);
    delayTimeAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "delayTime", delayTimeSlider);

    // Configurazione Slider: Dub Delay Feedback
    delayFeedbackSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    delayFeedbackSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible (delayFeedbackSlider);
    delayFeedbackLabel.attachToComponent (&delayFeedbackSlider, false);
    delayFeedbackAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "delayFeedback", delayFeedbackSlider);
}

AdvancedLooperAudioProcessorEditor::~AdvancedLooperAudioProcessorEditor()
{
}

//==============================================================================
void AdvancedLooperAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Background scuro
    g.fillAll (juce::Colour (0xff1e1e24));

    // Titolo
    g.setColour (juce::Colours::cyan);
    g.setFont (22.0f);
    g.drawText ("ADVANCED DUB LOOPER", getLocalBounds().removeFromTop (40), juce::Justification::centred, true);
}

void AdvancedLooperAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (20);
    area.removeFromTop (30); // Spazio per il titolo

    // Layout in griglia/sezioni
    auto topRow = area.removeFromTop (80);
    stateComboBox.setBounds (topRow.removeFromLeft (120).reduced (0, 20));
    loopLengthComboBox.setBounds (topRow.removeFromLeft (120).reduced (10, 20));
    reverseButton.setBounds (topRow.removeFromLeft (100).reduced (10, 20));

    auto middleRow = area.removeFromTop (80);
    stepReduceSlider.setBounds (middleRow.reduced (0, 20));

    auto bottomRow = area.removeFromTop (140);
    int knobWidth = bottomRow.getWidth() / 3;
    cutoffSlider.setBounds (bottomRow.removeFromLeft (knobWidth).reduced (10));
    delayTimeSlider.setBounds (bottomRow.removeFromLeft (knobWidth).reduced (10));
    delayFeedbackSlider.setBounds (bottomRow.removeFromLeft (knobWidth).reduced (10));
}
