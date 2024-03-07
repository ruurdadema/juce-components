#include "LevelMeter.h"

LevelMeter::LevelMeter()
{
    mSharedTimer->subscribe (*this);
}

LevelMeter::~LevelMeter()
{
    mSubscribers.call ([] (Subscriber& s) {
        s.reset();
    });
}

void LevelMeter::prepareToPlay (int numChannels)
{
    mPreparedToPlayInfo.numChannels = numChannels;

    mSubscribers.call ([numChannels] (Subscriber& s) {
        s.prepareToPlay (numChannels);
    });
}

template <typename SampleType>
void LevelMeter::measureBlock (const juce::AudioBuffer<SampleType>& audioBuffer)
{
    measureBlock (audioBuffer.getArrayOfReadPointers(), audioBuffer.getNumChannels(), audioBuffer.getNumSamples());
}

// Trigger symbol generation.
template void LevelMeter::measureBlock (const juce::AudioBuffer<float>& audioBuffer);
template void LevelMeter::measureBlock (const juce::AudioBuffer<double>& audioBuffer);

template <typename SampleType>
void LevelMeter::measureBlock (const SampleType* const* inputChannelData, int numChannels, int numSamples)
{
    jassert (numChannels >= 0);
    jassert (numSamples >= 0);

    // Measure levels
    for (int ch = 0; ch < numChannels; ch++)
    {
        // Peak level
        auto range = juce::FloatVectorOperations::findMinAndMax (inputChannelData[ch], numSamples);
        auto peak = juce::jmax (range.getStart(), -range.getStart(), range.getEnd(), -range.getEnd());

        pushMeasurement ({ ch, peak });
    }
}

// Trigger symbol generation.
template void LevelMeter::measureBlock (const float* const* inputChannelData, int numChannels, int numSamples);
template void LevelMeter::measureBlock (const double* const* inputChannelData, int numChannels, int numSamples);

void LevelMeter::pushMeasurement (Measurement&& measurement)
{
    mMeasurements.enqueue (measurement);
}

void LevelMeter::timerCallback()
{
    Measurement measurement;
    while (mMeasurements.try_dequeue (measurement))
    {
        mSubscribers.call ([&measurement] (Subscriber& s) {
            s.updateWithMeasurement (measurement);
        });
    }

    mSubscribers.call ([] (Subscriber& s) {
        s.measurementUpdatesFinished();
    });
}

void LevelMeter::Subscriber::prepareToPlay (int numChannels)
{
    mChannelData.resize (numChannels);

    for (auto& ch : mChannelData)
    {
        ch.peakLevel.setMinusInfinityDb (mScale.getMinusInfinityDb());
        ch.peakLevel.setPeakHoldTime (1000 / kRefreshRateHz);
        ch.peakHoldLevel.setMinusInfinityDb (mScale.getMinusInfinityDb());
        ch.peakHoldLevel.setPeakHoldTime (kPeakHoldValueTimeMs);
    }

    levelMeterPrepared (numChannels);
}

void LevelMeter::Subscriber::updateWithMeasurement (const LevelMeter::Measurement& measurement)
{
    auto chIndex = measurement.channelIndex;
    if (juce::isPositiveAndBelow (chIndex, mChannelData.size()))
    {
        auto& channelData = mChannelData.getReference (chIndex);
        channelData.peakLevel.updateLevel (measurement.peakLevel);
        channelData.peakHoldLevel.updateLevel (measurement.peakLevel);
        if (measurement.peakLevel >= kOverloadTriggerLevel)
            channelData.overloaded = true;
    }
}

void LevelMeter::Subscriber::subscribeToLevelMeter (LevelMeter& levelMeter)
{
    mSubscription.reset();
    mSubscription = levelMeter.mSubscribers.subscribe (this);
    prepareToPlay (levelMeter.mPreparedToPlayInfo.numChannels);
}

double LevelMeter::Subscriber::getPeakValue (int channelIndex)
{
    if (juce::isPositiveAndBelow (channelIndex, mChannelData.size()))
        return mChannelData.getReference (channelIndex).peakLevel.getNextLevel();
    return 0.0;
}

double LevelMeter::Subscriber::getPeakHoldValue (int channelIndex)
{
    if (juce::isPositiveAndBelow (channelIndex, mChannelData.size()))
        return mChannelData.getReference (channelIndex).peakHoldLevel.getNextLevel();
    return 0.0;
}

