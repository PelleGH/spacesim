#include "platform/SdlGlWindow.h"
#include "integration/GameSimulation.h"
#include "integration/GameRenderBridge.h"
#include "integration/GameHud.h"
#include "input/GameInput.h"
#include "components/ShipFlightComponent.h"
#include "renderer/SceneRenderer.h"
#include "renderer/opengl/GpuMesh.h"
#include <cmath>
#include "renderlab/SphereGenerator.h"
#include "renderlab/BoxGenerator.h"
#include <SDL3/SDL.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>
using namespace SpaceSim;
namespace {
void require(bool condition,const char* message){if(!condition) throw std::runtime_error(message);}
void tap(GameSimulation& game,int key,float dt=.016f){GameInput::clear();GameInput::clear();GameInput::inject(key,true);game.update(dt);GameInput::clear();}
void checkSimulation(){
    GameSimulation game;auto& world=game.world;
    tap(game,KEY_W,1);require(world.registry.get<ShipFlightComponent>(world.playerShip).throttle>0,"Throttle failed");
    tap(game,KEY_Z);require(world.registry.get<ShipFlightComponent>(world.playerShip).throttle==0,"Stop failed");
    tap(game,KEY_TWO);require(world.registry.get<ShipFlightComponent>(world.playerShip).preset==ShipPreset::Medium,"Preset failed");
    tap(game,KEY_X);require(!world.registry.get<ShipFlightComponent>(world.playerShip).flightAssist,"Assist failed");
    const int initial=world.selectedJumpTarget;tap(game,KEY_TAB);require(world.selectedJumpTarget!=initial,"Target cycling failed");
    tap(game,KEY_J);require(world.travelMode==TravelMode::FTLTravel,"FTL start failed");
    tap(game,KEY_C);require(world.travelMode==TravelMode::NormalFlight,"FTL cancel failed");
    tap(game,KEY_J);
    for(int i=0;i<2000 && world.travelMode==TravelMode::FTLTravel;++i){GameInput::clear();game.update(.05f);}
    require(world.travelMode==TravelMode::NormalFlight && world.activePoi==world.selectedJumpTarget,"FTL arrival failed");
    tap(game,KEY_LEFT_SHIFT);require(world.orbitalCruise.active,"Cruise failed");
    GameInput::clear();GameInput::clear();
    std::cout<<"PASS: throttle, stop, presets, assist, target cycling, FTL start/cancel/arrival, cruise\n";
}
struct CachedAtmosphere {
    AtmosphereParameters parameters;
    AtmosphereLuts luts;
    explicit CachedAtmosphere(const GlobalObject& body):parameters(gameAtmosphere(body)),luts(parameters){}
};
void capture(int width,int height,const char* path){
    std::vector<unsigned char> pixels(size_t(width)*height*3);glPixelStorei(GL_PACK_ALIGNMENT,1);
    glReadPixels(0,0,width,height,GL_RGB,GL_UNSIGNED_BYTE,pixels.data());
    std::ofstream file(path,std::ios::binary);file<<"P6\n"<<width<<" "<<height<<"\n255\n";
    for(int y=height-1;y>=0;--y)file.write(reinterpret_cast<const char*>(pixels.data()+size_t(y)*width*3),width*3);
}
}
int main(int argc,char** argv){
    const bool smoke=argc>1 && std::string(argv[1])=="--smoke-test";
    try{
        SdlGlWindow window(1280,720,"SpaceSim Next");
        if(const char* base=SDL_GetBasePath())std::filesystem::current_path(base);
        if(smoke)checkSimulation();
        GameSimulation game;
        SceneRenderer renderer;renderer.setExposure(1);renderer.setBloomStrength(.045f);renderer.setBloomThreshold(8);
        auto sphereData=generateSphere(256,128);GpuMesh sphere(sphereData.vertices,sphereData.indices);
        auto boxData=generateBox(1,1,1);GpuMesh box(boxData.vertices,boxData.indices);
        GlTextureCube environmentMap(64);
        std::vector<float> neutral(64*64*4,.002f);
        for(int i=3;i<int(neutral.size());i+=4)neutral[i]=1;
        for(int face=0;face<6;++face)glTextureSubImage3D(environmentMap.id(),0,0,0,face,64,64,1,GL_RGBA,GL_FLOAT,neutral.data());
        EnvironmentIbl environmentIbl(environmentMap);EnvironmentLight environment;
        environment.diffuseMultiplier={1,1,1};environment.specularMultiplier={1,1,1};
        GameHud hud;std::map<int,std::unique_ptr<CachedAtmosphere>> atmospheres;
        bool captured=!smoke;window.captureMouse(captured);
        Uint64 last=SDL_GetTicksNS();int frames=0;
        while(window.processEvents()){
            Uint64 now=SDL_GetTicksNS();float dt=std::min(float(now-last)*1e-9f,.1f);last=now;
            GameInput::beginFrame(captured);
            if(GameInput::keyPressed(KEY_F1)){captured=!captured;window.captureMouse(captured);}
            if(GameInput::keyPressed(KEY_F11))window.toggleFullscreen();
            game.update(smoke?1.0f/60:dt);
            int width=window.pixelWidth(),height=window.pixelHeight();if(width<=0 || height<=0){SDL_Delay(20);continue;}
            auto frame=makeGameRenderFrame(game.world,sphere,box);AtmosphereInstance atmosphere;
            require(std::isfinite(frame.camera.forward.x) && std::isfinite(frame.camera.forward.y) && std::isfinite(frame.camera.forward.z),"Invalid camera direction");
            if(frame.atmosphereBody>=0){
                auto& cached=atmospheres[frame.atmosphereBody];
                if(!cached)cached=std::make_unique<CachedAtmosphere>(game.world.starSystem.objects[frame.atmosphereBody]);
                atmosphere.parameters=&cached->parameters;atmosphere.luts=&cached->luts;
                atmosphere.planetCenterWorld=frame.atmosphereCenter;atmosphere.planetRadiusWorld=frame.atmosphereRadius;
            }
            renderer.render(width,height,frame.camera,frame.sun,environment,environmentMap,environmentIbl,frame.planets,frame.objects,atmosphere.valid()?&atmosphere:nullptr);
            hud.draw(game.world,game.controls.getVirtualStick(),game.controls.getControlRadius(),width,height,captured);
            if(smoke && ++frames==24){capture(width,height,"integration-check.ppm");require(glGetError()==GL_NO_ERROR,"OpenGL error during integration rendering");std::cout<<"PASS: 24 rendered game frames, capture saved, OpenGL error 0\n";break;}
            window.swapBuffers();
        }
        return 0;
    }catch(const std::exception& error){std::cerr<<"SpaceSimNext: "<<error.what()<<'\n';return 1;}
}
