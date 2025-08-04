#include "sprites.h"
#include <3ds/os.h>
C2D_SpriteSheet testsheet, logo, fridge;
C2D_Image test, vtubelogo, flyinglogo, fridge_image;
u64 lastFrameTime = 0;
int currentFrame = 0;

void ResetAnimState(SpriteAnimState* anim) {
    anim->currentFrame = 0;
    anim->lastFrameTime = osGetTime();
    anim->done = false;
    anim->loopedtimes = 0;
    anim->halting = false;
    anim->haltStartTime = 0;
}

void PlaySprite(float scale, C2D_SpriteSheet frames, int framerate, int framecount,
                float x, float y, SpriteAnimState* anim, int direction, float depth) {
    if (!frames || framecount == 0 || !anim) return;

    int totalFrames = C2D_SpriteSheetCount(frames);
    if (framecount > totalFrames) framecount = totalFrames;
    if (anim->done) return;

    u64 now = osGetTime();

    // Only update frame if not halting or halt has expired
    if (now - anim->lastFrameTime >= 1000 / framerate) {
        bool shouldAdvance = true;

        if (anim->halt_at_frame >= 0 && anim->currentFrame == anim->halt_at_frame) {
            if (!anim->halting) {
                anim->halting = true;
                anim->haltStartTime = now;
                shouldAdvance = false;
            } else {
                // Already halting
                if (now - anim->haltStartTime >= anim->halt_for_howlong) {
                    anim->halting = false;
                    shouldAdvance = true;
                } else {
                    shouldAdvance = false;
                }
            }
        }

        if (shouldAdvance) {
            anim->currentFrame++;
			if (anim->currentFrame >= framecount) {
				if (anim->loops == -1) {
					anim->currentFrame = 0;  // Infinite loop
				} else {
					anim->loopedtimes++;
					if (anim->loopedtimes >= anim->loops) {
						anim->done = true;
						anim->currentFrame = framecount - 1; // Stay on last frame
						return;
					} else {
						anim->currentFrame = 0; // Loop again
					}
				}
			}


            anim->lastFrameTime = now;
        }
    }
	
    if (anim->currentFrame < 0 || anim->currentFrame >= totalFrames) return;

    C2D_Image frameImage = C2D_SpriteSheetGetImage(frames, anim->currentFrame);
    if (!frameImage.subtex) return;
	C2D_DrawImageAt(frameImage, x, y, depth, NULL, direction * scale, scale);
}



void spritesInit() {
	srand(osGetTime());
	logo = C2D_SpriteSheetLoad("romfs:/gfx/logo.t3x");

	if (rand() % 2 == 0) {
		fridge = C2D_SpriteSheetLoad("romfs:/gfx/tehfridge2.t3x");
	} else {
		fridge = C2D_SpriteSheetLoad("romfs:/gfx/tehfridge.t3x");
	}

	fridge_image = C2D_SpriteSheetGetImage(fridge, 0);
	vtubelogo = C2D_SpriteSheetGetImage(logo, 0);
	flyinglogo = C2D_SpriteSheetGetImage(logo, 1);
}

