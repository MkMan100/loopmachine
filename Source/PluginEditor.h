#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class AdvancedLooperAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    explicit AdvancedLooperAudioProcessorEditor (AdvancedLooperAudioProcessor&);
    ~AdvancedLooperAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
private:
    AdvancedLooperAudioProcessor& audioProcessor;

    // Controlli UI
    juce::ComboBox stateComboBox;
    juce::ComboBox loopLengthComboBox;
    juce::Slider stepReduceSlider;
    juce::ToggleButton reverseButton { "REVERSE" };
    juce::Slider cutoffSlider;
    juce::Slider delayTimeSlider;
    juce::Slider delayFeedbackSlider;

    // Etichette
    juce::Label stateLabel { {}, "Mode" };
    juce::Label loopLengthLabel { {}, "Beats" };
    juce::Label stepReduceLabel { {}, "Step Reduction" };
    juce::Label cutoffLabel { {}, "Cutoff (Hz)" };
    juce::Label delayTimeLabel { {}, "Dub Delay Time (ms)" };
    juce::Label delayFeedbackLabel { {}, "Dub Feedback" };

    // Attachment APVTS per sincronizzazione bidirezionale
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using SliderAttachment   = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment   = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<ComboBoxAttachment> stateAttachment;
    std::unique_ptr<ComboBoxAttachment> loopLengthAttachment;
    std::unique_ptr<SliderAttachment>   stepReduceAttachment;
    std::unique_ptr<ButtonAttachment>   reverseAttachment;
    std::unique_ptr<SliderAttachment>   cutoffAttachment;
    std::unique_ptr<SliderAttachment>   delayTimeAttachment;
    std::unique_ptr<SliderAttachment>   delayFeedbackAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AdvancedLooperAudioProcessorEditor)
};
