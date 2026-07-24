#pragma once
#include <juce_core/juce_core.h>
#include <cmath>

enum class OscWaveform { Sine = 0, Saw, Square, Triangle };

class Oscillator
{
public:
    void setSampleRate (double sr) { sampleRate = sr; }

    void setWaveform (OscWaveform w) { waveform = w; }

    // frequency in Hz, semitone offset applied by caller before calling this
    void setFrequency (float freqHz)
    {
        frequency = juce::jmax (0.0f, freqHz);
        phaseIncrement = frequency / (float) sampleRate;
    }

    void resetPhase (float startPhase = 0.0f) { phase = startPhase; }

    float getNextSample()
    {
        float value = 0.0f;
        switch (waveform)
        {
            case OscWaveform::Sine:
                value = std::sin (phase * juce::MathConstants<float>::twoPi);
                break;
            case OscWaveform::Saw:
                value = 2.0f * phase - 1.0f;
                break;
            case OscWaveform::Square:
                value = phase < 0.5f ? 1.0f : -1.0f;
                break;
            case OscWaveform::Triangle:
                value = 4.0f * std::abs (phase - 0.5f) - 1.0f;
                break;
        }

        phase += phaseIncrement;
        if (phase >= 1.0f)
            phase -= 1.0f;

        return value;
    }

private:
    double sampleRate = 44100.0;
    OscWaveform waveform = OscWaveform::Saw;
    float frequency = 440.0f;
    float phase = 0.0f;
    float phaseIncrement = 0.0f;
};
