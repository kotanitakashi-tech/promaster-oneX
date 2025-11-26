/*
 * PROMASTER ONE - DSP Core Implementation
 * Audio processing algorithms
 */

#include "../include/PromasterOne.h"
#include <algorithm>
#include <numeric>

namespace PromasterOne {

//------------------------------------------------------------------------------
// CompressorBand Implementation
//------------------------------------------------------------------------------

void CompressorBand::setParameters(float thresh, float rat, float att, float rel, float makeup) {
    threshold = thresh;
    ratio = rat;
    attack = att;
    release = rel;
    makeupGain = makeup;
    updateCoefficients();
}

void CompressorBand::updateCoefficients() {
    // Convert ms to coefficients
    attackCoeff = std::exp(-1.0f / (sampleRate * attack / 1000.0f));
    releaseCoeff = std::exp(-1.0f / (sampleRate * release / 1000.0f));
}

float CompressorBand::process(float input) {
    // Get input level in dB
    float inputDb = Utils::linearToDb(std::abs(input));
    
    // Calculate gain reduction
    float targetGainReduction = 0.0f;
    if (inputDb > threshold) {
        float excess = inputDb - threshold;
        targetGainReduction = excess - (excess / ratio);
    }
    
    // Smooth envelope
    float coeff = (targetGainReduction > gainReduction) ? attackCoeff : releaseCoeff;
    gainReduction = coeff * gainReduction + (1.0f - coeff) * targetGainReduction;
    
    // Apply gain reduction and makeup
    float totalGain = Utils::dbToLinear(-gainReduction + makeupGain);
    return input * totalGain;
}

void CompressorBand::reset() {
    envelope = 0.0f;
    gainReduction = 0.0f;
}

//------------------------------------------------------------------------------
// EQBand Implementation
//------------------------------------------------------------------------------

void EQBand::setParameters(FilterType type, float freq, float g, float qVal) {
    filterType = type;
    frequency = freq;
    gain = g;
    q = qVal;
    updateCoefficients();
}

void EQBand::updateCoefficients() {
    float w0 = 2.0f * M_PI * frequency / sampleRate;
    float cosw0 = std::cos(w0);
    float sinw0 = std::sin(w0);
    float alpha = sinw0 / (2.0f * q);
    float A = std::pow(10.0f, gain / 40.0f);
    
    float a0;
    
    switch (filterType) {
        case FilterType::LowShelf: {
            float sqrtA = std::sqrt(A);
            a0 = (A + 1) + (A - 1) * cosw0 + 2 * sqrtA * alpha;
            b0 = A * ((A + 1) - (A - 1) * cosw0 + 2 * sqrtA * alpha) / a0;
            b1 = 2 * A * ((A - 1) - (A + 1) * cosw0) / a0;
            b2 = A * ((A + 1) - (A - 1) * cosw0 - 2 * sqrtA * alpha) / a0;
            a1 = -2 * ((A - 1) + (A + 1) * cosw0) / a0;
            a2 = ((A + 1) + (A - 1) * cosw0 - 2 * sqrtA * alpha) / a0;
            break;
        }
        case FilterType::HighShelf: {
            float sqrtA = std::sqrt(A);
            a0 = (A + 1) - (A - 1) * cosw0 + 2 * sqrtA * alpha;
            b0 = A * ((A + 1) + (A - 1) * cosw0 + 2 * sqrtA * alpha) / a0;
            b1 = -2 * A * ((A - 1) + (A + 1) * cosw0) / a0;
            b2 = A * ((A + 1) + (A - 1) * cosw0 - 2 * sqrtA * alpha) / a0;
            a1 = 2 * ((A - 1) - (A + 1) * cosw0) / a0;
            a2 = ((A + 1) - (A - 1) * cosw0 - 2 * sqrtA * alpha) / a0;
            break;
        }
        case FilterType::Peak:
        default: {
            a0 = 1 + alpha / A;
            b0 = (1 + alpha * A) / a0;
            b1 = -2 * cosw0 / a0;
            b2 = (1 - alpha * A) / a0;
            a1 = -2 * cosw0 / a0;
            a2 = (1 - alpha / A) / a0;
            break;
        }
        case FilterType::LowPass: {
            a0 = 1 + alpha;
            b0 = ((1 - cosw0) / 2) / a0;
            b1 = (1 - cosw0) / a0;
            b2 = ((1 - cosw0) / 2) / a0;
            a1 = -2 * cosw0 / a0;
            a2 = (1 - alpha) / a0;
            break;
        }
        case FilterType::HighPass: {
            a0 = 1 + alpha;
            b0 = ((1 + cosw0) / 2) / a0;
            b1 = -(1 + cosw0) / a0;
            b2 = ((1 + cosw0) / 2) / a0;
            a1 = -2 * cosw0 / a0;
            a2 = (1 - alpha) / a0;
            break;
        }
    }
}

float EQBand::process(float input) {
    float output = b0 * input + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
    
    // Update state
    x2 = x1;
    x1 = input;
    y2 = y1;
    y1 = output;
    
    return output;
}

void EQBand::reset() {
    x1 = x2 = y1 = y2 = 0.0f;
}

//------------------------------------------------------------------------------
// CrossoverFilter Implementation (Linkwitz-Riley 4th order)
//------------------------------------------------------------------------------

void CrossoverFilter::setSampleRate(float sr) {
    sampleRate = sr;
    updateCoefficients();
}

void CrossoverFilter::setFrequency(float freq) {
    cutoffFreq = freq;
    updateCoefficients();
}

void CrossoverFilter::updateCoefficients() {
    float w0 = 2.0f * M_PI * cutoffFreq / sampleRate;
    float cosw0 = std::cos(w0);
    float sinw0 = std::sin(w0);
    float alpha = sinw0 / (2.0f * 0.707f); // Butterworth Q
    
    float a0 = 1 + alpha;
    coeffs[0] = ((1 - cosw0) / 2) / a0; // b0 low
    coeffs[1] = (1 - cosw0) / a0;        // b1 low
    coeffs[2] = -2 * cosw0 / a0;         // a1
    coeffs[3] = (1 - alpha) / a0;        // a2
    coeffs[4] = ((1 + cosw0) / 2) / a0;  // b0 high
}

void CrossoverFilter::process(float input, float& low, float& high) {
    // First stage low-pass
    float lowOut1 = coeffs[0] * input + coeffs[1] * lowState[0] + coeffs[0] * lowState[1]
                  - coeffs[2] * lowState[2] - coeffs[3] * lowState[3];
    
    // Update low-pass states
    lowState[1] = lowState[0];
    lowState[0] = input;
    lowState[3] = lowState[2];
    lowState[2] = lowOut1;
    
    // Second stage for LR4
    float lowOut2 = coeffs[0] * lowOut1 + coeffs[1] * lowState[0] + coeffs[0] * lowState[1]
                  - coeffs[2] * lowState[2] - coeffs[3] * lowState[3];
    
    low = lowOut2;
    high = input - low; // Complementary filter
}

void CrossoverFilter::reset() {
    lowState.fill(0.0f);
    highState.fill(0.0f);
}

//------------------------------------------------------------------------------
// FFTProcessor Implementation
//------------------------------------------------------------------------------

FFTProcessor::FFTProcessor() {
    inputBuffer.resize(FFT_SIZE, 0.0f);
    window.resize(FFT_SIZE);
    fftBuffer.resize(FFT_SIZE);
    currentSpectrum.reset();
    
    // Create Hann window
    for (int i = 0; i < FFT_SIZE; ++i) {
        window[i] = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / (FFT_SIZE - 1)));
    }
}

