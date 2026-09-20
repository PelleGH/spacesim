#pragma once

#include <glad/gl.h>

#include <glm/vec3.hpp>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace SpaceSim
{
    class GlComputeShader
    {
    public:
        explicit GlComputeShader(
            const std::filesystem::path& path)
        {
            const std::string source =
                readTextFile(
                    path);

            const char* sourcePointer =
                source.c_str();


            const GLuint shader =
                glCreateShader(
                    GL_COMPUTE_SHADER);


            glShaderSource(
                shader,
                1,
                &sourcePointer,
                nullptr);


            glCompileShader(
                shader);


            GLint compileSuccess =
                GL_FALSE;


            glGetShaderiv(
                shader,
                GL_COMPILE_STATUS,
                &compileSuccess);


            if (compileSuccess != GL_TRUE)
            {
                GLint logLength =
                    0;


                glGetShaderiv(
                    shader,
                    GL_INFO_LOG_LENGTH,
                    &logLength);


                std::vector<char> log(
                    static_cast<std::size_t>(
                        logLength > 0
                            ? logLength
                            : 1));


                glGetShaderInfoLog(
                    shader,
                    logLength,
                    nullptr,
                    log.data());


                const std::string message =
                    "Compute shader compilation failed: " +
                    path.string() +
                    "\n" +
                    std::string(
                        log.data());


                glDeleteShader(
                    shader);


                throw std::runtime_error(
                    message);
            }


            m_program =
                glCreateProgram();


            glAttachShader(
                m_program,
                shader);


            glLinkProgram(
                m_program);


            glDeleteShader(
                shader);


            GLint linkSuccess =
                GL_FALSE;


            glGetProgramiv(
                m_program,
                GL_LINK_STATUS,
                &linkSuccess);


            if (linkSuccess != GL_TRUE)
            {
                GLint logLength =
                    0;


                glGetProgramiv(
                    m_program,
                    GL_INFO_LOG_LENGTH,
                    &logLength);


                std::vector<char> log(
                    static_cast<std::size_t>(
                        logLength > 0
                            ? logLength
                            : 1));


                glGetProgramInfoLog(
                    m_program,
                    logLength,
                    nullptr,
                    log.data());


                const std::string message =
                    "Compute shader linking failed:\n" +
                    std::string(
                        log.data());


                glDeleteProgram(
                    m_program);


                m_program =
                    0;


                throw std::runtime_error(
                    message);
            }
        }


        ~GlComputeShader()
        {
            if (m_program != 0)
            {
                glDeleteProgram(
                    m_program);
            }
        }


        GlComputeShader(
            const GlComputeShader&) = delete;


        GlComputeShader& operator=(
            const GlComputeShader&) = delete;


        void use() const
        {
            glUseProgram(
                m_program);
        }


        void setInt(
            const char* name,
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


        void setFloat(
            const char* name,
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


        void setVec3(
            const char* name,
            const glm::vec3& value) const
        {
            const GLint location =
                glGetUniformLocation(
                    m_program,
                    name);


            glProgramUniform3f(
                m_program,
                location,
                value.x,
                value.y,
                value.z);
        }


        GLuint id() const
        {
            return m_program;
        }


    private:
        static std::string readTextFile(
            const std::filesystem::path& path)
        {
            std::ifstream file(
                path);


            if (!file.is_open())
            {
                throw std::runtime_error(
                    "Failed to open compute shader: " +
                    path.string());
            }


            std::stringstream buffer;


            buffer <<
                file.rdbuf();


            return buffer.str();
        }


        GLuint m_program =
            0;
    };
}