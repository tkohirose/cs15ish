#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "DSP/BlepOscillator.h"
#include "DSP/LadderFilter.h"

class CS15AudioProcessor : public juce::AudioProcessor
{
public:
    CS15AudioProcessor();
    ~CS15AudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    void handleMidiEvent (const juce::MidiMessage& msg);
    void startNote (int midiNote, float velocity);
    void stopNote (int midiNote);
    float midiNoteToFreq (float note) const { return 440.0f * std::pow (2.0f, (note - 69.0f) / 12.0f); }

    BlepOscillator osc;
    LadderFilter filter;
    juce::ADSR envelope;
    juce::ADSR::Parameters envParams;

    std::vector<int> heldNotes; // last-note-priority stack, mono
    bool noteIsOn = false;
    int currentNote = -1;
    float currentFreq = 440.0f;
    float targetFreq = 440.0f;
    float glideCoeff = 0.0f;
    double sampleRate = 44100.0;

    std::atomic<float>* waveformParam = nullptr;
    std::atomic<float>* pulseWidthParam = nullptr;
    std::atomic<float>* cutoffParam = nullptr;
    std::atomic<float>* resonanceParam = nullptr;
    std::atomic<float>* envAmountParam = nullptr;
    std::atomic<float>* attackParam = nullptr;
    std::atomic<float>* decayParam = nullptr;
    std::atomic<float>* sustainParam = nullptr;
    std::atomic<float>* releaseParam = nullptr;
    std::atomic<float>* glideParam = nullptr;
    std::atomic<float>* volumeParam = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CS15AudioProcessor)
};
