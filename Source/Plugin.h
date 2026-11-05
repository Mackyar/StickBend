#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <atomic>
#include "StickMap.h"

// Pad.cpp. axes = LX, LY, RX, RY (-1..1, Y up = +), triggers = LT, RT (0..1), buttons = XInput bitmask.
bool readPad (int index, float axes[4], float triggers[2], unsigned& buttons);

// XInput button bits (kept here so nothing outside Pad.cpp needs <windows.h>).
namespace Btn { enum : unsigned { Up = 0x1, Down = 0x2, Left = 0x4, Right = 0x8, Start = 0x10, Back = 0x20,
                                  LS = 0x40, RS = 0x80, LB = 0x100, RB = 0x200, A = 0x1000, B = 0x2000, X = 0x4000, Y = 0x8000 }; }

enum Target { Off, Bend, CCBipolar, CCUnipolar };
inline const char* axisNames[4] = { "Left X", "Left Y", "Right X", "Right Y" };
// Display names only; the stored value is the index, so renaming is safe for saved projects.
inline const juce::StringArray targetNames { "Off", "Pitch Bend", "CC centred", "CC from 0" };

inline juce::AudioProcessorValueTreeState::ParameterLayout makeLayout()
{
    const int defaultTarget[4] = { Bend, CCUnipolar, Off, Off };
    const int defaultCC[4]     = { 1, 1, 11, 74 };

    juce::AudioProcessorValueTreeState::ParameterLayout l;
    l.add (std::make_unique<juce::AudioParameterInt> (juce::ParameterID { "pad", 1 }, "Controller", 1, 4, 1));
    l.add (std::make_unique<juce::AudioParameterInt> (juce::ParameterID { "channel", 1 }, "MIDI Channel", 1, 16, 1));
    l.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "deadzone", 1 }, "Deadzone",
                                                        juce::NormalisableRange<float> (0.0f, 0.5f), 0.24f));
    for (int i = 0; i < 4; ++i)
    {
        const juce::String n (axisNames[i]), id (i);
        l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "target" + id, 1 }, n + " sends", targetNames, defaultTarget[i]));
        l.add (std::make_unique<juce::AudioParameterInt> (juce::ParameterID { "cc" + id, 1 }, n + " CC #", 0, 127, defaultCC[i]));
    }
    // Output params: the plugin writes these from the stick. Map them in Reaper (Parameter modulation/link)
    // or record them as automation envelopes.
    for (int i = 0; i < 4; ++i)
        l.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "out" + juce::String (i), 1 },
                                                            juce::String ("Stick ") + axisNames[i], 0.0f, 1.0f, 0.5f));
    return l;
}

class StickBend : public juce::AudioProcessor, private juce::HighResolutionTimer, private juce::Timer
{
public:
    StickBend()
        : AudioProcessor (BusesProperties().withInput ("In", juce::AudioChannelSet::stereo(), true)
                                           .withOutput ("Out", juce::AudioChannelSet::stereo(), true)),
          state (*this, nullptr, "StickBend", makeLayout())
    {
        pad = state.getRawParameterValue ("pad");
        channel = state.getRawParameterValue ("channel");
        deadzone = state.getRawParameterValue ("deadzone");
        for (int i = 0; i < 4; ++i)
        {
            target[i] = state.getRawParameterValue ("target" + juce::String (i));
            cc[i] = state.getRawParameterValue ("cc" + juce::String (i));
            out[i] = state.getParameter ("out" + juce::String (i));
            outRaw[i] = state.getRawParameterValue ("out" + juce::String (i));
        }
        juce::HighResolutionTimer::startTimer (4); // ~250 Hz pad poll -> MIDI, off the audio thread
        juce::Timer::startTimerHz (60);            // message thread -> output params / automation
    }

    ~StickBend() override
    {
        juce::HighResolutionTimer::stopTimer();
        juce::Timer::stopTimer();
    }

    static int restValue (int mode) { return mode == Bend ? 8192 : mode == CCBipolar ? 64 : 0; }

    // What an axis is "doing" right now. A moved stick wins; a resting stick hands over to the
    // Stick output param, so recorded Stick envelopes play back as real bends/CCs.
    // The poll/timer only writes that param when the stick moves, so playback automation isn't overwritten.
    float effectiveAxis (int i) const
    {
        const float phys = axis[i].load();
        if (phys != 0.0f) return phys;
        const float a = outRaw[i]->load(); // 0..1, centre 0.5
        return std::isfinite (a) ? juce::jlimit (-1.0f, 1.0f, a * 2.0f - 1.0f) : 0.0f;
    }

