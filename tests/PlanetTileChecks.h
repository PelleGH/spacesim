#pragma once
#include "planet/PlanetTile.h"
#include <raymath.h>
#include <cmath>
#include <cstdio>

inline bool CheckPlanetTiles(int seed) {
    using namespace SpaceSim;
    PlanetSurfaceProvider provider(seed);
    // Every cube-face edge sample must exist on another face, including corners.
    for(int face=0;face<6;++face) for(int edge=0;edge<4;++edge) for(int i=0;i<=16;++i) {
        const float t=-1.0f+i/8.0f;
        const auto a=PlanetSurfaceProvider::direction(face,edge<2?(edge==0?-1:1):t,edge<2?t:(edge==2?-1:1));
        bool match=false;
        for(int other=0;other<6;++other) if(other!=face)
            for(int e=0;e<4;++e) for(int j=0;j<=16;++j) {
                const float s=-1.0f+j/8.0f;
                const auto b=PlanetSurfaceProvider::direction(other,e<2?(e==0?-1:1):s,e<2?s:(e==2?-1:1));
                match=match || Vector3DistanceSqr(a,b)<1e-12f;
            }
        if(!match) return false;
    }
    for(int face=0;face<6;++face) {
        const PlanetTileKey key{face,2,1,2};
        const auto children=key.children();
        for(int i=0;i<=16;++i) {
            const auto a=provider.tileDirection(children[0],1,i/16.0);
            const auto b=provider.tileDirection(children[1],0,i/16.0);
            const auto parent=provider.tileDirection(key,.5,i/32.0);
            if(Vector3DistanceSqr(a,b)>1e-12f || Vector3DistanceSqr(a,parent)>1e-12f) return false;
        }
        const auto tile=provider.generate(key);
        for(auto index:tile.indices) if(index>=tile.positions.size()/3) return false;
        for(int y=0;y<=16;++y) for(int x=0;x<=16;++x) {
            const int n=(y*17+x)*3;
            const auto p=Vector3Add(tile.origin,{tile.positions[n],tile.positions[n+1],tile.positions[n+2]});
            const auto expected=provider.position(provider.tileDirection(key,x/16.0,y/16.0));
            const Vector3 normal{tile.normals[n],tile.normals[n+1],tile.normals[n+2]};
            if(Vector3Distance(p,expected)>2e-7f || !std::isfinite(Vector3Length(normal)) || Vector3DotProduct(normal,p)<.5f) return false;
        }
        for(int t=0;t<16*16*6;t+=3) {
            const auto vertex=[&](int i) {const int n=tile.indices[i]*3; return Vector3{tile.positions[n],tile.positions[n+1],tile.positions[n+2]};};
            const auto a=vertex(t),b=vertex(t+1),c=vertex(t+2);
            if(Vector3DotProduct(Vector3CrossProduct(Vector3Subtract(b,a),Vector3Subtract(c,a)),Vector3Add(tile.origin,a))<=0) return false;
        }
    }
    const PlanetTileKey key{4,3,4,4};
    const auto direction=provider.tileDirection(key,.5,.5);
    if(!PlanetLodSelector::shouldSplit(key,Vector3Scale(direction,1.001f),600) ||
        PlanetLodSelector::shouldSplit(key,Vector3Scale(direction,5.0f),600)) return false;
    std::puts("Adaptive tile face seams, child continuity, surface agreement, normals, winding and LOD checks passed");
    return true;
}
