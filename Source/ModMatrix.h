#pragma once

// Sources available for modulation
enum class ModSource { Off = 0, ModEnv, Lfo };

// Destinations a mod slot can be routed to
enum class ModDestination { Off = 0, Osc1Pitch, Osc2Pitch, FilterCutoff, OscMix };

struct ModSlot
{
    ModSource source = ModSource::Off;
    ModDestination destination = ModDestination::Off;
    float depth = 0.0f; // -1..1
};

static constexpr int numModSlots = 3;

// Computes the total modulation amount (already scaled by depth) that should be
// applied to a given destination, given current mod source values for this sample/block.
struct ModMatrixResult
{
    float osc1PitchSemis = 0.0f; // added to osc1 pitch, in semitones
    float osc2PitchSemis = 0.0f; // added to osc2 pitch, in semitones
    float filterCutoffOctaves = 0.0f; // added to filter cutoff, in octaves
    float oscMixOffset = 0.0f; // added to osc1/osc2 mix balance, -1..1 range
};

inline ModMatrixResult evaluateModMatrix (const ModSlot (&slots)[numModSlots],
                                           float modEnvValue,
                                           float lfoValue)
{
    ModMatrixResult result;

    for (auto& slot : slots)
    {
        if (slot.source == ModSource::Off || slot.destination == ModDestination::Off)
            continue;

        float sourceValue = (slot.source == ModSource::ModEnv) ? modEnvValue : lfoValue;
        float amount = sourceValue * slot.depth;

        switch (slot.destination)
        {
            case ModDestination::Osc1Pitch:
                result.osc1PitchSemis += amount * 24.0f; // up to +/-2 octaves
                break;
            case ModDestination::Osc2Pitch:
                result.osc2PitchSemis += amount * 24.0f;
                break;
            case ModDestination::FilterCutoff:
                result.filterCutoffOctaves += amount * 5.0f; // up to +/-5 octaves
                break;
            case ModDestination::OscMix:
                result.oscMixOffset += amount;
                break;
            default:
                break;
        }
    }

    return result;
}
