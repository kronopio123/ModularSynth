#pragma once
#include <juce_core/juce_core.h>
#include <cmath>

enum class LfoWaveform { Sine = 0, Triangle, Square, Saw };

class LFO
{
public:
    void setSampleRate (double sr) { sampleRate = sr; }
    void setWaveform (LfoWaveform w) { waveform = w; }

    void setRate (float rateHz)
    {
        rate = juce::jmax (0.01f, rateHz);
        phaseIncrement = rate / (float) sampleRate;
    }

    void resetPhase() { phase = 0.0f; }

    // returns bipolar value in [-1, 1]
    float getNextSample()
    {
        float value = 0.0f;
        switch (waveform)
        {
            case LfoWaveform::Sine:
                value = std::sin (phase * juce::MathConstants<float>::twoPi);
                break;
            case LfoWaveform::Triangle:
                value = 4.0f * std::abs (phase - 0.5f) - 1.0f;
                break;
            case LfoWaveform::Square:
                value = phase < 0.5f ? 1.0f : -1.0f;
                break;
            case LfoWaveform::Saw:
                value = 2.0f * phase - 1.0f;
                break;
        }

        phase += phaseIncrement;
        if (phase >= 1.0f)
            phase -= 1.0f;

        return value;
    }

private:
    double sampleRate = 44100.0;
    LfoWaveform waveform = LfoWaveform::Sine;
    float rate = 2.0f;
    float phase = 0.0f;
    float phaseIncrement = 0.0f;
};
