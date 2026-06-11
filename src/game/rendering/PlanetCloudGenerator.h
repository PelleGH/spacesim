#pragma once

#include "world/StarSystem.h"

#include <raylib.h>

namespace SpaceSim
{
    Model GenerateCloudLayerModel(const GlobalObject& object, int faceResolution);
}