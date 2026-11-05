// Hostile-input test for project state loading. Exit code 0 = pass.
// Covers: random bytes, truncated/oversized length headers, wrong root tag, NaN/inf/huge values,
// then runs processBlock and checks every emitted MIDI message is well-formed.
#include "../Source/Plugin.h"
#include <cstdio>
#include <random>

static std::atomic<bool> fakeTilt { true };

bool readPad (int index, float axes[4], float t[2], unsigned& b) // fake pad: full tilt so MIDI is emitted, or at rest
{
    if (index < 0 || index > 3) { std::puts ("FAIL: pad index out of range"); std::exit (1); }
    const float k = fakeTilt ? 1.0f : 0.0f;
    axes[0] = k; axes[1] = -k; axes[2] = 0.7f * k; axes[3] = -0.7f * k;
    t[0] = t[1] = 0.0f; b = 0;
    return true;
}

static int failures = 0;
static void check (bool ok, const char* what) { if (! ok) { std::printf ("FAIL: %s\n", what); ++failures; } }

static void loadXml (StickBend& p, const juce::String& xml)
{
    juce::MemoryBlock mb;
    auto parsed = juce::parseXML (xml);
    if (parsed == nullptr) { std::puts ("note: test XML did not parse, skipped"); return; }
    juce::AudioProcessor::copyXmlToBinary (*parsed, mb);
    p.setStateInformation (mb.getData(), (int) mb.getSize());
}

static void checkSane (StickBend& p, const char* when)
{
    for (auto* prm : p.getParameters())
        check (std::isfinite (prm->getValue()) && prm->getValue() >= 0.0f && prm->getValue() <= 1.0f, when);

    juce::AudioBuffer<float> buf (2, 256);
    for (int n = 0; n < 5; ++n)
    {
        juce::MidiBuffer midi;
        buf.clear();
        p.processBlock (buf, midi);
        juce::Thread::sleep (5);
        for (const auto m : midi)
        {
            const auto msg = m.getMessage();
            check (msg.getChannel() >= 1 && msg.getChannel() <= 16, "MIDI channel out of range");
            check (msg.isPitchWheel() || msg.isController(), "unexpected MIDI message type");
            for (int i = 1; i < msg.getRawDataSize(); ++i)
                check (msg.getRawData()[i] < 0x80, "MIDI data byte >= 0x80");
        }
    }
}

int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    std::mt19937 rng (1234);

    auto pollAlive = [] (const char* when)
    {
        StickBend t;
        juce::Thread::sleep (80);
        if (t.axis[0].load() != 1.0f) { std::printf ("FAIL: pad not read within 80 ms (%s)\n", when); ++failures; }
    };
    pollAlive ("at start");
    { // Many create/destroy cycles: the poll thread must stop cleanly every time.
        for (int i = 0; i < 50; ++i) { StickBend p; juce::Thread::sleep (1); }
    }
    pollAlive ("after 50 cycles");

    StickBend p;
    p.prepareToPlay (48000.0, 256);
    juce::Thread::sleep (50);

    // 1. Random garbage of many sizes, with and without the JUCE XML magic header.
    for (int i = 0; i < 3000; ++i)
    {
        std::vector<unsigned char> blob ((size_t) (rng() % 600));
        for (auto& c : blob) c = (unsigned char) rng();
        if (blob.size() >= 8 && (i & 1))
        {
            const juce::uint32 magic = 0x21324356, len = rng();
            std::memcpy (blob.data(), &magic, 4);
            std::memcpy (blob.data() + 4, &len, 4); // lies about its length
        }
        p.setStateInformation (blob.data(), (int) blob.size());
    }
    p.setStateInformation (nullptr, 0);
    checkSane (p, "after random garbage");

    // 2. Valid XML, wrong root tag: must be ignored entirely.
    loadXml (p, "<Evil><PARAM id=\"channel\" value=\"16\"/></Evil>");
    check (p.state.getRawParameterValue ("channel")->load() == 1.0f, "wrong-tag state was applied");

    // 3. Right tag, hostile values.
    loadXml (p, "<StickBend>"
                "<PARAM id=\"pad\" value=\"nan\"/><PARAM id=\"channel\" value=\"-99999\"/>"
                "<PARAM id=\"deadzone\" value=\"inf\"/><PARAM id=\"target0\" value=\"1e308\"/>"
                "<PARAM id=\"cc0\" value=\"-inf\"/><PARAM id=\"target1\" value=\"3\"/><PARAM id=\"cc1\" value=\"99999\"/>"
                "<PARAM id=\"target2\" value=\"abc\"/><PARAM id=\"out0\" value=\"nan\"/>"
                "<PARAM id=\"doesNotExist\" value=\"1\"/></StickBend>");
    checkSane (p, "after hostile values");

    // 4. Deep nesting and a huge attribute.
    juce::String deep = "<StickBend>";
    for (int i = 0; i < 2000; ++i) deep << "<a>";
    for (int i = 0; i < 2000; ++i) deep << "</a>";
    deep << "<PARAM id=\"cc2\" value=\"" << juce::String::repeatedString ("9", 100000) << "\"/></StickBend>";
    loadXml (p, deep);
    checkSane (p, "after deep/huge XML");

    // 5. Round trip still works.
    juce::MemoryBlock saved;
    p.getStateInformation (saved);
    p.setStateInformation (saved.getData(), (int) saved.getSize());
    checkSane (p, "after round trip");

    // 6. Automation playback: stick at rest, a Stick envelope moves the out param -> real pitch bend.
    {
        StickBend q; // defaults: Left X = Pitch Bend
        q.prepareToPlay (48000.0, 256);
        auto lastBend = [&q]
        {
            juce::AudioBuffer<float> b (2, 256);
            juce::MidiBuffer m;
            q.processBlock (b, m);
            int v = -1;
            for (const auto e : m) if (e.getMessage().isPitchWheel()) v = e.getMessage().getPitchWheelValue();
            return v;
        };
        fakeTilt = false;
        juce::Thread::sleep (60);
        lastBend(); // baseline
        auto* outLX = q.state.getParameter ("out0");
        outLX->setValueNotifyingHost (1.0f);
        check (lastBend() == 16383, "envelope at max did not bend up");
        outLX->setValueNotifyingHost (0.5f);
        check (lastBend() == 8192, "envelope back at centre did not return bend to centre");
        outLX->setValueNotifyingHost (0.0f); // stick moved: physical stick must win over the envelope
        fakeTilt = true;
        juce::Thread::sleep (60);
        const int got = lastBend();
        if (got != 16383) std::printf ("  got %d, axis %.3f, effective %.3f\n", got, q.axis[0].load(), q.effectiveAxis (0));
        check (got == 16383, "moving the stick did not override the envelope");
    }

    std::printf (failures == 0 ? "statetest ok\n" : "statetest: %d failure(s)\n", failures);
    return failures == 0 ? 0 : 1;
}
