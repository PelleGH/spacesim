#pragma once

#include "world/GameWorld.h"

#include <raylib.h>
#include <vector>
#include <array>

namespace SpaceSim
{
    struct OrbitalPlanetRenderInfo
    {
        bool valid = false;
        Vector3 position{};
        float radius = 1.0f;
    };

    class PlanetSurfacePatch
    {
    public:
        PlanetSurfacePatch() = default;
        ~PlanetSurfacePatch();
        PlanetSurfacePatch(const PlanetSurfacePatch&) = delete;
        PlanetSurfacePatch& operator=(const PlanetSurfacePatch&) = delete;
        void update(
            const GameWorld& world,
            const OrbitalPlanetRenderInfo& planetInfo,
            int lightingDebugMode = 2);

        void draw() const;
        // Diagnostic counter for detecting unnecessary terrain rebuilds.
        unsigned int geometryRevision() const { return m_geometryRevision; }

        // Fixed vertex budget; progressively smaller footprint increases local
        // sampling density continuously, without discrete resolution switches.
        static float halfSizeForAltitude(float renderRadius, double worldRadius, double altitude);

    private:
        struct Vertex
        {
            Vector3 position{};
            float height = 0.0f;
            Color color = WHITE;
            Vector3 normal{};
        };

        // Sixteen independently drawable GPU tiles share a continuous sample grid.
        std::array<Mesh, 16> m_tiles{};
        std::vector<Vertex> m_vertices;
        Shader m_surfaceShader{};
        Matrix m_transform{};
        Vector3 m_cachedDirection{};
        float m_cachedFootprint = -1.0f;
        float m_cachedRadius = -1.0f;
        std::string m_cachedPlanet;
        int m_cachedSeed = 0;
        bool m_visible = false;
        unsigned int m_geometryRevision = 0;
    };
}
