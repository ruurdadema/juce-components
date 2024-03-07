add_library(juce-extensions INTERFACE)

target_sources(juce-extensions INTERFACE
        source/juce-extensions/audio/conversion/ChannelConversion.h

        source/juce-extensions/audio/metering/LevelMeter.h
        source/juce-extensions/audio/metering/LevelMeter.cpp
        source/juce-extensions/audio/metering/LevelPeakValue.h

        source/juce-extensions/components/metering/LevelMeterComponent.h
        source/juce-extensions/components/metering/LevelMeterComponent.cpp
        source/juce-extensions/components/metering/ScaleComponent.h
        source/juce-extensions/components/metering/ScaleComponent.cpp
        source/juce-extensions/components/metering/ScaledSlider.h
)

target_include_directories(juce-extensions INTERFACE
        source
)
