#include "renderer/opengl/GlShader.h"

#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
namespace SpaceSim
{
    void SpaceSim::GlShader::setFloat(
        const char *name,
        float value) const
    {
        const GLint location =
            glGetUniformLocation(
                m_program,
                name);

        glProgramUniform1f(
            m_program,
            location,
            value);
    }

    void SpaceSim::GlShader::setVec3(
        const char *name,
        const glm::vec3 &value) const
    {
        const GLint location =
            glGetUniformLocation(
                m_program,
                name);

        glProgramUniform3fv(
            m_program,
            location,
            1,
            glm::value_ptr(value));
    }

    void SpaceSim::GlShader::setMat4(
        const char *name,
        const glm::mat4 &value) const
    {
        const GLint location =
            glGetUniformLocation(
                m_program,
                name);

        glProgramUniformMatrix4fv(
            m_program,
            location,
            1,
            GL_FALSE,
            glm::value_ptr(value));
    }

    void GlShader::setInt(
        const char *name,
        int value) const
    {
        const GLint location =
            glGetUniformLocation(
                m_program,
                name);

        glProgramUniform1i(
            m_program,
            location,
            value);
    }
    namespace
    {
        std::string readTextFile(
            const std::filesystem::path &path)
        {
            std::ifstream file(path);

            if (!file.is_open())
            {
                throw std::runtime_error(
                    "Failed to open shader file: " +
                    path.string());
            }

            std::stringstream buffer;
            buffer << file.rdbuf();

            return buffer.str();
        }
    }

    GLuint GlShader::compileStage(
        GLenum type,
        const std::filesystem::path &path)
    {
        const std::string source =
            readTextFile(path);

        const char *sourcePtr =
            source.c_str();

        const GLuint shader =
            glCreateShader(type);

        glShaderSource(
            shader,
            1,
            &sourcePtr,
            nullptr);

        glCompileShader(shader);

        GLint success = GL_FALSE;

        glGetShaderiv(
            shader,
            GL_COMPILE_STATUS,
            &success);

        if (success != GL_TRUE)
        {
            GLint logLength = 0;

            glGetShaderiv(
                shader,
                GL_INFO_LOG_LENGTH,
                &logLength);

            std::vector<char> log(
                static_cast<std::size_t>(logLength));

            glGetShaderInfoLog(
                shader,
                logLength,
                nullptr,
                log.data());

            const std::string message =
                "Shader compilation failed: " +
                path.string() +
                "\n" +
                std::string(log.data());

            glDeleteShader(shader);

            throw std::runtime_error(message);
        }

        return shader;
    }

    GlShader::GlShader(
        const std::filesystem::path &vertexPath,
        const std::filesystem::path &fragmentPath)
    {
        const GLuint vertexShader =
            compileStage(
                GL_VERTEX_SHADER,
                vertexPath);

        const GLuint fragmentShader =
            compileStage(
                GL_FRAGMENT_SHADER,
                fragmentPath);

        m_program =
            glCreateProgram();

        glAttachShader(
            m_program,
            vertexShader);

        glAttachShader(
            m_program,
            fragmentShader);

        glLinkProgram(m_program);

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        GLint success = GL_FALSE;

        glGetProgramiv(
            m_program,
            GL_LINK_STATUS,
            &success);

        if (success != GL_TRUE)
        {
            GLint logLength = 0;

            glGetProgramiv(
                m_program,
                GL_INFO_LOG_LENGTH,
                &logLength);

            std::vector<char> log(
                static_cast<std::size_t>(logLength));

            glGetProgramInfoLog(
                m_program,
                logLength,
                nullptr,
                log.data());

            const std::string message =
                "Shader program linking failed:\n" +
                std::string(log.data());

            glDeleteProgram(m_program);
            m_program = 0;

            throw std::runtime_error(message);
        }
    }

    GlShader::~GlShader()
    {
        if (m_program != 0)
        {
            glDeleteProgram(m_program);
        }
    }

    void GlShader::use() const
    {
        glUseProgram(m_program);
    }
}