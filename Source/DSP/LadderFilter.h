#pragma once
#include <cmath>
#include <algorithm>

// A cheap 4-pole transistor-ladder-style lowpass (24 dB/oct), in the spirit
// of the CS-15's resonant VCF. Not a precise circuit model, just a simple
// nonlinear feedback cascade that gets the character (self-oscillation-ish
// resonance, soft saturation into the ladder) without heavy CPU cost.
class LadderFilter
{
public:
    void setSampleRate (double sr) { sampleRate = sr; }

    void reset()
    {
        for (auto& s : stage) s = 0.0f;
        feedback = 0.0f;
    }

    // cutoffHz: 20..20000, resonance: 0..1 (approaching self-oscillation near 1)
    void setParams (float cutoffHz, float resonance)
    {
        cutoff = std::clamp (cutoffHz, 20.0f, 0.49f * (float) sampleRate);
        res = std::clamp (resonance, 0.0f, 1.0f);
    }

    float processSample (float input)
    {
        // Normalized cutoff -> one-pole coefficient per stage.
        const float wc = 2.0f * (float) M_PI * cutoff / (float) sampleRate;
        const float g = std::clamp (wc, 0.0f, 1.2f);

        // Resonance feeds the last stage's output back to the input,
        // scaled so it can approach self-oscillation at res ~= 1.
        const float resAmount = res * 4.0f;
        float x = input - resAmount * feedback;

        // Soft clip entering the ladder, like a transistor stage softly saturating.
        x = std::tanh (x);

        for (int i = 0; i < 4; ++i)
        {
            stage[i] += g * (x - stage[i]);
            x = stage[i];
        }

        feedback = stage[3];
        return stage[3];
    }

private:
    double sampleRate = 44100.0;
    float cutoff = 1000.0f;
    float res = 0.0f;
    float stage[4] { 0.0f, 0.0f, 0.0f, 0.0f };
    float feedback = 0.0f;
};
