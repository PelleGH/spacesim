#pragma once

namespace SpaceSim
{
    struct CameraComponent
    {
        float verticalFovDegrees = 62.0f;

        // Physical local-gameplay clipping distances. RenderSystem converts
        // them to renderer units and extends the far plane for fake distant
        // bodies when necessary.
        float nearClipMeters = 0.10f;
        float farClipMeters = 100000.0f;
    };
}