FFTProcessor::~FFTProcessor() = default;

void FFTProcessor::process(const float* input, int numSamples) {
    for (int i = 0; i < numSamples; ++i) {
        inputBuffer[writePos] = input[i];
        writePos = (writePos + 1) % FFT_SIZE;
        
        // Perform FFT every FFT_SIZE / OVERLAP samples
        if (writePos % (FFT_SIZE / OVERLAP) == 0) {
            performFFT();
        }
    }
}

void FFTProcessor::performFFT() {
    // Copy and window the input
    for (int i = 0; i < FFT_SIZE; ++i) {
        int readPos = (writePos + i) % FFT_SIZE;
        fftBuffer[i] = std::complex<float>(inputBuffer[readPos] * window[i], 0.0f);
    }
    
    // Perform DFT
    dft(fftBuffer);
    
    // Calculate magnitudes
    float sumSquared = 0.0f;
    float peak = 0.0f;
    
    for (int i = 0; i < FFT_SIZE / 2; ++i) {
        float mag = std::abs(fftBuffer[i]) / (FFT_SIZE / 2);
        currentSpectrum.magnitudes[i] = mag;
        sumSquared += mag * mag;
        peak = std::max(peak, mag);
    }
    
    // Calculate levels
    currentSpectrum.rmsLevel = Utils::linearToDb(std::sqrt(sumSquared / (FFT_SIZE / 2)));
    currentSpectrum.peakLevel = Utils::linearToDb(peak);
    currentSpectrum.crestFactor = currentSpectrum.peakLevel - currentSpectrum.rmsLevel;
    
    calculateBandLevels();
}

