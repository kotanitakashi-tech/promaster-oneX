/*
 * PROMASTER ONE - Reference-Based Mastering VST Plugin
 * Copyright (c) 2025
 * 
 * A VST3 plugin for Cakewalk Sonar that analyzes reference tracks
 * and applies intelligent mastering to match the target sound.
 */

#pragma once

#include <vector>
#include <array>
#include <string>
#include <memory>
#include <complex>
#include <cmath>
#include <atomic>
#include <mutex>

namespace PromasterOne {

// Constants
constexpr int NUM_BANDS = 8;
constexpr int FFT_SIZE = 4096;
constexpr int OVERLAP = 4;
constexpr float MIN_DB = -60.0f;
constexpr float MAX_DB = 12.0f;

// Band frequency ranges (Hz)
constexpr std::array<float, NUM_BANDS + 1> BAND_FREQUENCIES = {
    20.0f, 60.0f, 250.0f, 500.0f, 1000.0f, 2000.0f, 6000.0f, 12000.0f, 20000.0f
};

// Band names for display
constexpr const char* BAND_NAMES[NUM_BANDS] = {
    "Sub Bass", "Bass", "Low Mid", "Mid", "Upper Mid", "Presence", "Brilliance", "Air"
};

//------------------------------------------------------------------------------
// Spectrum Analysis Data
//------------------------------------------------------------------------------
struct SpectrumData {
    std::array<float, FFT_SIZE / 2> magnitudes;
    std::array<float, NUM_BANDS> bandLevels;
    float rmsLevel;
    float peakLevel;
    float lufs;
    float dynamicRange;
    float crestFactor;
    
    void reset() {
        magnitudes.fill(0.0f);
        bandLevels.fill(MIN_DB);
        rmsLevel = MIN_DB;
        peakLevel = MIN_DB;
        lufs = -23.0f;
        dynamicRange = 0.0f;
        crestFactor = 0.0f;
    }
};

//------------------------------------------------------------------------------
// Reference Profile - Stores analyzed characteristics of reference track
//------------------------------------------------------------------------------
struct ReferenceProfile {
    std::string name;
    SpectrumData spectrum;
    
    // Detailed analysis
    std::array<float, NUM_BANDS> targetBandLevels;
    float targetLoudness;
    float targetDynamicRange;
    
    // Instrument detection hints
    float bassPresence;      // Sub + Bass bands
    float drumPresence;      // Transient detection
    float vocalPresence;     // Mid-range presence
    float brightnessRatio;   // High freq vs overall
    
    // Timestamp
    std::string createdDate;
    
    bool isValid() const { return !name.empty(); }
};

//------------------------------------------------------------------------------
// Mastering Suggestions
//------------------------------------------------------------------------------
struct MasteringSuggestion {
    enum class Type {
        BassBoost,
        BassReduce,
        DrumEnhance,
        MidBoost,
        MidReduce,
        HighBoost,
        HighReduce,
        LoudnessIncrease,
        LoudnessDecrease,
        DynamicsExpand,
        DynamicsCompress,
        GeneralOK
    };
    
    Type type;
    std::string message;
    std::string messageJP;  // Japanese message
    float severity;  // 0.0 - 1.0
};

//------------------------------------------------------------------------------
// Multiband Compressor Band
//------------------------------------------------------------------------------
class CompressorBand {
public:
    void setParameters(float threshold, float ratio, float attack, float release, float makeupGain);
    float process(float input);
    void reset();
    
    float getGainReduction() const { return gainReduction; }
    
private:
    float threshold = -20.0f;
    float ratio = 4.0f;
    float attack = 10.0f;   // ms
    float release = 100.0f; // ms
    float makeupGain = 0.0f;
    
    float envelope = 0.0f;
    float gainReduction = 0.0f;
    float sampleRate = 44100.0f;
    
    float attackCoeff = 0.0f;
    float releaseCoeff = 0.0f;
    
    void updateCoefficients();
};

//------------------------------------------------------------------------------
// Multiband EQ Band
//------------------------------------------------------------------------------
class EQBand {
public:
    enum class FilterType {
        LowShelf,
        HighShelf,
        Peak,
        LowPass,
        HighPass
    };
    
    void setParameters(FilterType type, float frequency, float gain, float q);
    void setSampleRate(float sr) { sampleRate = sr; updateCoefficients(); }
    float process(float input);
    void reset();
    
private:
    FilterType filterType = FilterType::Peak;
    float frequency = 1000.0f;
    float gain = 0.0f;
    float q = 0.707f;
    float sampleRate = 44100.0f;
    
    // Biquad coefficients
    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
    float a1 = 0.0f, a2 = 0.0f;
    
    // State variables
    float x1 = 0.0f, x2 = 0.0f;
    float y1 = 0.0f, y2 = 0.0f;
    
    void updateCoefficients();
};

//------------------------------------------------------------------------------
// Crossover Filter for multiband processing
//------------------------------------------------------------------------------
class CrossoverFilter {
public:
    void setSampleRate(float sr);
    void setFrequency(float freq);
    void process(float input, float& low, float& high);
    void reset();
    
private:
    float sampleRate = 44100.0f;
    float cutoffFreq = 1000.0f;
    
