#include "renderer/planet/AdaptivePlanetSurfaceRenderer.h"

#include "renderer/MeshVertex.h"
#include "renderer/RenderCamera.h"

#include "renderer/opengl/GpuMesh.h"

#include "renderer/planet/PlanetMaterial.h"
#include "renderer/planet/PlanetRenderObject.h"

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <future>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>


namespace
{
    // =============================================================
    // TILE KEY
    // =============================================================

    struct TileKey
    {
        int face = 0;

        int level = 0;

        int x = 0;

        int y = 0;


        bool operator==(
            const TileKey& other) const
        {
            return
                face == other.face
                &&
                level == other.level
                &&
                x == other.x
                &&
                y == other.y;
        }


        std::array<TileKey, 4> children() const
        {
            return
            {{
                {
                    face,
                    level + 1,
                    x * 2,
                    y * 2
                },

                {
                    face,
                    level + 1,
                    x * 2 + 1,
                    y * 2
                },

                {
                    face,
                    level + 1,
                    x * 2,
                    y * 2 + 1
                },

                {
                    face,
                    level + 1,
                    x * 2 + 1,
                    y * 2 + 1
                }
            }};
        }
    };


    struct TileKeyHash
    {
        std::size_t operator()(
            const TileKey& key) const
        {
            std::size_t hash =
                static_cast<std::size_t>(
                    key.face);


            hash =
                hash * 31u
                +
                static_cast<std::size_t>(
                    key.level);


            hash =
                hash * 31u
                +
                static_cast<std::size_t>(
                    key.x);


            hash =
                hash * 31u
                +
                static_cast<std::size_t>(
                    key.y);


            return
                hash;
        }
    };


    // =============================================================
    // CPU TILE DATA
    // =============================================================

    struct CpuTileData
    {
        std::vector<SpaceSim::MeshVertex> vertices;

        std::vector<std::uint32_t> indices;
    };


    constexpr int TileCells =
        16;


    constexpr int TileSide =
        TileCells + 1;


    constexpr int MaxLevel =
        10;


    // =============================================================
    // PROCEDURAL SURFACE
    // =============================================================
    //
    // This deliberately mirrors the CURRENT new-renderer
    // PlanetMaterial noise convention rather than dragging the
    // Raylib PlanetSurfaceSampler into the new renderer.
    //
    // Once this geometry path is proven, we will extract a proper
    // shared planet surface source for both renderer generations.


    float fract(
        float value)
    {
        return
            value
            -
            std::floor(
                value);
    }


    glm::vec3 fract(
        const glm::vec3& value)
    {
        return
        {
            fract(
                value.x),

            fract(
                value.y),

            fract(
                value.z)
        };
    }


    float smoothStep(
        float edge0,
        float edge1,
        float value)
    {
        const float denominator =
            std::max(
                edge1 - edge0,
                0.000001f);


        const float t =
            glm::clamp(
                (value - edge0)
                /
                denominator,
                0.0f,
                1.0f);


        return
            t
            *
            t
            *
            (
                3.0f
                -
                2.0f
                *
                t
            );
    }


    float hash31(
        glm::vec3 p)
    {
        p =
            fract(
                p
                *
                0.1031f);


        p +=
            glm::dot(
                p,
                glm::vec3(
                    p.y,
                    p.z,
                    p.x)
                +
                glm::vec3(
                    33.33f));


        return
            fract(
                (
                    p.x
                    +
                    p.y
                )
                *
                p.z);
    }


