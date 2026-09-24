cmake_minimum_required(VERSION 3.24)

project(SpaceSim LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SDL_DISABLE_INSTALL ON CACHE BOOL "" FORCE)

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.4.16
    GIT_SHALLOW TRUE
)

FetchContent_Declare(
    glad
    GIT_REPOSITORY https://github.com/Dav1dde/glad.git
    GIT_TAG v2.0.8
    GIT_SHALLOW TRUE
    SOURCE_SUBDIR cmake
)

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
    GIT_SHALLOW TRUE
)

FetchContent_Declare(
    EnTT
    GIT_REPOSITORY https://github.com/skypjack/entt.git
    GIT_TAG v3.15.0
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(SDL3 glad glm EnTT)

glad_add_library(
    glad_gl_core_45
    STATIC
    REPRODUCIBLE
    API gl:core=4.5
)

# ---------------------------------------------------------------------------
# Shared modern platform layer
# ---------------------------------------------------------------------------

add_library(SpaceSimPlatform STATIC
    src/engine/platform/SdlGlWindow.cpp
    src/engine/input/SdlInput.cpp
    src/engine/core/GameClock.cpp
)

target_include_directories(SpaceSimPlatform PUBLIC
    src
    src/engine
)

target_link_libraries(SpaceSimPlatform PUBLIC
    SDL3::SDL3
    glad_gl_core_45
    glm::glm
)

# ---------------------------------------------------------------------------
# Shared modern renderer
#
# RendererLab and the actual game both link this target. RendererLab remains a
# renderer regression/test harness; it is no longer the application's owner.
# ---------------------------------------------------------------------------

add_library(SpaceSimRenderer STATIC
    src/engine/geometry/SphereGenerator.cpp

    src/engine/renderer/RenderCamera.cpp
    src/engine/renderer/SceneRenderer.cpp
    src/engine/renderer/EnvironmentPass.cpp
    src/engine/renderer/BloomPass.cpp
    src/engine/renderer/StarPass.cpp
    src/engine/renderer/AutoExposurePass.cpp
    src/engine/renderer/TestEnvironment.cpp

    src/engine/renderer/opengl/GlShader.cpp
    src/engine/renderer/opengl/GpuMesh.cpp
    src/engine/renderer/opengl/HdrRenderTarget.cpp
    src/engine/renderer/opengl/PostProcessPass.cpp
    src/engine/renderer/opengl/ShadowMap.cpp
    src/engine/renderer/opengl/GlTextureCube.cpp

    src/engine/renderer/atmosphere/AtmospherePass.cpp
    src/engine/renderer/atmosphere/AtmosphereViewLuts.cpp

    src/engine/renderer/planet/PlanetPass.cpp
    src/engine/renderer/planet/AdaptivePlanetSurfaceRenderer.cpp
    src/engine/renderer/cloud/CloudPass.cpp
    src/engine/renderer/cloud/CloudNoiseVolume.cpp
    src/engine/renderer/cloud/CloudWeatherMap.cpp
)

target_include_directories(SpaceSimRenderer PUBLIC
    src
    src/engine
)

target_link_libraries(SpaceSimRenderer PUBLIC
    glad_gl_core_45
    glm::glm
)

# ---------------------------------------------------------------------------
# Actual game executable
#
# This is now SDL3/OpenGL + EnTT. The old Raylib application/integration code is
# intentionally left in the repository as reference, but it is not part of the
# normal build anymore.
# ---------------------------------------------------------------------------

add_executable(SpaceSim
    src/main.cpp

    src/game/runtime/GameApplication.cpp
    src/game/bootstrap/PrototypeWorld.cpp
    src/game/rendering/GameRenderResources.cpp
    src/game/rendering/RenderSystem.cpp
    src/game/systems/PlayerControlSystem.cpp
    src/game/systems/ShipMovementSystem.cpp
    src/game/systems/LocalBubbleRebaseSystem.cpp
    src/game/systems/HyperdriveSystem.cpp
    src/game/systems/ChaseCameraSystem.cpp
)

target_include_directories(SpaceSim PRIVATE
    src
    src/engine
)

target_link_libraries(SpaceSim PRIVATE
    SpaceSimPlatform
    SpaceSimRenderer
    EnTT::EnTT
)

# ---------------------------------------------------------------------------
# Renderer regression/test executable
# ---------------------------------------------------------------------------

add_executable(RendererLab
    src/renderlab/main.cpp
    src/renderlab/BoxGenerator.cpp
)

target_include_directories(RendererLab PRIVATE
    src
    src/engine
)

target_link_libraries(RendererLab PRIVATE
    SpaceSimPlatform
    SpaceSimRenderer
)

foreach(runtime_target SpaceSim RendererLab)
    add_custom_command(TARGET ${runtime_target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/data"
            "$<TARGET_FILE_DIR:${runtime_target}>/data"
        COMMENT "Copying game data beside ${runtime_target}"
    )
endforeach()

if(WIN32 AND CMAKE_CROSSCOMPILING)
    target_link_options(SpaceSim PRIVATE -static)
    target_link_options(RendererLab PRIVATE -static)
endif()

install(TARGETS SpaceSim
    RUNTIME DESTINATION .
    COMPONENT SpaceSimRuntime
)

install(DIRECTORY "${CMAKE_SOURCE_DIR}/data"
    DESTINATION .
    COMPONENT SpaceSimRuntime
)
