#pragma once
#include "renderer/opengl/GlShader.h"
#include "world/GameWorld.h"
#include <vector>
namespace SpaceSim {
class GameHud {
public:
    GameHud();~GameHud();
    void draw(const GameWorld& world,Vector2 stick,float stickRadius,int width,int height,bool captured);
private:
    void text(const char* message,float x,float y);
    void rect(float x,float y,float width,float height);
    GlShader shader;
    GLuint vao=0,vbo=0;
    std::vector<float> vertices;
    float screenWidth=1,screenHeight=1;
};
}
