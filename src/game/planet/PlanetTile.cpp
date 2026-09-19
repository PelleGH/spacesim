#include "planet/PlanetTile.h"
#include "planet/PlanetSurfaceSampler.h"
#include <raymath.h>
#include <algorithm>
#include <cmath>

namespace SpaceSim {
std::array<PlanetTileKey, 4> PlanetTileKey::children() const {
    return {{{face,level+1,x*2,y*2}, {face,level+1,x*2+1,y*2},
        {face,level+1,x*2,y*2+1}, {face,level+1,x*2+1,y*2+1}}};
}
Vector3 PlanetSurfaceProvider::direction(int face, double u, double v) {
    double a=0,b=0,c=0;
    switch(face) {
    case 0: a=1; b=v; c=-u; break;
    case 1: a=-1; b=v; c=u; break;
    case 2: a=u; b=1; c=-v; break;
    case 3: a=u; b=-1; c=v; break;
    case 4: a=u; b=v; c=1; break;
    default: a=-u; b=v; c=-1; break;
    }
    const double length=std::sqrt(a*a+b*b+c*c);
    return {static_cast<float>(a/length),static_cast<float>(b/length),static_cast<float>(c/length)};
}
Vector3 PlanetSurfaceProvider::tileDirection(PlanetTileKey key, double x, double y) {
    const double width=2.0/static_cast<double>(1u << key.level);
    return direction(key.face,-1.0+(key.x+x)*width,-1.0+(key.y+y)*width);
}
Vector3 PlanetSurfaceProvider::position(Vector3 dir) const {
    const auto surface=PlanetSurfaceSampler::sample(dir,m_seed,PlanetClass::OceanWorld);
    return Vector3Scale(dir,1.0f+surface.terrainHeight/PlanetSurfaceSampler::TerrainUnitsPerRadius);
}
PlanetTileData PlanetSurfaceProvider::generate(PlanetTileKey key) const {
    PlanetTileData data;
    constexpr int cells=PlanetTileData::Cells, side=cells+1;
    data.origin=position(tileDirection(key,.5,.5));
    const auto append=[&](Vector3 p,Vector3 n) {
        const auto local=Vector3Subtract(p,data.origin);
        data.positions.insert(data.positions.end(),{local.x,local.y,local.z});
        data.normals.insert(data.normals.end(),{n.x,n.y,n.z});
    };
    // Fixed angular derivative makes normals identical across tile/LOD boundaries.
    const auto normalAt=[&](Vector3 dir) {
        const Vector3 axis=std::fabs(dir.y)<.95f ? Vector3{0,1,0}:Vector3{1,0,0};
        const auto right=Vector3Normalize(Vector3CrossProduct(axis,dir));
        const auto up=Vector3CrossProduct(dir,right);
        constexpr float epsilon=.0001f;
        const auto dx=Vector3Subtract(position(Vector3Normalize(Vector3Add(dir,Vector3Scale(right,epsilon)))),
            position(Vector3Normalize(Vector3Subtract(dir,Vector3Scale(right,epsilon)))));
        const auto dy=Vector3Subtract(position(Vector3Normalize(Vector3Add(dir,Vector3Scale(up,epsilon)))),
            position(Vector3Normalize(Vector3Subtract(dir,Vector3Scale(up,epsilon)))));
        return Vector3Normalize(Vector3CrossProduct(dx,dy));
    };
    for(int y=0;y<=cells;++y) for(int x=0;x<=cells;++x) {
        const auto dir=tileDirection(key,static_cast<double>(x)/cells,static_cast<double>(y)/cells);
        append(position(dir),normalAt(dir));
    }
    const auto triangle=[&](int a,int b,int c) {
        data.indices.insert(data.indices.end(),{static_cast<unsigned short>(a),
            static_cast<unsigned short>(b),static_cast<unsigned short>(c)});
    };
    for(int y=0;y<cells;++y) for(int x=0;x<cells;++x) {
        const int a=y*side+x;
        triangle(a,a+1,a+side+1); triangle(a,a+side+1,a+side);
    }
    // Inward skirts cover mixed-resolution edge gaps while parents await children.
    // This is an interim seam treatment; edge stitching and geomorphing come later.
    const float cellWidth=2.0f/static_cast<float>((1u << key.level)*cells);
    const float skirtDepth=std::max(.00002f,cellWidth*cellWidth*2.0f+.00015f);
    for(int edge=0;edge<4;++edge) {
        const int start=static_cast<int>(data.positions.size()/3);
        for(int i=0;i<=cells;++i) {
            const int index=edge==0?i:edge==1?i*side+cells:edge==2?cells*side+cells-i:(cells-i)*side;
            const Vector3 p=Vector3Add(data.origin,{data.positions[index*3],data.positions[index*3+1],data.positions[index*3+2]});
            const Vector3 n{data.normals[index*3],data.normals[index*3+1],data.normals[index*3+2]};
            append(Vector3Subtract(p,Vector3Scale(Vector3Normalize(p),skirtDepth)),n);
            if(i<cells) {
                const int next=edge==0?index+1:edge==1?index+side:edge==2?index-1:index-side;
                triangle(index,start+i,next); triangle(next,start+i,start+i+1);
            }
        }
    }
    return data;
}
bool PlanetLodSelector::shouldSplit(PlanetTileKey key,Vector3 camera,float focal,float target) {
    if(key.level>=MaxLevel) return false;
    const float width=2.0f/static_cast<float>(1u<<key.level);
    const Vector3 center=PlanetSurfaceProvider::tileDirection(key,.5,.5);
    const float distance=std::max(.0001f,Vector3Distance(camera,center)-width*.75f);
    const float cell=width/PlanetTileData::Cells;
    // Estimated curvature plus procedural-relief allowance, in planet radii.
    const float error=cell*cell*.5f+width*.00005f;
    return error*focal/distance>target;
}
}