    float valueNoise(
        glm::vec3 p)
    {
        const glm::vec3 cell
        {
            std::floor(
                p.x),

            std::floor(
                p.y),

            std::floor(
                p.z)
        };


        glm::vec3 f =
            fract(
                p);


        const glm::vec3 u =
            f
            *
            f
            *
            (
                glm::vec3(
                    3.0f)
                -
                2.0f
                *
                f
            );


        const float n000 =
            hash31(
                cell
                +
                glm::vec3(
                    0.0f,
                    0.0f,
                    0.0f));


        const float n100 =
            hash31(
                cell
                +
                glm::vec3(
                    1.0f,
                    0.0f,
                    0.0f));


        const float n010 =
            hash31(
                cell
                +
                glm::vec3(
                    0.0f,
                    1.0f,
                    0.0f));


        const float n110 =
            hash31(
                cell
                +
                glm::vec3(
                    1.0f,
                    1.0f,
                    0.0f));


        const float n001 =
            hash31(
                cell
                +
                glm::vec3(
                    0.0f,
                    0.0f,
                    1.0f));


        const float n101 =
            hash31(
                cell
                +
                glm::vec3(
                    1.0f,
                    0.0f,
                    1.0f));


        const float n011 =
            hash31(
                cell
                +
                glm::vec3(
                    0.0f,
                    1.0f,
                    1.0f));


        const float n111 =
            hash31(
                cell
                +
                glm::vec3(
                    1.0f,
                    1.0f,
                    1.0f));


        const float nx00 =
            glm::mix(
                n000,
                n100,
                u.x);


        const float nx10 =
            glm::mix(
                n010,
                n110,
                u.x);


        const float nx01 =
            glm::mix(
                n001,
                n101,
                u.x);


        const float nx11 =
            glm::mix(
                n011,
                n111,
                u.x);


        const float nxy0 =
            glm::mix(
                nx00,
                nx10,
                u.y);


        const float nxy1 =
            glm::mix(
                nx01,
                nx11,
                u.y);


        return
            glm::mix(
                nxy0,
                nxy1,
                u.z);
    }


    float fbm(
        glm::vec3 p)
    {
        float result =
            0.0f;


        float amplitude =
            0.5f;


        float totalAmplitude =
            0.0f;


        for (int octave = 0;
             octave < 5;
             ++octave)
        {
            result +=
                valueNoise(
                    p)
                *
                amplitude;


            totalAmplitude +=
                amplitude;


            p =
                p
                *
                2.03f
                +
                glm::vec3(
                    17.1f,
                    31.7f,
                    11.3f);


            amplitude *=
                0.5f;
        }


        return
            result
            /
            std::max(
                totalAmplitude,
                0.0001f);
    }


    float terrainRadius(
        const glm::vec3& direction,
        const SpaceSim::PlanetMaterial& material)
    {
        return SpaceSim::samplePlanetSurface(direction, material).radiusScale;

        const glm::vec3 seedOffset
        {
            material.seed
                *
                1.371f,

            material.seed
                *
                2.113f,

            material.seed
                *
                3.731f
        };


        const float continents =
            fbm(
                direction
                *
                material.continentScale
                +
                seedOffset);


        const float detail =
            fbm(
                direction
                *
                material.detailScale
                +
                seedOffset
                *
                2.7f);


        const float height =
            continents
            *
            0.82f
            +
            detail
            *
            0.18f;


        const float landMask =
            smoothStep(
                material.oceanLevel
                -
                material.coastWidth,

                material.oceanLevel
                +
                material.coastWidth,

                height);


        if (landMask <=
            0.00001f)
        {
            // Sea level is exactly radius 1.
            return
                1.0f;
        }


        const float landElevation =
            glm::clamp(
                (
                    height
                    -
                    material.oceanLevel
                )
                /
                std::max(
                    1.0f
                    -
                    material.oceanLevel,
                    0.0001f),
                0.0f,
                1.0f);


        const float broadShape =
            std::pow(
                landElevation,
                1.35f);


        // Reuse the detail field to create broad connected ridges.
        //
        // Maximum relief remains close to the scale of the old
        // adaptive terrain prototype: roughly 0.15% of radius.
        const float ridge =
            1.0f
            -
            std::abs(
                detail
                *
                2.0f
                -
                1.0f);


        const float mountainShape =
            ridge
            *
            ridge
            *
            ridge
            *
            broadShape
            *
            broadShape;


        const float elevationFraction =
            landMask
            *
            std::max(
                material.terrainReliefScale,
                0.0f)
            *
            (
                0.00030f
                *
                broadShape
                +
                0.00115f
                *
                mountainShape
            );


        return
            1.0f
            +
            elevationFraction;
    }


