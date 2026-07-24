#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
void SynthVoice::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    osc1.setSampleRate (sampleRate);
    osc2.setSampleRate (sampleRate);
    lfo.setSampleRate (sampleRate);
    ampEnv.setSampleRate (sampleRate);
    modEnv.setSampleRate (sampleRate);
    filter.prepare (spec);
}

void SynthVoice::startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound*, int)
{
    currentMidiNote = midiNoteNumber;
    velocityLevel = velocity;
    baseFrequency = (float) juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber);

    osc1.resetPhase();
    osc2.resetPhase();
    lfo.resetPhase();
    ampEnv.noteOn();
    modEnv.noteOn();
}

void SynthVoice::stopNote (float, bool allowTailOff)
{
    if (allowTailOff)
    {
        ampEnv.noteOff();
        modEnv.noteOff();
    }
    else
    {
        clearCurrentNote();
        ampEnv.reset();
        modEnv.reset();
    }
}

void SynthVoice::renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    if (currentMidiNote < 0)
        return;

    auto& apvts = processor.apvts;

    // Pull current parameter values once per block (block-rate modulation is fine for this synth).
    auto osc1Wave = (OscWaveform) (int) apvts.getRawParameterValue ("osc1Wave")->load();
    auto osc2Wave = (OscWaveform) (int) apvts.getRawParameterValue ("osc2Wave")->load();
    float osc1Level = apvts.getRawParameterValue ("osc1Level")->load();
    float osc2Level = apvts.getRawParameterValue ("osc2Level")->load();
    float osc1Semi = apvts.getRawParameterValue ("osc1Semi")->load();
    float osc2Semi = apvts.getRawParameterValue ("osc2Semi")->load();

    int filterTypeChoice = (int) apvts.getRawParameterValue ("filterType")->load();
    float filterCutoff = apvts.getRawParameterValue ("filterCutoff")->load();
    float filterResonance = apvts.getRawParameterValue ("filterResonance")->load();

    float ampA = apvts.getRawParameterValue ("ampAttack")->load();
    float ampD = apvts.getRawParameterValue ("ampDecay")->load();
    float ampS = apvts.getRawParameterValue ("ampSustain")->load();
    float ampR = apvts.getRawParameterValue ("ampRelease")->load();

    float modA = apvts.getRawParameterValue ("modAttack")->load();
    float modD = apvts.getRawParameterValue ("modDecay")->load();
    float modS = apvts.getRawParameterValue ("modSustain")->load();
    float modR = apvts.getRawParameterValue ("modRelease")->load();

    auto lfoWave = (LfoWaveform) (int) apvts.getRawParameterValue ("lfoWave")->load();
    float lfoRate = apvts.getRawParameterValue ("lfoRate")->load();

    float masterGain = apvts.getRawParameterValue ("masterGain")->load();

    ampEnv.setParameters ({ ampA, ampD, ampS, ampR });
    modEnv.setParameters ({ modA, modD, modS, modR });
    osc1.setWaveform (osc1Wave);
    osc2.setWaveform (osc2Wave);
    lfo.setWaveform (lfoWave);
    lfo.setRate (lfoRate);

    switch (filterTypeChoice)
    {
        case 0: filter.setType (juce::dsp::StateVariableTPTFilterType::lowpass); break;
        case 1: filter.setType (juce::dsp::StateVariableTPTFilterType::bandpass); break;
        default: filter.setType (juce::dsp::StateVariableTPTFilterType::highpass); break;
    }
    filter.setResonance (filterResonance);

    for (int sample = 0; sample < numSamples; ++sample)
    {
        float modEnvValue = modEnv.getNextSample();
        float lfoValue = lfo.getNextSample();

        auto modResult = evaluateModMatrix (processor.modSlots, modEnvValue, lfoValue);

        float osc1Freq = baseFrequency * std::pow (2.0f, (osc1Semi + modResult.osc1PitchSemis) / 12.0f);
        float osc2Freq = baseFrequency * std::pow (2.0f, (osc2Semi + modResult.osc2PitchSemis) / 12.0f);
        osc1.setFrequency (osc1Freq);
        osc2.setFrequency (osc2Freq);

        float mixBalance = juce::jlimit (0.0f, 1.0f, 0.5f + modResult.oscMixOffset);
        float oscSum = osc1.getNextSample() * osc1Level * (1.0f - mixBalance) * 2.0f
                     + osc2.getNextSample() * osc2Level * mixBalance * 2.0f;

        float cutoffHz = filterCutoff * std::pow (2.0f, modResult.filterCutoffOctaves);
        cutoffHz = juce::jlimit (20.0f, 20000.0f, cutoffHz);
        filter.setCutoffFrequency (cutoffHz);

        float filtered = filter.processSample (0, oscSum);

        float envValue = ampEnv.getNextSample();
        float outSample = filtered * envValue * velocityLevel * masterGain;

        for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch)
            outputBuffer.addSample (ch, startSample + sample, outSample);

        if (! ampEnv.isActive())
        {
            clearCurrentNote();
            currentMidiNote = -1;
            break;
        }
    }
}

