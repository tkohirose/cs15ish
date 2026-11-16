#pragma once
#include <cmath>
#include <algorithm>

// Minimal-BLEP bandlimited oscillator producing saw / pulse / triangle,
// modelled after the CS-15's VCO waveform set (ramp, pulse w/ width, triangle).
class BlepOscillator
{
public:
    enum class Waveform { Saw, Pulse, Triangle };

    void setSampleRate (double sr) { sampleRate = sr; }
    void setWaveform (Waveform w)  { waveform = w; }
    void setPulseWidth (float pw)  { pulseWidth = std::clamp (pw, 0.02f, 0.98f); }
    void setFrequency (float hz)   { frequency = hz; }

    void reset() { phase = 0.0; lastOutput = 0.0f; }

    // Returns the next sample in roughly [-1, 1].
    float getNextSample()
    {
        const double inc = frequency / sampleRate;
        float out = 0.0f;

        switch (waveform)
        {
            case Waveform::Saw:
            {
                out = (float) (2.0 * phase - 1.0);
                out -= polyBlep (phase, inc);
                break;
            }
            case Waveform::Pulse:
            {
                out = phase < pulseWidth ? 1.0f : -1.0f;
                out += polyBlep (phase, inc);
                double shiftedPhase = phase + (1.0 - pulseWidth);
                shiftedPhase -= std::floor (shiftedPhase);
                out -= polyBlep (shiftedPhase, inc);
                break;
            }
            case Waveform::Triangle:
            {
                // Leaky-integrated square gives a smooth, bandlimited triangle.
                float square = phase < 0.5 ? 1.0f : -1.0f;
                square += polyBlep (phase, inc);
                double shiftedPhase = phase + 0.5;
                shiftedPhase -= std::floor (shiftedPhase);
                square -= polyBlep (shiftedPhase, inc);

                triIntegratorState = triIntegratorState * 0.999 + square * (float) inc * 4.0f;
                out = triIntegratorState;
                break;
            }
        }

        phase += inc;
        phase -= std::floor (phase);
        lastOutput = out;
        return out;
    }

private:
    static float polyBlep (double t, double dt)
    {
        if (t < dt)
        {
            double x = t / dt;
            return (float) (x + x - x * x - 1.0);
        }
        else if (t > 1.0 - dt)
        {
            double x = (t - 1.0) / dt;
            return (float) (x * x + x + x + 1.0);
        }
        return 0.0f;
    }

    double sampleRate = 44100.0;
    double phase = 0.0;
    float frequency = 440.0f;
    float pulseWidth = 0.5f;
    float lastOutput = 0.0f;
    float triIntegratorState = 0.0f;
    Waveform waveform = Waveform::Saw;
};
