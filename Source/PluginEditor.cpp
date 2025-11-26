/*
  PROMASTER ONE - GUI Editor Implementation
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

PromasterOneAudioProcessorEditor::PromasterOneAudioProcessorEditor(PromasterOneAudioProcessor& p)
    : AudioProcessorEditor(&p),
      audioProcessor(p),
      spectrumDisplay(p),
      suggestionsPanel(p),
      adjustButton(p),
      presetSelector(p)
{
    titleLabel.setText("PROMASTER ONE", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(28.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, juce::Colour(80, 180, 255));
    addAndMakeVisible(titleLabel);
    
    referenceLabel.setText("Reference: (None)", juce::dontSendNotification);
    referenceLabel.setFont(juce::Font(12.0f));
    referenceLabel.setColour(juce::Label::textColourId, juce::Colour(160, 160, 175));
    addAndMakeVisible(referenceLabel);
    
    addAndMakeVisible(spectrumDisplay);
    addAndMakeVisible(suggestionsPanel);
    addAndMakeVisible(adjustButton);
    addAndMakeVisible(presetSelector);
    
    inputGainSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    inputGainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 18);
    inputGainSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(80, 180, 255));
    addAndMakeVisible(inputGainSlider);
    
    inputGainLabel.setText("Input", juce::dontSendNotification);
    inputGainLabel.setJustificationType(juce::Justification::centred);
    inputGainLabel.setColour(juce::Label::textColourId, juce::Colour(160, 160, 175));
    addAndMakeVisible(inputGainLabel);
    
    inputGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "inputGain", inputGainSlider);
    
    outputGainSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    outputGainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 18);
    outputGainSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(255, 140, 80));
    addAndMakeVisible(outputGainSlider);
    
    outputGainLabel.setText("Output", juce::dontSendNotification);
    outputGainLabel.setJustificationType(juce::Justification::centred);
    outputGainLabel.setColour(juce::Label::textColourId, juce::Colour(160, 160, 175));
    addAndMakeVisible(outputGainLabel);
    
    outputGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "outputGain", outputGainSlider);
    
    setSize(800, 500);
    startTimer(500);
}

PromasterOneAudioProcessorEditor::~PromasterOneAudioProcessorEditor()
{
    stopTimer();
}

void PromasterOneAudioProcessorEditor::timerCallback()
{
    auto& profile = audioProcessor.getReferenceProfile();
    if (profile.isValid())
        referenceLabel.setText("Reference: " + profile.name, juce::dontSendNotification);
    else
        referenceLabel.setText("Reference: (None)", juce::dontSendNotification);
}

void PromasterOneAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(25, 25, 30));
    
    g.setColour(juce::Colour(55, 55, 65));
    g.drawHorizontalLine(50, 20, static_cast<float>(getWidth() - 20));
}

void PromasterOneAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    
    titleLabel.setBounds(20, 10, 250, 35);
    referenceLabel.setBounds(280, 20, 300, 20);
    
    auto contentArea = bounds.withTrimmedTop(60).reduced(15);
    
    auto rightPanel = contentArea.removeFromRight(180);
    presetSelector.setBounds(rightPanel.removeFromTop(70));
    
    auto knobArea = rightPanel.removeFromTop(100);
    inputGainLabel.setBounds(knobArea.getX(), knobArea.getY(), 80, 16);
    inputGainSlider.setBounds(knobArea.getX(), knobArea.getY() + 16, 80, 70);
    outputGainLabel.setBounds(knobArea.getX() + 90, knobArea.getY(), 80, 16);
    outputGainSlider.setBounds(knobArea.getX() + 90, knobArea.getY() + 16, 80, 70);
    
    spectrumDisplay.setBounds(contentArea.removeFromTop(220).reduced(5));
    
    auto buttonArea = contentArea.removeFromTop(65);
    int buttonWidth = 220;
    int buttonX = (buttonArea.getWidth() - buttonWidth) / 2;
    adjustButton.setBounds(buttonArea.getX() + buttonX, buttonArea.getY() + 5, buttonWidth, 55);
    
    suggestionsPanel.setBounds(contentArea.reduced(5));
}
