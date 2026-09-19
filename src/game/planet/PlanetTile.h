#pragma once

#include <array>
#include <cstdint>
#include <vector>
#include <raylib.h>

namespace SpaceSim {
// Keys are scoped to one planet's cache. Dyadic coordinates preserve shared edges.
struct PlanetTileKey {
    int face = 0, level = 0, x = 0, y = 0;
    bool operator==(const PlanetTileKey& b) const {
        return face == b.face && level == b.level && x == b.x && y == b.y;
    }
    std::array<PlanetTileKey, 4> children() const;
};
struct PlanetTileHash {
    size_t operator()(const PlanetTileKey& k) const {
        return (static_cast<size_t>(k.face) << 40) ^ (static_cast<size_t>(k.level) << 32)
            ^ (static_cast<size_t>(k.x) << 16) ^ static_cast<size_t>(k.y);
    }
};
struct PlanetTileData {
    static constexpr int Cells = 16;
    Vector3 origin{}; // Planet-local unit-radius origin; vertices are relative to it.
    std::vector<float> positions, normals;
    std::vector<unsigned short> indices;
};

class PlanetSurfaceProvider {
public:
    explicit PlanetSurfaceProvider(int seed) : m_seed(seed) {}
    static Vector3 direction(int face, double u, double v);
    static Vector3 tileDirection(PlanetTileKey key, double x, double y);
    Vector3 position(Vector3 direction) const;
    PlanetTileData generate(PlanetTileKey key) const;
private:
    int m_seed;
};

class PlanetLodSelector {
public:
    static constexpr int MaxLevel = 10;
    static bool shouldSplit(PlanetTileKey key, Vector3 cameraInPlanetRadii,
        float focalLengthPixels, float targetErrorPixels = 2.0f);
};
}
