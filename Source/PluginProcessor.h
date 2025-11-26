/*
  PROMASTER ONE - Reference-Based Mastering Plugin
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <vector>

namespace Constants
{
    constexpr int NUM_BANDS = 8;
    constexpr int FFT_ORDER = 12;
    constexpr int FFT_SIZE = 1 << FFT_ORDER;
    constexpr float MIN_DB = -60.0f;
    constexpr float MAX_DB = 12.0f;
    
    const std::array<float, NUM_BANDS + 1> BAND_FREQUENCIES = {
        20.0f, 60.0f, 250.0f, 500.0f, 1000.0f, 2000.0f, 6000.0f, 12000.0f, 20000.0f
    };
    
    const juce::StringArray BAND_NAMES = {
        "Sub", "Bass", "LowMid", "Mid", "UpMid", "Pres", "Brill", "Air"
    };
}

struct SpectrumData
{
    std::array<float, Constants::NUM_BANDS> bandLevels;
    float rmsLevel = Constants::MIN_DB;
    
    void reset()
    {
        bandLevels.fill(Constants::MIN_DB);
        rmsLevel = Constants::MIN_DB;
    }
};

struct ReferenceProfile
{
    juce::String name;
    std::array<float, Constants::NUM_BANDS> targetBandLevels;
    float targetLoudness = -14.0f;
    
    bool isValid() const { return name.isNotEmpty(); }
};

struct MasteringSuggestion
{
    juce::String message;
    juce::String messageJP;
    float severity;
};

class FFTAnalyzer
{
public:
    FFTAnalyzer() : fft(Constants::FFT_ORDER), 
                    window(Constants::FFT_SIZE, juce::dsp::WindowingFunction<float>::hann)
    {
        fftData.fill(0.0f);
        fifo.fill(0.0f);
    }
    
    void setSampleRate(double sr) { sampleRate = sr; }
    
    void pushSamples(const float* samples, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            fifo[(size_t)fifoIndex] = samples[i];
            fifoIndex++;
            
            if (fifoIndex >= Constants::FFT_SIZE)
            {
                std::copy(fifo.begin(), fifo.end(), fftData.begin());
                window.multiplyWithWindowingTable(fftData.data(), Constants::FFT_SIZE);
                fft.performFrequencyOnlyForwardTransform(fftData.data());
                fifoIndex = 0;
                newDataAvailable = true;
            }
        }
    }
    
    bool isNewDataAvailable() const { return newDataAvailable; }
    
    SpectrumData getSpectrum()
    {
        newDataAvailable = false;
        SpectrumData spectrum;
        
        float binWidth = static_cast<float>(sampleRate) / Constants::FFT_SIZE;
        
        for (int band = 0; band < Constants::NUM_BANDS; ++band)
        {
            float lowFreq = Constants::BAND_FREQUENCIES[(size_t)band];
            float highFreq = Constants::BAND_FREQUENCIES[(size_t)band + 1];
            
            int lowBin = juce::jlimit(0, Constants::FFT_SIZE / 2 - 1, static_cast<int>(lowFreq / binWidth));
            int highBin = juce::jlimit(lowBin + 1, Constants::FFT_SIZE / 2, static_cast<int>(highFreq / binWidth));
            
            float sum = 0.0f;
            for (int i = lowBin; i < highBin; ++i)
                sum += fftData[(size_t)i] * fftData[(size_t)i];
            
            float rms = std::sqrt(sum / std::max(1, highBin - lowBin));
            spectrum.bandLevels[(size_t)band] = juce::Decibels::gainToDecibels(rms / (Constants::FFT_SIZE / 2));
        }
        
        return spectrum;
    }
    
    void reset()
    {
        fifo.fill(0.0f);
        fftData.fill(0.0f);
        fifoIndex = 0;
        newDataAvailable = false;
    }
    
private:
    juce::dsp::FFT fft;
    juce::dsp::WindowingFunction<float> window;
    std::array<float, Constants::FFT_SIZE> fifo;
    std::array<float, Constants::FFT_SIZE * 2> fftData;
    int fifoIndex = 0;
    double sampleRate = 44100.0;
    bool newDataAvailable = false;
};

class PromasterOneAudioProcessor : public juce::AudioProcessor
{
public:
    PromasterOneAudioProcessor();
    ~PromasterOneAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    bool loadReferenceFromFile(const juce::File& file);
    
    const SpectrumData& getCurrentSpectrum() const { return currentSpectrum; }
    const ReferenceProfile& getReferenceProfile() const { return referenceProfile; }
    std::array<float, Constants::NUM_BANDS> getBandDifferences() const;
    std::vector<MasteringSuggestion> getSuggestions() const;
    
    void applyAdjust() { adjustAmount = 1.0f; }
    float getAdjustAmount() const { return adjustAmount; }
    
    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }
    
    const std::vector<ReferenceProfile>& getPresets() const { return presets; }
    void selectPreset(int index);
    
private:
    juce::AudioProcessorValueTreeState apvts;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    
    std::atomic<float>* bypassParam = nullptr;
    std::atomic<float>* inputGainParam = nullptr;
    std::atomic<float>* outputGainParam = nullptr;
    
    float adjustAmount = 0.0f;
    
    FFTAnalyzer fftAnalyzer;
    SpectrumData currentSpectrum;
    ReferenceProfile referenceProfile;
    
    std::array<float, Constants::NUM_BANDS> targetGains;
    
    std::vector<ReferenceProfile> presets;
    int activePresetIndex = -1;
    
    juce::AudioFormatManager formatManager;
    
    void loadBuiltInPresets();
    void calculateAdjustments();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PromasterOneAudioProcessor)
};
