#pragma once

#include "PluginProcessor.h"

class ModularSynthAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit ModularSynthAudioProcessorEditor (ModularSynthAudioProcessor&);
    ~ModularSynthAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    ModularSynthAudioProcessor& audioProcessor;

    struct Knob
    {
        juce::Slider slider { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow };
        juce::Label label;
        std::unique_ptr<SliderAttachment> attachment;
    };

    struct Choice
    {
        juce::ComboBox box;
        juce::Label label;
        std::unique_ptr<ComboAttachment> attachment;
    };

    void setupKnob (Knob& knob, const juce::String& paramID, const juce::String& labelText);
    void setupChoice (Choice& choice, const juce::String& paramID, const juce::String& labelText);

    // Oscillators
    Choice osc1Wave, osc2Wave;
    Knob osc1Level, osc1Semi, osc2Level, osc2Semi;

    // Filter
    Choice filterType;
    Knob filterCutoff, filterResonance;

    // Envelopes
    Knob ampA, ampD, ampS, ampR;
    Knob modA, modD, modS, modR;

    // LFO
    Choice lfoWave;
    Knob lfoRate;

    // Mod matrix
    Choice modSource[numModSlots];
    Choice modDest[numModSlots];
    Knob modDepth[numModSlots];

    Knob masterGain;

    juce::Label sectionLabels[6];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModularSynthAudioProcessorEditor)
};