    glm::vec3 surfacePosition(
        glm::vec3 direction,
        const SpaceSim::PlanetMaterial& material)
    {
        direction =
            glm::normalize(
                direction);


        return
            direction
            *
            terrainRadius(
                direction,
                material);
    }


    // =============================================================
    // CUBE-SPHERE COORDINATES
    // =============================================================

    glm::vec3 cubeSphereDirection(
        int face,
        double u,
        double v)
    {
        double a =
            0.0;


        double b =
            0.0;


        double c =
            0.0;


        switch (face)
        {
        case 0:
            a = 1.0;
            b = v;
            c = -u;
            break;

        case 1:
            a = -1.0;
            b = v;
            c = u;
            break;

        case 2:
            a = u;
            b = 1.0;
            c = -v;
            break;

        case 3:
            a = u;
            b = -1.0;
            c = v;
            break;

        case 4:
            a = u;
            b = v;
            c = 1.0;
            break;

        default:
            a = -u;
            b = v;
            c = -1.0;
            break;
        }


        const double length =
            std::sqrt(
                a * a
                +
                b * b
                +
                c * c);


        return
        {
            static_cast<float>(
                a / length),

            static_cast<float>(
                b / length),

            static_cast<float>(
                c / length)
        };
    }


    glm::vec3 tileDirection(
        const TileKey& key,
        double x,
        double y)
    {
        const double width =
            2.0
            /
            static_cast<double>(
                1u << key.level);


        return
            cubeSphereDirection(
                key.face,

                -1.0
                +
                (
                    static_cast<double>(
                        key.x)
                    +
                    x
                )
                *
                width,

                -1.0
                +
                (
                    static_cast<double>(
                        key.y)
                    +
                    y
                )
                *
                width);
    }


    // =============================================================
    // CPU TILE GENERATION
    // =============================================================

