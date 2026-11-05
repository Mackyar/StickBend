#pragma once
#include <algorithm>
#include <cmath>

// Pure stick -> MIDI maths, kept JUCE-free so tests/stickmap_test.cpp can check it.

// v in -1..1, dz in 0..<1. Inside the deadzone -> 0, outside rescaled so full tilt is still 1.
inline float applyDeadzone (float v, float dz)
{
    const float a = std::abs (v);
    if (a <= dz) return 0.0f;
    return std::copysign (std::min (1.0f, (a - dz) / (1.0f - dz)), v);
}

// 14-bit pitch bend, 8192 = centre.
inline int toPitchBend (float v) { return std::clamp ((int) std::lround (8192.0f + v * 8191.0f), 0, 16383); }

// unipolar: rest = 0, either direction raises it (mod wheel style). bipolar: rest = 64.
inline int toCC (float v, bool unipolar)
{
    if (unipolar) return std::clamp ((int) std::lround (std::abs (v) * 127.0f), 0, 127);
    return std::clamp ((int) std::lround (63.5f + v * 63.5f), 0, 127);
}
