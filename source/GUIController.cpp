/*
 * PROMASTER ONE - GUI Controller
 * User interface implementation with spectrum analyzer and one-button adjust
 */

#include "../include/PromasterOne.h"
#include <sstream>
#include <iomanip>

namespace PromasterOne {

//------------------------------------------------------------------------------
// Color Definitions
//------------------------------------------------------------------------------
struct Color {
    uint8_t r, g, b, a;
    
    static Color fromRGB(uint8_t r, uint8_t g, uint8_t b) { return {r, g, b, 255}; }
    static Color fromRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a) { return {r, g, b, a}; }
};

namespace Colors {
    const Color Background = Color::fromRGB(25, 25, 30);
    const Color Panel = Color::fromRGB(35, 35, 42);
    const Color PanelBorder = Color::fromRGB(55, 55, 65);
    const Color Text = Color::fromRGB(220, 220, 230);
    const Color TextDim = Color::fromRGB(140, 140, 155);
    const Color Accent = Color::fromRGB(80, 180, 255);     // Cyan blue
    const Color AccentDark = Color::fromRGB(40, 100, 160);
    const Color Reference = Color::fromRGB(255, 140, 80);   // Orange
    const Color Current = Color::fromRGB(80, 220, 150);     // Green
    const Color Difference = Color::fromRGB(220, 80, 120);  // Pink/Red
    const Color AdjustButton = Color::fromRGB(60, 200, 140);
    const Color AdjustButtonHover = Color::fromRGB(80, 230, 160);
    const Color WarningYellow = Color::fromRGB(255, 200, 80);
    const Color ErrorRed = Color::fromRGB(255, 90, 90);
}

//------------------------------------------------------------------------------
// GUI Dimensions
//------------------------------------------------------------------------------
namespace Dimensions {
    const int PluginWidth = 800;
    const int PluginHeight = 500;
    
    const int SpectrumX = 20;
    const int SpectrumY = 60;
    const int SpectrumWidth = 500;
    const int SpectrumHeight = 280;
    
    const int ControlPanelX = 540;
    const int ControlPanelY = 60;
    const int ControlPanelWidth = 240;
    
    const int SuggestionsY = 360;
    const int SuggestionsHeight = 120;
    
    const int AdjustButtonWidth = 200;
    const int AdjustButtonHeight = 50;
}

//------------------------------------------------------------------------------
// GUI Element Base
//------------------------------------------------------------------------------
class GUIElement {
public:
    virtual ~GUIElement() = default;
    virtual void draw() = 0;
    virtual bool hitTest(int x, int y) = 0;
    virtual void onMouseDown(int x, int y) {}
    virtual void onMouseUp(int x, int y) {}
    virtual void onMouseMove(int x, int y) {}
    
protected:
    int x = 0, y = 0, width = 0, height = 0;
    bool visible = true;
    bool enabled = true;
};

//------------------------------------------------------------------------------
// Spectrum Display
//------------------------------------------------------------------------------
class SpectrumDisplay : public GUIElement {
public:
    void setBounds(int x_, int y_, int w, int h) {
        x = x_; y = y_; width = w; height = h;
    }
    
    void setCurrentSpectrum(const SpectrumData& spectrum) {
        currentSpectrum = spectrum;
    }
    
    void setReferenceSpectrum(const SpectrumData& spectrum) {
        referenceSpectrum = spectrum;
        hasReference = true;
    }
    
    void setBandDifferences(const std::array<float, NUM_BANDS>& diff) {
        bandDifferences = diff;
    }
    
    void draw() override {
        // This would be implemented with actual graphics API
        // Here we describe the visual structure
        
        /*
         * Visual layout:
         * 
         * ┌─────────────────────────────────────────────────┐
         * │  PROMASTER ONE    [Reference: Pop Master.pmo]   │
         * ├─────────────────────────────────────────────────┤
         * │                                                 │
         * │  +12dB ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─   │
         * │                   ___                           │
         * │   0dB ─ ─ ─ ─ ─ ─│░░░│─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─    │
         * │        ████ ████ │░░░│████                      │
         * │        ████ ████ │░░░│████ ████ ████            │
         * │  -24dB ████ ████ │░░░│████ ████ ████ ████ ████  │
         * │                                                 │
         * │  -48dB ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─   │
         * │                                                 │
         * │   Sub   Bass  Low   Mid  UpMid  Pres  Bril  Air │
         * │   20Hz  60Hz  250Hz 500Hz 1kHz  2kHz  6kHz 12kHz│
         * │                                                 │
         * │  ■ Current   ■ Reference   ▲ Difference         │
         * └─────────────────────────────────────────────────┘
         */
    }
    