void FFTProcessor::calculateBandLevels() {
    float binWidth = sampleRate / FFT_SIZE;
    
    for (int band = 0; band < NUM_BANDS; ++band) {
        float lowFreq = BAND_FREQUENCIES[band];
        float highFreq = BAND_FREQUENCIES[band + 1];
        
        int lowBin = static_cast<int>(lowFreq / binWidth);
        int highBin = static_cast<int>(highFreq / binWidth);
        
        lowBin = std::max(0, std::min(lowBin, FFT_SIZE / 2 - 1));
        highBin = std::max(lowBin + 1, std::min(highBin, FFT_SIZE / 2));
        
        float sum = 0.0f;
        for (int i = lowBin; i < highBin; ++i) {
            sum += currentSpectrum.magnitudes[i] * currentSpectrum.magnitudes[i];
        }
        
        float rms = std::sqrt(sum / std::max(1, highBin - lowBin));
        currentSpectrum.bandLevels[band] = Utils::linearToDb(rms);
    }
}

void FFTProcessor::dft(std::vector<std::complex<float>>& data) {
    // Simple radix-2 Cooley-Tukey FFT
    int n = data.size();
    if (n <= 1) return;
    
    // Bit-reversal permutation
    int j = 0;
    for (int i = 0; i < n; ++i) {
        if (i < j) std::swap(data[i], data[j]);
        int m = n / 2;
        while (j >= m && m >= 1) {
            j -= m;
            m /= 2;
        }
        j += m;
    }
    
    // Cooley-Tukey iterative FFT
    for (int len = 2; len <= n; len *= 2) {
        float angle = -2.0f * M_PI / len;
        std::complex<float> wn(std::cos(angle), std::sin(angle));
        
        for (int i = 0; i < n; i += len) {
            std::complex<float> w(1.0f, 0.0f);
            for (int k = 0; k < len / 2; ++k) {
                std::complex<float> t = w * data[i + k + len / 2];
                std::complex<float> u = data[i + k];
                data[i + k] = u + t;
                data[i + k + len / 2] = u - t;
                w *= wn;
            }
        }
    }
}

void FFTProcessor::reset() {
    std::fill(inputBuffer.begin(), inputBuffer.end(), 0.0f);
    writePos = 0;
    currentSpectrum.reset();
}

//------------------------------------------------------------------------------
// PromasterProcessor Implementation
//------------------------------------------------------------------------------

PromasterProcessor::PromasterProcessor() {
    fftProcessor = std::make_unique<FFTProcessor>();
    
    // Initialize band buffers
    for (int i = 0; i < NUM_BANDS; ++i) {
        bandBuffersL[i].resize(8192, 0.0f);
        bandBuffersR[i].resize(8192, 0.0f);
    }
    
    // Set default crossover frequencies
    for (int i = 0; i < NUM_BANDS - 1; ++i) {
        crossovers[i].setFrequency(BAND_FREQUENCIES[i + 1]);
    }
    
    // Initialize EQ bands
    for (int i = 0; i < NUM_BANDS; ++i) {
        float centerFreq = std::sqrt(BAND_FREQUENCIES[i] * BAND_FREQUENCIES[i + 1]);
        equalizers[i].setParameters(EQBand::FilterType::Peak, centerFreq, 0.0f, 1.0f);
    }
    
    // Initialize compressors with gentle settings
    for (int i = 0; i < NUM_BANDS; ++i) {
        compressors[i].setParameters(-20.0f, 2.0f, 20.0f, 200.0f, 0.0f);
    }
    
    currentAnalysis.reset();
}

