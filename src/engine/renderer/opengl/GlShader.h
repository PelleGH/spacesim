#pragma once

#include <glad/gl.h>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <filesystem>

namespace SpaceSim
{
    class GlShader
    {
    public:
        GlShader(
            const std::filesystem::path& vertexPath,
            const std::filesystem::path& fragmentPath);

        ~GlShader();

        GlShader(const GlShader&) = delete;
        GlShader& operator=(const GlShader&) = delete;

        void use() const;

        void setFloat(
            const char* name,
            float value) const;

        void setVec3(
            const char* name,
            const glm::vec3& value) const;

        void setMat4(
            const char* name,
            const glm::mat4& value) const;

        GLuint id() const
        {
            return m_program;
        }

    private:
        static GLuint compileStage(
            GLenum type,
            const std::filesystem::path& path);

        GLuint m_program = 0;
    };
}