    CpuTileData generateTile(
        const TileKey& key,
        const SpaceSim::PlanetMaterial& material)
    {
        CpuTileData data;


        data.vertices.reserve(
            TileSide
            *
            TileSide
            +
            4
            *
            TileSide);


        data.indices.reserve(
            TileCells
            *
            TileCells
            *
            6
            +
            4
            *
            TileCells
            *
            6);


        const auto normalAt =
            [&](
                glm::vec3 direction)
            {
                direction =
                    glm::normalize(
                        direction);


                const glm::vec3 helper =
                    std::abs(
                        direction.y)
                    <
                    0.95f
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


                const glm::vec3 right =
                    glm::normalize(
                        glm::cross(
                            helper,
                            direction));


                const glm::vec3 up =
                    glm::cross(
                        direction,
                        right);


                constexpr float epsilon =
                    0.0001f;


                const glm::vec3 dx =
                    surfacePosition(
                        glm::normalize(
                            direction
                            +
                            right
                            *
                            epsilon),
                        material)
                    -
                    surfacePosition(
                        glm::normalize(
                            direction
                            -
                            right
                            *
                            epsilon),
                        material);


                const glm::vec3 dy =
                    surfacePosition(
                        glm::normalize(
                            direction
                            +
                            up
                            *
                            epsilon),
                        material)
                    -
                    surfacePosition(
                        glm::normalize(
                            direction
                            -
                            up
                            *
                            epsilon),
                        material);


                glm::vec3 normal =
                    glm::normalize(
                        glm::cross(
                            dx,
                            dy));


                if (glm::dot(
                        normal,
                        direction)
                    <
                    0.0f)
                {
                    normal =
                        -normal;
                }


                return
                    normal;
            };


        // ---------------------------------------------------------
        // MAIN TILE GRID
        // ---------------------------------------------------------

        for (int y = 0;
             y <= TileCells;
             ++y)
        {
            for (int x = 0;
                 x <= TileCells;
                 ++x)
            {
                const double localX =
                    static_cast<double>(
                        x)
                    /
                    static_cast<double>(
                        TileCells);


                const double localY =
                    static_cast<double>(
                        y)
                    /
                    static_cast<double>(
                        TileCells);


                const glm::vec3 direction =
                    tileDirection(
                        key,
                        localX,
                        localY);


                SpaceSim::MeshVertex vertex{};


                vertex.position =
                    surfacePosition(
                        direction,
                        material);


                vertex.normal =
                    normalAt(
                        direction);


                vertex.texCoord =
                {
                    static_cast<float>(
                        localX),

                    static_cast<float>(
                        localY)
                };


                data.vertices.push_back(
                    vertex);
            }
        }


        const auto triangle =
            [&](
                std::uint32_t a,
                std::uint32_t b,
                std::uint32_t c)
            {
                data.indices.push_back(
                    a);


                data.indices.push_back(
                    b);


                data.indices.push_back(
                    c);
            };


        for (int y = 0;
             y < TileCells;
             ++y)
        {
            for (int x = 0;
                 x < TileCells;
                 ++x)
            {
                const std::uint32_t a =
                    static_cast<std::uint32_t>(
                        y
                        *
                        TileSide
                        +
                        x);


                const std::uint32_t b =
                    a
                    +
                    1;


                const std::uint32_t c =
                    a
                    +
                    TileSide;


                const std::uint32_t d =
                    c
                    +
                    1;


                triangle(
                    a,
                    b,
                    d);


                triangle(
                    a,
                    d,
                    c);
            }
        }


        // ---------------------------------------------------------
        // SKIRTS
        // ---------------------------------------------------------
        //
        // This preserves the old prototype's interim mixed-LOD
        // crack treatment.
        //
        // Proper neighbor balancing / edge stitching comes later.

        const float cellWidth =
            2.0f
            /
            static_cast<float>(
                (
                    1u
                    <<
                    key.level
                )
                *
                TileCells);


        const float skirtDepth =
            std::max(
                0.00002f,

                cellWidth
                *
                cellWidth
                *
                2.0f
                +
                0.00015f);


        for (int edge = 0;
             edge < 4;
             ++edge)
        {
            const std::uint32_t skirtStart =
                static_cast<std::uint32_t>(
                    data.vertices.size());


            for (int i = 0;
                 i <= TileCells;
                 ++i)
            {
                int originalIndex =
                    0;


                switch (edge)
                {
                case 0:
                    originalIndex =
                        i;
                    break;

                case 1:
                    originalIndex =
                        i
                        *
                        TileSide
                        +
                        TileCells;
                    break;

                case 2:
                    originalIndex =
                        TileCells
                        *
                        TileSide
                        +
                        TileCells
                        -
                        i;
                    break;

                default:
                    originalIndex =
                        (
                            TileCells
                            -
                            i
                        )
                        *
                        TileSide;
                    break;
                }


                SpaceSim::MeshVertex skirtVertex =
                    data.vertices[
                        static_cast<std::size_t>(
                            originalIndex)];


                const glm::vec3 radialDirection =
                    glm::normalize(
                        skirtVertex.position);


                skirtVertex.position -=
                    radialDirection
                    *
                    skirtDepth;


                data.vertices.push_back(
                    skirtVertex);


                if (i >=
                    TileCells)
                {
                    continue;
                }


                int nextIndex =
                    0;


                switch (edge)
                {
                case 0:
                    nextIndex =
                        originalIndex
                        +
                        1;
                    break;

                case 1:
                    nextIndex =
                        originalIndex
                        +
                        TileSide;
                    break;

                case 2:
                    nextIndex =
                        originalIndex
                        -
                        1;
                    break;

                default:
                    nextIndex =
                        originalIndex
                        -
                        TileSide;
                    break;
                }


                triangle(
                    static_cast<std::uint32_t>(
                        originalIndex),

                    skirtStart
                    +
                    static_cast<std::uint32_t>(
                        i),

                    static_cast<std::uint32_t>(
                        nextIndex));


                triangle(
                    static_cast<std::uint32_t>(
                        nextIndex),

                    skirtStart
                    +
                    static_cast<std::uint32_t>(
                        i),

                    skirtStart
                    +
                    static_cast<std::uint32_t>(
                        i + 1));
            }
        }


        return
            data;
    }