PromasterProcessor::~PromasterProcessor() = default;

void PromasterProcessor::setSampleRate(float sr) {
    sampleRate = sr;
    fftProcessor->setSampleRate(sr);
    
    for (auto& crossover : crossovers) {
        crossover.setSampleRate(sr);
    }
    
    for (auto& eq : equalizers) {
        eq.setSampleRate(sr);
    }
}

void PromasterProcessor::process(float* leftChannel, float* rightChannel, int numSamples) {
    if (bypassed || numSamples == 0) return;
    
    // Analyze input
    fftProcessor->process(leftChannel, numSamples);
    currentAnalysis = fftProcessor->getSpectrum();
    
    // Apply processing if adjust is active
    if (adjustAmount > 0.0f && referenceProfile.isValid()) {
        // Simple processing: apply EQ adjustments based on band differences
        auto differences = getBandDifferences();
        
        for (int i = 0; i < numSamples; ++i) {
            float sumL = 0.0f;
            float sumR = 0.0f;
            
            // Apply EQ corrections to each sample
            for (int band = 0; band < NUM_BANDS; ++band) {
                float gain = Utils::dbToLinear(differences[band] * adjustAmount * 0.5f);
                
                // Simple frequency-weighted processing
                float sampleL = leftChannel[i] * gain;
                float sampleR = rightChannel[i] * gain;
                
                // Apply compression
                sampleL = compressors[band].process(sampleL);
                sampleR = compressors[band].process(sampleR);
                
                sumL += sampleL / NUM_BANDS;
                sumR += sampleR / NUM_BANDS;
            }
            
            // Mix with dry signal
            leftChannel[i] = Utils::lerp(leftChannel[i], sumL, adjustAmount);
            rightChannel[i] = Utils::lerp(rightChannel[i], sumR, adjustAmount);
        }
        
        // Apply loudness adjustment
        float loudnessDiff = referenceProfile.targetLoudness - currentAnalysis.lufs;
        float loudnessGain = Utils::dbToLinear(loudnessDiff * adjustAmount * 0.3f);
        
        for (int i = 0; i < numSamples; ++i) {
            leftChannel[i] *= loudnessGain;
            rightChannel[i] *= loudnessGain;
            
            // Soft clip to prevent harsh clipping
            leftChannel[i] = std::tanh(leftChannel[i]);
            rightChannel[i] = std::tanh(rightChannel[i]);
        }
    }
}

void PromasterProcessor::reset() {
    fftProcessor->reset();
    
    for (auto& crossover : crossovers) {
        crossover.reset();
    }
    
    for (auto& eq : equalizers) {
        eq.reset();
    }
    
    for (auto& comp : compressors) {
        comp.reset();
    }
    
    currentAnalysis.reset();
}

bool PromasterProcessor::analyzeReference(const float* leftChannel, const float* rightChannel, int numSamples) {
    if (numSamples == 0) return false;
    
    // Create temporary FFT processor for analysis
    FFTProcessor refAnalyzer;
    refAnalyzer.setSampleRate(sampleRate);
    
    // Analyze the reference
    for (int i = 0; i < numSamples; i += FFT_SIZE) {
        int blockSize = std::min(FFT_SIZE, numSamples - i);
        refAnalyzer.process(leftChannel + i, blockSize);
    }
    
    referenceProfile.spectrum = refAnalyzer.getSpectrum();
    
    // Copy band levels as targets
    for (int i = 0; i < NUM_BANDS; ++i) {
        referenceProfile.targetBandLevels[i] = referenceProfile.spectrum.bandLevels[i];
    }
    
    referenceProfile.targetLoudness = referenceProfile.spectrum.lufs;
    referenceProfile.targetDynamicRange = referenceProfile.spectrum.crestFactor;
    
    // Calculate instrument presence hints
    referenceProfile.bassPresence = 
        (referenceProfile.spectrum.bandLevels[0] + referenceProfile.spectrum.bandLevels[1]) / 2.0f;
    referenceProfile.vocalPresence = 
        (referenceProfile.spectrum.bandLevels[3] + referenceProfile.spectrum.bandLevels[4]) / 2.0f;
    referenceProfile.brightnessRatio = 
        (referenceProfile.spectrum.bandLevels[6] + referenceProfile.spectrum.bandLevels[7]) / 2.0f
        - referenceProfile.spectrum.rmsLevel;
    
    calculateAdjustments();
    
    return true;
}

