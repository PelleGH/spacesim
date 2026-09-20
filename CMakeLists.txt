cmake_minimum_required(VERSION 3.24)

project(SpaceSim LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

# ---------------------------------------------------------------------------
# New renderer backend dependencies
# ---------------------------------------------------------------------------

set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SDL_DISABLE_INSTALL ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(ASSIMP_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)
set(ASSIMP_NO_EXPORT ON CACHE BOOL "" FORCE)
FetchContent_Declare(
    assimp
    GIT_REPOSITORY https://github.com/assimp/assimp.git
    GIT_TAG v6.0.5
    GIT_SHALLOW TRUE
)
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
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5
    GIT_SHALLOW TRUE
)

FetchContent_Declare(
    EnTT
    GIT_REPOSITORY https://github.com/skypjack/entt.git
    GIT_TAG v3.15.0
    GIT_SHALLOW TRUE
)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.12.0
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(
    raylib
    EnTT
    nlohmann_json
    SDL3
    glad
    glm
    assimp
)
glad_add_library(
    glad_gl_core_45
    STATIC
    REPRODUCIBLE
    API gl:core=4.5
)
add_executable(SpaceSim
    src/main.cpp

    src/engine/core/Application.cpp
    src/engine/core/Application.h

    src/engine/renderer/Renderer.cpp
    src/engine/renderer/Renderer.h

    src/game/world/GameWorld.h
    src/game/lighting/SceneLighting.h

    src/game/components/TransformComponent.h
    src/game/components/ShipFlightComponent.h
    src/game/components/PlayerControlledComponent.h

    src/game/systems/ShipControlSystem.cpp
    src/game/systems/ShipControlSystem.h

    src/game/systems/CameraSystem.cpp
    src/game/systems/CameraSystem.h

    src/game/systems/RenderSystem.cpp
    src/game/systems/RenderSystem.h
    src/game/systems/BubbleSystem.cpp
    src/game/systems/BubbleSystem.h

    src/game/world/StarSystem.h

    src/game/systems/StarSystemSystem.cpp
    src/game/systems/StarSystemSystem.h

    src/game/systems/FTLSystem.cpp
    src/game/systems/FTLSystem.h

    src/game/systems/PlayerGlobalPositionSystem.cpp
    src/game/systems/PlayerGlobalPositionSystem.h
    src/game/systems/PlanetProximitySystem.cpp
    src/game/systems/PlanetProximitySystem.h
    src/game/systems/OrbitalCruiseSystem.cpp
    src/game/systems/OrbitalCruiseSystem.h
    src/game/systems/AtmosphereSystem.cpp
    src/game/systems/AtmosphereSystem.h
    src/game/systems/LightingSystem.cpp
    src/game/systems/LightingSystem.h

    src/game/world/StarSystemLoader.cpp
    src/game/world/StarSystemLoader.h
    src/game/world/PlanetVisualGenerator.cpp
    src/game/world/PlanetTerrainGenerator.cpp

    src/game/planet/PlanetSurfaceSampler.cpp
    src/game/planet/PlanetSurfaceSampler.h
    

    src/game/rendering/SpaceBackgroundRenderer.cpp
    src/game/rendering/DistantBodyRenderer.cpp
    src/game/rendering/PrototypeMeshRenderer.cpp
    src/game/rendering/PlanetMeshGenerator.cpp
    src/game/rendering/PlanetCloudGenerator.cpp
    src/game/rendering/PlanetSurfacePatch.cpp
    src/game/rendering/PlanetSurfacePatch.h
)

target_include_directories(SpaceSim PRIVATE
    src
    src/engine
    src/game
)

target_link_libraries(SpaceSim PRIVATE
    raylib
    EnTT::EnTT
    nlohmann_json::nlohmann_json
)
# ---------------------------------------------------------------------------
# Experimental SDL3/OpenGL renderer.
#
# This runs beside the existing Raylib game while the renderer is rebuilt.
# ---------------------------------------------------------------------------

