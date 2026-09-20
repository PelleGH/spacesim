#pragma once

#include "renderer/opengl/GlComputeShader.h"

#include <glad/gl.h>

#include <chrono>

namespace SpaceSim
{
    class AutoExposurePass
    {
    public:
        AutoExposurePass();

        ~AutoExposurePass();

        AutoExposurePass(
            const AutoExposurePass &) = delete;

        AutoExposurePass &operator=(
            const AutoExposurePass &) = delete;

        void update(
            GLuint hdrTexture,
            int width,
            int height);

        GLuint exposureTexture() const
        {
            return m_exposureTextures[m_currentExposureIndex];
        }

    private:
        static constexpr int HistogramBinCount =
            256;

        // Index 256 stores the number of pixels which actually
        // participated in exposure metering.
        static constexpr int HistogramValueCount =
            HistogramBinCount +
            1;

        GlComputeShader m_histogramShader;

        GlComputeShader m_adaptationShader;

        GLuint m_histogramBuffer =
            0;

        GLuint m_exposureTextures[2]{
            0,
            0};

        int m_currentExposureIndex =
            0;

        // =========================================================
        // METERING RANGE
        // =========================================================
        //
        // Histogram values are stored in log2 luminance.
        //
        // -12 ~= extremely dark
        //  +8 ~= extremely bright HDR

        float m_minLogLuminance =
            -12.0f;

        float m_maxLogLuminance =
            8.0f;

        // Ignore practically-black pixels.
        //
        // This is especially important in space where most of the
        // screen can legitimately be black.
        float m_meteringFloor =
            0.0001f;

        // Ignore the most extreme parts of the remaining histogram
        // when deciding exposure.
        float m_lowPercentile =
            0.05f;

        float m_highPercentile =
            0.95f;

        // Middle-gray target.
        float m_keyValue =
            0.10f;

        float m_minExposure =
            0.03f;

        float m_maxExposure =
            3.0f;

        // Going from bright -> dark:
        // exposure has to rise.
        //
        // Human eyes generally do this more slowly.
        float m_brighteningSpeed =
            1.25f;

        // Going from dark -> bright:
        // exposure has to fall.
        //
        // This is intentionally faster.
        float m_darkeningSpeed =
            3.0f;

        bool m_hasPreviousTime =
            false;

        std::chrono::steady_clock::time_point m_previousTime;
    };
}