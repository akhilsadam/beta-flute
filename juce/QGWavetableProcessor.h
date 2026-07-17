#pragma once

#include <JuceHeader.h>
#include "QGWavetable.h"

class QGWavetableAudioProcessor : public juce::AudioProcessor
{
public:
    QGWavetableAudioProcessor();
    ~QGWavetableAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    juce::String getServerStatus() const;
    void setSamplingPosition(int x, int y);
    QGWavetableBuffer* getWaveBuffer() { return &waveBuffer; }

    const juce::String getName() const override { return "QG Wavetable"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override {}
    void setStateInformation(const void* data, int sizeInBytes) override {}

    // Parameters
    juce::AudioParameterFloat* attackParam;
    juce::AudioParameterFloat* releaseParam;
    juce::AudioParameterFloat* cutoffParam;
    juce::AudioParameterFloat* resonanceParam;

private:
    QGWavetableBuffer waveBuffer;
    QGServerProcess serverProcess;
    QGSocketClient socketClient;

    double sampleRate = 44100.0;
    bool serverStarted = false;

    // Voice state (monophonic for now)
    float phase = 0.0f;
    float velocity = 0.0f;
    float envelope = 0.0f;
    bool noteOn = false;
    int currentMidiNote = 60;  // Middle C
    int samplingPosX = 0;
    int samplingPosY = 0;

    // Simple 1-pole lowpass
    float filterState = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(QGWavetableAudioProcessor)
};
