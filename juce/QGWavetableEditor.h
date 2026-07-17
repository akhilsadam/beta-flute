#pragma once

#include <JuceHeader.h>
#include "QGWavetableProcessor.h"
#include "QGWavetableComponents.h"

class QGWavetableEditor : public juce::AudioProcessorEditor,
                          private juce::Timer
{
public:
    QGWavetableEditor(QGWavetableAudioProcessor& p)
        : AudioProcessorEditor(&p), processor(p), visualizer(p.getWaveBuffer()),
          samplingPad(256, 256)
    {
        samplingPad.setCallback([this](int x, int y) {
            processor.setSamplingPosition(x, y);
        });

        setSize(700, 500);
        setResizable(true, true);

        // Status label
        addAndMakeVisible(statusLabel);
        statusLabel.setText("Initializing...", juce::dontSendNotification);
        statusLabel.setJustificationType(juce::Justification::centredLeft);
        statusLabel.setColour(juce::Label::textColourId, juce::Colour(0xff00ff00));

        // Waveform visualizer
        addAndMakeVisible(visualizer);

        // XY sampling pad
        addAndMakeVisible(samplingPad);

        // Config panel
        addAndMakeVisible(configPanel);

        // ADSR + Filter params
        addAndMakeVisible(attackLabel);
        attackLabel.setText("Attack", juce::dontSendNotification);
        addAndMakeVisible(attackSlider);
        attackSlider.setSliderStyle(juce::Slider::Rotary);
        attackSlider.setRange(0.0, 1.0, 0.01);
        if (auto* param = processor.attackParam)
            attackSlider.setValue(param->get());

        addAndMakeVisible(releaseLabel);
        releaseLabel.setText("Release", juce::dontSendNotification);
        addAndMakeVisible(releaseSlider);
        releaseSlider.setSliderStyle(juce::Slider::Rotary);
        releaseSlider.setRange(0.0, 1.0, 0.01);
        if (auto* param = processor.releaseParam)
            releaseSlider.setValue(param->get());

        addAndMakeVisible(cutoffLabel);
        cutoffLabel.setText("Cutoff", juce::dontSendNotification);
        addAndMakeVisible(cutoffSlider);
        cutoffSlider.setSliderStyle(juce::Slider::Rotary);
        cutoffSlider.setRange(20.0, 20000.0, 1.0);
        cutoffSlider.setSkewFactorFromMidPoint(5000.0);  // Log scale
        if (auto* param = processor.cutoffParam)
            cutoffSlider.setValue(param->get());

        addAndMakeVisible(resonanceLabel);
        resonanceLabel.setText("Resonance", juce::dontSendNotification);
        addAndMakeVisible(resonanceSlider);
        resonanceSlider.setSliderStyle(juce::Slider::Rotary);
        resonanceSlider.setRange(0.1, 10.0, 0.1);
        if (auto* param = processor.resonanceParam)
            resonanceSlider.setValue(param->get());

        startTimer(500);
    }

    ~QGWavetableEditor() override
    {
        stopTimer();
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xff1a1a1a));
        g.setColour(juce::Colour(0xff333333));
        g.drawRect(getLocalBounds(), 1);

        // Titles
        g.setColour(juce::Colour(0xffcccccc));
        g.setFont(14.0f);
        g.drawText("QG Wavetable Synth", 10, 10, 200, 20, juce::Justification::topLeft);
    }

    void resized() override
    {
        int w = getWidth();
        int h = getHeight();

        // Top status
        statusLabel.setBounds(10, 30, w - 20, 20);

        // Layout: Left (controls), Middle (visualizer + pad), Right (config)
        int leftW = 150;
        int rightW = 200;
        int midW = w - leftW - rightW - 30;

        // Left: ADSR + Filter knobs
        int knobSize = 60;
        int x = 10, y = 60;

        attackLabel.setBounds(x, y, knobSize, 15);
        attackSlider.setBounds(x, y + 15, knobSize, knobSize);
        y += knobSize + 40;

        releaseLabel.setBounds(x, y, knobSize, 15);
        releaseSlider.setBounds(x, y + 15, knobSize, knobSize);
        y += knobSize + 40;

        cutoffLabel.setBounds(x, y, knobSize, 15);
        cutoffSlider.setBounds(x, y + 15, knobSize, knobSize);
        y += knobSize + 40;

        resonanceLabel.setBounds(x, y, knobSize, 15);
        resonanceSlider.setBounds(x, y + 15, knobSize, knobSize);

        // Middle: Visualizer and Sampling Pad
        x = leftW + 20;
        visualizer.setBounds(x, 60, midW - 10, 120);

        int padSize = juce::jmin(200, midW - 10);
        samplingPad.setBounds(x, 190, padSize, padSize);

        // Right: Config panel
        x = w - rightW - 10;
        configPanel.setBounds(x, 60, rightW, 400);
    }

    void timerCallback() override
    {
        auto status = processor.getServerStatus();
        statusLabel.setText(status, juce::dontSendNotification);

        if (auto* param = processor.attackParam)
            attackSlider.setValue(param->get(), juce::dontSendNotification);
        if (auto* param = processor.releaseParam)
            releaseSlider.setValue(param->get(), juce::dontSendNotification);
        if (auto* param = processor.cutoffParam)
            cutoffSlider.setValue(param->get(), juce::dontSendNotification);
        if (auto* param = processor.resonanceParam)
            resonanceSlider.setValue(param->get(), juce::dontSendNotification);
    }

    void setSamplingPoint(int x, int y)
    {
        samplingPad.setSamplingPoint(x, y);
    }

private:
    QGWavetableAudioProcessor& processor;

    juce::Label statusLabel;
    QGWavetableVisualizer visualizer;
    QGSamplingPad samplingPad;
    QGConfigPanel configPanel;

    juce::Label attackLabel, releaseLabel, cutoffLabel, resonanceLabel;
    juce::Slider attackSlider, releaseSlider, cutoffSlider, resonanceSlider;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(QGWavetableEditor)
};
