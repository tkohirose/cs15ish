#include "PluginProcessor.h"
#include "PluginEditor.h"

CS15AudioProcessor::CS15AudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createParameterLayout())
{
    waveformParam   = apvts.getRawParameterValue ("waveform");
    pulseWidthParam = apvts.getRawParameterValue ("pulseWidth");
    cutoffParam     = apvts.getRawParameterValue ("cutoff");
    resonanceParam  = apvts.getRawParameterValue ("resonance");
    envAmountParam  = apvts.getRawParameterValue ("envAmount");
    attackParam     = apvts.getRawParameterValue ("attack");
    decayParam      = apvts.getRawParameterValue ("decay");
    sustainParam    = apvts.getRawParameterValue ("sustain");
    releaseParam    = apvts.getRawParameterValue ("release");
    glideParam      = apvts.getRawParameterValue ("glide");
    volumeParam     = apvts.getRawParameterValue ("volume");
}

juce::AudioProcessorValueTreeState::ParameterLayout CS15AudioProcessor::createParameterLayout()
{
    using Range = juce::NormalisableRange<float>;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "waveform", "VCO Waveform", juce::StringArray { "Saw", "Pulse", "Triangle" }, 0));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "pulseWidth", "Pulse Width", Range (0.02f, 0.98f, 0.001f), 0.5f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "cutoff", "VCF Cutoff", Range (20.0f, 20000.0f, 1.0f, 0.3f), 2000.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "resonance", "VCF Resonance", Range (0.0f, 1.0f, 0.001f), 0.2f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "envAmount", "Filter Env Amount", Range (0.0f, 1.0f, 0.001f), 0.5f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "attack", "Attack", Range (0.001f, 5.0f, 0.001f, 0.3f), 0.005f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "decay", "Decay", Range (0.001f, 5.0f, 0.001f, 0.3f), 0.15f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "sustain", "Sustain", Range (0.0f, 1.0f, 0.001f), 0.7f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "release", "Release", Range (0.001f, 5.0f, 0.001f, 0.3f), 0.2f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "glide", "Portamento", Range (0.0f, 2.0f, 0.001f, 0.3f), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "volume", "Volume", Range (0.0f, 1.0f, 0.001f), 0.7f));

    return { params.begin(), params.end() };
}

void CS15AudioProcessor::prepareToPlay (double newSampleRate, int)
{
    sampleRate = newSampleRate;
    osc.setSampleRate (sampleRate);
    osc.reset();
    filter.setSampleRate (sampleRate);
    filter.reset();
    envelope.setSampleRate (sampleRate);
    heldNotes.clear();
    noteIsOn = false;
}

bool CS15AudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void CS15AudioProcessor::handleMidiEvent (const juce::MidiMessage& msg)
{
    if (msg.isNoteOn())
        startNote (msg.getNoteNumber(), msg.getFloatVelocity());
    else if (msg.isNoteOff())
        stopNote (msg.getNoteNumber());
    else if (msg.isAllNotesOff() || msg.isAllSoundOff())
    {
        heldNotes.clear();
        noteIsOn = false;
        envelope.noteOff();
    }
}

void CS15AudioProcessor::startNote (int midiNote, float /*velocity*/)
{
    heldNotes.erase (std::remove (heldNotes.begin(), heldNotes.end(), midiNote), heldNotes.end());
    heldNotes.push_back (midiNote);

    currentNote = midiNote;
    targetFreq = midiNoteToFreq ((float) midiNote);
    if (! noteIsOn)
        currentFreq = targetFreq; // no glide into the very first note

    noteIsOn = true;
    envelope.noteOn();
}

void CS15AudioProcessor::stopNote (int midiNote)
{
    heldNotes.erase (std::remove (heldNotes.begin(), heldNotes.end(), midiNote), heldNotes.end());

    if (heldNotes.empty())
    {
        noteIsOn = false;
        envelope.noteOff();
    }
    else if (midiNote == currentNote)
    {
        // Mono last-note priority: fall back to the previous held note.
        currentNote = heldNotes.back();
        targetFreq = midiNoteToFreq ((float) currentNote);
    }
}

void CS15AudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    keyboardState.processNextMidiBuffer (midiMessages, 0, buffer.getNumSamples(), true);

    const float glideTime = glideParam->load();
    glideCoeff = glideTime > 0.0001f
        ? 1.0f - std::exp (-1.0f / ((float) sampleRate * glideTime))
        : 1.0f;

    osc.setWaveform (static_cast<BlepOscillator::Waveform> ((int) waveformParam->load()));
    osc.setPulseWidth (pulseWidthParam->load());

    envParams.attack  = attackParam->load();
    envParams.decay   = decayParam->load();
    envParams.sustain = sustainParam->load();
    envParams.release = releaseParam->load();
    envelope.setParameters (envParams);

    const float baseCutoff = cutoffParam->load();
    const float resonance  = resonanceParam->load();
    const float envAmount  = envAmountParam->load();
    const float volume     = volumeParam->load();

    auto* left  = buffer.getWritePointer (0);
    auto* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;

    int sampleIndex = 0;
    for (const auto metadata : midiMessages)
    {
        const int eventSample = metadata.samplePosition;
        for (; sampleIndex < eventSample && sampleIndex < buffer.getNumSamples(); ++sampleIndex)
        {
            currentFreq += glideCoeff * (targetFreq - currentFreq);
            osc.setFrequency (currentFreq);

            const float envVal = envelope.getNextSample();
            const float cutoffHz = baseCutoff * std::pow (2.0f, envAmount * envVal * 4.0f);
            filter.setParams (cutoffHz, resonance);

            const float raw = osc.getNextSample();
            const float sample = filter.processSample (raw) * envVal * volume;

            left[sampleIndex] = sample;
            if (right != nullptr) right[sampleIndex] = sample;
        }

        handleMidiEvent (metadata.getMessage());
    }

    for (; sampleIndex < buffer.getNumSamples(); ++sampleIndex)
    {
        currentFreq += glideCoeff * (targetFreq - currentFreq);
        osc.setFrequency (currentFreq);

        const float envVal = envelope.getNextSample();
        const float cutoffHz = baseCutoff * std::pow (2.0f, envAmount * envVal * 4.0f);
        filter.setParams (cutoffHz, resonance);

        const float raw = osc.getNextSample();
        const float sample = filter.processSample (raw) * envVal * volume;

        left[sampleIndex] = sample;
        if (right != nullptr) right[sampleIndex] = sample;
    }
}

juce::AudioProcessorEditor* CS15AudioProcessor::createEditor()
{
    return new CS15AudioProcessorEditor (*this);
}

void CS15AudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState().createXml())
        copyXmlToBinary (*state, destData);
}

void CS15AudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CS15AudioProcessor();
}