    // Linkwitz-Riley 4th order (two cascaded 2nd order Butterworth)
    std::array<float, 4> lowState = {0.0f, 0.0f, 0.0f, 0.0f};
    std::array<float, 4> highState = {0.0f, 0.0f, 0.0f, 0.0f};
    
    float coeffs[5];
    
    void updateCoefficients();
};

//------------------------------------------------------------------------------
// FFT Processor for spectrum analysis
//------------------------------------------------------------------------------
class FFTProcessor {
public:
    FFTProcessor();
    ~FFTProcessor();
    
    void setSampleRate(float sr) { sampleRate = sr; }
    void process(const float* input, int numSamples);
    const SpectrumData& getSpectrum() const { return currentSpectrum; }
    void reset();
    
private:
    float sampleRate = 44100.0f;
    std::vector<float> inputBuffer;
    std::vector<float> window;
    std::vector<std::complex<float>> fftBuffer;
    SpectrumData currentSpectrum;
    int writePos = 0;
    
    void performFFT();
    void applyWindow();
    void calculateBandLevels();
    
    // Simple DFT for portability (can be replaced with FFTW for performance)
    void dft(std::vector<std::complex<float>>& data);
};

//------------------------------------------------------------------------------
// Main Processor
//------------------------------------------------------------------------------
class PromasterProcessor {
public:
    PromasterProcessor();
    ~PromasterProcessor();
    
    // Audio processing
    void setSampleRate(float sr);
    void process(float* leftChannel, float* rightChannel, int numSamples);
    void reset();
    
    // Reference management
    bool loadReference(const std::string& audioFilePath);
    bool analyzeReference(const float* leftChannel, const float* rightChannel, int numSamples);
    void setActivePreset(int index);
    bool savePreset(const std::string& name);
    bool loadPreset(const std::string& filePath);
    
    // Get analysis data
    const SpectrumData& getCurrentSpectrum() const { return currentAnalysis; }
    const SpectrumData& getReferenceSpectrum() const { return referenceProfile.spectrum; }
    const ReferenceProfile& getReferenceProfile() const { return referenceProfile; }
    
    // Suggestions
    std::vector<MasteringSuggestion> getSuggestions() const;
    
    // Adjust button - apply automatic mastering
    void applyAdjust();
    void setAdjustAmount(float amount) { adjustAmount = amount; } // 0.0 - 1.0
    float getAdjustAmount() const { return adjustAmount; }
    
    // Bypass
    void setBypass(bool bypass) { bypassed = bypass; }
    bool isBypassed() const { return bypassed; }
    
    // Get band differences for GUI display
    std::array<float, NUM_BANDS> getBandDifferences() const;
    
private:
    float sampleRate = 44100.0f;
    bool bypassed = false;
    float adjustAmount = 0.0f;
    
    // Analysis
    std::unique_ptr<FFTProcessor> fftProcessor;
    SpectrumData currentAnalysis;
    ReferenceProfile referenceProfile;
    
    // Multiband processing
    std::array<CrossoverFilter, NUM_BANDS - 1> crossovers;
    std::array<CompressorBand, NUM_BANDS> compressors;
    std::array<EQBand, NUM_BANDS> equalizers;
    
    // Calculated adjustments
    std::array<float, NUM_BANDS> targetGains;
    std::array<float, NUM_BANDS> targetCompression;
    float targetLoudnessGain = 0.0f;
    
    // Processing buffers
    std::array<std::vector<float>, NUM_BANDS> bandBuffersL;
    std::array<std::vector<float>, NUM_BANDS> bandBuffersR;
    
    // Preset management
    std::vector<ReferenceProfile> presets;
    int activePresetIndex = -1;
    
    // Internal methods
    void calculateAdjustments();
    void splitBands(const float* input, int numSamples, bool isLeft);
    void combineBands(float* output, int numSamples, bool isLeft);
    void processMultiband(int numSamples);
};

//------------------------------------------------------------------------------
// Preset Manager
//------------------------------------------------------------------------------
class PresetManager {
public:
    static bool savePreset(const ReferenceProfile& profile, const std::string& filePath);
    static bool loadPreset(ReferenceProfile& profile, const std::string& filePath);
    static std::vector<std::string> getPresetList(const std::string& directory);
    static std::string getDefaultPresetDirectory();
};

//------------------------------------------------------------------------------
// Audio File Loader (for reference tracks)
//------------------------------------------------------------------------------
class AudioFileLoader {
public:
    struct AudioData {
        std::vector<float> leftChannel;
        std::vector<float> rightChannel;
        float sampleRate;
        int numSamples;
        bool isStereo;
    };
    
    static bool loadFile(const std::string& filePath, AudioData& data);
    static bool isSupported(const std::string& filePath);
};

//------------------------------------------------------------------------------
// Utility Functions
//------------------------------------------------------------------------------
namespace Utils {
    inline float dbToLinear(float db) {
        return std::pow(10.0f, db / 20.0f);
    }
    
    inline float linearToDb(float linear) {
        if (linear <= 0.0f) return MIN_DB;
        return 20.0f * std::log10(linear);
    }
    
    inline float clamp(float value, float min, float max) {
        return std::min(std::max(value, min), max);
    }
    
    inline float lerp(float a, float b, float t) {
        return a + t * (b - a);
    }
}

} // namespace PromasterOne