void PromasterProcessor::calculateAdjustments() {
    if (!referenceProfile.isValid()) return;
    
    for (int i = 0; i < NUM_BANDS; ++i) {
        targetGains[i] = referenceProfile.targetBandLevels[i] - currentAnalysis.bandLevels[i];
        targetGains[i] = Utils::clamp(targetGains[i], -12.0f, 12.0f);
        
        // Adjust EQ
        float centerFreq = std::sqrt(BAND_FREQUENCIES[i] * BAND_FREQUENCIES[i + 1]);
        equalizers[i].setParameters(EQBand::FilterType::Peak, centerFreq, targetGains[i], 1.0f);
    }
    
    targetLoudnessGain = referenceProfile.targetLoudness - currentAnalysis.lufs;
    targetLoudnessGain = Utils::clamp(targetLoudnessGain, -12.0f, 12.0f);
}

std::array<float, NUM_BANDS> PromasterProcessor::getBandDifferences() const {
    std::array<float, NUM_BANDS> diff;
    
    if (!referenceProfile.isValid()) {
        diff.fill(0.0f);
        return diff;
    }
    
    for (int i = 0; i < NUM_BANDS; ++i) {
        diff[i] = referenceProfile.targetBandLevels[i] - currentAnalysis.bandLevels[i];
    }
    
    return diff;
}

std::vector<MasteringSuggestion> PromasterProcessor::getSuggestions() const {
    std::vector<MasteringSuggestion> suggestions;
    
    if (!referenceProfile.isValid()) {
        suggestions.push_back({
            MasteringSuggestion::Type::GeneralOK,
            "Load a reference track to get suggestions",
            "リファレンストラックを読み込むと提案が表示されます",
            0.0f
        });
        return suggestions;
    }
    
    auto diff = getBandDifferences();
    
    // Bass analysis
    float bassDiff = (diff[0] + diff[1]) / 2.0f;
    if (bassDiff > 3.0f) {
        suggestions.push_back({
            MasteringSuggestion::Type::BassBoost,
            "Bass is lacking compared to reference. Consider boosting low frequencies.",
            "リファレンスと比較して低音が不足しています。低域をブーストすることを検討してください。",
            std::abs(bassDiff) / 12.0f
        });
    } else if (bassDiff < -3.0f) {
        suggestions.push_back({
            MasteringSuggestion::Type::BassReduce,
            "Bass is too loud compared to reference. Consider reducing low frequencies.",
            "リファレンスと比較して低音が過剰です。低域を下げることを検討してください。",
            std::abs(bassDiff) / 12.0f
        });
    }
    
    // Mid analysis
    float midDiff = (diff[3] + diff[4]) / 2.0f;
    if (midDiff > 3.0f) {
        suggestions.push_back({
            MasteringSuggestion::Type::MidBoost,
            "Midrange lacks presence. Vocals and instruments may sound distant.",
            "中域のプレゼンスが不足しています。ボーカルや楽器が遠く聞こえる可能性があります。",
            std::abs(midDiff) / 12.0f
        });
    } else if (midDiff < -3.0f) {
        suggestions.push_back({
            MasteringSuggestion::Type::MidReduce,
            "Midrange is too prominent. Consider reducing for better balance.",
            "中域が突出しています。バランスを取るために下げることを検討してください。",
            std::abs(midDiff) / 12.0f
        });
    }
    
    // High frequency analysis
    float highDiff = (diff[6] + diff[7]) / 2.0f;
    if (highDiff > 3.0f) {
        suggestions.push_back({
            MasteringSuggestion::Type::HighBoost,
            "High frequencies are lacking. Track may sound dull.",
            "高域が不足しています。トラックが鈍く聞こえる可能性があります。",
            std::abs(highDiff) / 12.0f
        });
    } else if (highDiff < -3.0f) {
        suggestions.push_back({
            MasteringSuggestion::Type::HighReduce,
            "High frequencies are too bright. Consider reducing for a smoother sound.",
            "高域が明るすぎます。スムーズなサウンドのために下げることを検討してください。",
            std::abs(highDiff) / 12.0f
        });
    }
    
    // Loudness analysis
    float loudnessDiff = referenceProfile.targetLoudness - currentAnalysis.lufs;
    if (loudnessDiff > 3.0f) {
        suggestions.push_back({
            MasteringSuggestion::Type::LoudnessIncrease,
            "Track is quieter than reference. May need more gain or compression.",
            "トラックがリファレンスより静かです。ゲインまたはコンプレッションが必要かもしれません。",
            std::abs(loudnessDiff) / 12.0f
        });
    } else if (loudnessDiff < -3.0f) {
        suggestions.push_back({
            MasteringSuggestion::Type::LoudnessDecrease,
            "Track is louder than reference. Consider reducing overall level.",
            "トラックがリファレンスより音量が大きいです。全体レベルを下げることを検討してください。",
            std::abs(loudnessDiff) / 12.0f
        });
    }
    
    if (suggestions.empty()) {
        suggestions.push_back({
            MasteringSuggestion::Type::GeneralOK,
            "Your mix is well balanced compared to the reference!",
            "あなたのミックスはリファレンスと比較して良好にバランスが取れています！",
            0.0f
        });
    }
    
    return suggestions;
}