//==============================================================================
ModularSynthAudioProcessor::ModularSynthAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    for (int i = 0; i < 16; ++i)
        synth.addVoice (new SynthVoice (*this));

    synth.addSound (new SynthSound());
}

juce::AudioProcessorValueTreeState::ParameterLayout ModularSynthAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    auto waveChoices = juce::StringArray { "Sine", "Saw", "Square", "Triangle" };
    auto lfoWaveChoices = juce::StringArray { "Sine", "Triangle", "Square", "Saw" };
    auto filterTypeChoices = juce::StringArray { "Low Pass", "Band Pass", "High Pass" };
    auto modSourceChoices = juce::StringArray { "Off", "Mod Env", "LFO" };
    auto modDestChoices = juce::StringArray { "Off", "Osc 1 Pitch", "Osc 2 Pitch", "Filter Cutoff", "Osc Mix" };

    params.push_back (std::make_unique<juce::AudioParameterChoice> ("osc1Wave", "Osc 1 Waveform", waveChoices, 1));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("osc1Level", "Osc 1 Level", 0.0f, 1.0f, 0.8f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("osc1Semi", "Osc 1 Tune (semi)", -24.0f, 24.0f, 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterChoice> ("osc2Wave", "Osc 2 Waveform", waveChoices, 1));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("osc2Level", "Osc 2 Level", 0.0f, 1.0f, 0.5f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("osc2Semi", "Osc 2 Tune (semi)", -24.0f, 24.0f, 7.0f));

    params.push_back (std::make_unique<juce::AudioParameterChoice> ("filterType", "Filter Type", filterTypeChoices, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("filterCutoff",
        "Filter Cutoff",
        juce::NormalisableRange<float> (20.0f, 20000.0f, 1.0f, 0.3f), 2000.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("filterResonance", "Filter Resonance", 0.1f, 10.0f, 0.7f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> ("ampAttack", "Amp Attack", 0.001f, 5.0f, 0.01f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("ampDecay", "Amp Decay", 0.001f, 5.0f, 0.2f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("ampSustain", "Amp Sustain", 0.0f, 1.0f, 0.8f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("ampRelease", "Amp Release", 0.001f, 5.0f, 0.3f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> ("modAttack", "Mod Env Attack", 0.001f, 5.0f, 0.01f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("modDecay", "Mod Env Decay", 0.001f, 5.0f, 0.3f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("modSustain", "Mod Env Sustain", 0.0f, 1.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("modRelease", "Mod Env Release", 0.001f, 5.0f, 0.3f));

    params.push_back (std::make_unique<juce::AudioParameterChoice> ("lfoWave", "LFO Waveform", lfoWaveChoices, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("lfoRate",
        "LFO Rate",
        juce::NormalisableRange<float> (0.01f, 20.0f, 0.01f, 0.4f), 2.0f));

    for (int i = 0; i < numModSlots; ++i)
    {
        auto suffix = juce::String (i + 1);
        params.push_back (std::make_unique<juce::AudioParameterChoice> ("mod" + suffix + "Source", "Mod " + suffix + " Source", modSourceChoices, 0));
        params.push_back (std::make_unique<juce::AudioParameterChoice> ("mod" + suffix + "Dest", "Mod " + suffix + " Destination", modDestChoices, 0));
        params.push_back (std::make_unique<juce::AudioParameterFloat> ("mod" + suffix + "Depth", "Mod " + suffix + " Depth", -1.0f, 1.0f, 0.0f));
    }

    params.push_back (std::make_unique<juce::AudioParameterFloat> ("masterGain", "Master Gain", 0.0f, 1.0f, 0.7f));

    return { params.begin(), params.end() };
}

void ModularSynthAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    synth.setCurrentPlaybackSampleRate (sampleRate);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32) samplesPerBlock;
    spec.numChannels = 2;

    for (int i = 0; i < synth.getNumVoices(); ++i)
        if (auto* voice = dynamic_cast<SynthVoice*> (synth.getVoice (i)))
            voice->prepare (spec);
}

void ModularSynthAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    // Sync mod matrix slots from parameters into the plain struct read by voices.
    for (int i = 0; i < numModSlots; ++i)
    {
        auto suffix = juce::String (i + 1);
        modSlots[i].source = (ModSource) (int) apvts.getRawParameterValue ("mod" + suffix + "Source")->load();
        modSlots[i].destination = (ModDestination) (int) apvts.getRawParameterValue ("mod" + suffix + "Dest")->load();
        modSlots[i].depth = apvts.getRawParameterValue ("mod" + suffix + "Depth")->load();
    }

    synth.renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());
}

juce::AudioProcessorEditor* ModularSynthAudioProcessor::createEditor()
{
    return new ModularSynthAudioProcessorEditor (*this);
}

void ModularSynthAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); true)
    {
        std::unique_ptr<juce::XmlElement> xml (state.createXml());
        copyXmlToBinary (*xml, destData);
    }
}

void ModularSynthAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ModularSynthAudioProcessor();
}
