#include "integration/GameHud.h"
#include "components/ShipFlightComponent.h"
#include <array>
#include <cctype>
#include <cstdio>
#include <cmath>
namespace SpaceSim {
namespace {
const char* chars="ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-:/[]+%";
const unsigned char font[][5]={
{126,17,17,17,126},{127,73,73,73,54},{62,65,65,65,34},{127,65,65,34,28},{127,73,73,73,65},{127,9,9,9,1},
{62,65,73,73,122},{127,8,8,8,127},{0,65,127,65,0},{32,64,65,63,1},{127,8,20,34,65},{127,64,64,64,64},
{127,2,12,2,127},{127,4,8,16,127},{62,65,65,65,62},{127,9,9,9,6},{62,65,81,33,94},{127,9,25,41,70},
{70,73,73,73,49},{1,1,127,1,1},{63,64,64,64,63},{31,32,64,32,31},{63,64,56,64,63},{99,20,8,20,99},
{7,8,112,8,7},{97,81,73,69,67},{62,81,73,69,62},{0,66,127,64,0},{66,97,81,73,70},{33,65,69,75,49},
{24,20,18,127,16},{39,69,69,69,57},{60,74,73,73,48},{1,113,9,5,3},{54,73,73,73,54},{6,73,73,41,30},
{0,96,96,0,0},{8,8,8,8,8},{0,54,54,0,0},{32,16,8,4,2},{0,127,65,65,0},{0,65,65,127,0},{8,8,62,8,8},{99,19,8,100,99}};
}
GameHud::GameHud():shader("data/shaders/renderer/game_hud.vert","data/shaders/renderer/game_hud.frag") {
    glGenVertexArrays(1,&vao);glGenBuffers(1,&vbo);glBindVertexArray(vao);glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glEnableVertexAttribArray(0);glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,2*sizeof(float),nullptr);glBindVertexArray(0);
}
GameHud::~GameHud(){glDeleteBuffers(1,&vbo);glDeleteVertexArrays(1,&vao);}
void GameHud::rect(float x,float y,float w,float h) {
    float a=2*x/screenWidth-1,b=1-2*y/screenHeight,c=2*(x+w)/screenWidth-1,d=1-2*(y+h)/screenHeight;
    vertices.insert(vertices.end(),{a,b,c,b,c,d,a,b,c,d,a,d});
}
void GameHud::text(const char* s,float x,float y) {
    for(;*s;++s,x+=12) {int index=0;char c=char(std::toupper(static_cast<unsigned char>(*s)));while(chars[index] && chars[index]!=c) ++index;
        if(!chars[index]) continue;for(int col=0;col<5;++col) for(int row=0;row<7;++row) if(font[index][col]&(1<<row)) rect(x+col*2,y+row*2,2,2);
    }
}
void GameHud::draw(const GameWorld& world,Vector2 stick,float radius,int width,int height,bool captured) {
    screenWidth=float(width);screenHeight=float(height);vertices.clear();
    char line[200];const auto& flight=world.registry.get<ShipFlightComponent>(world.playerShip);
    text("SPACESIM / SDL OPENGL INTEGRATION",16,16);
    text("W/S THROTTLE  Z STOP  A/D STRAFE  SPACE/CTRL VERTICAL",16,38);
    text("Q/E ROLL  SHIFT CRUISE  TAB TARGET  J FTL  C CANCEL",16,60);
    text("1/2/3 SHIP  X ASSIST  T THRUST MODE  R CENTER STICK",16,82);
    text("F11 FULLSCREEN  F1 MOUSE CAPTURE  ESC EXIT",16,104);
    std::snprintf(line,sizeof(line),"SPEED %.1f / %.1f U/S  THROTTLE %.0f%%",std::sqrt(flight.velocity.x*flight.velocity.x+flight.velocity.y*flight.velocity.y+flight.velocity.z*flight.velocity.z),flight.maxSpeed,flight.throttle*100);text(line,16,138);
    std::snprintf(line,sizeof(line),"ASSIST %s  MOUSE %s  TRAVEL %s",flight.flightAssist?"ON":"OFF",captured?"CAPTURED":"FREE",world.travelMode==TravelMode::FTLTravel?"FTL":"NORMAL");text(line,16,160);
    if(world.selectedJumpTarget>=0) {const auto& target=world.starSystem.objects[world.selectedJumpTarget];std::snprintf(line,sizeof(line),"TARGET %s  DISTANCE %.0f U",target.name.c_str(),Length(target.position-world.globalPlayerPosition));text(line,16,182);}
    if(world.planetTransition.closestPlanetIndex>=0) {const auto& p=world.starSystem.objects[world.planetTransition.closestPlanetIndex];std::snprintf(line,sizeof(line),"PLANET %s  ALTITUDE %.0f U",p.name.c_str(),world.planetTransition.altitude);text(line,16,204);}
    std::snprintf(line,sizeof(line),"CRUISE %s %.0f U/S  ATMOSPHERE %s",world.orbitalCruise.active?"ON":"OFF",world.orbitalCruise.currentSpeed,world.atmosphere.insideAtmosphere?"INSIDE":"OUTSIDE");text(line,16,226);
    const float cx=width*.5f,cy=height*.5f;rect(cx-3,cy-1,6,2);rect(cx-1,cy-3,2,6);
    for(int i=0;i<80;++i) {float a=i*6.2831853f/80;rect(cx+std::cos(a)*radius,cy+std::sin(a)*radius,1,1);}
    rect(cx+stick.x-2,cy+stick.y-2,4,4);
    glDisable(GL_DEPTH_TEST);glDisable(GL_CULL_FACE);glDisable(GL_BLEND);shader.use();
    glBindVertexArray(vao);glBindBuffer(GL_ARRAY_BUFFER,vbo);glBufferData(GL_ARRAY_BUFFER,vertices.size()*sizeof(float),vertices.data(),GL_STREAM_DRAW);
    glDrawArrays(GL_TRIANGLES,0,GLsizei(vertices.size()/2));glBindVertexArray(0);glEnable(GL_DEPTH_TEST);
}
}
