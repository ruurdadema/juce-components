#pragma once

#include "LevelMeterConstants.h"

#include <cstdint>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

/**
 * Little class which keeps track of a level over time, and makes sure this value never decreases more that a certain
 * amount of decibels per second. Ideal to ease level meters.
 * @tparam SampleType The type of the sample to use. Probably float or double.
 */
template <class SampleType>
class LevelPeakValue
{
public:
    explicit LevelPeakValue (double const minusInfinityDb = LevelMeterConstants::kDefaultMinusInfinityDb) :
        mMinusInfinityDb (minusInfinityDb)
    {
    }

    /**
     * Sets the return rate.
     * @param returnRateDbPerSecond The return rate in decibels per second.
     */
    void setReturnRate (SampleType returnRateDbPerSecond)
    {
        mReturnRateDbPerSecond = returnRateDbPerSecond;
    }

    /**
     * Updates the current level, taking into account the return rate, which means only a higher level will actually
     * change anything.
     * @param level The new level.
     */
    void updateLevel (SampleType level)
    {
        if (level > mHighestLevel)
        {
            mHighestLevel = level;

            if (mHighestLevel > mReturningLevel)
                mPeakHoldTimeLeft = mPeakHoldTime;
        }
    }

    /**
     * Gets the next level to show on a meter, taking into account the return rate. The level will be calculated for
     * this point in time using a monotonic system clock.
     * @return The level for this point in time.
     */
    SampleType getNextLevel()
    {
        auto deltaTime = getDeltaTime();
        SampleType declineDb = deltaTime / 1000.0 * mReturnRateDbPerSecond;

        auto declineGain = juce::Decibels::decibelsToGain (-declineDb, mMinusInfinityDb);

        mPeakHoldTimeLeft = mPeakHoldTimeLeft > deltaTime ? mPeakHoldTimeLeft - deltaTime : 0;

        if (mPeakHoldTimeLeft == 0)
            mReturningLevel *= declineGain;

        if (mHighestLevel > mReturningLevel)
        {
            mReturningLevel = mHighestLevel;
            mHighestLevel = {};
        }

        return mReturningLevel;
    }

    /**
     * Sets minus infinity.
     * @param minusInfinityDb The level in decibels which equals zero gain.
     */
    void setMinusInfinityDb (double minusInfinityDb)
    {
        mMinusInfinityDb = minusInfinityDb;
    }

    /**
     * Sets peak hold time.
     * @param peakHoldTime The hold time in milliseconds.
     */
    void setPeakHoldTime (uint32_t const peakHoldTime)
    {
        mPeakHoldTime = peakHoldTime;
    }

    /**
     * Resets this value to zero.
     */
    void reset()
    {
        mHighestLevel = {};
        mReturningLevel = {};
        mPreviousTime = {};
        mPeakHoldTime = {};
        mPeakHoldTimeLeft = {};
    }

private:
    /// Return rate in dB per second.
    SampleType mReturnRateDbPerSecond { LevelMeterConstants::kDefaultReturnRate };

    /// The current level which should be presented as level meter value.
    SampleType mHighestLevel { 0.0 };

    /// Used for storing the highest level passed to UpdateLevel().
    SampleType mReturningLevel { 0.0 };

    /// Used for finding the time since the previous call to DeltaTime().
    uint32_t mPreviousTime { 0 };

    /// Specifies the lowest level of audio which equals to zero gain.
    double mMinusInfinityDb = { LevelMeterConstants::kDefaultMinusInfinityDb };

    /// Runtime setting for the amount of time the value needs to be held at the highest value.
    uint32_t mPeakHoldTime { 0 };

    /// Keeps track of the time the value still needs to hold.
    uint32_t mPeakHoldTimeLeft { 0 };

    /**
     * @return The amount of time (in milliseconds) since the previous call to this method.
     */
    uint32_t getDeltaTime()
    {
        auto currentTime = juce::Time::getMillisecondCounter();
        auto deltaTime = currentTime - mPreviousTime;
        mPreviousTime = currentTime;
        return deltaTime;
    }
};
