#include "renderer/planet/PlanetPass.h"

#include "renderer/MeshVertex.h"

#include "renderer/atmosphere/AtmosphereParameters.h"

#include "renderer/opengl/GpuMesh.h"

#include <glad/gl.h>

#include <glm/common.hpp>
#include <glm/geometric.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <memory>
#include <vector>
#include <map>
#include <cstdlib>
#include <stdexcept>
#include <iostream>


namespace
{
    float rendererTimeSeconds()
    {
        using Clock =
            std::chrono::steady_clock;


        static const Clock::time_point startTime =
            Clock::now();


        const Clock::time_point now =
            Clock::now();


        return
            std::chrono::duration<float>(
                now -
                startTime)
                .count();
    }


    // =============================================================
    // LOCAL HIGH-RESOLUTION OCEAN GRID
    // =============================================================
    //
    // Dense centre plus eleven power-of-two annuli.
    //
    // The extra outer level gives us a 524.288 km half-width,
    // meaning the complete local ocean patch is a little over
    // 1,048 km across.
    //
    // Inner holes avoid overdraw. Fine outer edges collapse
    // alternate samples onto the coarse ring's lattice.
    //

    std::unique_ptr<SpaceSim::GpuMesh> createOceanPatchMesh()
    {
        constexpr int cells =
            128;


        constexpr int side =
            cells
            +
            1;


        constexpr int levels =
            12;


        std::vector<SpaceSim::MeshVertex> vertices;

        std::vector<std::uint32_t> indices;


        for (
            int level = 0;
            level < levels;
            ++level)
        {
            const auto offset =
                static_cast<std::uint32_t>(
                    vertices.size());


            const float halfSize =
                0.256f
                *
                static_cast<float>(
                    1u
                    <<
                    level);


            for (
                int y = 0;
                y <= cells;
                ++y)
            {
                for (
                    int x = 0;
                    x <= cells;
                    ++x)
                {
                    int sx =
                        x;


                    int sy =
                        y;


                    if (
                        level
                            <
                            levels - 1)
                    {
                        if (
                            (
                                y == 0
                                ||
                                y == cells
                            )
                            &&
                            (
                                x & 1
                            ))
                        {
                            --sx;
                        }


                        if (
                            (
                                x == 0
                                ||
                                x == cells
                            )
                            &&
                            (
                                y & 1
                            ))
                        {
                            --sy;
                        }
                    }


                    SpaceSim::MeshVertex vertex{};


                    vertex.position =
                    {
                        halfSize
                            *
                            (
                                2.0f
                                *
                                sx
                                /
                                cells
                                -
                                1.0f
                            ),

                        0.0f,

                        halfSize
                            *
                            (
                                2.0f
                                *
                                sy
                                /
                                cells
                                -
                                1.0f
                            )
                    };


                    vertex.normal =
                    {
                        0.0f,
                        1.0f,
                        0.0f
                    };


                    vertex.texCoord =
                    {
                        0.0f,
                        0.0f
                    };


                    vertices.push_back(
                        vertex);
                }
            }


            for (
                int y = 0;
                y < cells;
                ++y)
            {
                for (
                    int x = 0;
                    x < cells;
                    ++x)
                {
                    if (
                        level > 0
                        &&
                        x >= cells / 4
                        &&
                        x < 3 * cells / 4
                        &&
                        y >= cells / 4
                        &&
                        y < 3 * cells / 4)
                    {
                        continue;
                    }


                    const auto a =
                        offset
                        +
                        y * side
                        +
                        x;


                    const auto b =
                        a
                        +
                        1;


                    const auto c =
                        a
                        +
                        side;


                    const auto d =
                        c
                        +
                        1;


                    indices.insert(
                        indices.end(),
                        {
                            a, b, d,
                            a, d, c
                        });
                }
            }
        }


        // =========================================================
        // OPTIONAL TOPOLOGY CHECK
        // =========================================================

        if (
            std::getenv(
                "SPACESIM_HIDDEN_CHECK"))
        {
            using Point =
                std::pair<int, int>;


            using Edge =
                std::pair<Point, Point>;


            std::map<Edge, int> edges;


            double area =
                0.0;


            const auto point =
                [&](std::uint32_t index)
                {
                    const auto p =
                        vertices.at(
                            index)
                            .position;


                    return Point
                    {
                        int(
                            std::lround(
                                p.x
                                /
                                .004f)),

                        int(
                            std::lround(
                                p.z
                                /
                                .004f))
                    };
                };


            for (
                std::size_t i = 0;
                i < indices.size();
                i += 3)
            {
                const Point a =
                    point(
                        indices[i]);


                const Point b =
                    point(
                        indices[i + 1]);


                const Point c =
                    point(
                        indices[i + 2]);


                const double cross =
                    double(
                        b.first
                        -
                        a.first)
                    *
                    (
                        c.second
                        -
                        a.second
                    )
                    -
                    double(
                        b.second
                        -
                        a.second)
                    *
                    (
                        c.first
                        -
                        a.first
                    );


                if (cross < 0)
                {
                    throw std::runtime_error(
                        "Ocean ring winding inverted");
                }


                if (cross == 0)
                {
                    // Collapsed stitch triangle.
                    continue;
                }


                area +=
                    cross
                    *
                    .5;


                const auto edge =
                    [&](Point u, Point v)
                    {
                        if (v < u)
                        {
                            std::swap(
                                u,
                                v);
                        }


                        ++edges[
                            {
                                u,
                                v
                            }];
                    };


                edge(
                    a,
                    b);


                edge(
                    b,
                    c);


                edge(
                    c,
                    a);
            }


            constexpr int boundary =
                131072;


            for (
                const auto& entry :
                edges)
            {
                const auto a =
                    entry.first.first;


                const auto b =
                    entry.first.second;


                const bool outer =
                    (
                        a.first
                            ==
                            b.first
                        &&
                        std::abs(
                            a.first)
                            ==
                            boundary
                    )
                    ||
                    (
                        a.second
                            ==
                            b.second
                        &&
                        std::abs(
                            a.second)
                            ==
                            boundary
                    );


                if (
                    entry.second
                        !=
                        (
                            outer
                                ?
                                1
                                :
                                2
                        ))
                {
                    throw std::runtime_error(
                        "Ocean ring gap or duplicate coverage");
                }
            }


            const double expected =
                4.0
                *
                boundary
                *
                boundary;


            if (
                std::abs(
                    area
                    -
                    expected)
                    >
                    1.0)
            {
                throw std::runtime_error(
                    "Ocean ring area mismatch");
            }


            std::cout
                <<
                "Ocean topology check passed: "
                <<
                vertices.size()
                <<
                " vertices, "
                <<
                indices.size() / 3
                <<
                " triangles, shared boundaries and complete coverage"
                <<
                std::endl;
        }


        return
            std::make_unique<
                SpaceSim::GpuMesh>(
                    vertices,
                    indices);
    }
}


