#pragma once

#include <JuceHeader.h>
#include "QGWavetable.h"

class QGWavetableVisualizer : public juce::Component,
                               private juce::Timer
{
public:
    QGWavetableVisualizer(QGWavetableBuffer* buf) : waveBuffer(buf)
    {
        setSize(300, 100);
        startTimer(50);
    }

    ~QGWavetableVisualizer() override
    {
        stopTimer();
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xff1a1a1a));
        g.setColour(juce::Colour(0xff00aa00));

        juce::Path waveform;
        int w = getWidth();
        int h = getHeight();

        for (int i = 0; i < w; ++i)
        {
            float phase = (float)i / w;
            float sample = waveBuffer->getSample(phase);
            float y = h * 0.5f - sample * h * 0.4f;

            if (i == 0)
                waveform.startNewSubPath(i, y);
            else
                waveform.lineTo(i, y);
        }

        g.strokePath(waveform, juce::PathStrokeType(1.0f));

        // Grid
        g.setColour(juce::Colour(0xff333333));
        g.drawHorizontalLine(h / 2, 0, w);
    }

    void timerCallback() override
    {
        repaint();
    }

private:
    QGWavetableBuffer* waveBuffer;
};


class QGSamplingPad : public juce::Component,
                      public juce::MouseListener
{
public:
    QGSamplingPad(int gridW, int gridH)
        : gridWidth(gridW), gridHeight(gridH), selectedX(0), selectedY(0)
    {
        addMouseListener(this, true);
        setSize(200, 200);
    }

    ~QGSamplingPad() override
    {
        removeMouseListener(this);
    }

    void setCallback(std::function<void(int, int)> cb)
    {
        callback = cb;
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xff0a0a0a));

        // Grid background
        g.setColour(juce::Colour(0xff222222));
        for (int i = 0; i <= 4; ++i)
        {
            int x = (i * getWidth()) / 4;
            int y = (i * getHeight()) / 4;
            g.drawVerticalLine(x, 0, getHeight());
            g.drawHorizontalLine(y, 0, getWidth());
        }

        // Selected point
        float px = (float)selectedX / gridWidth * getWidth();
        float py = (float)selectedY / gridHeight * getHeight();

        g.setColour(juce::Colour(0xffff6600));
        g.fillEllipse(px - 5, py - 5, 10, 10);

        g.setColour(juce::Colour(0xffcccccc));
        g.setFont(12.0f);
        g.drawText(juce::String(selectedX) + ", " + juce::String(selectedY),
                   juce::Rectangle<int>(5, 5, getWidth() - 10, 20),
                   juce::Justification::topLeft, true);
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        updatePosition(e.getPosition());
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        updatePosition(e.getPosition());
    }

    void updatePosition(juce::Point<int> pos)
    {
        selectedX = (pos.x * gridWidth) / getWidth();
        selectedY = (pos.y * gridHeight) / getHeight();

        selectedX = juce::jlimit(0, gridWidth - 1, selectedX);
        selectedY = juce::jlimit(0, gridHeight - 1, selectedY);

        if (callback)
            callback(selectedX, selectedY);

        repaint();
    }

    void setSamplingPoint(int x, int y)
    {
        selectedX = x;
        selectedY = y;
        repaint();
    }

private:
    int gridWidth, gridHeight;
    int selectedX, selectedY;
    std::function<void(int, int)> callback;
};


class QGConfigPanel : public juce::Component
{
public:
    QGConfigPanel() : gridNxValue(256), gridNyValue(256), viscosityValue(1e-5f),
                      restartIntervalValue(5.0f)
    {
        // Grid resolution
        addAndMakeVisible(gridNxLabel);
        gridNxLabel.setText("Grid Nx:", juce::dontSendNotification);

        addAndMakeVisible(gridNxSlider);
        gridNxSlider.setRange(64, 512, 1);
        gridNxSlider.setValue(gridNxValue);
        gridNxSlider.onValueChange = [this]() {
            gridNxValue = (int)gridNxSlider.getValue();
        };

        addAndMakeVisible(gridNyLabel);
        gridNyLabel.setText("Grid Ny:", juce::dontSendNotification);

        addAndMakeVisible(gridNySlider);
        gridNySlider.setRange(64, 512, 1);
        gridNySlider.setValue(gridNyValue);
        gridNySlider.onValueChange = [this]() {
            gridNyValue = (int)gridNySlider.getValue();
        };

        // Viscosity
        addAndMakeVisible(viscosityLabel);
        viscosityLabel.setText("Viscosity:", juce::dontSendNotification);

        addAndMakeVisible(viscositySlider);
        viscositySlider.setRange(-10, -1, 0.5);  // Log scale: 1e-10 to 1e-1
        viscositySlider.setValue(-5);
        viscositySlider.onValueChange = [this]() {
            viscosityValue = std::pow(10.0f, (float)viscositySlider.getValue());
        };

        // Restart interval
        addAndMakeVisible(restartLabel);
        restartLabel.setText("Restart Interval (s):", juce::dontSendNotification);

        addAndMakeVisible(restartSlider);
        restartSlider.setRange(1.0, 30.0, 0.5);
        restartSlider.setValue(restartIntervalValue);
        restartSlider.onValueChange = [this]() {
            restartIntervalValue = (float)restartSlider.getValue();
        };

        // Reinit on note toggle
        addAndMakeVisible(reinitOnNoteToggle);
        reinitOnNoteToggle.setButtonText("Reinit on Note");
        reinitOnNoteToggle.setToggleState(true, juce::dontSendNotification);
        reinitOnNoteToggle.onStateChange = [this]() {
            reinitOnNoteValue = reinitOnNoteToggle.getToggleState();
        };

        setSize(200, 150);
    }

    void resized() override
    {
        int y = 5;
        int h = 20;
        int gap = 5;

        gridNxLabel.setBounds(5, y, 80, h);
        gridNxSlider.setBounds(85, y, 110, h);
        y += h + gap;

        gridNyLabel.setBounds(5, y, 80, h);
        gridNySlider.setBounds(85, y, 110, h);
        y += h + gap;

        viscosityLabel.setBounds(5, y, 80, h);
        viscositySlider.setBounds(85, y, 110, h);
        y += h + gap;

        restartLabel.setBounds(5, y, 80, h);
        restartSlider.setBounds(85, y, 110, h);
        y += h + gap + 5;

        reinitOnNoteToggle.setBounds(5, y, 190, 25);
    }

    int getGridNx() const { return gridNxValue; }
    int getGridNy() const { return gridNyValue; }
    float getViscosity() const { return viscosityValue; }
    float getRestartInterval() const { return restartIntervalValue; }
    bool getReinitOnNote() const { return reinitOnNoteValue; }

private:
    juce::Label gridNxLabel, gridNyLabel, viscosityLabel, restartLabel;
    juce::Slider gridNxSlider, gridNySlider, viscositySlider, restartSlider;
    juce::ToggleButton reinitOnNoteToggle;

    int gridNxValue, gridNyValue;
    float viscosityValue, restartIntervalValue;
    bool reinitOnNoteValue;
};
