#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class AdvancedLooperAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    explicit AdvancedLooperAudioProcessorEditor (AdvancedLooperAudioProcessor&);
    ~AdvancedLooperAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    AdvancedLooperAudioProcessor& audioProcessor;

    // --- Controllo Controlli GUI (Slidres & Buttons) ---
    juce::Slider stateSlider;
    juce::Slider loopLengthSlider;
    juce::Slider stepReduceSlider;
    juce::Slider cutoffSlider;
    juce::Slider delayTimeSlider;
    juce::Slider delayFeedbackSlider;

    juce::ToggleButton reverseButton { "REVERSE" };

    // --- Pulsanti Registrazione Automazioni (Rec Toggles) ---
    juce::ToggleButton recCutoffButton    { "REC AUTO" };
    juce::ToggleButton recDelayTimeButton { "REC AUTO" };
    juce::ToggleButton recDelayFBButton   { "REC AUTO" };
    juce::ToggleButton recStepDivButton   { "REC AUTO" };

    // --- Etichette ---
    juce::Label stateLabel        { {}, "Mode" };
    juce::Label loopLengthLabel   { {}, "Beats" };
    juce::Label stepReduceLabel   { {}, "Step Div" };
    juce::Label cutoffLabel       { {}, "Cutoff (Hz)" };
    juce::Label delayTimeLabel    { {}, "Delay Time" };
    juce::Label delayFeedbackLabel{ {}, "Feedback" };

    // --- Attachments APVTS ---
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAttachment> stateAttachment;
    std::unique_ptr<SliderAttachment> loopLengthAttachment;
    std::unique_ptr<SliderAttachment> stepReduceAttachment;
    std::unique_ptr<ButtonAttachment> reverseAttachment;
    std::unique_ptr<SliderAttachment> cutoffAttachment;
    std::unique_ptr<SliderAttachment> delayTimeAttachment;
    std::unique_ptr<SliderAttachment> delayFeedbackAttachment;

    // Attachments per le automazioni
    std::unique_ptr<ButtonAttachment> recCutoffAttachment;
    std::unique_ptr<ButtonAttachment> recDelayTimeAttachment;
    std::unique_ptr<ButtonAttachment> recDelayFBAttachment;
    std::unique_ptr<ButtonAttachment> recStepDivAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AdvancedLooperAudioProcessorEditor)
};