namespace SpaceSim
{
    PlanetPass::PlanetPass()
        :
        m_shader(
            "data/shaders/planet/planet.vert",
            "data/shaders/planet/planet.frag"),

        m_oceanPatchShader(
            "data/shaders/planet/ocean_patch.vert",
            "data/shaders/planet/ocean.frag"),

        m_oceanPatchMesh(
            createOceanPatchMesh())
    {
        m_shader.setInt(
            "atmosphereTransmittanceLut",
            7);


        m_shader.setInt(
            "atmosphereSkyIrradianceLut",
            8);


        m_shader.setInt(
            "atmosphereMultipleScatteringLut",
            9);


        m_oceanPatchShader.setInt(
            "atmosphereTransmittanceLut",
            7);


        m_oceanPatchShader.setInt(
            "atmosphereSkyIrradianceLut",
            8);


        m_oceanPatchShader.setInt(
            "atmosphereMultipleScatteringLut",
            9);


        m_oceanPatchShader.setInt(
            "atmosphereSkyReflectionLut",
            10);
    }


    PlanetPass::~PlanetPass() =
        default;


    void PlanetPass::render(
        const RenderCamera& camera,
        float aspectRatio,
        const DirectionalLight& sun,
        const std::vector<PlanetRenderObject>& planets,
        const AtmosphereInstance* atmosphere,
        GLuint skyReflectionTexture)
    {
        if (planets.empty())
        {
            return;
        }


        const bool atmosphereActive =
            atmosphere
                !=
                nullptr
            &&
            atmosphere->valid();


        const float currentTimeSeconds =
            rendererTimeSeconds();


        float kmPerWorldUnit =
            1.0f;


        if (atmosphereActive)
        {
            const AtmosphereParameters& parameters =
                *atmosphere->parameters;


            kmPerWorldUnit =
                parameters.bottomRadiusKm
                /
                atmosphere->planetRadiusWorld;
        }


        // =========================================================
        // COMMON SHADER STATE
        // =========================================================

        const auto configureCommonShader =
            [&](
                GlShader& shader)
            {
                shader.use();


                shader.setMat4(
                    "view",
                    camera.viewMatrix());


                shader.setMat4(
                    "projection",
                    camera.projectionMatrix(
                        aspectRatio));


                shader.setVec3(
                    "cameraPosition",
                    camera.position);


                shader.setVec3(
                    "sunDirection",
                    sun.direction);


                shader.setVec3(
                    "sunRadiance",
                    sun.radiance);


                shader.setFloat(
                    "sunAngularRadiusRadians",
                    sun.sourceAngularRadiusRadians);


                shader.setFloat(
                    "timeSeconds",
                    currentTimeSeconds);


                shader.setInt(
                    "atmosphereEnabled",
                    atmosphereActive
                        ?
                        1
                        :
                        0);


                if (!atmosphereActive)
                {
                    return;
                }


                const AtmosphereParameters& parameters =
                    *atmosphere->parameters;


                shader.setVec3(
                    "atmospherePlanetCenterWorld",
                    atmosphere->planetCenterWorld);


                shader.setFloat(
                    "atmosphereKmPerWorldUnit",
                    kmPerWorldUnit);


                shader.setFloat(
                    "atmosphereBottomRadiusKm",
                    parameters.bottomRadiusKm);


                shader.setFloat(
                    "atmosphereTopRadiusKm",
                    parameters.topRadiusKm);


                shader.setVec3(
                    "atmosphereRayleighScatteringPerKm",
                    parameters.rayleighScatteringPerKm);


                shader.setFloat(
                    "atmosphereRayleighScaleHeightKm",
                    parameters.rayleighScaleHeightKm);


                shader.setVec3(
                    "atmosphereMieScatteringPerKm",
                    parameters.mieScatteringPerKm);


                shader.setVec3(
                    "atmosphereMieExtinctionPerKm",
                    parameters.mieExtinctionPerKm);


                shader.setFloat(
                    "atmosphereMieScaleHeightKm",
                    parameters.mieScaleHeightKm);


                shader.setFloat(
                    "atmosphereMieAnisotropy",
                    parameters.mieAnisotropy);


                shader.setVec3(
                    "atmosphereOzoneAbsorptionPerKm",
                    parameters.ozoneAbsorptionPerKm);


                shader.setFloat(
                    "atmosphereOzoneCenterHeightKm",
                    parameters.ozoneCenterHeightKm);


                shader.setFloat(
                    "atmosphereOzoneHalfWidthKm",
                    parameters.ozoneHalfWidthKm);


                glBindTextureUnit(
                    7,
                    atmosphere
                        ->luts
                        ->transmittance()
                        .id());


                glBindTextureUnit(
                    8,
                    atmosphere
                        ->luts
                        ->skyIrradiance()
                        .id());


                glBindTextureUnit(
                    9,
                    atmosphere
                        ->luts
                        ->multipleScattering()
                        .id());


                glBindTextureUnit(
                    10,
                    skyReflectionTexture);
            };


        // =========================================================
        // SHARED MATERIAL
        // =========================================================

        const auto configureMaterial =
            [](
                GlShader& shader,
                const PlanetMaterial& material)
            {
                shader.setVec3(
                    "deepOceanColor",
                    material.deepOceanColor);


                shader.setVec3(
                    "shallowOceanColor",
                    material.shallowOceanColor);


                shader.setFloat(
                    "oceanRoughness",
                    material.oceanRoughness);


                shader.setFloat(
                    "oceanWaveScale",
                    material.oceanWaveScale);


                shader.setFloat(
                    "oceanWaveStrength",
                    material.oceanWaveStrength);


                shader.setFloat(
                    "oceanWaveSpeed",
                    material.oceanWaveSpeed);


                shader.setVec3(
                    "lowLandColor",
                    material.lowLandColor);


                shader.setVec3(
                    "highLandColor",
                    material.highLandColor);


                shader.setFloat(
                    "landRoughness",
                    material.landRoughness);


                shader.setFloat(
                    "continentScale",
                    material.continentScale);


                shader.setFloat(
                    "detailScale",
                    material.detailScale);


                shader.setFloat(
                    "oceanLevel",
                    material.oceanLevel);


                shader.setFloat(
                    "coastWidth",
                    material.coastWidth);


                shader.setFloat(
                    "planetSeed",
                    material.seed);
            };


        // =========================================================
        // FIND LOCAL SURFACE/OCEAN HOST
        // =========================================================
        //
        // Only planets with a stable surface frame are eligible.
        //
        // In the game that frame is generated from global dvec3
        // coordinates before conversion to renderer floats.
        //

        const PlanetRenderObject* host =
            nullptr;


        float bestAltitudeKm =
            24.0f;


        glm::vec3 hostCenter
        {
            0.0f
        };


        float hostRadius =
            0.0f;


        for (
            const auto& candidate :
            planets)
        {
            if (
                !candidate.mesh
                ||
                !candidate.hasOcean
                ||
                candidate.radiusKm
                    <=
                    0.0f
                ||
                !candidate.surfaceFrameValid)
            {
                continue;
            }


            const float radius =
                glm::length(
                    glm::vec3(
                        candidate.modelMatrix[0]));


            if (
                radius
                    <=
                    0.0f
                ||
                std::abs(
                    glm::length(
                        glm::vec3(
                            candidate.modelMatrix[1]))
                    -
                    radius)
                    >
                    radius
                    *
                    1e-4f
                ||
                std::abs(
                    glm::length(
                        glm::vec3(
                            candidate.modelMatrix[2]))
                    -
                    radius)
                    >
                    radius
                    *
                    1e-4f)
            {
                continue;
            }


            const float altitude =
                candidate.surfaceAltitudeKm;


            if (
                altitude
                    >=
                    0.0f
                &&
                altitude
                    <
                    bestAltitudeKm)
            {
                host =
                    &candidate;


                bestAltitudeKm =
                    altitude;


                hostCenter =
                    glm::vec3(
                        candidate.modelMatrix[3]);


                hostRadius =
                    radius;
            }
        }


        // =========================================================
        // LOCAL SURFACE BASIS
        // =========================================================

        glm::vec3 anchorNormal
        {
            0.0f
        };


        glm::vec3 anchorRight
        {
            0.0f
        };


        glm::vec3 anchorForward
        {
            0.0f
        };


        constexpr float outerKm =
            524.288f;


        // =========================================================
        // LOCAL GEOMETRIC OCEAN
        // =========================================================

        if (host)
        {
            // This normal was created from the authoritative
            // double-precision camera/planet positions.
            anchorNormal =
                glm::normalize(
                    host
                        ->surfaceAnchorNormalWorld);


            // Keep tangent orientation continuous while travelling
            // over the sphere.
            if (
                glm::length(
                    m_anchorRight)
                    <
                    0.5f
                ||
                std::abs(
                    glm::dot(
                        m_anchorRight,
                        anchorNormal))
                    >
                    .99f)
            {
                const glm::vec3 helper =
                    std::abs(
                        anchorNormal.y)
                        <
                        .9f
                    ?
                    glm::vec3(
                        0.0f,
                        1.0f,
                        0.0f)
                    :
                    glm::vec3(
                        1.0f,
                        0.0f,
                        0.0f);


                m_anchorRight =
                    glm::normalize(
                        glm::cross(
                            helper,
                            anchorNormal));
            }


            anchorRight =
                glm::normalize(
                    m_anchorRight
                    -
                    anchorNormal
                    *
                    glm::dot(
                        m_anchorRight,
                        anchorNormal));


            anchorForward =
                glm::cross(
                    anchorNormal,
                    anchorRight);


            m_anchorRight =
                anchorRight;


            configureCommonShader(
                m_oceanPatchShader);


            configureMaterial(
                m_oceanPatchShader,
                host->material);


            m_oceanPatchShader.setFloat(
                "oceanOuterKm",
                outerKm);


            m_oceanPatchShader.setFloat(
                "oceanRadiusKm",
                host->radiusKm);


            m_oceanPatchShader.setFloat(
                "oceanGravity",
                host
                    ->oceanGravityMetersPerSecondSquared
                *
                .001f);


            m_oceanPatchShader.setVec3(
                "planetCenterWorld",
                hostCenter);


            m_oceanPatchShader.setFloat(
                "planetRadiusWorld",
                hostRadius);


            m_oceanPatchShader.setFloat(
                "worldUnitsPerKm",
                hostRadius
                /
                host->radiusKm);


            m_oceanPatchShader.setVec3(
                "oceanAnchorNormal",
                anchorNormal);


            m_oceanPatchShader.setVec3(
                "oceanAnchorRight",
                anchorRight);


            m_oceanPatchShader.setVec3(
                "oceanAnchorForward",
                anchorForward);


            // IMPORTANT:
            //
            // This is now already a small camera-relative value.
            //
            // At an 8 km arrival it corresponds directly to
            // approximately 8 km downward toward sea level.
            //
            // No giant-float subtraction happens here.
            m_oceanPatchShader.setVec3(
                "oceanAnchorRelative",
                host
                    ->surfaceAnchorRelativeWorld);


            m_oceanPatchShader.setMat4(
                "planetInverseModel",
                glm::inverse(
                    host->modelMatrix));


            // Full local geometric waves below 16 km.
            //
            // Fade them smoothly away between 16 and 24 km so
            // there is no sudden handoff.
            const float geometricWaveVisibility =
                1.0f
                -
                glm::smoothstep(
                    16.0f,
                    24.0f,
                    bestAltitudeKm);


            m_oceanPatchShader.setFloat(
                "geometricWaveVisibility",
                geometricWaveVisibility);


            m_oceanPatchMesh->draw();
        }


        // =========================================================
        // WHOLE PLANETS
        // =========================================================

        configureCommonShader(
            m_shader);


        for (
            const PlanetRenderObject& planet :
            planets)
        {
            if (!planet.mesh)
            {
                continue;
            }


            m_shader.setMat4(
                "model",
                planet.modelMatrix);


            configureMaterial(
                m_shader,
                planet.material);


            m_shader.setInt(
                "oceanSurfacePass",
                0);


            // The globe continues to draw land and the distant
            // ocean.
            //
            // Inside the local patch footprint the globe's ocean
            // fragments are discarded so the stable local surface
            // owns the nearby water.
            m_shader.setInt(
                "oceanLocalCoverage",
                &planet == host
                    ?
                    1
                    :
                    0);


            m_shader.setFloat(
                "oceanOuterKm",
                outerKm);


            m_shader.setFloat(
                "oceanRadiusKm",
                planet.radiusKm);


            m_shader.setVec3(
                "planetCenterWorld",
                glm::vec3(
                    planet.modelMatrix[3]));


            m_shader.setFloat(
                "planetRadiusWorld",
                glm::length(
                    glm::vec3(
                        planet.modelMatrix[0])));


            m_shader.setVec3(
                "oceanAnchorNormal",
                anchorNormal);


            m_shader.setVec3(
                "oceanAnchorRight",
                anchorRight);


            m_shader.setVec3(
                "oceanAnchorForward",
                anchorForward);


            m_shader.setMat4(
                "planetInverseModel",
                glm::inverse(
                    planet.modelMatrix));


            m_adaptiveTerrain.drawOrMesh(
                planet,
                camera);
        }
    }
}