void PromasterProcessor::applyAdjust() {
    if (!referenceProfile.isValid()) return;
    
    // Smoothly increase adjust amount
    adjustAmount = 1.0f;
    calculateAdjustments();
}

bool PromasterProcessor::savePreset(const std::string& name) {
    referenceProfile.name = name;
    presets.push_back(referenceProfile);
    return true;
}

void PromasterProcessor::setActivePreset(int index) {
    if (index >= 0 && index < static_cast<int>(presets.size())) {
        activePresetIndex = index;
        referenceProfile = presets[index];
        calculateAdjustments();
    }
}

//------------------------------------------------------------------------------
// PresetManager Implementation
//------------------------------------------------------------------------------

bool PresetManager::savePreset(const ReferenceProfile& profile, const std::string& filePath) {
    // Simple JSON-like format for preset storage
    FILE* file = fopen(filePath.c_str(), "w");
    if (!file) return false;
    
    fprintf(file, "{\n");
    fprintf(file, "  \"name\": \"%s\",\n", profile.name.c_str());
    fprintf(file, "  \"targetLoudness\": %.2f,\n", profile.targetLoudness);
    fprintf(file, "  \"targetDynamicRange\": %.2f,\n", profile.targetDynamicRange);
    fprintf(file, "  \"bassPresence\": %.2f,\n", profile.bassPresence);
    fprintf(file, "  \"vocalPresence\": %.2f,\n", profile.vocalPresence);
    fprintf(file, "  \"brightnessRatio\": %.2f,\n", profile.brightnessRatio);
    fprintf(file, "  \"bandLevels\": [");
    for (int i = 0; i < NUM_BANDS; ++i) {
        fprintf(file, "%.2f%s", profile.targetBandLevels[i], i < NUM_BANDS - 1 ? ", " : "");
    }
    fprintf(file, "]\n");
    fprintf(file, "}\n");
    
    fclose(file);
    return true;
}

std::string PresetManager::getDefaultPresetDirectory() {
#ifdef _WIN32
    return std::string(getenv("APPDATA")) + "\\PromasterOne\\Presets\\";
#else
    return std::string(getenv("HOME")) + "/.promaster-one/presets/";
#endif
}

} // namespace PromasterOne
