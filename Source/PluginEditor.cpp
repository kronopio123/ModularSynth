#include "PluginProcessor.h"
#include "PluginEditor.h"

void ModularSynthAudioProcessorEditor::setupKnob (Knob& knob, const juce::String& paramID, const juce::String& labelText)
{
    addAndMakeVisible (knob.slider);
    knob.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 16);
    knob.attachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, paramID, knob.slider);

    knob.label.setText (labelText, juce::dontSendNotification);
    knob.label.setJustificationType (juce::Justification::centred);
    knob.label.setFont (juce::Font (12.0f));
    addAndMakeVisible (knob.label);
}

void ModularSynthAudioProcessorEditor::setupChoice (Choice& choice, const juce::String& paramID, const juce::String& labelText)
{
    addAndMakeVisible (choice.box);
    if (auto* param = dynamic_cast<juce::AudioParameterChoice*> (audioProcessor.apvts.getParameter (paramID)))
        choice.box.addItemList (param->choices, 1);
    choice.attachment = std::make_unique<ComboAttachment> (audioProcessor.apvts, paramID, choice.box);

    choice.label.setText (labelText, juce::dontSendNotification);
    choice.label.setJustificationType (juce::Justification::centred);
    choice.label.setFont (juce::Font (12.0f));
    addAndMakeVisible (choice.label);
}

ModularSynthAudioProcessorEditor::ModularSynthAudioProcessorEditor (ModularSynthAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setupChoice (osc1Wave, "osc1Wave", "Osc 1 Wave");
    setupKnob (osc1Level, "osc1Level", "Osc 1 Level");
    setupKnob (osc1Semi, "osc1Semi", "Osc 1 Tune");

    setupChoice (osc2Wave, "osc2Wave", "Osc 2 Wave");
    setupKnob (osc2Level, "osc2Level", "Osc 2 Level");
    setupKnob (osc2Semi, "osc2Semi", "Osc 2 Tune");

    setupChoice (filterType, "filterType", "Filter Type");
    setupKnob (filterCutoff, "filterCutoff", "Cutoff");
    setupKnob (filterResonance, "filterResonance", "Resonance");

    setupKnob (ampA, "ampAttack", "Attack");
    setupKnob (ampD, "ampDecay", "Decay");
    setupKnob (ampS, "ampSustain", "Sustain");
    setupKnob (ampR, "ampRelease", "Release");

    setupKnob (modA, "modAttack", "Attack");
    setupKnob (modD, "modDecay", "Decay");
    setupKnob (modS, "modSustain", "Sustain");
    setupKnob (modR, "modRelease", "Release");

    setupChoice (lfoWave, "lfoWave", "LFO Wave");
    setupKnob (lfoRate, "lfoRate", "Rate");

    for (int i = 0; i < numModSlots; ++i)
    {
        auto suffix = juce::String (i + 1);
        setupChoice (modSource[i], "mod" + suffix + "Source", "Source " + suffix);
        setupChoice (modDest[i], "mod" + suffix + "Dest", "Dest " + suffix);
        setupKnob (modDepth[i], "mod" + suffix + "Depth", "Depth " + suffix);
    }

    setupKnob (masterGain, "masterGain", "Master");

    const char* sectionNames[] = { "OSCILLATORS", "FILTER", "AMP ENVELOPE", "MOD ENVELOPE", "LFO", "MOD MATRIX" };
    for (int i = 0; i < 6; ++i)
    {
        sectionLabels[i].setText (sectionNames[i], juce::dontSendNotification);
        sectionLabels[i].setFont (juce::Font (14.0f, juce::Font::bold));
        addAndMakeVisible (sectionLabels[i]);
    }

    setSize (900, 640);
}

void ModularSynthAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1e1e24));
}

// Simple helper: lays out a row of knob/choice components under a section label.
static juce::Rectangle<int> layoutRow (juce::Rectangle<int> area, int height)
{
    return area.removeFromTop (height);
}

void ModularSynthAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (10);
    const int knobW = 80, knobH = 90, comboH = 24, labelH = 18, gap = 10;

    auto placeChoice = [&] (Choice& c, juce::Rectangle<int> cell)
    {
        c.label.setBounds (cell.removeFromTop (labelH));
        c.box.setBounds (cell.removeFromTop (comboH));
    };

    auto placeKnob = [&] (Knob& k, juce::Rectangle<int> cell)
    {
        k.slider.setBounds (cell);
    };

    // OSCILLATORS section
    auto oscSection = layoutRow (area, 130);
    sectionLabels[0].setBounds (oscSection.removeFromTop (18));
    {
        auto row = oscSection;
        placeChoice (osc1Wave, row.removeFromLeft (knobW));
        placeKnob (osc1Level, row.removeFromLeft (knobW).withTrimmedTop (comboH + labelH));
        placeKnob (osc1Semi, row.removeFromLeft (knobW).withTrimmedTop (comboH + labelH));
        row.removeFromLeft (gap);
        placeChoice (osc2Wave, row.removeFromLeft (knobW));
        placeKnob (osc2Level, row.removeFromLeft (knobW).withTrimmedTop (comboH + labelH));
        placeKnob (osc2Semi, row.removeFromLeft (knobW).withTrimmedTop (comboH + labelH));
    }
    area.removeFromTop (gap);

    // FILTER section
    auto filterSection = layoutRow (area, 130);
    sectionLabels[1].setBounds (filterSection.removeFromTop (18));
    {
        auto row = filterSection;
        placeChoice (filterType, row.removeFromLeft (knobW));
        placeKnob (filterCutoff, row.removeFromLeft (knobW).withTrimmedTop (comboH + labelH));
        placeKnob (filterResonance, row.removeFromLeft (knobW).withTrimmedTop (comboH + labelH));
    }
    area.removeFromTop (gap);

    // AMP + MOD ENVELOPE side by side
    auto envSection = layoutRow (area, 130);
    auto ampArea = envSection.removeFromLeft (envSection.getWidth() / 2);
    auto modArea = envSection;
    sectionLabels[2].setBounds (ampArea.removeFromTop (18));
    sectionLabels[3].setBounds (modArea.removeFromTop (18));
    {
        auto row = ampArea;
        placeKnob (ampA, row.removeFromLeft (knobW));
        placeKnob (ampD, row.removeFromLeft (knobW));
        placeKnob (ampS, row.removeFromLeft (knobW));
        placeKnob (ampR, row.removeFromLeft (knobW));
    }
    {
        auto row = modArea;
        placeKnob (modA, row.removeFromLeft (knobW));
        placeKnob (modD, row.removeFromLeft (knobW));
        placeKnob (modS, row.removeFromLeft (knobW));
        placeKnob (modR, row.removeFromLeft (knobW));
    }
    area.removeFromTop (gap);

    // LFO + MASTER
    auto lfoSection = layoutRow (area, 110);
    sectionLabels[4].setBounds (lfoSection.removeFromTop (18));
    {
        auto row = lfoSection;
        placeChoice (lfoWave, row.removeFromLeft (knobW));
        placeKnob (lfoRate, row.removeFromLeft (knobW).withTrimmedTop (comboH + labelH));
        row.removeFromLeft (gap * 3);
        placeKnob (masterGain, row.removeFromLeft (knobW).withTrimmedTop (comboH + labelH));
    }
    area.removeFromTop (gap);

    // MOD MATRIX (3 slots, each: source / dest / depth)
    auto matrixSection = area;
    sectionLabels[5].setBounds (matrixSection.removeFromTop (18));
    int slotWidth = matrixSection.getWidth() / numModSlots;
    for (int i = 0; i < numModSlots; ++i)
    {
        auto slotArea = matrixSection.removeFromLeft (slotWidth).reduced (4);
        placeChoice (modSource[i], slotArea.removeFromTop (comboH + labelH));
        placeChoice (modDest[i], slotArea.removeFromTop (comboH + labelH));
        placeKnob (modDepth[i], slotArea);
    }
}