    bool hitTest(int px, int py) override {
        return px >= x && px < x + width && py >= y && py < y + height;
    }
    
    // Get band index from x position (for tooltips)
    int getBandAtPosition(int px) const {
        int bandWidth = width / NUM_BANDS;
        int localX = px - x;
        return localX / bandWidth;
    }
    
private:
    SpectrumData currentSpectrum;
    SpectrumData referenceSpectrum;
    std::array<float, NUM_BANDS> bandDifferences;
    bool hasReference = false;
    
    // Drawing helpers
    float dbToY(float db) const {
        float normalized = (db - MIN_DB) / (MAX_DB - MIN_DB);
        return y + height - (normalized * height);
    }
};

//------------------------------------------------------------------------------
// Adjust Button
//------------------------------------------------------------------------------
class AdjustButton : public GUIElement {
public:
    using Callback = std::function<void()>;
    
    void setBounds(int x_, int y_, int w, int h) {
        x = x_; y = y_; width = w; height = h;
    }
    
    void setCallback(Callback cb) {
        callback = std::move(cb);
    }
    
    void setEnabled(bool en) {
        enabled = en;
    }
    
    void setProgress(float p) {
        progress = p;
    }
    
    void draw() override {
        /*
         * Visual:
         * 
         * ┌────────────────────────────────┐
         * │                                │
         * │       ★  A D J U S T  ★        │  <- Large, prominent button
         * │                                │
         * │  [▓▓▓▓▓▓▓▓▓▓▓▓▓▓░░░░░░]       │  <- Progress bar when active
         * └────────────────────────────────┘
         * 
         * Glow effect when hovering
         * Pulsing animation when ready to adjust
         */
    }
    
    bool hitTest(int px, int py) override {
        return enabled && px >= x && px < x + width && py >= y && py < y + height;
    }
    
    void onMouseDown(int px, int py) override {
        if (hitTest(px, py) && callback) {
            callback();
        }
    }
    
private:
    Callback callback;
    float progress = 0.0f;
    bool hovered = false;
    bool pressed = false;
};

//------------------------------------------------------------------------------
// Reference Loader Panel
//------------------------------------------------------------------------------
class ReferenceLoaderPanel : public GUIElement {
public:
    using LoadCallback = std::function<void(const std::string&)>;
    
    void setLoadCallback(LoadCallback cb) {
        loadCallback = std::move(cb);
    }
    
    void setReferenceName(const std::string& name) {
        referenceName = name;
    }
    
    void draw() override {
        /*
         * ┌─ Reference Track ─────────────────┐
         * │                                   │
         * │  📁 [Load Reference...]           │
         * │                                   │
         * │  Current: Pop Master 2024.wav     │
         * │  Duration: 3:24                   │
         * │  Analyzed: ✓                      │
         * │                                   │
         * └───────────────────────────────────┘
         */
    }
    
    bool hitTest(int px, int py) override {
        return px >= x && px < x + width && py >= y && py < y + height;
    }
    
private:
    LoadCallback loadCallback;
    std::string referenceName;
};

//------------------------------------------------------------------------------
// Preset Panel
//------------------------------------------------------------------------------
class PresetPanel : public GUIElement {
public:
    using SelectCallback = std::function<void(int)>;
    using SaveCallback = std::function<void(const std::string&)>;
    
    void setPresets(const std::vector<std::string>& names) {
        presetNames = names;
    }
    
    void setSelectedIndex(int index) {
        selectedIndex = index;
    }
    
    void draw() override {
        /*
         * ┌─ Presets ─────────────────────────┐
         * │                                   │
         * │  ○ Pop Master 2024                │
         * │  ● Rock Energy                    │  <- Selected
         * │  ○ Jazz Warmth                    │
         * │  ○ Electronic Punch               │
         * │                                   │
         * │  [+ Save Current] [🗑 Delete]     │
         * └───────────────────────────────────┘
         */
    }
    
    bool hitTest(int px, int py) override {
        return px >= x && px < x + width && py >= y && py < y + height;
    }
    
private:
    std::vector<std::string> presetNames;
    int selectedIndex = -1;
    SelectCallback selectCallback;
    SaveCallback saveCallback;
};

//------------------------------------------------------------------------------
// Suggestions Panel
//------------------------------------------------------------------------------
class SuggestionsPanel : public GUIElement {
public:
    void setSuggestions(const std::vector<MasteringSuggestion>& sugg) {
        suggestions = sugg;
    }
    
