#include "PluginEditor.h"

Kcompressor2AudioProcessorEditor::Kcompressor2AudioProcessorEditor (Kcompressor2AudioProcessor& p)
: AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (520, 260);

    titleLabel.setText ("Kcompressor2", juce::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    titleLabel.setFont (juce::Font (18.0f, juce::Font::bold));
    addAndMakeVisible (titleLabel);

    auto make = [&](juce::Slider& s, juce::Label& l, const juce::String& name)
    {
        styleSlider (s);
        addAndMakeVisible (s);

        l.setText (name, juce::dontSendNotification);
        styleLabel (l);
        addAndMakeVisible (l);
    };

    make (inputSlider,    inputLabel,    "Input");
    make (outputSlider,   outputLabel,   "Output");
    make (lookSlider,     lookLabel,     "Lookahead (ms)");
    make (aTimingSlider,  aTimingLabel,  "A timing");
    make (driveSlider,    driveLabel,    "Drive");
    make (pressureSlider, pressureLabel, "Pressure");

    modeLabel.setText ("Delta Mode", juce::dontSendNotification);
    styleLabel (modeLabel);
    addAndMakeVisible (modeLabel);

    modeBox.addItem ("Normal", 1);
    modeBox.addItem ("GR delta", 2);
    modeBox.addItem ("Pressure delta", 3);
    modeBox.setColour (juce::ComboBox::textColourId, juce::Colours::white);
    modeBox.setColour (juce::ComboBox::backgroundColourId, juce::Colours::black);
    modeBox.setColour (juce::ComboBox::outlineColourId, juce::Colours::white.withAlpha (0.25f));
    addAndMakeVisible (modeBox);

    // Attachments
    inputAtt    = std::make_unique<SliderAttachment> (audioProcessor.apvts, Kcompressor2AudioProcessor::paramInputId, inputSlider);
    outputAtt   = std::make_unique<SliderAttachment> (audioProcessor.apvts, Kcompressor2AudioProcessor::paramOutputId, outputSlider);
    lookAtt     = std::make_unique<SliderAttachment> (audioProcessor.apvts, Kcompressor2AudioProcessor::paramLookaheadId, lookSlider);
    aTimingAtt  = std::make_unique<SliderAttachment> (audioProcessor.apvts, Kcompressor2AudioProcessor::paramATimingId, aTimingSlider);
    driveAtt    = std::make_unique<SliderAttachment> (audioProcessor.apvts, Kcompressor2AudioProcessor::paramDriveId, driveSlider);
    pressureAtt = std::make_unique<SliderAttachment> (audioProcessor.apvts, Kcompressor2AudioProcessor::paramPressureId, pressureSlider);
    modeAtt     = std::make_unique<ComboAttachment>  (audioProcessor.apvts, Kcompressor2AudioProcessor::paramModeId, modeBox);
}

Kcompressor2AudioProcessorEditor::~Kcompressor2AudioProcessorEditor() {}

void Kcompressor2AudioProcessorEditor::styleSlider (juce::Slider& s)
{
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 90, 20);
    s.setColour (juce::Slider::textBoxTextColourId, juce::Colours::white);
    s.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::black);
    s.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::white.withAlpha (0.25f));
    s.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colours::white.withAlpha (0.25f));
    s.setColour (juce::Slider::rotarySliderFillColourId, juce::Colours::white);
}

void Kcompressor2AudioProcessorEditor::styleLabel (juce::Label& l)
{
    l.setColour (juce::Label::textColourId, juce::Colours::white);
    l.setJustificationType (juce::Justification::centred);
    l.setFont (juce::Font (13.0f));
}

void Kcompressor2AudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
    g.setColour (juce::Colours::white.withAlpha (0.12f));
    g.drawRoundedRectangle (12.0f, 12.0f, (float) getWidth() - 24.0f, (float) getHeight() - 24.0f, 12.0f, 1.5f);
}

void Kcompressor2AudioProcessorEditor::resized()
{
    const int pad = 16;
    const int top = 16;

    titleLabel.setBounds (pad, top, getWidth() - pad*2, 24);

    const int rowY1 = 52;
    const int rowY2 = 150;

    const int knobW = 90;
    const int knobH = 90;
    const int gap = 14;

    int x = pad;

    auto place = [&](juce::Slider& s, juce::Label& l, int y)
    {
        l.setBounds (x, y, knobW, 18);
        s.setBounds (x, y + 18, knobW, knobH);
        x += knobW + gap;
    };

    x = pad;
    place (inputSlider,   inputLabel,   rowY1);
    place (outputSlider,  outputLabel,  rowY1);
    place (lookSlider,    lookLabel,    rowY1);
    place (aTimingSlider, aTimingLabel, rowY1);

    x = pad;
    place (driveSlider,    driveLabel,    rowY2);
    place (pressureSlider, pressureLabel, rowY2);

    modeLabel.setBounds (x, rowY2, 120, 18);
    modeBox.setBounds (x, rowY2 + 22, 160, 26);
}
