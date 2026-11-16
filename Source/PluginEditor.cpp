#include "PluginEditor.h"

CS15AudioProcessorEditor::CS15AudioProcessorEditor (CS15AudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    waveformBox.addItemList ({ "Saw", "Pulse", "Triangle" }, 1);
    addAndMakeVisible (waveformBox);
    waveformAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        processor.apvts, "waveform", waveformBox);

    waveformLabel.setText ("VCO", juce::dontSendNotification);
    waveformLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (waveformLabel);

    setupKnob (pulseWidthKnob, "pulseWidth", "Width");
    setupKnob (cutoffKnob,     "cutoff",     "Cutoff");
    setupKnob (resonanceKnob,  "resonance",  "Res");
    setupKnob (envAmountKnob,  "envAmount",  "Env Amt");
    setupKnob (attackKnob,     "attack",     "Attack");
    setupKnob (decayKnob,      "decay",      "Decay");
    setupKnob (sustainKnob,    "sustain",    "Sustain");
    setupKnob (releaseKnob,    "release",    "Release");
    setupKnob (glideKnob,      "glide",      "Glide");
    setupKnob (volumeKnob,     "volume",     "Volume");

    addAndMakeVisible (keyboard);

    setSize (760, 380);
}

void CS15AudioProcessorEditor::setupKnob (KnobWithLabel& knob, const juce::String& paramId, const juce::String& labelText)
{
    knob.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    knob.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 16);
    addAndMakeVisible (knob.slider);

    knob.label.setText (labelText, juce::dontSendNotification);
    knob.label.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (knob.label);

    knob.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.apvts, paramId, knob.slider);
}

void CS15AudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff2b2b2b));
    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (20.0f, juce::Font::bold));
    g.drawText ("CS-15 Style Synth (MVP)", getLocalBounds().removeFromTop (28),
                juce::Justification::centred);
}

void CS15AudioProcessorEditor::resized()
{
    auto full = getLocalBounds();
    auto keyboardArea = full.removeFromBottom (70);
    keyboard.setBounds (keyboardArea.reduced (10, 4));

    auto area = full.withTrimmedTop (34).reduced (10);

    auto waveArea = area.removeFromLeft (90);
    waveformLabel.setBounds (waveArea.removeFromTop (18));
    waveformBox.setBounds (waveArea.removeFromTop (24));

    KnobWithLabel* knobs[] = { &pulseWidthKnob, &cutoffKnob, &resonanceKnob, &envAmountKnob,
                               &attackKnob, &decayKnob, &sustainKnob, &releaseKnob,
                               &glideKnob, &volumeKnob };

    const int knobWidth = area.getWidth() / (int) (sizeof (knobs) / sizeof (knobs[0]));
    for (auto* k : knobs)
    {
        auto col = area.removeFromLeft (knobWidth);
        k->label.setBounds (col.removeFromTop (18));
        k->slider.setBounds (col.reduced (4));
    }
}
