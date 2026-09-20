#include "renderer/StarPass.h"

#include "renderer/CameraRayReconstruction.h"

#include <glm/geometric.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <random>
#include <vector>


namespace
{
    constexpr float Pi =
        3.14159265359f;


    // =============================================================
    // BACKGROUND STAR POPULATION
    // =============================================================
    //
    // These stars represent the extremely distant background sky,
    // NOT stars belonging to the currently generated solar system.
    //
    // The local system's stars will eventually come from physical
    // generated properties such as:
    //
    //     position
    //     radius
    //     luminosity
    //     effective temperature
    //
    // and will be rendered as actual stellar sources.


    constexpr int BackgroundStarCount =
        20000;


    // Approximate apparent-magnitude range represented by this
    // background catalog.
    //
    // Smaller magnitude = brighter.
    constexpr float BrightestMagnitude =
        -1.5f;


    constexpr float FaintestMagnitude =
        6.5f;


    // Renderer calibration for a magnitude-0 unresolved star.
    //
    // This is NOT yet an absolute SI photometric calibration.
    //
    // It simply connects astronomical relative magnitudes to our
    // current HDR radiance scale.
    //
    // The important part is that all stars now have physically
    // sensible relative brightness instead of arbitrary 0.1 -> 3.0
    // values.
    constexpr float MagnitudeZeroRadiance =
        6.0f;


    struct StarVertex
    {
        float directionX;
        float directionY;
        float directionZ;

        float radianceR;
        float radianceG;
        float radianceB;

        float sizePixels;
    };


    glm::vec3 lerpColor(
        const glm::vec3& a,
        const glm::vec3& b,
        float t)
    {
        return
            a
            +
            (
                b -
                a
            )
            *
            t;
    }


    // =============================================================
    // APPARENT MAGNITUDE -> RELATIVE FLUX
    // =============================================================
    //
    // Astronomy's magnitude system uses:
    //
    //     flux ratio = 10^(-0.4 * magnitudeDifference)
    //
    // Therefore:
    //
    // magnitude  0 -> 1.000
    // magnitude +1 -> 0.398
    // magnitude +5 -> 0.010
    // magnitude -1 -> 2.512

    float magnitudeToRelativeBrightness(
        float magnitude)
    {
        return
            std::pow(
                10.0f,
                -0.4f *
                magnitude);
    }


    // =============================================================
    // SAMPLE STAR MAGNITUDE
    // =============================================================
    //
    // We deliberately do NOT choose magnitudes uniformly.
    //
    // There are vastly more faint stars than bright ones.
    //
    // A simple approximation for cumulative star counts is:
    //
    //     N(<m) proportional to 10^(0.6m)
    //
    // This is not intended to reproduce a literal astronomical
    // catalog, but it gives us the important population structure:
    //
    //     very few bright stars
    //     some medium stars
    //     lots of faint stars

    float sampleMagnitude(
        float randomValue)
    {
        const float minimumPopulation =
            std::pow(
                10.0f,
                0.6f *
                BrightestMagnitude);


        const float maximumPopulation =
            std::pow(
                10.0f,
                0.6f *
                FaintestMagnitude);


        const float populationValue =
            minimumPopulation
            +
            randomValue
            *
            (
                maximumPopulation
                -
                minimumPopulation
            );


        return
            std::log10(
                populationValue)
            /
            0.6f;
    }
}


namespace SpaceSim
{
    StarPass::StarPass()
        : m_shader(
              "data/shaders/renderer/environment.vert",
              "data/shaders/renderer/star_disk.frag"),

