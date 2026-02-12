#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class Kcompressor2AudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    explicit Kcompressor2AudioProcessorEditor (Kcompressor2AudioProcessor&);
    ~Kcompressor2AudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    Kcompressor2AudioProcessor& audioProcessor;

    juce::Slider inputSlider, outputSlider, lookSlider, aTimingSlider, driveSlider, pressureSlider;
    juce::ComboBox modeBox;

    juce::Label inputLabel, outputLabel, lookLabel, aTimingLabel, driveLabel, pressureLabel, modeLabel, titleLabel;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<SliderAttachment> inputAtt, outputAtt, lookAtt, aTimingAtt, driveAtt, pressureAtt;
    std::unique_ptr<ComboAttachment>  modeAtt;

    void styleSlider (juce::Slider& s);
    void styleLabel (juce::Label& l);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Kcompressor2AudioProcessorEditor)
};
