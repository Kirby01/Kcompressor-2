#pragma once
#include <JuceHeader.h>

class Kcompressor2AudioProcessor  : public juce::AudioProcessor,
                                    private juce::AudioProcessorValueTreeState::Listener
{
public:
    Kcompressor2AudioProcessor();
    ~Kcompressor2AudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    juce::AudioProcessorValueTreeState apvts;

    static constexpr const char* paramInputId     = "input";
    static constexpr const char* paramOutputId    = "output";
    static constexpr const char* paramLookaheadId = "lookaheadMs";
    static constexpr const char* paramATimingId   = "aTiming";
    static constexpr const char* paramDriveId     = "drive";
    static constexpr const char* paramPressureId  = "pressure";
    static constexpr const char* paramModeId      = "deltaMode";

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    void parameterChanged (const juce::String& parameterID, float newValue) override;
    void updateLatencyFromParam();

    // Lookahead buffer (max ~1s like your maxlen=48001 @48k)
    void allocateDelayIfNeeded (double sr);

    juce::AudioBuffer<float> delayBuffer; // 2ch ring buffer
    int maxDelaySamples = 1;
    int wpos = 0;

    std::atomic<int> desiredLatencySamples { 0 };
    int currentLatencySamples = 0;

    // Core state
    double a = 1.0;
    double b = 1.0;

    double currentSampleRate = 48000.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Kcompressor2AudioProcessor)
};
