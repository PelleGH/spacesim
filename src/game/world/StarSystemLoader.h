#pragma once

#include "StarSystem.h"

#include <string>

namespace SpaceSim
{
    StarSystem LoadStarSystemFromJson(const std::string& path);
}