/*
  PROMASTER ONE - GUI Editor
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class SpectrumDisplay : public juce::Component, public juce::Timer
{
public:
    SpectrumDisplay(PromasterOneAudioProcessor& p) : processor(p) { startTimerHz(30); }
    
    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        
        g.setColour(juce::Colour(35, 35, 42));
        g.fillRoundedRectangle(bounds, 8.0f);
        
        g.setColour(juce::Colour(55, 55, 65));
        g.drawRoundedRectangle(bounds, 8.0f, 1.0f);
        
        auto spectrum = processor.getCurrentSpectrum();
        auto refProfile = processor.getReferenceProfile();
        auto differences = processor.getBandDifferences();
        
        float bandWidth = (bounds.getWidth() - 40) / Constants::NUM_BANDS;
        float barWidth = bandWidth * 0.35f;
        
        for (int band = 0; band < Constants::NUM_BANDS; ++band)
        {
            float x = bounds.getX() + 20 + band * bandWidth + bandWidth / 2;
            
            float currentLevel = spectrum.bandLevels[(size_t)band];
            float currentHeight = juce::jmap(currentLevel, Constants::MIN_DB, Constants::MAX_DB, 0.0f, bounds.getHeight() - 40);
            currentHeight = std::max(2.0f, currentHeight);
            
            juce::Rectangle<float> currentBar(x - barWidth - 2, bounds.getBottom() - 20 - currentHeight, barWidth, currentHeight);
            g.setColour(juce::Colour(80, 180, 255));
            g.fillRoundedRectangle(currentBar, 3.0f);
            
            if (refProfile.isValid())
            {
                float refLevel = refProfile.targetBandLevels[(size_t)band];
                float refHeight = juce::jmap(refLevel, Constants::MIN_DB, Constants::MAX_DB, 0.0f, bounds.getHeight() - 40);
                refHeight = std::max(2.0f, refHeight);
                
                juce::Rectangle<float> refBar(x + 2, bounds.getBottom() - 20 - refHeight, barWidth, refHeight);
                g.setColour(juce::Colour(255, 140, 80));
                g.fillRoundedRectangle(refBar, 3.0f);
            }
            
            g.setColour(juce::Colour(140, 140, 155));
            g.setFont(10.0f);
            g.drawText(Constants::BAND_NAMES[band], 
                       static_cast<int>(x - bandWidth / 2), static_cast<int>(bounds.getBottom() - 15), 
                       static_cast<int>(bandWidth), 12, juce::Justification::centred);
        }
        
        int legendY = 8;
        int legendX = static_cast<int>(bounds.getRight() - 180);
        
        g.setColour(juce::Colour(80, 180, 255));
        g.fillRoundedRectangle(static_cast<float>(legendX), static_cast<float>(legendY), 10.0f, 10.0f, 2.0f);
        g.setColour(juce::Colour(200, 200, 210));
        g.drawText("Current", legendX + 15, legendY - 1, 50, 12, juce::Justification::left);
        
        g.setColour(juce::Colour(255, 140, 80));
        g.fillRoundedRectangle(static_cast<float>(legendX + 70), static_cast<float>(legendY), 10.0f, 10.0f, 2.0f);
        g.drawText("Reference", legendX + 85, legendY - 1, 60, 12, juce::Justification::left);
    }
    
    void timerCallback() override { repaint(); }
    
private:
    PromasterOneAudioProcessor& processor;
};

class SuggestionsPanel : public juce::Component, public juce::Timer
{
public:
    SuggestionsPanel(PromasterOneAudioProcessor& p) : processor(p) { startTimerHz(10); }
    
    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        
        g.setColour(juce::Colour(35, 35, 42));
        g.fillRoundedRectangle(bounds, 8.0f);
        
        g.setColour(juce::Colour(140, 140, 155));
        g.setFont(juce::Font(12.0f, juce::Font::bold));
        g.drawText("Suggestions", bounds.getX() + 10, bounds.getY() + 5, bounds.getWidth() - 20, 20, juce::Justification::left);
        
        auto suggestions = processor.getSuggestions();
        float y = bounds.getY() + 30;
        
        for (const auto& suggestion : suggestions)
        {
            if (y + 30 > bounds.getBottom()) break;
            
            juce::Colour iconColour = suggestion.severity > 0.5f ? juce::Colour(255, 90, 90) : 
                                      suggestion.severity > 0.0f ? juce::Colour(255, 200, 80) : 
                                      juce::Colour(80, 220, 150);
            
            g.setColour(iconColour);
            g.fillEllipse(bounds.getX() + 15, y + 3, 8, 8);
            
            g.setColour(juce::Colour(220, 220, 230));
            g.setFont(11.0f);
            g.drawText(suggestion.messageJP, static_cast<int>(bounds.getX() + 30), static_cast<int>(y), 
                       static_cast<int>(bounds.getWidth() - 40), 14, juce::Justification::left);
            
            g.setColour(juce::Colour(160, 160, 175));
            g.setFont(10.0f);
            g.drawText(suggestion.message, static_cast<int>(bounds.getX() + 30), static_cast<int>(y + 12), 
                       static_cast<int>(bounds.getWidth() - 40), 12, juce::Justification::left);
            
            y += 28;
        }
    }
    
    void timerCallback() override { repaint(); }
    
private:
    PromasterOneAudioProcessor& processor;
};

class AdjustButton : public juce::Component, public juce::Timer
{
public:
    AdjustButton(PromasterOneAudioProcessor& p) : processor(p) { startTimerHz(30); }
    
    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(2.0f);
        
        float adj = processor.getAdjustAmount();
        if (adj > 0.0f || isHovered)
        {
            g.setColour(juce::Colour(60, 200, 140).withAlpha(isHovered ? 0.4f : adj * 0.3f));
            g.fillRoundedRectangle(bounds.expanded(5.0f), 12.0f);
        }
        
        juce::Colour buttonColour = juce::Colour(60, 200, 140);
        if (isPressed) buttonColour = buttonColour.darker(0.2f);
        else if (isHovered) buttonColour = buttonColour.brighter(0.2f);
        
        g.setColour(buttonColour);
        g.fillRoundedRectangle(bounds, 10.0f);
        
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(24.0f, juce::Font::bold));
        g.drawText("ADJUST", bounds, juce::Justification::centred);
    }
    
    void mouseEnter(const juce::MouseEvent&) override { isHovered = true; repaint(); }
    void mouseExit(const juce::MouseEvent&) override { isHovered = false; repaint(); }
    void mouseDown(const juce::MouseEvent&) override { isPressed = true; repaint(); }
    
    void mouseUp(const juce::MouseEvent& e) override
    {
        isPressed = false;
        if (getLocalBounds().contains(e.getPosition()))
            processor.applyAdjust();
        repaint();
    }
    
    void timerCallback() override { repaint(); }
    
private:
    PromasterOneAudioProcessor& processor;
    bool isHovered = false;
    bool isPressed = false;
};

class PresetSelector : public juce::Component
{
public:
    PresetSelector(PromasterOneAudioProcessor& p) : processor(p)
    {
        comboBox.addItem("-- Select Preset --", 1);
        const auto& presets = processor.getPresets();
        for (int i = 0; i < static_cast<int>(presets.size()); ++i)
            comboBox.addItem(presets[(size_t)i].name, i + 2);
        
        comboBox.setSelectedId(1);
        comboBox.onChange = [this]() {
            int id = comboBox.getSelectedId();
            if (id > 1)
                processor.selectPreset(id - 2);
        };
        
        addAndMakeVisible(comboBox);
        
        loadButton.setButtonText("Load Audio");
        loadButton.onClick = [this]() { loadReferenceFile(); };
        addAndMakeVisible(loadButton);
    }
    
    void resized() override
    {
        auto bounds = getLocalBounds();
        comboBox.setBounds(bounds.removeFromTop(30));
        bounds.removeFromTop(5);
        loadButton.setBounds(bounds.removeFromTop(30));
    }
    
private:
    void loadReferenceFile()
    {
        fileChooser = std::make_unique<juce::FileChooser>(
            "Select Reference Track",
            juce::File::getSpecialLocation(juce::File::userMusicDirectory),
            "*.wav;*.mp3;*.aiff;*.flac");
        
        fileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this](const juce::FileChooser& fc) {
                auto file = fc.getResult();
                if (file.existsAsFile())
                    processor.loadReferenceFromFile(file);
            });
    }
    
    PromasterOneAudioProcessor& processor;
    juce::ComboBox comboBox;
    juce::TextButton loadButton;
    std::unique_ptr<juce::FileChooser> fileChooser;
};

class PromasterOneAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    PromasterOneAudioProcessorEditor(PromasterOneAudioProcessor&);
    ~PromasterOneAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    
    PromasterOneAudioProcessor& audioProcessor;
    
    SpectrumDisplay spectrumDisplay;
    SuggestionsPanel suggestionsPanel;
    AdjustButton adjustButton;
    PresetSelector presetSelector;
    
    juce::Label titleLabel;
    juce::Label referenceLabel;
    
    juce::Slider inputGainSlider, outputGainSlider;
    juce::Label inputGainLabel, outputGainLabel;
    
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> inputGainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputGainAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PromasterOneAudioProcessorEditor)
};
