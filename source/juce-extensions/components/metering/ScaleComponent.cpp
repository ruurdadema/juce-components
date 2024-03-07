#include "ScaleComponent.h"
#include "LevelMeterComponent.h"

void ScaleComponent::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    bool isHorizontal = getWidth() > getHeight();

    const auto& divisions = mScale.getDivisions();

    const auto scaleLineLength = 6.f;
    for (auto division = divisions.begin(); division != divisions.end(); ++division)
    {
        const auto scaleNumberWidth = 30;

        bool isFirst = division == divisions.begin();

        if (isHorizontal)
        {
            auto xPos = b.getX() + (b.getWidth() - LevelMeterComponent::kOverloadAreaSize) *
                                       mScale.calculateProportionForLevelDb (*division);

            if (!isFirst)
            {
                g.drawVerticalLine (juce::roundToInt (xPos), 0.f, scaleLineLength);
                g.drawText (
                    juce::String (*division),
                    juce::roundToInt (xPos) - scaleNumberWidth / 2,
                    juce::roundToInt (scaleLineLength),
                    scaleNumberWidth,
                    juce::roundToInt (b.getHeight()),
                    juce::Justification::centredTop);
            }
        }
        else
        {
            const auto scaleNumberHeight = 20;

            auto yPos = b.getBottom() - (b.getHeight() - LevelMeterComponent::kOverloadAreaSize) *
                                            mScale.calculateProportionForLevelDb (*division);

            if (!isFirst)
            {
                g.drawHorizontalLine (juce::roundToInt (yPos), 0.f, scaleLineLength);
                g.drawText (
                    juce::String (*division),
                    juce::roundToInt (scaleLineLength),
                    juce::roundToInt (yPos) - scaleNumberHeight / 2,
                    scaleNumberWidth,
                    scaleNumberHeight,
                    juce::Justification::centred);
            }
        }
    }
}
