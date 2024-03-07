#pragma once

#include <juce-extensions/audio/metering/LevelMeter.h>
#include <juce_gui_basics/juce_gui_basics.h>

/**
 * Class which displays a scale.
 */
class ScaleComponent : public juce::Component
{
public:
    explicit ScaleComponent (const LevelMeter::Scale& scale = LevelMeter::Scale::getDefaultScale()) : mScale (scale) {}

    void paint (juce::Graphics& g) override;

private:
    const LevelMeter::Scale& mScale;
};
