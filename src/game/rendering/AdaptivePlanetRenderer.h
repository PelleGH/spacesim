#pragma once
#include "planet/PlanetTile.h"
#include "world/StarSystem.h"
#include "lighting/SceneLighting.h"
#include <future>
#include <unordered_map>
#include <unordered_set>
#include <string>

namespace SpaceSim {
class AdaptivePlanetRenderer {
public:
    struct Statistics {
        int resident=0, visible=0, pending=0, deepest=0, uploaded=0;
        double coveredFaces=0; // A complete tree cut covers exactly six cube faces.
    };
    AdaptivePlanetRenderer()=default;
    ~AdaptivePlanetRenderer();
    AdaptivePlanetRenderer(const AdaptivePlanetRenderer&)=delete;
    AdaptivePlanetRenderer& operator=(const AdaptivePlanetRenderer&)=delete;
    // False means roots are loading; caller must keep the legacy globe visible.
    bool update(const GlobalObject& planet,Vector3 center,float radius,const Camera3D& camera,int viewportHeight);
    void draw(const GlobalObject& planet,const SceneLighting& lighting,Vector3 center,float radius,Vector3 camera,
        int lightingDebugMode = 2) const;
    Statistics statistics() const { return m_stats; }
private:
    struct Resident { Mesh mesh{}; Vector3 origin{}; unsigned long long used=0; };
    struct Job { PlanetTileKey key; std::future<PlanetTileData> result; };
    static constexpr int MaxResident=512, MaxJobs=2, MaxUploads=2;
    void reset();
    void upload(PlanetTileKey key,PlanetTileData data);
    bool request(PlanetTileKey key);
    void select(PlanetTileKey key,Vector3 camera,float focal);
    std::unordered_map<PlanetTileKey,Resident,PlanetTileHash> m_cache;
    std::unordered_set<PlanetTileKey,PlanetTileHash> m_wanted;
    std::vector<PlanetTileKey> m_requests,m_visible;
    std::vector<Job> m_jobs;
    std::string m_planet;
    int m_seed=0;
    unsigned long long m_frame=0;
    Statistics m_stats;
};
}