bool LevelMeter::Subscriber::getOverloaded (int channelIndex) const
{
    if (juce::isPositiveAndBelow (channelIndex, mChannelData.size()))
        return mChannelData.getReference (channelIndex).overloaded;
    return false;
}

void LevelMeter::Subscriber::resetOverloaded()
{
    for (auto& ch : mChannelData)
        ch.overloaded = false;
}

const LevelMeter::Scale& LevelMeter::Subscriber::getScale()
{
    return mScale;
}

LevelMeter::Subscriber::Subscriber (const Scale& scale) : mScale (scale) {}

int LevelMeter::Subscriber::getNumChannels() const
{
    return mChannelData.size();
}

void LevelMeter::Subscriber::unsubscribeFromLevelMeter()
{
    mSubscription.reset();
    reset();
}

void LevelMeter::Subscriber::reset()
{
    for (auto& ch : mChannelData)
    {
        ch.peakLevel.reset();
        ch.peakHoldLevel.reset();
        ch.overloaded = false;
    }

    measurementUpdatesFinished();
}

LevelMeter::Scale::Scale (double minusInfinityDb, std::initializer_list<double> divisions) :
    mMinusInfinityDb (minusInfinityDb),
    mDivisions (divisions)
{
}

const std::vector<double>& LevelMeter::Scale::getDivisions() const
{
    return mDivisions;
}

double LevelMeter::Scale::calculateProportionForLevel (double level) const
{
    return calculateProportionForLevelDb (juce::Decibels::gainToDecibels (level, mMinusInfinityDb));
}

double LevelMeter::Scale::calculateProportionForLevelDb (double levelDb) const
{
    if (mDivisions.empty())
        return 0.0;

    if (levelDb <= mDivisions.front())
        return 0.0;

    if (levelDb >= mDivisions.back())
        return 1.0;

    auto amountOfDivisions = static_cast<double> (mDivisions.size() - 1);
    auto proportionPerDivision = 1.0 / amountOfDivisions;

    for (auto div = mDivisions.begin(); div != mDivisions.end(); ++div)
    {
        if (levelDb <= *div)
        {
            auto i = std::distance (mDivisions.begin(), div);
            double positionForStartOfCurrentDivision = static_cast<double> (i - 1) / amountOfDivisions;
            auto decibelsForCurrentDivision = div == mDivisions.begin() ? 0.0 : *(div--) - *div;
            auto decibelsIntoDivision = std::abs (levelDb - *div);
            auto divisionProportion = decibelsIntoDivision / decibelsForCurrentDivision;
            auto proportion = positionForStartOfCurrentDivision + proportionPerDivision * divisionProportion;
            return proportion;
        }
    }

    return 1.0; // Fallback
}

double LevelMeter::Scale::calculateLevelDbForProportion (double proportion) const
{
    if (mDivisions.empty())
        return mMinusInfinityDb;

    if (proportion <= 0)
        return mDivisions.front();

    if (proportion >= 1.0)
        return mDivisions.back();

    auto amountOfDivisions = static_cast<double> (mDivisions.size() - 1);
    auto proportionPerDivision = 1.0 / amountOfDivisions;

    auto currenDivisionIndex = static_cast<size_t> (proportion / proportionPerDivision);
    jassert (currenDivisionIndex < mDivisions.size()); // If hit, then the index is out of bounds.

    auto proportionRemainder = proportion - (static_cast<double> (currenDivisionIndex) * proportionPerDivision);

    auto divisionLow = mDivisions[currenDivisionIndex];
    auto decibelsForCurrentDivision = (divisionLow - mDivisions[currenDivisionIndex + 1]) * -1;
    auto decibelsIntoCurrentDivision = (proportionRemainder / proportionPerDivision) * decibelsForCurrentDivision;

    return divisionLow + decibelsIntoCurrentDivision;
}

double LevelMeter::Scale::getMinusInfinityDb() const
{
    return mMinusInfinityDb;
}

const LevelMeter::Scale& LevelMeter::Scale::getDefaultScale()
{
    static const Scale scale {
        kDefaultMinusInfinityDb,
        { kDefaultMinusInfinityDb, -80.0, -60.0, -40.0, -30.0, -24.0, -20.0, -16.0, -12.0, -9.0, -6.0, -3.0, 0.0 }
    };
    return scale;
}
