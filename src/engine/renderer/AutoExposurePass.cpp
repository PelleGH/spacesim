#include "renderer/AutoExposurePass.h"

#include <algorithm>
#include <cstdint>


namespace SpaceSim
{
    AutoExposurePass::AutoExposurePass()
        : m_histogramShader(
              "data/shaders/renderer/"
              "auto_exposure_histogram.comp"),

          m_adaptationShader(
              "data/shaders/renderer/"
              "auto_exposure_adapt.comp")
    {
        // =========================================================
        // HISTOGRAM BUFFER
        // =========================================================
        //
        // Layout:
        //
        // [0 ... 255] = luminance histogram
        // [256]       = valid metered pixel count

        glCreateBuffers(
            1,
            &m_histogramBuffer);


        glNamedBufferStorage(
            m_histogramBuffer,
            static_cast<GLsizeiptr>(
                HistogramValueCount *
                sizeof(std::uint32_t)),
            nullptr,
            GL_DYNAMIC_STORAGE_BIT);


        // =========================================================
        // EXPOSURE HISTORY
        // =========================================================
        //
        // We double-buffer this because one frame reads the previous
        // exposure while writing the next one.

        glCreateTextures(
            GL_TEXTURE_2D,
            2,
            m_exposureTextures);


        constexpr float initialExposure =
            1.0f;


        for (GLuint texture :
             m_exposureTextures)
        {
            glTextureStorage2D(
                texture,
                1,
                GL_R32F,
                1,
                1);


            glTextureParameteri(
                texture,
                GL_TEXTURE_MIN_FILTER,
                GL_NEAREST);


            glTextureParameteri(
                texture,
                GL_TEXTURE_MAG_FILTER,
                GL_NEAREST);


            glTextureParameteri(
                texture,
                GL_TEXTURE_WRAP_S,
                GL_CLAMP_TO_EDGE);


            glTextureParameteri(
                texture,
                GL_TEXTURE_WRAP_T,
                GL_CLAMP_TO_EDGE);


            glClearTexImage(
                texture,
                0,
                GL_RED,
                GL_FLOAT,
                &initialExposure);
        }


        // =========================================================
        // SAMPLER BINDINGS
        // =========================================================

        m_histogramShader.setInt(
            "hdrTexture",
            0);


        m_adaptationShader.setInt(
            "previousExposureTexture",
            0);
    }


    AutoExposurePass::~AutoExposurePass()
    {
        if (m_histogramBuffer != 0)
        {
            glDeleteBuffers(
                1,
                &m_histogramBuffer);


            m_histogramBuffer =
                0;
        }


        glDeleteTextures(
            2,
            m_exposureTextures);


        m_exposureTextures[0] =
            0;


        m_exposureTextures[1] =
            0;
    }


    void AutoExposurePass::update(
        GLuint hdrTexture,
        int width,
        int height)
    {
        if (hdrTexture == 0 ||
            width <= 0 ||
            height <= 0)
        {
            return;
        }


        // =========================================================
        // FRAME TIME
        // =========================================================
        //
        // Exposure adaptation needs real elapsed time so it behaves
        // similarly at 60 FPS and 144 FPS.

        const auto now =
            std::chrono::steady_clock::now();


        float deltaTimeSeconds =
            0.0f;


        if (m_hasPreviousTime)
        {
            deltaTimeSeconds =
                std::chrono::duration<float>(
                    now -
                    m_previousTime)
                    .count();
        }
        else
        {
            m_hasPreviousTime =
                true;


            deltaTimeSeconds =
                1.0f /
                60.0f;
        }


        m_previousTime =
            now;


        // Avoid a huge exposure jump after breakpoints,
        // dragging the window, etc.
        deltaTimeSeconds =
            std::clamp(
                deltaTimeSeconds,
                0.0f,
                0.1f);


        // =========================================================
        // CLEAR HISTOGRAM
        // =========================================================

        constexpr std::uint32_t zero =
            0;


        glClearNamedBufferData(
            m_histogramBuffer,
            GL_R32UI,
            GL_RED_INTEGER,
            GL_UNSIGNED_INT,
            &zero);


        // =========================================================
        // PASS 1: BUILD LUMINANCE HISTOGRAM
        // =========================================================

        m_histogramShader.use();


        m_histogramShader.setFloat(
            "minLogLuminance",
            m_minLogLuminance);


        m_histogramShader.setFloat(
            "maxLogLuminance",
            m_maxLogLuminance);


        m_histogramShader.setFloat(
            "meteringFloor",
            m_meteringFloor);


        glBindTextureUnit(
            0,
            hdrTexture);


        glBindBufferBase(
            GL_SHADER_STORAGE_BUFFER,
            0,
            m_histogramBuffer);


        constexpr int localSize =
            16;


        const GLuint groupCountX =
            static_cast<GLuint>(
                (
                    width +
                    localSize -
                    1
                )
                /
                localSize);


        const GLuint groupCountY =
            static_cast<GLuint>(
                (
                    height +
                    localSize -
                    1
                )
                /
                localSize);


        glDispatchCompute(
            groupCountX,
            groupCountY,
            1);


        glMemoryBarrier(
            GL_SHADER_STORAGE_BARRIER_BIT);


        // =========================================================
        // PASS 2: CHOOSE + ADAPT EXPOSURE
        // =========================================================

        const int previousIndex =
            m_currentExposureIndex;


        const int nextIndex =
            1 -
            previousIndex;


        m_adaptationShader.use();


        m_adaptationShader.setFloat(
            "minLogLuminance",
            m_minLogLuminance);


        m_adaptationShader.setFloat(
            "maxLogLuminance",
            m_maxLogLuminance);


        m_adaptationShader.setFloat(
            "lowPercentile",
            m_lowPercentile);


        m_adaptationShader.setFloat(
            "highPercentile",
            m_highPercentile);


        m_adaptationShader.setFloat(
            "keyValue",
            m_keyValue);


        m_adaptationShader.setFloat(
            "minimumExposure",
            m_minExposure);


        m_adaptationShader.setFloat(
            "maximumExposure",
            m_maxExposure);


        m_adaptationShader.setFloat(
            "brighteningSpeed",
            m_brighteningSpeed);


        m_adaptationShader.setFloat(
            "darkeningSpeed",
            m_darkeningSpeed);


        m_adaptationShader.setFloat(
            "deltaTimeSeconds",
            deltaTimeSeconds);


        glBindBufferBase(
            GL_SHADER_STORAGE_BUFFER,
            0,
            m_histogramBuffer);


        glBindTextureUnit(
            0,
            m_exposureTextures[
                previousIndex]);


        glBindImageTexture(
            0,
            m_exposureTextures[
                nextIndex],
            0,
            GL_FALSE,
            0,
            GL_WRITE_ONLY,
            GL_R32F);


        glDispatchCompute(
            1,
            1,
            1);


        glMemoryBarrier(
            GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
            GL_TEXTURE_FETCH_BARRIER_BIT);


        m_currentExposureIndex =
            nextIndex;
    }
}