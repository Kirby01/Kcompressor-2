#include "PluginProcessor.h"
#include "PluginEditor.h"

static inline double softsignPressure (double x, double shapeK)
{
    // d / (1 + shapeK * abs(d))  (your "no tanh" soft clamp)
    return x / (1.0 + shapeK * std::abs(x));
}

Kcompressor2AudioProcessor::Kcompressor2AudioProcessor()
: AudioProcessor (BusesProperties()
                  .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                  .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
  apvts (*this, nullptr, "PARAMS", createParameterLayout())
{
    apvts.addParameterListener (paramLookaheadId, this);
}

Kcompressor2AudioProcessor::~Kcompressor2AudioProcessor()
{
    apvts.removeParameterListener (paramLookaheadId, this);
}

const juce::String Kcompressor2AudioProcessor::getName() const { return "Kcompressor2"; }
bool Kcompressor2AudioProcessor::acceptsMidi() const { return false; }
bool Kcompressor2AudioProcessor::producesMidi() const { return false; }
bool Kcompressor2AudioProcessor::isMidiEffect() const { return false; }
double Kcompressor2AudioProcessor::getTailLengthSeconds() const { return 0.0; }

int Kcompressor2AudioProcessor::getNumPrograms() { return 1; }
int Kcompressor2AudioProcessor::getCurrentProgram() { return 0; }
void Kcompressor2AudioProcessor::setCurrentProgram (int) {}
const juce::String Kcompressor2AudioProcessor::getProgramName (int) { return {}; }
void Kcompressor2AudioProcessor::changeProgramName (int, const juce::String&) {}

juce::AudioProcessorValueTreeState::ParameterLayout
Kcompressor2AudioProcessor::createParameterLayout()
{
    using APF = juce::AudioParameterFloat;
    using APC = juce::AudioParameterChoice;

    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    p.push_back (std::make_unique<APF> (paramInputId, "Input",
                                        juce::NormalisableRange<float> (0.0f, 4.0f, 0.01f), 1.0f));

    p.push_back (std::make_unique<APF> (paramOutputId, "Output",
                                        juce::NormalisableRange<float> (0.0f, 4.0f, 0.01f), 1.0f));

    p.push_back (std::make_unique<APF> (paramLookaheadId, "Lookahead (ms)",
                                        juce::NormalisableRange<float> (0.0f, 50.0f, 0.1f), 5.0f));

    p.push_back (std::make_unique<APF> (paramATimingId, "A timing",
                                        juce::NormalisableRange<float> (0.0001f, 0.03f, 0.0001f), 0.01f));

    p.push_back (std::make_unique<APF> (paramDriveId, "Drive",
                                        juce::NormalisableRange<float> (0.01f, 20.0f, 0.01f), 10.0f));

    p.push_back (std::make_unique<APF> (paramPressureId, "Pressure",
                                        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.27f));

    p.push_back (std::make_unique<APC> (paramModeId, "Delta Mode",
                                        juce::StringArray { "Normal", "GR delta", "Pressure delta" }, 0));

    return { p.begin(), p.end() };
}

void Kcompressor2AudioProcessor::allocateDelayIfNeeded (double sr)
{
    // maxlen ~ 1 second (like your 48001 @48k)
    maxDelaySamples = (int) std::ceil (sr * 1.0) + 1;
    if (maxDelaySamples < 2) maxDelaySamples = 2;

    delayBuffer.setSize (2, maxDelaySamples);
    delayBuffer.clear();
    wpos = 0;
}

void Kcompressor2AudioProcessor::prepareToPlay (double sr, int)
{
    currentSampleRate = sr > 0.0 ? sr : 48000.0;

    allocateDelayIfNeeded (currentSampleRate);

    a = 1.0;
    b = 1.0;

    updateLatencyFromParam();
    setLatencySamples (desiredLatencySamples.load());
    currentLatencySamples = desiredLatencySamples.load();
}

void Kcompressor2AudioProcessor::releaseResources() {}

bool Kcompressor2AudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in  = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    return (in == juce::AudioChannelSet::stereo() && out == juce::AudioChannelSet::stereo());
}

void Kcompressor2AudioProcessor::parameterChanged (const juce::String& parameterID, float)
{
    if (parameterID == paramLookaheadId)
        updateLatencyFromParam();
}

