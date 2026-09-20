#include "core/GameClock.h"

namespace SpaceSim
{
    GameClock::GameClock()
        : m_previous(Clock::now())
    {
    }

    double GameClock::tick()
    {
        const Clock::time_point now = Clock::now();
        const std::chrono::duration<double> elapsed = now - m_previous;
        m_previous = now;
        return elapsed.count();
    }
}
