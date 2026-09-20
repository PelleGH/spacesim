#pragma once

#include <chrono>

namespace SpaceSim
{
    class GameClock
    {
    public:
        GameClock();

        // Returns wall-clock seconds elapsed since the previous call.
        double tick();

    private:
        using Clock = std::chrono::steady_clock;
        Clock::time_point m_previous;
    };
}
