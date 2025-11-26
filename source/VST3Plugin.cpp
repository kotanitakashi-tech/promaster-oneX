/*
 * PROMASTER ONE - VST3 Plugin Implementation
 * Main plugin entry point for Cakewalk Sonar compatibility
 */

#include "../include/PromasterOne.h"

// VST3 SDK headers would be included here in full implementation
// #include "public.sdk/source/vst/vstaudioeffect.h"
// #include "pluginterfaces/vst/ivstparameterchanges.h"

namespace PromasterOne {

//------------------------------------------------------------------------------
// Parameter IDs
//------------------------------------------------------------------------------
enum ParamID : uint32_t {
    kBypass = 0,
    kAdjustAmount,
    kInputGain,
    kOutputGain,
    kReferenceLoaded,
    kPresetIndex,
    
    // Band gains (for manual override)
    kBandGain0,
    kBandGain1,
    kBandGain2,
    kBandGain3,
    kBandGain4,
    kBandGain5,
    kBandGain6,
    kBandGain7,
    
    kNumParams
};

//------------------------------------------------------------------------------
// VST3 Audio Processor
//------------------------------------------------------------------------------
class PromasterVST3Processor {
public:
    PromasterVST3Processor();
    ~PromasterVST3Processor();
    
    // Lifecycle
    bool initialize(float sampleRate, int maxBlockSize);
    void terminate();
    
    // Processing
    void process(float** inputs, float** outputs, int numChannels, int numSamples);
    void processParameterChanges(uint32_t paramId, float value);
    
    // State
    bool getState(void* data, int& size);
    bool setState(const void* data, int size);
    
    // Reference
    bool loadReferenceFromFile(const char* filePath);
    bool loadReferenceFromBuffer(const float* left, const float* right, int numSamples, float sampleRate);
    
    // Presets
    bool saveCurrentAsPreset(const char* name, const char* filePath);
    bool loadPreset(const char* filePath);
    int getNumPresets() const;
    const char* getPresetName(int index) const;
    void selectPreset(int index);
    
    // Analysis data for GUI
    const SpectrumData& getCurrentSpectrum() const { return processor.getCurrentSpectrum(); }
    const SpectrumData& getReferenceSpectrum() const { return processor.getReferenceSpectrum(); }
    std::array<float, NUM_BANDS> getBandDifferences() const { return processor.getBandDifferences(); }
    std::vector<MasteringSuggestion> getSuggestions() const { return processor.getSuggestions(); }
    
    // Adjust
    void applyAdjust() { processor.applyAdjust(); }
    float getAdjustAmount() const { return processor.getAdjustAmount(); }
    
private:
    PromasterProcessor processor;
    float sampleRate = 44100.0f;
    int maxBlockSize = 512;
    
    // Parameters
    std::array<float, kNumParams> parameters;
    
    // Preset storage
    std::vector<std::pair<std::string, ReferenceProfile>> presets;
};

PromasterVST3Processor::PromasterVST3Processor() {
    parameters.fill(0.0f);
    parameters[kAdjustAmount] = 0.0f;
    parameters[kInputGain] = 1.0f;
    parameters[kOutputGain] = 1.0f;
}

PromasterVST3Processor::~PromasterVST3Processor() {
    terminate();
}

bool PromasterVST3Processor::initialize(float sr, int blockSize) {
    sampleRate = sr;
    maxBlockSize = blockSize;
    processor.setSampleRate(sr);
    processor.reset();
    return true;
}

void PromasterVST3Processor::terminate() {
    processor.reset();
}

void PromasterVST3Processor::process(float** inputs, float** outputs, int numChannels, int numSamples) {
    if (numChannels < 2 || numSamples == 0) return;
    
    // Copy input to output
    for (int ch = 0; ch < numChannels; ++ch) {
        if (inputs[ch] != outputs[ch]) {
            std::copy(inputs[ch], inputs[ch] + numSamples, outputs[ch]);
        }
    }
    
    // Apply input gain
    float inputGain = parameters[kInputGain];
    if (inputGain != 1.0f) {
        for (int i = 0; i < numSamples; ++i) {
            outputs[0][i] *= inputGain;
            outputs[1][i] *= inputGain;
        }
    }
    
    // Process
    if (parameters[kBypass] < 0.5f) {
        processor.setAdjustAmount(parameters[kAdjustAmount]);
        processor.process(outputs[0], outputs[1], numSamples);
    }
    
    // Apply output gain
    float outputGain = parameters[kOutputGain];
    if (outputGain != 1.0f) {
        for (int i = 0; i < numSamples; ++i) {
            outputs[0][i] *= outputGain;
            outputs[1][i] *= outputGain;
        }
    }
}

void PromasterVST3Processor::processParameterChanges(uint32_t paramId, float value) {
    if (paramId < kNumParams) {
        parameters[paramId] = value;
        
        if (paramId == kBypass) {
            processor.setBypass(value > 0.5f);
        } else if (paramId == kAdjustAmount) {
            processor.setAdjustAmount(value);
        } else if (paramId == kPresetIndex) {
            int index = static_cast<int>(value * presets.size());
            selectPreset(index);
        }
    }
}

bool PromasterVST3Processor::loadReferenceFromBuffer(const float* left, const float* right, 
                                                       int numSamples, float sr) {
    if (sr != sampleRate) {
        // Would need resampling here - simplified for now
    }
    return processor.analyzeReference(left, right, numSamples);
}

bool PromasterVST3Processor::saveCurrentAsPreset(const char* name, const char* filePath) {
    processor.savePreset(name);
    return PresetManager::savePreset(processor.getReferenceProfile(), filePath);
}

int PromasterVST3Processor::getNumPresets() const {
    return static_cast<int>(presets.size());
}

const char* PromasterVST3Processor::getPresetName(int index) const {
    if (index >= 0 && index < static_cast<int>(presets.size())) {
        return presets[index].first.c_str();
    }
    return "";
}

void PromasterVST3Processor::selectPreset(int index) {
    processor.setActivePreset(index);
}

//------------------------------------------------------------------------------
// Plugin Factory (VST3 entry point)
//------------------------------------------------------------------------------

/*
 * In a complete VST3 implementation, this would include:
 * 
 * BEGIN_FACTORY_DEF("Promaster Audio", "https://promaster.audio", "mailto:support@promaster.audio")
 *     DEF_CLASS2(INLINE_UID(...),
 *         PClassInfo::kManyInstances,
 *         kVstAudioEffectClass,
 *         "PROMASTER ONE",
 *         Vst::kDistributable,
 *         Vst::PlugType::kFxMastering,
 *         "1.0.0",
 *         kVstVersionString,
 *         PromasterVST3Processor::createInstance)
 * END_FACTORY
 */

} // namespace PromasterOne

//------------------------------------------------------------------------------
// C-style exports for VST3
//------------------------------------------------------------------------------

extern "C" {

#ifdef _WIN32
#define VST_EXPORT __declspec(dllexport)
#else
#define VST_EXPORT __attribute__((visibility("default")))
#endif

// These would be the actual VST3 SDK exports
// VST_EXPORT void* GetPluginFactory() { ... }
// VST_EXPORT bool InitDll() { return true; }
// VST_EXPORT bool ExitDll() { return true; }

}