    // =============================================================
    // LOD SELECTION
    // =============================================================

    bool shouldSplit(
        const TileKey& key,
        const glm::vec3& cameraInPlanetRadii,
        float focalLengthPixels,
        float targetErrorPixels =
            2.0f)
    {
        if (key.level >=
            MaxLevel)
        {
            return
                false;
        }


        const float width =
            2.0f
            /
            static_cast<float>(
                1u
                <<
                key.level);


        const glm::vec3 center =
            tileDirection(
                key,
                0.5,
                0.5);


        const float distance =
            std::max(
                0.0001f,

                glm::distance(
                    cameraInPlanetRadii,
                    center)
                -
                width
                *
                0.75f);


        const float cell =
            width
            /
            static_cast<float>(
                TileCells);


        // Same basic projected-curvature estimate as the old
        // Raylib adaptive renderer.
        const float error =
            cell
            *
            cell
            *
            0.5f
            +
            width
            *
            0.00005f;


        return
            error
            *
            focalLengthPixels
            /
            distance
            >
            targetErrorPixels;
    }
}


namespace SpaceSim
{
    // =============================================================
    // IMPLEMENTATION
    // =============================================================

    struct AdaptivePlanetSurfaceRenderer::Impl
    {
        struct ResidentTile
        {
            std::unique_ptr<GpuMesh> mesh;

            std::uint64_t lastUsedFrame =
                0;
        };


        struct PendingJob
        {
            TileKey key;

            std::future<CpuTileData> result;
        };


        static constexpr std::size_t MaxResidentTiles =
            512;


        static constexpr std::size_t MaxJobs =
            2;


        static constexpr int MaxUploadsPerFrame =
            2;


        std::unordered_map<
            TileKey,
            ResidentTile,
            TileKeyHash>
            cache;


        std::unordered_set<
            TileKey,
            TileKeyHash>
            wanted;


        std::vector<TileKey> requests;

        std::vector<TileKey> visible;

        std::vector<PendingJob> jobs;


        PlanetMaterial terrainMaterial;


        bool signatureValid =
            false;


        bool rootsWereReady =
            false;


        std::uint64_t frame =
            0;


        // ---------------------------------------------------------
        // MATERIAL SIGNATURE
        // ---------------------------------------------------------

        bool materialMatches(
            const PlanetMaterial& material) const
        {
            if (!signatureValid)
            {
                return
                    false;
            }


            return
                terrainMaterial.seed
                    ==
                    material.seed
                &&
                terrainMaterial.continentScale
                    ==
                    material.continentScale
                &&
                terrainMaterial.detailScale
                    ==
                    material.detailScale
                &&
                terrainMaterial.oceanLevel
                    ==
                    material.oceanLevel
                &&
                terrainMaterial.coastWidth
                    ==
                    material.coastWidth
                &&
                terrainMaterial.terrainReliefScale
                    ==
                    material.terrainReliefScale;
        }


        void reset(
            const PlanetMaterial& material)
        {
            // std::future destruction waits for any outstanding
            // std::async generation work before the jobs disappear.
            jobs.clear();


            cache.clear();

            wanted.clear();

            requests.clear();

            visible.clear();


            terrainMaterial =
                material;


            signatureValid =
                true;


            rootsWereReady =
                false;
        }