          m_starfieldShader(
              "data/shaders/renderer/starfield.vert",
              "data/shaders/renderer/starfield.frag")
    {
        // =========================================================
        // PRIMARY STAR FULLSCREEN TRIANGLE
        // =========================================================

        glCreateVertexArrays(
            1,
            &m_vertexArray);


        // =========================================================
        // DISTANT STAR CATALOG
        // =========================================================
        //
        // Fixed seed:
        //
        // restarting RendererLab gives us the exact same distant
        // universe every time.
        //
        // Eventually a galaxy/universe seed can replace this.

        std::mt19937 randomGenerator(
            0x53A71F1u);


        std::uniform_real_distribution<float> unitRandom(
            0.0f,
            1.0f);


        std::vector<StarVertex> stars;


        stars.reserve(
            BackgroundStarCount);


        for (int index = 0;
             index < BackgroundStarCount;
             ++index)
        {
            // =====================================================
            // FIXED DIRECTION ON CELESTIAL SPHERE
            // =====================================================

            const float vertical =
                unitRandom(
                    randomGenerator)
                *
                2.0f
                -
                1.0f;


            const float azimuth =
                unitRandom(
                    randomGenerator)
                *
                2.0f
                *
                Pi;


            const float horizontalRadius =
                std::sqrt(
                    std::max(
                        0.0f,
                        1.0f
                        -
                        vertical *
                        vertical));


            const glm::vec3 direction
            {
                horizontalRadius *
                std::cos(
                    azimuth),

                vertical,

                horizontalRadius *
                std::sin(
                    azimuth)
            };


            // =====================================================
            // APPARENT MAGNITUDE
            // =====================================================

            const float magnitude =
                sampleMagnitude(
                    unitRandom(
                        randomGenerator));


            const float relativeBrightness =
                magnitudeToRelativeBrightness(
                    magnitude);


            // =====================================================
            // STAR COLOR
            // =====================================================
            //
            // Still a simplified stellar-color population.
            //
            // Local procedural stars will eventually derive color
            // properly from effective temperature.

            const float colorRandom =
                unitRandom(
                    randomGenerator);


            const glm::vec3 warmColor
            {
                1.00f,
                0.72f,
                0.50f
            };


            const glm::vec3 neutralColor
            {
                1.00f,
                0.96f,
                0.88f
            };


            const glm::vec3 coolColor
            {
                0.70f,
                0.82f,
                1.00f
            };


            glm::vec3 starColor;


            if (colorRandom <
                0.18f)
            {
                const float t =
                    colorRandom /
                    0.18f;


                starColor =
                    lerpColor(
                        warmColor,
                        neutralColor,
                        t);
            }
            else if (colorRandom >
                     0.78f)
            {
                const float t =
                    (
                        colorRandom -
                        0.78f
                    )
                    /
                    0.22f;


                starColor =
                    lerpColor(
                        neutralColor,
                        coolColor,
                        t);
            }
            else
            {
                starColor =
                    neutralColor;
            }


            // =====================================================
            // HDR STAR RADIANCE
            // =====================================================

            const glm::vec3 radiance =
                starColor
                *
                MagnitudeZeroRadiance
                *
                relativeBrightness;


            // =====================================================
            // APPARENT POINT SIZE
            // =====================================================
            //
            // Real stars are unresolved points at these resolutions.
            //
            // The slight size increase for the brightest ones is a
            // rendering approximation for their stronger apparent
            // image response.
            //
            // Bloom will eventually provide most of the perceived
            // spread around bright stars.

            const float brightnessRank =
                1.0f
                -
                std::clamp(
                    (
                        magnitude
                        -
                        BrightestMagnitude
                    )
                    /
                    (
                        FaintestMagnitude
                        -
                        BrightestMagnitude
                    ),
                    0.0f,
                    1.0f);


            const float sizePixels =
                1.0f
                +
                std::pow(
                    brightnessRank,
                    2.5f)
                *
                1.4f;


            stars.push_back(
                StarVertex
                {
                    direction.x,
                    direction.y,
                    direction.z,

                    radiance.r,
                    radiance.g,
                    radiance.b,

                    sizePixels
                });
        }


        m_starCount =
            static_cast<GLsizei>(
                stars.size());


        // =========================================================
        // STARFIELD GPU BUFFER
        // =========================================================

        glCreateVertexArrays(
            1,
            &m_starfieldVertexArray);


        glCreateBuffers(
            1,
            &m_starfieldVertexBuffer);


        glNamedBufferData(
            m_starfieldVertexBuffer,
            static_cast<GLsizeiptr>(
                stars.size()
                *
                sizeof(
                    StarVertex)),
            stars.data(),
            GL_STATIC_DRAW);


        glVertexArrayVertexBuffer(
            m_starfieldVertexArray,
            0,
            m_starfieldVertexBuffer,
            0,
            sizeof(
                StarVertex));


        // =========================================================
        // ATTRIBUTE 0: DIRECTION
        // =========================================================

        glEnableVertexArrayAttrib(
            m_starfieldVertexArray,
            0);


        glVertexArrayAttribFormat(
            m_starfieldVertexArray,
            0,
            3,
            GL_FLOAT,
            GL_FALSE,
            offsetof(
                StarVertex,
                directionX));


        glVertexArrayAttribBinding(
            m_starfieldVertexArray,
            0,
            0);


        // =========================================================
        // ATTRIBUTE 1: HDR RADIANCE
        // =========================================================

        glEnableVertexArrayAttrib(
            m_starfieldVertexArray,
            1);


        glVertexArrayAttribFormat(
            m_starfieldVertexArray,
            1,
            3,
            GL_FLOAT,
            GL_FALSE,
            offsetof(
                StarVertex,
                radianceR));


        glVertexArrayAttribBinding(
            m_starfieldVertexArray,
            1,
            0);


        // =========================================================
        // ATTRIBUTE 2: POINT SIZE
        // =========================================================

        glEnableVertexArrayAttrib(
            m_starfieldVertexArray,
            2);


        glVertexArrayAttribFormat(
            m_starfieldVertexArray,
            2,
            1,
            GL_FLOAT,
            GL_FALSE,
            offsetof(
                StarVertex,
                sizePixels));


        glVertexArrayAttribBinding(
            m_starfieldVertexArray,
            2,
            0);
    }


