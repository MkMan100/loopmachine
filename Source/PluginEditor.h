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

    // --- Manopole UI (Rotary Sliders) ---
    juce::Slider stateSlider;
    juce::Slider loopLengthSlider;
    juce::Slider stepReduceSlider;
    juce::Slider cutoffSlider;
    juce::Slider delayTimeSlider;
    juce::Slider delayFeedbackSlider;

    juce::ToggleButton reverseButton { "REVERSE" };

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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AdvancedLooperAudioProcessorEditor)
};
