// Kept apart from JUCE so <windows.h> never meets JUCE headers.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <Xinput.h>

// ponytail: XInput only (Xbox-style pads). PlayStation/Switch need SDL3 here; swap this file if needed.
// axes = left X, left Y, right X, right Y in -1..1 (Y up = positive). Returns false if no pad in that slot.
bool readPad (int index, float axes[4], float triggers[2], unsigned& buttons)
{
    XINPUT_STATE s {};
    if (XInputGetState ((DWORD) index, &s) != ERROR_SUCCESS) return false;
    auto n = [] (SHORT v) { return v < 0 ? v / 32768.0f : v / 32767.0f; };
    axes[0] = n (s.Gamepad.sThumbLX);
    axes[1] = n (s.Gamepad.sThumbLY);
    axes[2] = n (s.Gamepad.sThumbRX);
    axes[3] = n (s.Gamepad.sThumbRY);
    triggers[0] = s.Gamepad.bLeftTrigger / 255.0f;
    triggers[1] = s.Gamepad.bRightTrigger / 255.0f;
    buttons = s.Gamepad.wButtons;
    return true;
}
