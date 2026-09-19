#pragma once

#include "assets/ModelData.h"

#include <filesystem>

namespace SpaceSim
{
    class ModelLoader
    {
    public:
        static ModelData load(
            const std::filesystem::path& path);
    };
}