    StarPass::~StarPass()
    {
        if (m_starfieldVertexBuffer != 0)
        {
            glDeleteBuffers(
                1,
                &m_starfieldVertexBuffer);


            m_starfieldVertexBuffer =
                0;
        }


        if (m_starfieldVertexArray != 0)
        {
            glDeleteVertexArrays(
                1,
                &m_starfieldVertexArray);


            m_starfieldVertexArray =
                0;
        }


        if (m_vertexArray != 0)
        {
            glDeleteVertexArrays(
                1,
                &m_vertexArray);


            m_vertexArray =
                0;
        }
    }


    void StarPass::render(
        const RenderCamera& camera,
        float aspectRatio,
        const DirectionalLight& star)
    {
        // =========================================================
        // BACKGROUND RENDER STATE
        // =========================================================

        glDisable(
            GL_DEPTH_TEST);


        glDepthMask(
            GL_FALSE);


        glEnable(
            GL_BLEND);


        glBlendEquation(
            GL_FUNC_ADD);


        glBlendFunc(
            GL_ONE,
            GL_ONE);


        // =========================================================
        // DISTANT STARFIELD
        // =========================================================

        m_starfieldShader.use();


        m_starfieldShader.setMat4(
            "view",
            camera.viewMatrix());


        m_starfieldShader.setMat4(
            "projection",
            camera.projectionMatrix(
                aspectRatio));


        glEnable(
            GL_PROGRAM_POINT_SIZE);


        glBindVertexArray(
            m_starfieldVertexArray);


        glDrawArrays(
            GL_POINTS,
            0,
            m_starCount);


        glBindVertexArray(
            0);


        glDisable(
            GL_PROGRAM_POINT_SIZE);


        // =========================================================
        // VISIBLE PRIMARY SYSTEM STAR
        // =========================================================

        if (star.sourceVisible &&
            star.sourceAngularRadiusRadians >
            0.0f)
        {
            const glm::mat4 rayReconstructionMatrix =
                makeCameraRayReconstructionMatrix(
                    camera,
                    aspectRatio);


            m_shader.use();


            m_shader.setMat4(
                "inverseViewProjection",
                rayReconstructionMatrix);


            m_shader.setVec3(
                "cameraPosition",
                camera.position);


            m_shader.setVec3(
                "starDirection",
                glm::normalize(
                    star.direction));


            m_shader.setVec3(
                "starDiskRadiance",
                star.sourceDiskRadiance);


            m_shader.setFloat(
                "starAngularRadiusRadians",
                star.sourceAngularRadiusRadians);


            m_shader.setFloat(
                "limbDarkening",
                star.sourceLimbDarkening);


            glBindVertexArray(
                m_vertexArray);


            glDrawArrays(
                GL_TRIANGLES,
                0,
                3);


            glBindVertexArray(
                0);
        }


        // =========================================================
        // RESTORE STATE
        // =========================================================

        glDisable(
            GL_BLEND);


        glDepthMask(
            GL_TRUE);


        glEnable(
            GL_DEPTH_TEST);
    }
}