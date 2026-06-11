#include "systems/RenderSystem.h"

#include "components/RenderableComponent.h"
#include "components/TransformComponent.h"

#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
namespace SpaceSim
{

    void RenderSystem::renderSky(GameWorld& world, Renderer& renderer)
    {
        (void)renderer;

        // Camera rotation only. Position is locked to origin so the sky does not
        // slide around when the player moves locally.
        Vector3 cameraForward = Vector3Normalize(Vector3Subtract(
            world.camera.target,
            world.camera.position
        ));

        Camera3D skyCamera{};
        skyCamera.position = Vector3{ 0.0f, 0.0f, 0.0f };
        skyCamera.target = cameraForward;
        skyCamera.up = world.camera.up;
        skyCamera.fovy = world.camera.fovy;
        skyCamera.projection = world.camera.projection;

        BeginMode3D(skyCamera);

        // disable depth test and depth write so the sky always renders behind everything else
        
        rlDisableDepthTest();
        rlDisableDepthMask();

        m_spaceBackgroundRenderer.render(skyCamera);
        rlDrawRenderBatchActive();
        rlSetTexture(0);

        rlEnableDepthTest();
        rlEnableDepthMask();

        m_distantBodyRenderer.render(world);
        rlDrawRenderBatchActive();


        rlEnableDepthMask();
        rlEnableDepthTest();

        EndMode3D();
    }


    void RenderSystem::renderWorld(GameWorld& world, Renderer& renderer)
    {
        (void)renderer;

        if (world.travelMode == TravelMode::FTLTravel)
        {
            return;
        }

        auto view = world.registry.view<TransformComponent, RenderableComponent>();

        for (auto entity : view)
        {
            const auto& transform = view.get<TransformComponent>(entity);
            const auto& renderable = view.get<RenderableComponent>(entity);

            m_prototypeMeshRenderer.render(transform, renderable);
        }
    }
}