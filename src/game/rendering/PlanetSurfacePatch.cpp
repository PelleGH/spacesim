#include "rendering/PlanetSurfacePatch.h"

#include "planet/PlanetSurfaceSampler.h"
#include "rendering/DistantBodyRenderer.h"

#include <algorithm>
#include <cmath>
#include <raymath.h>
#include <rlgl.h>

namespace SpaceSim
{
    namespace
    {
        constexpr int CellsPerSide = 64;

    }

    float PlanetSurfacePatch::halfSizeForAltitude(float renderRadius, double worldRadius, double altitude)
    {
        const double angularFootprint = std::clamp(altitude / std::max(worldRadius, 1.0), 0.01, 0.20);
        return renderRadius * static_cast<float>(angularFootprint);
    }

    void PlanetSurfacePatch::update(
        const GameWorld& world,
        const OrbitalPlanetRenderInfo& planetInfo,
        int lightingDebugMode)
    {
        m_visible = false;
        m_surfaceShader = {};


        if (world.planetTransition.mode == PlanetRenderMode::Distant ||
            !planetInfo.valid)
        {
            return;
        }

        const int planetIndex = world.planetTransition.closestPlanetIndex;

        if (planetIndex < 0 || planetIndex >= static_cast<int>(world.starSystem.objects.size()))
        {
            return;
        }

        const GlobalObject& planet = world.starSystem.objects[planetIndex];

        if (world.planetTransition.altitude > planet.visualRadius * 0.20)
            return;

        const float patchHalfSize = halfSizeForAltitude(planetInfo.radius,
            planet.visualRadius, world.planetTransition.altitude);

        if (!planet.hasPlanetData ||
            planet.planetData.planetClass == PlanetClass::GasGiant ||
            planet.planetData.planetClass == PlanetClass::IceGiant)
        {
            return;
        }

        int terrainSeed = static_cast<int>(planet.planetData.seed);
        if (terrainSeed == 0)
        {
            terrainSeed = PlanetSurfaceSampler::seedFromId(planet.id);
        }

        const bool oceanWorld = planet.planetData.planetClass == PlanetClass::OceanWorld;
        if (oceanWorld)
        {
            m_surfaceShader = DistantBodyRenderer::oceanSurfaceShader(
                planet, world.lighting, planetInfo.position, planetInfo.radius,
                world.camera.position, lightingDebugMode);
        }


        const Vector3 surfaceNormal = Vector3Normalize(
            Vector3Subtract(world.camera.position, planetInfo.position));

        m_transform = MatrixMultiply(MatrixScale(planetInfo.radius, planetInfo.radius, planetInfo.radius),
            MatrixTranslate(planetInfo.position.x, planetInfo.position.y, planetInfo.position.z));
        const float footprint = patchHalfSize / planetInfo.radius;
        const float tolerance = footprint / static_cast<float>(CellsPerSide) * 0.5f;
        if (m_cachedPlanet == planet.id && m_cachedSeed == terrainSeed &&
            std::fabs(footprint - m_cachedFootprint) < tolerance &&
            Vector3DistanceSqr(surfaceNormal, m_cachedDirection) < tolerance * tolerance &&
            (oceanWorld || std::fabs(planetInfo.radius - m_cachedRadius) < 0.001f))
        {
            m_visible = true;
            return;
        }

        Vector3 referenceAxis{ 0.0f, 1.0f, 0.0f };
        if (std::fabs(Vector3DotProduct(surfaceNormal, referenceAxis)) > 0.95f)
        {
            referenceAxis = Vector3{ 1.0f, 0.0f, 0.0f };
        }

        const Vector3 tangentRight = Vector3Normalize(
            Vector3CrossProduct(referenceAxis, surfaceNormal));
        const Vector3 tangentForward = Vector3Normalize(
            Vector3CrossProduct(surfaceNormal, tangentRight));

        const auto sampleSurface = [&](float rightOffset, float forwardOffset) -> Vertex
        {
            Vector3 radialVector = Vector3Scale(surfaceNormal, planetInfo.radius);
            radialVector = Vector3Add(radialVector, Vector3Scale(tangentRight, rightOffset));
            radialVector = Vector3Add(radialVector, Vector3Scale(tangentForward, forwardOffset));

            const Vector3 radialDirection = Vector3Normalize(radialVector);

            PlanetSurfaceSample surface = PlanetSurfaceSampler::sample(
                radialDirection,
                terrainSeed,
                planet.planetData.planetClass);
            if (oceanWorld)
            {
                // The globe uses this same normalized displacement. Scaling it
                // here keeps shoreline and relief fixed throughout the descent.
                surface.terrainHeight /= PlanetSurfaceSampler::TerrainUnitsPerRadius;
            }

            if (!oceanWorld) surface.terrainHeight /= planetInfo.radius;

            return Vertex{
                Vector3Scale(radialDirection, 1.0f + surface.terrainHeight),
                surface.terrainHeight,
                surface.color
            };
        };

        const float cellSize = patchHalfSize * 2.0f / static_cast<float>(CellsPerSide);

        // Adjacent cells share corners. Sample each grid vertex only once.
        constexpr int verticesPerSide = CellsPerSide + 1;
        m_vertices.resize(verticesPerSide * verticesPerSide);
        for (int row = 0; row < verticesPerSide; ++row)
        {
            for (int column = 0; column < verticesPerSide; ++column)
            {
                m_vertices[row * verticesPerSide + column] = sampleSurface(
                    -patchHalfSize + static_cast<float>(column) * cellSize,
                    -patchHalfSize + static_cast<float>(row) * cellSize);
            }
        }

        // Reuse neighboring samples to estimate geometric normals without any
        // additional noise evaluations. Vertex colors remain albedo/material data;
        // lighting is evaluated by the draw shader rather than baked into the mesh.
        for (int row = 0; row < verticesPerSide; ++row)
        {
            for (int column = 0; column < verticesPerSide; ++column)
            {
                Vertex& vertex = m_vertices[row * verticesPerSide + column];
                const Vector3 dx = Vector3Subtract(
                    m_vertices[row * verticesPerSide + std::min(column + 1, CellsPerSide)].position,
                    m_vertices[row * verticesPerSide + std::max(column - 1, 0)].position);
                const Vector3 dz = Vector3Subtract(
                    m_vertices[std::min(row + 1, CellsPerSide) * verticesPerSide + column].position,
                    m_vertices[std::max(row - 1, 0) * verticesPerSide + column].position);
                Vector3 normal = Vector3Normalize(Vector3CrossProduct(dx, dz));
                if (Vector3DotProduct(normal, vertex.position) < 0.0f)
                    normal = Vector3Negate(normal);
                vertex.normal = normal;
            }
        }

        // Build a 4x4 set of indexed tiles. GPU allocations are retained between rebuilds.
        constexpr int TileCells = 16;
        constexpr int TileVertices = 17;
        for (int tileRow = 0; tileRow < 4; ++tileRow)
        for (int tileColumn = 0; tileColumn < 4; ++tileColumn)
        {
            Mesh& mesh = m_tiles[tileRow * 4 + tileColumn];
            const bool firstUpload = mesh.vertices == nullptr;
            if (firstUpload)
            {
                mesh.vertexCount = TileVertices * TileVertices;
                mesh.vertices = static_cast<float*>(MemAlloc(mesh.vertexCount * 3 * sizeof(float)));
                mesh.normals = static_cast<float*>(MemAlloc(mesh.vertexCount * 3 * sizeof(float)));
                mesh.colors = static_cast<unsigned char*>(MemAlloc(mesh.vertexCount * 4));
                mesh.indices = static_cast<unsigned short*>(MemAlloc(TileCells * TileCells * 6 * sizeof(unsigned short)));
                std::fill_n(mesh.indices, TileCells * TileCells * 6, 0);
            }
            for (int row = 0; row < TileVertices; ++row)
            for (int column = 0; column < TileVertices; ++column)
            {
                const auto& vertex = m_vertices[(tileRow * TileCells + row) * verticesPerSide + tileColumn * TileCells + column];
                const int v = row * TileVertices + column;
                mesh.vertices[v*3] = vertex.position.x;
                mesh.vertices[v*3+1] = vertex.position.y;
                mesh.vertices[v*3+2] = vertex.position.z;
                mesh.normals[v*3] = vertex.normal.x;
                mesh.normals[v*3+1] = vertex.normal.y;
                mesh.normals[v*3+2] = vertex.normal.z;
                mesh.colors[v*4] = vertex.color.r;
                mesh.colors[v*4+1] = vertex.color.g;
                mesh.colors[v*4+2] = vertex.color.b;
                mesh.colors[v*4+3] = 255;
            }
            int count = 0;
            for (int row = 0; row < TileCells; ++row)
            for (int column = 0; column < TileCells; ++column)
            {
                const int corner = (tileRow * TileCells + row) * verticesPerSide + tileColumn * TileCells + column;
                const int local = row * TileVertices + column;
                const auto add = [&](int a, int b, int c, int ia, int ib, int ic) {
                    if (oceanWorld && m_vertices[a].height <= 0 && m_vertices[b].height <= 0 && m_vertices[c].height <= 0) return;
                    mesh.indices[count++] = static_cast<unsigned short>(ia);
                    mesh.indices[count++] = static_cast<unsigned short>(ib);
                    mesh.indices[count++] = static_cast<unsigned short>(ic);
                };
                add(corner, corner+1, corner+verticesPerSide+1, local, local+1, local+TileVertices+1);
                add(corner, corner+verticesPerSide+1, corner+verticesPerSide, local, local+TileVertices+1, local+TileVertices);
            }
            // Reserve the full index buffer even when this tile currently contains only water.
            if (firstUpload) { mesh.triangleCount = TileCells * TileCells * 2; UploadMesh(&mesh, true); }
            mesh.triangleCount = count / 3;
            UpdateMeshBuffer(mesh, 0, mesh.vertices, mesh.vertexCount * 3 * sizeof(float), 0);
            UpdateMeshBuffer(mesh, 2, mesh.normals, mesh.vertexCount * 3 * sizeof(float), 0);
            UpdateMeshBuffer(mesh, 3, mesh.colors, mesh.vertexCount * 4, 0);
            if (count > 0) UpdateMeshBuffer(mesh, 6, mesh.indices, count * sizeof(unsigned short), 0);
        }
        m_cachedPlanet = planet.id;
        m_cachedSeed = terrainSeed;
        m_cachedDirection = surfaceNormal;
        m_cachedFootprint = patchHalfSize / planetInfo.radius;
        m_cachedRadius = planetInfo.radius;
        ++m_geometryRevision;
        m_visible = true;
    }

    PlanetSurfacePatch::~PlanetSurfacePatch()
    {
        for (Mesh& mesh : m_tiles) if (mesh.vertices != nullptr) UnloadMesh(mesh);
    }

    void PlanetSurfacePatch::draw() const
    {
        if (!m_visible) return;
        MaterialMap maps[12]{};
        maps[MATERIAL_MAP_ALBEDO].texture.id = rlGetTextureIdDefault();
        maps[MATERIAL_MAP_ALBEDO].color = WHITE;
        Material material{};
        material.maps = maps;
        material.shader = m_surfaceShader.id != 0 ? m_surfaceShader :
            Shader{rlGetShaderIdDefault(), rlGetShaderLocsDefault()};
        for (const Mesh& mesh : m_tiles)
            if (mesh.triangleCount > 0) DrawMesh(mesh, material, m_transform);
    }
}
