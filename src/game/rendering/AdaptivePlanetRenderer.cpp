#include "rendering/AdaptivePlanetRenderer.h"
#include "rendering/DistantBodyRenderer.h"
#include "planet/PlanetSurfaceSampler.h"
#include <raymath.h>
#include <rlgl.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>

namespace SpaceSim {
AdaptivePlanetRenderer::~AdaptivePlanetRenderer() { reset(); }
void AdaptivePlanetRenderer::reset() {
    // Jobs own their provider, never this renderer or GPU resources.
    m_jobs.clear();
    for(auto& entry:m_cache) UnloadMesh(entry.second.mesh);
    m_cache.clear(); m_visible.clear(); m_requests.clear(); m_wanted.clear();
}
void AdaptivePlanetRenderer::upload(PlanetTileKey key,PlanetTileData data) {
    Mesh mesh{};
    mesh.vertexCount=static_cast<int>(data.positions.size()/3);
    mesh.triangleCount=static_cast<int>(data.indices.size()/3);
    mesh.vertices=static_cast<float*>(MemAlloc(static_cast<unsigned int>(data.positions.size()*sizeof(float))));
    mesh.normals=static_cast<float*>(MemAlloc(static_cast<unsigned int>(data.normals.size()*sizeof(float))));
    mesh.indices=static_cast<unsigned short*>(MemAlloc(static_cast<unsigned int>(data.indices.size()*sizeof(unsigned short))));
    std::memcpy(mesh.vertices,data.positions.data(),data.positions.size()*sizeof(float));
    std::memcpy(mesh.normals,data.normals.data(),data.normals.size()*sizeof(float));
    std::memcpy(mesh.indices,data.indices.data(),data.indices.size()*sizeof(unsigned short));
    UploadMesh(&mesh,false);
    m_cache.emplace(key,Resident{mesh,data.origin,m_frame});
}
bool AdaptivePlanetRenderer::request(PlanetTileKey key) {
    if(!m_wanted.insert(key).second) return m_cache.count(key)!=0;
    auto found=m_cache.find(key);
    if(found!=m_cache.end()) { found->second.used=m_frame; return true; }
    m_requests.push_back(key);
    return false;
}
void AdaptivePlanetRenderer::select(PlanetTileKey key,Vector3 camera,float focal) {
    // Preserve a bounded complete cut of the tree, even while approaching quickly.
    if(m_wanted.size()<MaxResident-8 && PlanetLodSelector::shouldSplit(key,camera,focal)) {
        const auto children=key.children();
        bool ready=true;
        for(auto child:children) ready=request(child)&&ready;
        if(ready) {
            for(auto child:children) select(child,camera,focal);
            return;
        }
    }
    m_visible.push_back(key);
    m_stats.coveredFaces += std::ldexp(1.0,-2*key.level);
    m_stats.deepest=std::max(m_stats.deepest,key.level);
}
bool AdaptivePlanetRenderer::update(const GlobalObject& planet,Vector3 center,float radius,
    const Camera3D& camera,int viewportHeight) {
    if(!planet.hasPlanetData || planet.planetData.planetClass!=PlanetClass::OceanWorld || radius<=0) return false;
    const int seed=planet.planetData.seed==0?PlanetSurfaceSampler::seedFromId(planet.id):static_cast<int>(planet.planetData.seed);
    if(m_planet!=planet.id || m_seed!=seed) { reset(); m_planet=planet.id; m_seed=seed; }
    ++m_frame; m_stats={}; m_visible.clear(); m_requests.clear(); m_wanted.clear();
    for(auto it=m_jobs.begin();it!=m_jobs.end() && m_stats.uploaded<MaxUploads;) {
        if(it->result.wait_for(std::chrono::seconds(0))!=std::future_status::ready) { ++it; continue; }
        auto data=it->result.get();
        upload(it->key,std::move(data)); ++m_stats.uploaded;
        it=m_jobs.erase(it);
    }
    bool rootsReady=true;
    for(int face=0;face<6;++face) rootsReady=request({face,0,0,0})&&rootsReady;
    if(rootsReady) {
        const auto localCamera=Vector3Scale(Vector3Subtract(camera.position,center),1.0f/radius);
        const float focal=std::max(viewportHeight,1)*.5f/std::tan(camera.fovy*DEG2RAD*.5f);
        for(int face=0;face<6;++face) select({face,0,0,0},localCamera,focal);
    }
    // Keep ancestors and selected/requested tiles. Evict least-recently-used extras.
    while(m_cache.size()+m_jobs.size()>=MaxResident && !m_cache.empty()) {
        auto oldest=m_cache.end();
        for(auto it=m_cache.begin();it!=m_cache.end();++it)
            if(!m_wanted.count(it->first) && (oldest==m_cache.end() || it->second.used<oldest->second.used)) oldest=it;
        if(oldest==m_cache.end()) break;
        UnloadMesh(oldest->second.mesh); m_cache.erase(oldest);
    }
    // Breadth-first requests stop a nearby face monopolizing refinement jobs.
    std::stable_sort(m_requests.begin(),m_requests.end(),[](auto a,auto b){return a.level<b.level;});
    for(auto key:m_requests) {
        if(m_jobs.size()>=MaxJobs || m_cache.size()+m_jobs.size()>=MaxResident) break;
        if(std::any_of(m_jobs.begin(),m_jobs.end(),[&](const Job& job){return job.key==key;})) continue;
        m_jobs.push_back({key,std::async(std::launch::async,[seed,key]{return PlanetSurfaceProvider(seed).generate(key);})});
    }
    m_stats.resident=static_cast<int>(m_cache.size());
    m_stats.visible=static_cast<int>(m_visible.size());
    m_stats.pending=static_cast<int>(m_jobs.size());
    return rootsReady;
}
void AdaptivePlanetRenderer::draw(const GlobalObject& planet,const SceneLighting& lighting,Vector3 center,float radius,Vector3 camera,
    int lightingDebugMode) const {
    MaterialMap maps[12]{};
    maps[MATERIAL_MAP_ALBEDO].texture.id=rlGetTextureIdDefault();
    maps[MATERIAL_MAP_ALBEDO].color=WHITE;
    Material material{}; material.maps=maps;
    material.shader=DistantBodyRenderer::oceanSurfaceShader(planet,lighting,center,radius,camera,lightingDebugMode);
    for(auto key:m_visible) {
        const auto& tile=m_cache.at(key);
        const auto origin=Vector3Add(center,Vector3Scale(tile.origin,radius));
        const auto transform=MatrixMultiply(MatrixScale(radius,radius,radius),MatrixTranslate(origin.x,origin.y,origin.z));
        DrawMesh(tile.mesh,material,transform);
    }
}
}
