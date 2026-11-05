#include "Plugin.h"

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new StickBend(); }
