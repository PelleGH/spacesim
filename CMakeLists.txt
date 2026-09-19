cmake_minimum_required(VERSION 3.24)

project(SpaceSim LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

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

# Make each build runnable directly from Explorer, like the installed package.
add_custom_command(TARGET SpaceSim POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/data"
        "$<TARGET_FILE_DIR:SpaceSim>/data"
    COMMENT "Copying game data beside SpaceSim"
)

# Keep the Windows build portable.
if(WIN32 AND CMAKE_CROSSCOMPILING)
    target_link_options(SpaceSim PRIVATE -static)
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
