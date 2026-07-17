/*
  Wrapper for VST3/AU export
*/

#include <JuceHeader.h>
#include "QGWavetableProcessor.h"

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new QGWavetableAudioProcessor();
}
