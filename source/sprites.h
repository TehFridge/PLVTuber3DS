// sprites.h
#ifndef SPRITES_H
#define SPRITES_H
#include <citro2d.h>
#include <stdlib.h>
typedef struct {
    int currentFrame;
    u64 lastFrameTime;
    int loopedtimes;
    int loops;
    bool done;
    
    int halt_at_frame;
    int halt_for_howlong; // in ms
    u64 haltStartTime;    // time when halt started
    bool halting;         // whether we're currently halting
} SpriteAnimState;


extern C2D_SpriteSheet testsheet, logo, fridge;
extern C2D_Image test, vtubelogo, flyinglogo, fridge_image;

void ResetAnimState(SpriteAnimState* anim);
void PlaySprite(float scale, C2D_SpriteSheet frames, int framerate, int framecount, float x, float y, SpriteAnimState* anim, int direction, float depth);
void spritesInit();
#endif