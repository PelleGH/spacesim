#pragma once

#include "components/RenderableComponent.h"
#include "components/TransformComponent.h"

namespace SpaceSim
{
    class PrototypeMeshRenderer
    {
    public:
        void render(
            const TransformComponent& transform,
            const RenderableComponent& renderable
        ) const;

    private:
        void drawShip(Color color) const;
        void drawSatellite(Color panelColor) const;
    };
}