add_executable(RendererLab
    src/renderlab/main.cpp

    src/renderlab/SphereGenerator.cpp
    src/renderlab/SphereGenerator.h

    src/engine/platform/SdlGlWindow.cpp
    src/engine/platform/SdlGlWindow.h

    src/engine/renderer/opengl/GlShader.cpp
    src/engine/renderer/opengl/GlShader.h

    src/engine/renderer/opengl/GpuMesh.cpp
    src/engine/renderer/opengl/GpuMesh.h

    src/engine/renderer/opengl/HdrRenderTarget.cpp
    src/engine/renderer/opengl/HdrRenderTarget.h

    src/engine/renderer/opengl/PostProcessPass.cpp
    src/engine/renderer/opengl/PostProcessPass.h

    src/engine/renderer/RenderCamera.cpp
    src/engine/renderer/RenderCamera.h

    src/engine/renderer/RenderObject.h

    src/engine/renderer/SceneRenderer.cpp
    src/engine/renderer/SceneRenderer.h

    src/engine/renderer/lighting/DirectionalLight.h

    src/engine/renderer/material/PbrMaterial.h

    src/engine/assets/ModelData.h
    src/engine/assets/ModelLoader.cpp
    src/engine/assets/ModelLoader.h

    src/engine/renderer/MeshVertex.h

    src/renderlab/BoxGenerator.cpp
    src/renderlab/BoxGenerator.h

    src/engine/renderer/opengl/ShadowMap.cpp
    src/engine/renderer/opengl/ShadowMap.h

    src/renderlab/TestEnvironment.cpp
    src/renderlab/TestEnvironment.h

    src/engine/renderer/opengl/GlTextureCube.cpp
    src/engine/renderer/opengl/GlTextureCube.h

    src/engine/renderer/EnvironmentPass.cpp
    src/engine/renderer/EnvironmentPass.h

    src/engine/renderer/opengl/GlTextureCube.cpp
    src/engine/renderer/opengl/GlTextureCube.h

    src/engine/renderer/BloomPass.cpp
    src/engine/renderer/BloomPass.h

    src/engine/renderer/atmosphere/AtmosphereInstance.h
    src/engine/renderer/atmosphere/AtmospherePass.cpp
    src/engine/renderer/atmosphere/AtmospherePass.h

    src/engine/renderer/atmosphere/AtmosphereViewLuts.cpp
    src/engine/renderer/atmosphere/AtmosphereViewLuts.h

    src/engine/renderer/StarPass.cpp
    src/engine/renderer/StarPass.h

    src/engine/renderer/AutoExposurePass.cpp
    src/engine/renderer/AutoExposurePass.h
)

target_include_directories(RendererLab PRIVATE
    src
    src/engine
)

target_link_libraries(RendererLab PRIVATE
    SDL3::SDL3
    glad_gl_core_45
    glm::glm
    assimp::assimp
)
# Make each build runnable directly from Explorer, like the installed package.
add_custom_command(TARGET SpaceSim POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/data"
        "$<TARGET_FILE_DIR:SpaceSim>/data"
    COMMENT "Copying game data beside SpaceSim"
)
add_custom_command(TARGET RendererLab POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/data"
        "$<TARGET_FILE_DIR:RendererLab>/data"
    COMMENT "Copying renderer data beside RendererLab"
)
# Keep the Windows build portable.
if(WIN32 AND CMAKE_CROSSCOMPILING)
    target_link_options(SpaceSim PRIVATE -static)
    target_link_options(RendererLab PRIVATE -static)
endif()

# Package executable + game data.
install(TARGETS SpaceSim
    RUNTIME DESTINATION .
    COMPONENT SpaceSimRuntime
)

install(DIRECTORY "${CMAKE_SOURCE_DIR}/data"
    DESTINATION .
    COMPONENT SpaceSimRuntime
)

# Explicit diagnostic target: does not add work to ordinary game builds.
add_executable(OceanSurfaceCheck EXCLUDE_FROM_ALL
    tests/OceanSurfaceCheck.cpp
    src/game/world/StarSystemLoader.cpp
    src/game/world/PlanetVisualGenerator.cpp
    src/game/world/PlanetTerrainGenerator.cpp
    src/game/planet/PlanetSurfaceSampler.cpp
    src/game/rendering/DistantBodyRenderer.cpp
    src/game/rendering/PlanetMeshGenerator.cpp
    src/game/rendering/PlanetCloudGenerator.cpp
    src/game/rendering/PlanetSurfacePatch.cpp
)
target_include_directories(OceanSurfaceCheck PRIVATE src src/engine src/game)
target_link_libraries(OceanSurfaceCheck PRIVATE raylib EnTT::EnTT nlohmann_json::nlohmann_json)

foreach(planet_target SpaceSim OceanSurfaceCheck)
    target_sources(${planet_target} PRIVATE
        src/game/planet/PlanetTile.cpp
        src/game/rendering/AdaptivePlanetRenderer.cpp)
endforeach()
