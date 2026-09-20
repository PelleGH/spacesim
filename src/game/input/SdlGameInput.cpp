#include "input/GameInput.h"
#include <SDL3/SDL.h>
#include <array>
namespace SpaceSim::GameInput {
namespace { std::array<bool,512> held{}, previous{}; Vector2 delta{};
SDL_Scancode scancode(int key) {
    if(key>='A' && key<='Z') return SDL_Scancode(SDL_SCANCODE_A+key-'A');
    if(key>='1' && key<='9') return SDL_Scancode(SDL_SCANCODE_1+key-'1');
    switch(key) {
    case KEY_SPACE:return SDL_SCANCODE_SPACE; case KEY_TAB:return SDL_SCANCODE_TAB;
    case KEY_LEFT_SHIFT:return SDL_SCANCODE_LSHIFT;case KEY_LEFT_CONTROL:return SDL_SCANCODE_LCTRL;
    case KEY_F1:return SDL_SCANCODE_F1;case KEY_LEFT_ALT:return SDL_SCANCODE_LALT;case KEY_F11:return SDL_SCANCODE_F11;
    default:return SDL_SCANCODE_UNKNOWN;
    }
}}
void beginFrame(bool captureMouse) {
    previous=held; held.fill(false);
    const bool* keyboard=SDL_GetKeyboardState(nullptr);
    if(SDL_GetKeyboardFocus()) for(int i=0;i<512;++i) {auto code=scancode(i); if(code!=SDL_SCANCODE_UNKNOWN) held[i]=keyboard[code];}
    float x=0,y=0;SDL_GetRelativeMouseState(&x,&y);
    delta=captureMouse?Vector2{x,y}:Vector2{};
}
bool keyDown(int key){return key>=0 && key<512 && held[key];}
bool keyPressed(int key){return keyDown(key) && !previous[key];}
Vector2 mouseDelta(){return delta;}
void clear(){previous=held;held.fill(false);delta={};}
void inject(int key,bool value){if(key>=0 && key<512) held[key]=value;}
}
