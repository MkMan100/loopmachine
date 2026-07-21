#include "PluginProcessor.h"
#include "PluginEditor.h"

AdvancedLooperAudioProcessorEditor::AdvancedLooperAudioProcessorEditor (AdvancedLooperAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (650, 380);

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

    addAndMakeVisible (reverseButton);

    // Collega gli Attachments all'APVTS (Rende tutto automabile e mappabile MIDI!)
    stateAttachment       = std::make_unique<SliderAttachment> (audioProcessor.apvts, "state", stateSlider);
    loopLengthAttachment  = std::make_unique<SliderAttachment> (audioProcessor.apvts, "loopLength", loopLengthSlider);
    stepReduceAttachment  = std::make_unique<SliderAttachment> (audioProcessor.apvts, "stepReduce", stepReduceSlider);
    reverseAttachment     = std::make_unique<ButtonAttachment> (audioProcessor.apvts, "reverse", reverseButton);
    cutoffAttachment      = std::make_unique<SliderAttachment> (audioProcessor.apvts, "cutoff", cutoffSlider);
    delayTimeAttachment   = std::make_unique<SliderAttachment> (audioProcessor.apvts, "delayTime", delayTimeSlider);
    delayFeedbackAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "delayFeedback", delayFeedbackSlider);
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

    // Prima riga: Controlli Looper (Mode, Beats, Step Div, Reverse)
    auto topRow = area.removeFromTop (130);
    int topWidth = topRow.getWidth() / 4;
    
    stateSlider.setBounds (topRow.removeFromLeft (topWidth).reduced (10));
    loopLengthSlider.setBounds (topRow.removeFromLeft (topWidth).reduced (10));
    stepReduceSlider.setBounds (topRow.removeFromLeft (topWidth).reduced (10));
    reverseButton.setBounds (topRow.removeFromLeft (topWidth).reduced (10, 40));

    // Seconda riga: Controlli Dub (Cutoff, Delay Time, Feedback)
    auto bottomRow = area.removeFromTop (130);
    int bottomWidth = bottomRow.getWidth() / 3;

    cutoffSlider.setBounds (bottomRow.removeFromLeft (bottomWidth).reduced (10));
    delayTimeSlider.setBounds (bottomRow.removeFromLeft (bottomWidth).reduced (10));
    delayFeedbackSlider.setBounds (bottomRow.removeFromLeft (bottomWidth).reduced (10));
}