    void setLanguage(bool japanese) {
        useJapanese = japanese;
    }
    
    void draw() override {
        /*
         * ┌─ Mastering Suggestions ───────────────────────────────────────┐
         * │                                                               │
         * │  ⚠ Bass is lacking compared to reference                     │
         * │    低音が不足しています                                        │
         * │    Severity: [▓▓▓▓▓▓▓░░░] High                               │
         * │                                                               │
         * │  ℹ Midrange is well balanced                                  │
         * │    中域のバランスは良好です                                    │
         * │                                                               │
         * └───────────────────────────────────────────────────────────────┘
         */
    }
    
    bool hitTest(int px, int py) override {
        return px >= x && px < x + width && py >= y && py < y + height;
    }
    
private:
    std::vector<MasteringSuggestion> suggestions;
    bool useJapanese = true;
    
    Color getSeverityColor(float severity) const {
        if (severity > 0.7f) return Colors::ErrorRed;
        if (severity > 0.4f) return Colors::WarningYellow;
        return Colors::Current;
    }
};

//------------------------------------------------------------------------------
// Main GUI Controller
//------------------------------------------------------------------------------
class GUIController {
public:
    GUIController(PromasterVST3Processor& proc);
    ~GUIController();
    
    // Lifecycle
    bool open(void* parentWindow);
    void close();
    
    // Updates
    void idle();
    void updateFromProcessor();
    
    // Event handling
    void onMouseDown(int x, int y, int button);
    void onMouseUp(int x, int y, int button);
    void onMouseMove(int x, int y);
    void onKeyDown(int keyCode);
    void onFileDrop(const std::string& filePath);
    
    // Get plugin size
    int getWidth() const { return Dimensions::PluginWidth; }
    int getHeight() const { return Dimensions::PluginHeight; }
    
private:
    PromasterVST3Processor& processor;
    
    // GUI elements
    std::unique_ptr<SpectrumDisplay> spectrumDisplay;
    std::unique_ptr<AdjustButton> adjustButton;
    std::unique_ptr<ReferenceLoaderPanel> referencePanel;
    std::unique_ptr<PresetPanel> presetPanel;
    std::unique_ptr<SuggestionsPanel> suggestionsPanel;
    
    // State
    bool isOpen = false;
    int lastMouseX = 0;
    int lastMouseY = 0;
    
    // Animation
    float animationPhase = 0.0f;
    
    void setupLayout();
    void drawBackground();
    void drawHeader();
    void drawFooter();
};

GUIController::GUIController(PromasterVST3Processor& proc) 
    : processor(proc) {
    
    spectrumDisplay = std::make_unique<SpectrumDisplay>();
    adjustButton = std::make_unique<AdjustButton>();
    referencePanel = std::make_unique<ReferenceLoaderPanel>();
    presetPanel = std::make_unique<PresetPanel>();
    suggestionsPanel = std::make_unique<SuggestionsPanel>();
    
    setupLayout();
}

GUIController::~GUIController() {
    close();
}

void GUIController::setupLayout() {
    using namespace Dimensions;
    
    spectrumDisplay->setBounds(SpectrumX, SpectrumY, SpectrumWidth, SpectrumHeight);
    
    // Adjust button centered below spectrum
    int adjustX = SpectrumX + (SpectrumWidth - AdjustButtonWidth) / 2;
    int adjustY = SpectrumY + SpectrumHeight + 10;
    adjustButton->setBounds(adjustX, adjustY, AdjustButtonWidth, AdjustButtonHeight);
    adjustButton->setCallback([this]() {
        processor.applyAdjust();
    });
    
    // Control panels on the right
    referencePanel->setBounds(ControlPanelX, ControlPanelY, ControlPanelWidth, 120);
    presetPanel->setBounds(ControlPanelX, ControlPanelY + 130, ControlPanelWidth, 150);
    
    // Suggestions at bottom
    suggestionsPanel->setBounds(SpectrumX, SuggestionsY, PluginWidth - 40, SuggestionsHeight);
    suggestionsPanel->setLanguage(true); // Japanese by default
}

void GUIController::updateFromProcessor() {
    // Update spectrum display
    spectrumDisplay->setCurrentSpectrum(processor.getCurrentSpectrum());
    spectrumDisplay->setReferenceSpectrum(processor.getReferenceSpectrum());
    spectrumDisplay->setBandDifferences(processor.getBandDifferences());
    
    // Update suggestions
    suggestionsPanel->setSuggestions(processor.getSuggestions());
    
    // Update adjust button state
    adjustButton->setProgress(processor.getAdjustAmount());
}

