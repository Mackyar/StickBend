#include "../Source/StickMap.h"
#include <cassert>
#include <cstdio>

int main()
{
    assert (applyDeadzone (0.1f, 0.24f) == 0.0f);
    assert (applyDeadzone (1.0f, 0.24f) == 1.0f);
    assert (applyDeadzone (-1.0f, 0.24f) == -1.0f);
    assert (applyDeadzone (0.62f, 0.24f) > 0.49f && applyDeadzone (0.62f, 0.24f) < 0.51f);

    assert (toPitchBend (0.0f) == 8192);
    assert (toPitchBend (1.0f) == 16383);
    assert (toPitchBend (-1.0f) == 1);
    assert (toPitchBend (-2.0f) == 0);

    assert (toCC (0.0f, false) == 64 && toCC (-1.0f, false) == 0 && toCC (1.0f, false) == 127);
    assert (toCC (0.0f, true) == 0 && toCC (-1.0f, true) == 127 && toCC (1.0f, true) == 127);

    std::puts ("stickmap ok");
}
