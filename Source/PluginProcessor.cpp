/*
  PROMASTER ONE - Reference-Based Mastering Plugin
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

PromasterOneAudioProcessor::PromasterOneAudioProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    bypassParam = apvts.getRawParameterValue("bypass");
    inputGainParam = apvts.getRawParameterValue("inputGain");
    outputGainParam = apvts.getRawParameterValue("outputGain");
    
    formatManager.registerBasicFormats();
    targetGains.fill(0.0f);
    currentSpectrum.reset();
    loadBuiltInPresets();
}

PromasterOneAudioProcessor::~PromasterOneAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout PromasterOneAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("bypass", 1), "Bypass", false));
    
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("inputGain", 1), "Input Gain",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f));
    
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("outputGain", 1), "Output Gain",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f));
    
    return { params.begin(), params.end() };
}

void PromasterOneAudioProcessor::loadBuiltInPresets()
{
    ReferenceProfile popMaster;
    popMaster.name = "Pop Master 2024";
    popMaster.targetLoudness = -14.0f;
    popMaster.targetBandLevels = { -18.0f, -14.0f, -10.0f, -8.5f, -9.0f, -10.5f, -14.0f, -20.0f };
    presets.push_back(popMaster);
    
    ReferenceProfile rockEnergy;
    rockEnergy.name = "Rock Energy";
    rockEnergy.targetLoudness = -12.0f;
    rockEnergy.targetBandLevels = { -16.0f, -10.0f, -7.0f, -6.0f, -7.0f, -9.0f, -12.0f, -18.0f };
    presets.push_back(rockEnergy);
    
    ReferenceProfile jazzWarmth;
    jazzWarmth.name = "Jazz Warmth";
    jazzWarmth.targetLoudness = -18.0f;
    jazzWarmth.targetBandLevels = { -20.0f, -16.0f, -12.0f, -10.0f, -11.0f, -13.0f, -17.0f, -24.0f };
    presets.push_back(jazzWarmth);
    
    ReferenceProfile electronicPunch;
    electronicPunch.name = "Electronic Punch";
    electronicPunch.targetLoudness = -10.0f;
    electronicPunch.targetBandLevels = { -12.0f, -8.0f, -10.0f, -9.0f, -8.0f, -7.0f, -10.0f, -16.0f };
    presets.push_back(electronicPunch);
}

const juce::String PromasterOneAudioProcessor::getName() const { return "PROMASTER ONE"; }
bool PromasterOneAudioProcessor::acceptsMidi() const { return false; }
bool PromasterOneAudioProcessor::producesMidi() const { return false; }
bool PromasterOneAudioProcessor::isMidiEffect() const { return false; }
double PromasterOneAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int PromasterOneAudioProcessor::getNumPrograms() { return static_cast<int>(presets.size()); }
int PromasterOneAudioProcessor::getCurrentProgram() { return activePresetIndex >= 0 ? activePresetIndex : 0; }

void PromasterOneAudioProcessor::setCurrentProgram(int index) { selectPreset(index); }

const juce::String PromasterOneAudioProcessor::getProgramName(int index)
{
    if (index >= 0 && index < static_cast<int>(presets.size()))
        return presets[(size_t)index].name;
    return {};
}

void PromasterOneAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
    if (index >= 0 && index < static_cast<int>(presets.size()))
        presets[(size_t)index].name = newName;
}

void PromasterOneAudioProcessor::prepareToPlay(double sampleRate, int)
{
    fftAnalyzer.setSampleRate(sampleRate);
}

void PromasterOneAudioProcessor::releaseResources()
{
    fftAnalyzer.reset();
}

bool PromasterOneAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
    return true;
}

void PromasterOneAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    
    if (bypassParam->load() > 0.5f)
        return;

    float inGain = juce::Decibels::decibelsToGain(inputGainParam->load());
    if (inGain != 1.0f)
        buffer.applyGain(inGain);

    fftAnalyzer.pushSamples(buffer.getReadPointer(0), buffer.getNumSamples());
    if (fftAnalyzer.isNewDataAvailable())
        currentSpectrum = fftAnalyzer.getSpectrum();

    if (adjustAmount > 0.0f && referenceProfile.isValid())
    {
        calculateAdjustments();
        
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            auto* channelData = buffer.getWritePointer(channel);
            
            for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            {
                float input = channelData[sample];
                float output = input;
                
                for (int band = 0; band < Constants::NUM_BANDS; ++band)
                {
                    float gain = juce::Decibels::decibelsToGain(targetGains[(size_t)band] * adjustAmount * 0.3f);
                    output *= std::pow(gain, 1.0f / Constants::NUM_BANDS);
                }
                
                channelData[sample] = juce::jmap(adjustAmount, input, output);
            }
        }
    }

    float outGain = juce::Decibels::decibelsToGain(outputGainParam->load());
    if (outGain != 1.0f)
        buffer.applyGain(outGain);

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            channelData[sample] = std::tanh(channelData[sample]);
    }
}

bool PromasterOneAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* PromasterOneAudioProcessor::createEditor()
{
    return new PromasterOneAudioProcessorEditor(*this);
}

void PromasterOneAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void PromasterOneAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

bool PromasterOneAudioProcessor::loadReferenceFromFile(const juce::File& file)
{
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    
    if (reader == nullptr)
        return false;
    
    juce::AudioBuffer<float> buffer(static_cast<int>(reader->numChannels),
                                     static_cast<int>(std::min(reader->lengthInSamples, (juce::int64)reader->sampleRate * 30)));
    reader->read(&buffer, 0, buffer.getNumSamples(), 0, true, true);
    
    FFTAnalyzer refAnalyzer;
    refAnalyzer.setSampleRate(reader->sampleRate);
    
    for (int i = 0; i < buffer.getNumSamples(); i += Constants::FFT_SIZE)
    {
        int chunkSize = std::min(Constants::FFT_SIZE, buffer.getNumSamples() - i);
        refAnalyzer.pushSamples(buffer.getReadPointer(0) + i, chunkSize);
    }
    
    if (refAnalyzer.isNewDataAvailable())
    {
        auto refSpectrum = refAnalyzer.getSpectrum();
        referenceProfile.name = file.getFileNameWithoutExtension();
        referenceProfile.targetBandLevels = refSpectrum.bandLevels;
        referenceProfile.targetLoudness = refSpectrum.rmsLevel;
        calculateAdjustments();
        return true;
    }
    
    return false;
}

void PromasterOneAudioProcessor::calculateAdjustments()
{
    if (!referenceProfile.isValid())
        return;
    
    for (int i = 0; i < Constants::NUM_BANDS; ++i)
    {
        targetGains[(size_t)i] = referenceProfile.targetBandLevels[(size_t)i] - currentSpectrum.bandLevels[(size_t)i];
        targetGains[(size_t)i] = juce::jlimit(-12.0f, 12.0f, targetGains[(size_t)i]);
    }
}

std::array<float, Constants::NUM_BANDS> PromasterOneAudioProcessor::getBandDifferences() const
{
    std::array<float, Constants::NUM_BANDS> diff;
    
    if (!referenceProfile.isValid())
    {
        diff.fill(0.0f);
        return diff;
    }
    
    for (int i = 0; i < Constants::NUM_BANDS; ++i)
        diff[(size_t)i] = referenceProfile.targetBandLevels[(size_t)i] - currentSpectrum.bandLevels[(size_t)i];
    
    return diff;
}

std::vector<MasteringSuggestion> PromasterOneAudioProcessor::getSuggestions() const
{
    std::vector<MasteringSuggestion> suggestions;
    
    if (!referenceProfile.isValid())
    {
        suggestions.push_back({"Load a reference track", 
                               "リファレンストラックを読み込んでください", 0.0f});
        return suggestions;
    }
    
    auto diff = getBandDifferences();
    
    float bassDiff = (diff[0] + diff[1]) / 2.0f;
    if (bassDiff > 3.0f)
        suggestions.push_back({"Bass is lacking", "低音が不足しています", 
                               std::min(1.0f, std::abs(bassDiff) / 12.0f)});
    else if (bassDiff < -3.0f)
        suggestions.push_back({"Bass is too loud", "低音が過剰です", 
                               std::min(1.0f, std::abs(bassDiff) / 12.0f)});
    
    float midDiff = (diff[3] + diff[4]) / 2.0f;
    if (midDiff > 3.0f)
        suggestions.push_back({"Midrange lacks presence", "中域が不足しています", 
                               std::min(1.0f, std::abs(midDiff) / 12.0f)});
    else if (midDiff < -3.0f)
        suggestions.push_back({"Midrange is too prominent", "中域が突出しています", 
                               std::min(1.0f, std::abs(midDiff) / 12.0f)});
    
    float highDiff = (diff[6] + diff[7]) / 2.0f;
    if (highDiff > 3.0f)
        suggestions.push_back({"High frequencies lacking", "高域が不足しています", 
                               std::min(1.0f, std::abs(highDiff) / 12.0f)});
    else if (highDiff < -3.0f)
        suggestions.push_back({"High frequencies too bright", "高域が明るすぎます", 
                               std::min(1.0f, std::abs(highDiff) / 12.0f)});
    
    if (suggestions.empty())
        suggestions.push_back({"Mix is well balanced!", "ミックスは良好です！", 0.0f});
    
    return suggestions;
}

void PromasterOneAudioProcessor::selectPreset(int index)
{
    if (index >= 0 && index < static_cast<int>(presets.size()))
    {
        activePresetIndex = index;
        referenceProfile = presets[(size_t)index];
        calculateAdjustments();
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PromasterOneAudioProcessor();
}