        // ---------------------------------------------------------
        // TILE CACHE
        // ---------------------------------------------------------

        bool request(
            const TileKey& key)
        {
            if (!wanted.insert(
                    key)
                    .second)
            {
                return
                    cache.find(
                        key)
                    !=
                    cache.end();
            }


            auto found =
                cache.find(
                    key);


            if (found !=
                cache.end())
            {
                found->second.lastUsedFrame =
                    frame;


                return
                    true;
            }


            requests.push_back(
                key);


            return
                false;
        }


        void select(
            const TileKey& key,
            const glm::vec3& camera,
            float focalLengthPixels)
        {
            if (wanted.size()
                    <
                    MaxResidentTiles
                    -
                    8
                &&
                shouldSplit(
                    key,
                    camera,
                    focalLengthPixels))
            {
                const std::array<TileKey, 4> children =
                    key.children();


                bool allChildrenReady =
                    true;


                for (const TileKey& child :
                     children)
                {
                    allChildrenReady =
                        request(
                            child)
                        &&
                        allChildrenReady;
                }


                if (allChildrenReady)
                {
                    for (const TileKey& child :
                         children)
                    {
                        select(
                            child,
                            camera,
                            focalLengthPixels);
                    }


                    return;
                }
            }


            visible.push_back(
                key);
        }


        // ---------------------------------------------------------
        // COMPLETED JOB UPLOADS
        // ---------------------------------------------------------

        void processCompletedJobs()
        {
            int uploaded =
                0;


            for (auto it =
                     jobs.begin();
                 it !=
                     jobs.end()
                 &&
                 uploaded <
                     MaxUploadsPerFrame;)
            {
                if (it->result.wait_for(
                        std::chrono::seconds(
                            0))
                    !=
                    std::future_status::ready)
                {
                    ++it;

                    continue;
                }


                CpuTileData cpuData =
                    it->result.get();


                ResidentTile resident;


                resident.mesh =
                    std::make_unique<GpuMesh>(
                        cpuData.vertices,
                        cpuData.indices);


                resident.lastUsedFrame =
                    frame;


                cache.emplace(
                    it->key,
                    std::move(
                        resident));


                it =
                    jobs.erase(
                        it);


                ++uploaded;
            }
        }


        // ---------------------------------------------------------
        // EVICTION
        // ---------------------------------------------------------

        void evictUnusedTiles()
        {
            while (
                cache.size()
                    +
                    jobs.size()
                >=
                MaxResidentTiles
                &&
                !cache.empty())
            {
                auto oldest =
                    cache.end();


                for (auto it =
                         cache.begin();
                     it !=
                         cache.end();
                     ++it)
                {
                    if (wanted.find(
                            it->first)
                        !=
                        wanted.end())
                    {
                        continue;
                    }


                    if (oldest ==
                            cache.end()
                        ||
                        it->second.lastUsedFrame
                            <
                            oldest->second.lastUsedFrame)
                    {
                        oldest =
                            it;
                    }
                }


                if (oldest ==
                    cache.end())
                {
                    break;
                }


                cache.erase(
                    oldest);
            }
        }


        // ---------------------------------------------------------
        // START NEW ASYNC JOBS
        // ---------------------------------------------------------

        void launchRequests()
        {
            std::stable_sort(
                requests.begin(),
                requests.end(),
                [](
                    const TileKey& a,
                    const TileKey& b)
                {
                    return
                        a.level
                        <
                        b.level;
                });


            for (const TileKey& key :
                 requests)
            {
                if (jobs.size()
                        >=
                        MaxJobs
                    ||
                    cache.size()
                        +
                        jobs.size()
                    >=
                    MaxResidentTiles)
                {
                    break;
                }


                const bool alreadyPending =
                    std::any_of(
                        jobs.begin(),
                        jobs.end(),
                        [&](
                            const PendingJob& job)
                        {
                            return
                                job.key
                                ==
                                key;
                        });


                if (alreadyPending)
                {
                    continue;
                }


                const PlanetMaterial materialCopy =
                    terrainMaterial;


                PendingJob job;


                job.key =
                    key;


                job.result =
                    std::async(
                        std::launch::async,

                        [
                            key,
                            materialCopy
                        ]()
                        {
                            return
                                generateTile(
                                    key,
                                    materialCopy);
                        });


                jobs.push_back(
                    std::move(
                        job));
            }
        }


