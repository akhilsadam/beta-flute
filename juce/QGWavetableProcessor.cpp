#include "QGWavetableProcessor.h"
#include "QGWavetableEditor.h"

QGWavetableAudioProcessor::QGWavetableAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    addParameter(attackParam = new juce::AudioParameterFloat("attack", "Attack", 0.0f, 1.0f, 0.01f));
    addParameter(releaseParam = new juce::AudioParameterFloat("release", "Release", 0.0f, 1.0f, 0.1f));
    addParameter(cutoffParam = new juce::AudioParameterFloat("cutoff", "Cutoff", 20.0f, 20000.0f, 5000.0f));
    addParameter(resonanceParam = new juce::AudioParameterFloat("resonance", "Resonance", 0.1f, 10.0f, 1.0f));

    // Auto-start server on initialization
    if (serverProcess.start())
    {
        serverStarted = true;
        std::this_thread::sleep_for(std::chrono::milliseconds(serverProcess.getStartupDelay()));
        socketClient.start(&waveBuffer);
    }
}

QGWavetableAudioProcessor::~QGWavetableAudioProcessor()
{
    socketClient.stop();
    serverProcess.stop();
}

void QGWavetableAudioProcessor::prepareToPlay(double sr, int samplesPerBlock)
{
    sampleRate = sr;
}

void QGWavetableAudioProcessor::releaseResources()
{
}

void QGWavetableAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    auto outputL = buffer.getWritePointer(0);
    auto outputR = buffer.getWritePointer(1);
    auto numSamples = buffer.getNumSamples();

    waveBuffer.updateBuffer();

    float attackSamples = attackParam->get() * sampleRate * 0.5f;  // Max 0.5 sec
    float releaseSamples = releaseParam->get() * sampleRate * 2.0f;  // Max 2 sec
    float cutoff = cutoffParam->get();
    float resonance = resonanceParam->get();

    // Simple lowpass coefficient
    float cutoffNorm = cutoff / (sampleRate * 0.5f);
    cutoffNorm = juce::jlimit(0.001f, 0.999f, cutoffNorm);
    float alpha = cutoffNorm / (resonance + cutoffNorm);

    for (int i = 0; i < numSamples; ++i)
    {
        // Process MIDI
        for (auto midi : midiMessages)
        {
            auto msg = midi.getMessage();
            if (msg.isNoteOn())
            {
                noteOn = true;
                velocity = msg.getVelocity() / 127.0f;
                currentMidiNote = msg.getNoteNumber();
                phase = 0.0f;
                envelope = 0.0f;

                // Trigger server reinit if enabled
                socketClient.sendCommand("NOTE");
            }
            else if (msg.isNoteOff())
            {
                noteOn = false;
            }
        }

        // ADSR envelope
        if (noteOn && envelope < 1.0f)
        {
            envelope += 1.0f / juce::jmax(1.0f, attackSamples);
            envelope = juce::jmin(1.0f, envelope);
        }
        else if (!noteOn && envelope > 0.0f)
        {
            envelope -= 1.0f / juce::jmax(1.0f, releaseSamples);
            envelope = juce::jmax(0.0f, envelope);
        }

        // Pitch: MIDI note → frequency
        float freq = 440.0f * std::pow(2.0f, (currentMidiNote - 69) / 12.0f);  // A4 = MIDI 69
        float phase_increment = freq / (float)sampleRate;

        // Get sample from wavetable
        float sample = waveBuffer.getSample(phase);
        sample *= velocity * envelope;

        // Apply filter
        filterState += alpha * (sample - filterState);
        sample = filterState;

        outputL[i] = sample;
        outputR[i] = sample;

        phase += phase_increment;
        if (phase >= 1.0f)
            phase -= 1.0f;
    }

    midiMessages.clear();
}

juce::AudioProcessorEditor* QGWavetableAudioProcessor::createEditor()
{
    return new QGWavetableEditor(*this);
}

juce::String QGWavetableAudioProcessor::getServerStatus() const
{
    if (!serverStarted)
        return "Server failed to start. Ensure Python 3 + PyTorch are installed.";

    if (socketClient.isConnected())
        return "✓ Server running and connected\nStreaming waveforms at ~100 fps";

    return "⏳ Waiting for server to initialize...\n(This can take 2-3 seconds on first load)";
}

void QGWavetableAudioProcessor::setSamplingPosition(int x, int y)
{
    samplingPosX = x;
    samplingPosY = y;
    socketClient.sendSamplingPosition(x, y);
}