void Kcompressor2AudioProcessor::updateLatencyFromParam()
{
    const float lookMs = apvts.getRawParameterValue (paramLookaheadId)->load();
    int buflen = (int) std::floor (lookMs * (float) currentSampleRate * 0.001f + 0.5f);

    if (buflen < 0) buflen = 0;
    if (buflen > (maxDelaySamples - 1)) buflen = (maxDelaySamples - 1);

    desiredLatencySamples.store (buflen);
}

void Kcompressor2AudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    const int numCh = buffer.getNumChannels();

    if (numCh < 2)
        return;

    // Apply PDC changes live
    const int wantLatency = desiredLatencySamples.load();
    if (wantLatency != currentLatencySamples)
    {
        currentLatencySamples = wantLatency;
        setLatencySamples (currentLatencySamples);
    }

    const float inGain  = apvts.getRawParameterValue (paramInputId)->load();
    const float outGain = apvts.getRawParameterValue (paramOutputId)->load();
    const float gaF     = apvts.getRawParameterValue (paramATimingId)->load();
    const float drvF    = apvts.getRawParameterValue (paramDriveId)->load();
    const float presF   = apvts.getRawParameterValue (paramPressureId)->load();
    const int mode      = (int) apvts.getRawParameterValue (paramModeId)->load();

    const double ga  = (double) gaF;
    const double drv = (double) drvF;
    const double pres = (double) presF;

    constexpr double shapeK = 6.0;   // your constant
    constexpr double eps = 1e-12;    // tiny safety so JUCE doesn’t NaN-bomb on rare edge cases

    auto* inLptr  = buffer.getWritePointer (0);
    auto* inRptr  = buffer.getWritePointer (1);

    auto* dLptr = delayBuffer.getWritePointer (0);
    auto* dRptr = delayBuffer.getWritePointer (1);

for (int n = 0; n < numSamples; ++n)
{
    // current (non-delayed) input
    const double inL = (double) inLptr[n] * (double) inGain;
    const double inR = (double) inRptr[n] * (double) inGain;

    // write to ring buffer
    dLptr[wpos] = (float) inL;
    dRptr[wpos] = (float) inR;

    // read delayed sample
    int rd = wpos - currentLatencySamples;
    if (rd < 0) rd += maxDelaySamples;

    const double l = (double) dLptr[rd];
    const double r = (double) dRptr[rd];

    // detector (EXACT)
    const double det = 0.5 * (inL + inR);

    // -------- EXACT CORE (NO EPS, NO SAFETY) --------
    // a = (1-ga)*b + ga*abs(det)*drv;
    a = (1.0 - ga) * b + ga * std::abs(det) * drv;

    // b = (1-abs(b-a)/a^2)*(a+abs(b-a)) + (abs(b-a)/a^2)*pow(b, abs(a/b));
    const double da = std::abs(b - a);
    const double t  = da / (a * a);

    b = (1.0 - t) * (a + da)
      + t * std::pow(b, std::abs(a / b));
    // -----------------------------------------------

    // wet output (lookahead-aligned)
    const double wetL = (l / b) * (double) outGain;
    const double wetR = (r / b) * (double) outGain;

    // pressed version (1 / b^2)
    const double pressedL = wetL / b;
    const double pressedR = wetR / b;

    const double dL = pressedL - wetL;
    const double dR = pressedR - wetR;

    // pressure soft-clamp (unchanged)
    const double dLs = dL / (1.0 + shapeK * std::abs(dL));
    const double dRs = dR / (1.0 + shapeK * std::abs(dR));

    const double outL = wetL + pres * dLs;
    const double outR = wetR + pres * dRs;

    // delta modes
    if (mode == 0)
    {
        inLptr[n] = (float) outL;
        inRptr[n] = (float) outR;
    }
    else if (mode == 1)
    {
        const double dryL = l * (double) outGain;
        const double dryR = r * (double) outGain;
        inLptr[n] = (float) (dryL - wetL);
        inRptr[n] = (float) (dryR - wetR);
    }
    else
    {
        inLptr[n] = (float) (outL - wetL);
        inRptr[n] = (float) (outR - wetR);
    }

    // advance ring buffer
    ++wpos;
    if (wpos >= maxDelaySamples)
        wpos = 0;
}

}

bool Kcompressor2AudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* Kcompressor2AudioProcessor::createEditor()
{
    return new Kcompressor2AudioProcessorEditor (*this);
}

void Kcompressor2AudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void Kcompressor2AudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));

    if (xml && xml->hasTagName (apvts.state.getType()))
    {
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
        updateLatencyFromParam();
    }
}

// This creates new instances of the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Kcompressor2AudioProcessor();
}
