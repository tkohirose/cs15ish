#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

class CS15AudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit CS15AudioProcessorEditor (CS15AudioProcessor&);
    ~CS15AudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct KnobWithLabel
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    void setupKnob (KnobWithLabel& knob, const juce::String& paramId, const juce::String& labelText);

    CS15AudioProcessor& processor;

    juce::ComboBox waveformBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> waveformAttachment;
    juce::Label waveformLabel;

    KnobWithLabel pulseWidthKnob, cutoffKnob, resonanceKnob, envAmountKnob,
                  attackKnob, decayKnob, sustainKnob, releaseKnob, glideKnob, volumeKnob;

    juce::MidiKeyboardComponent keyboard;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CS15AudioProcessorEditor)
};
