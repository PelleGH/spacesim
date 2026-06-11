#pragma once

#include "world/StarSystem.h"

#include <raylib.h>

namespace SpaceSim
{
    Model GenerateLowDetailPlanetModel(const GlobalObject& object, int faceResolution);
}