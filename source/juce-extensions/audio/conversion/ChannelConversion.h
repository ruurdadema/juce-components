#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

/**
 * Converts and adds channels from src to dst.
 * @tparam SampleType The type of the samples (float or double)
 * @param src The source channels.
 * @param dst The destination channels.
 * @return True if all input channels were converted to one or more output channels, or false if at least one input
 * channel got lost.
 */
template <class SampleType>
bool addConvertChannels (const juce::AudioBuffer<SampleType>& src, juce::AudioBuffer<SampleType>& dst)
{
    const float minus_3db = 1.0f / sqrtf (2.0f);

    auto numInputChannels = src.getNumChannels();
    auto numOutputChannels = dst.getNumChannels();

    if (numOutputChannels <= 0)
        return false; // With no output channels there is nothing we can do here.

    // Mono source
    if (numInputChannels == 1)
    {
        // Stereo destination
        if (numOutputChannels == 2)
        {
            // Route the mono input channel to both left and right.
            dst.addFrom (0, 0, src, 0, 0, src.getNumSamples());
            dst.addFrom (1, 0, src, 0, 0, src.getNumSamples());
            return true;
        }
    }
    // Stereo source
    else if (numInputChannels == 2)
    {
        // Mono destination
        if (numOutputChannels == 1)
        {
            // Sum left and right and reduce gain by -3.01dB per channel.
            dst.addFrom (0, 0, src, 0, 0, src.getNumSamples(), minus_3db);
            dst.addFrom (0, 0, src, 1, 0, src.getNumSamples(), minus_3db);
            return true;
        }
    }

    // At this point we just pass the channels 1:1 for at most std::min(src, dst) channels.
    for (int ch = 0; ch < std::min (numInputChannels, numOutputChannels); ch++)
        dst.addFrom (ch, 0, src, ch, 0, std::min (src.getNumSamples(), dst.getNumSamples()));

    // Return true if all input channels were added to the output buffer, otherwise return false.
    return numInputChannels <= numOutputChannels;
}