void GUIController::idle() {
    updateFromProcessor();
    
    // Update animation
    animationPhase += 0.016f;
    if (animationPhase > 1.0f) animationPhase -= 1.0f;
}

void GUIController::onFileDrop(const std::string& filePath) {
    // Check if it's an audio file
    std::string ext = filePath.substr(filePath.find_last_of('.') + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    if (ext == "wav" || ext == "mp3" || ext == "aiff" || ext == "flac" || ext == "ogg") {
        // Load as reference
        processor.loadReferenceFromFile(filePath.c_str());
        
        // Update reference panel
        std::string filename = filePath.substr(filePath.find_last_of("/\\") + 1);
        referencePanel->setReferenceName(filename);
    } else if (ext == "pmo") {
        // Load as preset
        processor.loadPreset(filePath.c_str());
    }
}

//------------------------------------------------------------------------------
// GUI Layout Description (for documentation)
//------------------------------------------------------------------------------

/*
 * PROMASTER ONE GUI Layout
 * ========================
 * 
 * ┌─────────────────────────────────────────────────────────────────────────────┐
 * │  PROMASTER ONE                                          [?] [Settings] [×]  │
 * ├─────────────────────────────────────────────────────────────────────────────┤
 * │                                                                             │
 * │  ┌─ Spectrum Analyzer ──────────────────────┐  ┌─ Reference Track ───────┐ │
 * │  │                                          │  │                         │ │
 * │  │  +12dB ─────────────────────────────     │  │  📁 Load Reference...   │ │
 * │  │                                          │  │                         │ │
 * │  │        ▓▓▓▓                              │  │  ✓ Pop Master 2024.wav  │ │
 * │  │   0dB  ████ ▓▓▓▓                         │  │    Duration: 3:24       │ │
 * │  │        ████ ████ ▓▓▓▓                    │  │                         │ │
 * │  │        ████ ████ ████ ▓▓▓▓               │  └─────────────────────────┘ │
 * │  │  -24dB ████ ████ ████ ████ ▓▓▓▓ ▓▓▓▓     │                              │
 * │  │        ████ ████ ████ ████ ████ ████     │  ┌─ Presets ───────────────┐ │
 * │  │                                          │  │  ○ Pop Master 2024      │ │
 * │  │  -48dB ─────────────────────────────     │  │  ● Rock Energy          │ │
 * │  │                                          │  │  ○ Jazz Warmth          │ │
 * │  │   Sub  Bass  LMid  Mid  UMid Pres Bril   │  │  ○ Electronic Punch     │ │
 * │  │                                          │  │                         │ │
 * │  │  ■ Current  ■ Reference  △ Difference    │  │  [+ Save] [🗑 Delete]   │ │
 * │  └──────────────────────────────────────────┘  └─────────────────────────┘ │
 * │                                                                             │
 * │                  ╔═══════════════════════════════╗                          │
 * │                  ║                               ║                          │
 * │                  ║    ★  A D J U S T  ★          ║   <- Big glowing button  │
 * │                  ║                               ║                          │
 * │                  ╚═══════════════════════════════╝                          │
 * │                                                                             │
 * ├─ Mastering Suggestions ─────────────────────────────────────────────────────┤
 * │                                                                             │
 * │  ⚠ 低音がリファレンスと比較して不足しています                                │
 * │    Bass is lacking compared to reference                                    │
 * │    重要度: [▓▓▓▓▓▓▓░░░] 高                                                  │
 * │                                                                             │
 * │  ✓ 中域のバランスは良好です                                                  │
 * │    Midrange is well balanced                                                │
 * │                                                                             │
 * └─────────────────────────────────────────────────────────────────────────────┘
 * 
 * Color Scheme:
 * - Background: Dark charcoal (#19191E)
 * - Current spectrum bars: Cyan/Teal (#50B4FF)
 * - Reference spectrum: Orange (#FF8C50)
 * - Difference indicators: Pink (#DC5078)
 * - Adjust button: Bright green (#3CC88C) with glow
 * - Warning messages: Yellow (#FFC850)
 * - Error messages: Red (#FF5A5A)
 * 
 * Interactions:
 * - Drag & drop audio files onto plugin to load as reference
 * - Drag & drop .pmo files to load presets
 * - Click "Adjust" to apply automatic mastering
 * - Hover over spectrum bands to see detailed values
 * - Click presets to switch between saved profiles
 */

} // namespace PromasterOne
