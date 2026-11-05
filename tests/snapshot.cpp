// Renders the plugin editor to a PNG with a fixed demo pose, so the UI can be checked without a DAW.
#include "../Source/Plugin.h"

// Stands in for Pad.cpp: a fixed pose instead of a real controller.
bool readPad (int, float axes[4], float triggers[2], unsigned& buttons)
{
    axes[0] = 0.72f; axes[1] = 0.41f; axes[2] = -0.12f; axes[3] = 0.06f;
    triggers[0] = 0.0f; triggers[1] = 0.65f;
    buttons = Btn::A | Btn::RB | Btn::Right;
    return true;
}

int main (int argc, char** argv)
{
    if (argc < 2) return 2;
    juce::ScopedJuceInitialiser_GUI init;
    StickBend p;
    juce::Thread::sleep (1200); // let the poll thread pick up the fake pose
    std::unique_ptr<juce::AudioProcessorEditor> e (p.createEditor());
    if (argc > 2) // snapshot <out.png> about  -> render with the About panel open
        if (auto* a = e->findChildWithID ("about")) a->setVisible (true);
    const auto img = e->createComponentSnapshot (e->getLocalBounds(), true, 1.5f);
    juce::File f (juce::String::fromUTF8 (argv[1]));
    f.deleteFile();
    juce::FileOutputStream os (f);
    return juce::PNGImageFormat().writeImageToStream (img, os) ? 0 : 1;
}