    // float -> int that is never UB: NaN/inf fall back, everything else is clamped to [lo, hi].
    static int toInt (float v, int lo, int hi, int fallback)
    {
        return std::isfinite (v) ? juce::jlimit (lo, hi, (int) juce::jlimit ((float) lo, (float) hi, v)) : fallback;
    }

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer& midi) override
    {
        // Audio and incoming keyboard MIDI pass straight through; we only add events.
        // Clamp everything that becomes a MIDI byte: the params are range-limited already, this is the backstop.
        const int ch = toInt (channel->load(), 1, 16, 1);
        for (int i = 0; i < 4; ++i)
        {
            const int mode = toInt (target[i]->load(), Off, CCUnipolar, Off);
            const int key = toInt (cc[i]->load(), 0, 127, 1);
            if (mode != lastMode[i] || key != lastKey[i]) // settings changed: re-baseline, send nothing
            {
                lastMode[i] = mode; lastKey[i] = key; lastSent[i] = restValue (mode);
            }
            if (mode == Off) continue;

            const float v = effectiveAxis (i);
            const int val = mode == Bend ? toPitchBend (v) : toCC (v, mode == CCUnipolar);
            // Only send on change: a resting stick stays silent, so recorded bends/CCs play back untouched.
            if (val == lastSent[i]) continue;
            lastSent[i] = val;
            midi.addEvent (mode == Bend ? juce::MidiMessage::pitchWheel (ch, val)
                                        : juce::MidiMessage::controllerEvent (ch, key, val), 0);
        }
    }

    using AudioProcessor::processBlock;

    bool isBusesLayoutSupported (const BusesLayout& l) const override
    {
        return l.getMainInputChannelSet() == l.getMainOutputChannelSet() && ! l.getMainOutputChannelSet().isDisabled();
    }

    void getStateInformation (juce::MemoryBlock& dest) override
    {
        if (auto xml = state.copyState().createXml()) copyXmlToBinary (*xml, dest);
    }

    // Project files are untrusted input (shared projects, presets from the internet).
    // getXmlFromBinary bounds-checks the blob; we only accept our own tag, and reset any
    // non-finite value to its default so NaN/inf can never reach the MIDI maths.
    void setStateInformation (const void* data, int size) override
    {
        auto xml = getXmlFromBinary (data, size);
        if (xml == nullptr || ! xml->hasTagName (state.state.getType())) return;
        state.replaceState (juce::ValueTree::fromXml (*xml));
        for (auto* p : getParameters())
            if (! std::isfinite (p->getValue()))
                p->setValueNotifyingHost (p->getDefaultValue());
    }

    juce::AudioProcessorEditor* createEditor() override; // Editor.cpp
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "StickBend"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void prepareToPlay (double, int) override {}
    void releaseResources() override {}

    juce::AudioProcessorValueTreeState state;

    // Live pad state, written by the poll thread, read by the audio thread and the editor.
    std::array<std::atomic<float>, 4> raw {}, axis {}; // raw = before deadzone (for the visualizer)
    std::array<std::atomic<float>, 2> trig {};
    std::atomic<unsigned> buttons { 0 };
    std::atomic<bool> connected { false };

private:
    void hiResTimerCallback() override
    {
        // XInputGetState is slow on an empty slot, so only retry a missing pad ~once a second.
        if (! connected && (retry++ % 250) != 0) return; // post-increment: the very first tick reads straight away
        float a[4] {}, t[2] {};
        unsigned b = 0;
        connected = readPad (toInt (pad->load(), 1, 4, 1) - 1, a, t, b); // disconnected -> all zeros -> centre
        const float dz = std::isfinite (deadzone->load()) ? juce::jlimit (0.0f, 0.5f, deadzone->load()) : 0.24f;
        for (int i = 0; i < 4; ++i)
        {
            raw[i].store (a[i]);
            axis[i].store (applyDeadzone (a[i], dz));
        }
        trig[0].store (t[0]);
        trig[1].store (t[1]);
        buttons.store (b);
    }

    // Must be the message thread: JUCE's VST3 wrapper silently drops gestures from other threads,
    // and without them Reaper never records the param as automation.
    void timerCallback() override
    {
        for (int i = 0; i < 4; ++i)
        {
            const float v = axis[i].load();
            if (std::abs (v - lastOut[i]) < 0.002f && ! (v == 0.0f && lastOut[i] != 0.0f)) continue;
            if (lastOut[i] == 0.0f) out[i]->beginChangeGesture();
            out[i]->setValueNotifyingHost (0.5f + 0.5f * v);
            if (v == 0.0f) out[i]->endChangeGesture();
            lastOut[i] = v;
        }
    }

    std::atomic<float>* pad {};
    std::atomic<float>* channel {};
    std::atomic<float>* deadzone {};
    std::array<std::atomic<float>*, 4> target {}, cc {}, outRaw {};
    std::array<juce::RangedAudioParameter*, 4> out {};

    std::array<float, 4> lastOut {}; // message thread only
    std::array<int, 4> lastMode { -1, -1, -1, -1 }, lastKey { -1, -1, -1, -1 }, lastSent {}; // audio thread only
    unsigned retry = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StickBend)
};
