// Loads the built plugin like a DAW would, runs ~10 s, counts the MIDI it emits and
// how far the "Stick" output params moved. Move the left stick while it runs.
// Usage: hosttest <path to StickBend.vst3>
#include <juce_audio_processors/juce_audio_processors.h>
#include <cstdio>

int main (int argc, char** argv)
{
    if (argc < 2) { std::puts ("usage: hosttest <plugin.vst3>"); return 2; }
    juce::ScopedJuceInitialiser_GUI init;

    juce::AudioPluginFormatManager fm;
    juce::addDefaultFormatsToManager (fm);
    juce::OwnedArray<juce::PluginDescription> descs;
    juce::VST3PluginFormat vst3;
    vst3.findAllTypesForFile (descs, juce::String::fromUTF8 (argv[1]));
    if (descs.isEmpty()) { std::puts ("plugin not found"); return 1; }

    juce::String err;
    auto p = fm.createPluginInstance (*descs[0], 48000.0, 480, err);
    if (p == nullptr) { std::printf ("load failed: %s\n", err.toRawUTF8()); return 1; }
    p->enableAllBuses();
    p->prepareToPlay (48000.0, 480);

    juce::AudioBuffer<float> buf (2, 480);
    int bends = 0, ccs = 0;
    float lo = 1.0f, hi = 0.0f;
    auto* stickLX = p->getParameters().getLast(); // last 4 params are the Stick outputs; take one of them
    for (auto* prm : p->getParameters())
        if (prm->getName (64) == "Stick Left X") stickLX = prm;

    for (int n = 0; n < 1000; ++n) // 10 ms blocks ~ 10 s
    {
        juce::MidiBuffer midi;
        buf.clear();
        p->processBlock (buf, midi);
        for (const auto meta : midi)
        {
            const auto m = meta.getMessage();
            if (m.isPitchWheel()) ++bends;
            else if (m.isController()) ++ccs;
        }
        juce::MessageManager::getInstance()->runDispatchLoopUntil (10);
        lo = std::min (lo, stickLX->getValue());
        hi = std::max (hi, stickLX->getValue());
    }
    std::printf ("pitch bends: %d  CCs: %d  'Stick Left X' param range: %.2f..%.2f\n", bends, ccs, lo, hi);
    p->releaseResources();
    return 0;
}