        // ---------------------------------------------------------
        // UPDATE
        // ---------------------------------------------------------

        bool update(
            const PlanetRenderObject& planet,
            const RenderCamera& camera)
        {
            if (!materialMatches(
                    planet.material))
            {
                reset(
                    planet.material);
            }


            ++frame;


            visible.clear();

            requests.clear();

            wanted.clear();


            processCompletedJobs();


            bool rootsReady =
                true;


            for (int face = 0;
                 face < 6;
                 ++face)
            {
                rootsReady =
                    request(
                        {
                            face,
                            0,
                            0,
                            0
                        })
                    &&
                    rootsReady;
            }


            if (rootsReady)
            {
                const glm::mat4 inverseModel =
                    glm::inverse(
                        planet.modelMatrix);


                const glm::vec3 cameraLocal =
                    glm::vec3(
                        inverseModel
                        *
                        glm::vec4(
                            camera.position,
                            1.0f));


                // PlanetPass currently receives aspect ratio rather
                // than absolute viewport dimensions.
                //
                // Use a 1080-pixel reference height for the first
                // renderer port. Once the terrain path is proven we
                // will pass the real viewport height down.
                constexpr float referenceViewportHeight =
                    1080.0f;


                const float fovRadians =
                    glm::radians(
                        camera.verticalFovDegrees);


                const float focalLengthPixels =
                    referenceViewportHeight
                    *
                    0.5f
                    /
                    std::tan(
                        fovRadians
                        *
                        0.5f);


                for (int face = 0;
                     face < 6;
                     ++face)
                {
                    select(
                        {
                            face,
                            0,
                            0,
                            0
                        },
                        cameraLocal,
                        focalLengthPixels);
                }
            }


            evictUnusedTiles();

            launchRequests();


            if (rootsReady
                &&
                !rootsWereReady)
            {
                std::cout
                    <<
                    "Adaptive planet terrain active: "
                    <<
                    cache.size()
                    <<
                    " resident tiles"
                    <<
                    std::endl;
            }


            rootsWereReady =
                rootsReady;


            return
                rootsReady;
        }


        // ---------------------------------------------------------
        // DRAW
        // ---------------------------------------------------------

        void drawVisible() const
        {
            for (const TileKey& key :
                 visible)
            {
                const auto found =
                    cache.find(
                        key);


                if (found ==
                    cache.end())
                {
                    continue;
                }


                found
                    ->second
                    .mesh
                    ->draw();
            }
        }
    };


    // =============================================================
    // PUBLIC WRAPPER
    // =============================================================

    AdaptivePlanetSurfaceRenderer::
        AdaptivePlanetSurfaceRenderer()
        : m_impl(
              std::make_unique<Impl>())
    {
    }


    AdaptivePlanetSurfaceRenderer::
        ~AdaptivePlanetSurfaceRenderer() =
        default;


    void AdaptivePlanetSurfaceRenderer::drawOrMesh(
        const PlanetRenderObject& planet,
        const RenderCamera& camera)
    {
        if (!planet.mesh)
        {
            return;
        }


        if (!planet.useAdaptiveTerrain && !planet.surfaceFrameValid)
        {
            planet.mesh->draw();

            return;
        }


        const bool rootsReady =
            m_impl->update(
                planet,
                camera);


        if (!rootsReady)
        {
            // Never show a hole while the async tile system starts.
            planet.mesh->draw();

            return;
        }


        m_impl->drawVisible();
    }
}