#include <3ds.h>
#include <citro2d.h>
#include "scene_intro.h"
#include "scene_manager.h"
#include "main.h"
#include "sprites.h"
#include "cwav_shit.h"
#include <math.h>
#include <stdlib.h>

float easeOutCubic(float t) {
    return 1 - powf(1 - t, 3);
}
static float introTimer = 0.0f;
static float waitTimer = 0.0f;
static float secondTimer = 0.0f;

static bool animationDone = false;
static bool waitingDone = false;
static bool secondAnimDone = false;

extern CWAVInfo cwavList[8];

C2D_TextBuf textBuf;
C2D_Text text;

static bool textAnimationStarted = false;

static float bounceTimer = 0.0f;
static int bounceStage = 0; 

#define BOUNCE_DURATION 0.41f 
static int bounceCount = 0;
static bool bounceFinished = false;
int lastBounceStage = -1;

// Flash variables
static float flashTimer = 0.0f;
#define FLASH_DURATION 1.0f
static float fadeTimer = 0.0f;
#define FADE_DURATION 1.0f  
static bool fadeStarted = false;
void sceneIntroInit(void) {
    CWAV* intro = cwavList[2].cwav;
    cwavPlay(intro, 0, 1); 


    textBuf = C2D_TextBufNew(128);
}

void sceneIntroUpdate(uint32_t kDown, uint32_t kHeld) {
    if (!animationDone) {
        introTimer += 0.023f;
        if (introTimer >= 1.0f) {
            introTimer = 1.0f;
            animationDone = true;
        }
    } else if (!waitingDone) {
        waitTimer += 1.0f / 60.0f; 
        if (waitTimer >= 0.35f) {
            waitingDone = true;
        }
    } else if (!secondAnimDone) {
        secondTimer += 0.06f;
        if (secondTimer >= 0.87f) {
            secondTimer = 1.0f;
            secondAnimDone = true;
        }
    }

    if (secondAnimDone) {
        textAnimationStarted = true;

        if (!bounceFinished) {
            bounceTimer += 1.0f / 60.0f; 

            if (bounceTimer >= BOUNCE_DURATION) {
                bounceTimer = 0.0f;
                bounceStage = (bounceStage + 1) % 3;  

                const char* stages[] = { "TehFridge", "Teh", "TehFri" };
                if (bounceStage != lastBounceStage) {
                    C2D_TextParse(&text, textBuf, stages[bounceStage]);
                    C2D_TextOptimize(&text);
                    lastBounceStage = bounceStage;
                }

                bounceCount++;
                if (bounceCount >= 3) {  
                    bounceFinished = true;
                    bounceStage = 0;
                    C2D_TextParse(&text, textBuf, stages[bounceStage]);
                    C2D_TextOptimize(&text);
                    lastBounceStage = bounceStage;

                    flashTimer = FLASH_DURATION;  
                }
            }
        }
    }

    if (flashTimer > 0.0f) {
        flashTimer -= 1.0f / 60.0f;
        if (flashTimer < 0.0f) flashTimer = 0.0f;
    } else if (bounceFinished && !fadeStarted) {

        fadeTimer += 1.0f / 60.0f;
        if (fadeTimer >= 1.0f) {
            fadeStarted = true;
            fadeTimer = 0.0f;  
        }
    }

    if (fadeStarted) {
        fadeTimer += 1.0f / 60.0f;
        if (fadeTimer >= FADE_DURATION) {
            fadeTimer = FADE_DURATION;
        
            sceneManagerSwitchTo(SCENE_MAIN_MENU);
        }
    }
}

void sceneIntroRender(void) {
    C2D_SceneBegin(top);
    C2D_TargetClear(top, C2D_Color32(0, 0, 0, 255));

    float scale;

    if (!animationDone) {
        float t = easeOutCubic(introTimer);
        scale = (1.0f - t) * 40.0f + t * 0.3f;  
    } else if (!waitingDone) {
        scale = 0.3f;  
    } else if (!secondAnimDone) {
        float t = easeOutCubic(secondTimer);
        scale = (1.0f - t) * 0.3f + t * 0.22f;  
    } else {
        scale = 0.22f;
    }

    float imgW = fridge_image.subtex->width * scale;
    float imgH = fridge_image.subtex->height * scale;

    float centerX = 400.0f / 2.0f;
    float centerY = 240.0f / 2.0f;

    float drawX = centerX - imgW / 2.0f;
    float drawY = centerY - imgH / 2.0f;

    C2D_DrawImageAt(
        fridge_image,
        drawX, drawY + 10.0f,
        0.1f,
        NULL,
        scale, scale
    );

    if (textAnimationStarted) {
        const char* stages[] = { "TehFridge", "Teh", "TehFri" };
        const char* textToDraw = stages[bounceStage];

		float bounceScale = 1.0f;

		if (!bounceFinished) {
			float bounceProgress = bounceTimer / BOUNCE_DURATION;
			float eased = powf(sinf(bounceProgress * M_PI), 0.14f);  
			bounceScale = 1.2f - 0.2f * eased;  // Start at 1.2 and ease down to 1.0
		}

        float textX = 400.0f / 2.0f;
        float textY = 180.0f;

        float textWidth, textHeight;
        C2D_TextGetDimensions(&text, 1.0f, 1.0f, &textWidth, &textHeight);

        float drawX = textX - (textWidth * bounceScale) / 2.0f;

        C2D_DrawText(&text, C2D_WithColor, drawX, 20.0f, 1.0f, bounceScale, bounceScale, C2D_Color32(235, 0, 146, 255));
    }


    // Draw the screen flash if active
    if (flashTimer > 0.0f) {
        float alpha = (flashTimer / FLASH_DURATION) * 255.0f;
        C2D_DrawRectSolid(0, 0, 1.0f, 400, 240, C2D_Color32(255, 255, 255, (uint8_t)alpha));
    }

    // Draw fade to black overlay if fading started
    if (fadeStarted) {
        float alpha = (fadeTimer / FADE_DURATION) * 255.0f;
        C2D_DrawRectSolid(0, 0, 1.0f, 400, 240, C2D_Color32(0, 0, 0, (uint8_t)alpha));
    }

    C2D_SceneBegin(bottom);
    C2D_TargetClear(bottom, C2D_Color32(0, 0, 0, 255));
}

void sceneIntroExit(void) {
    if (textBuf) {
        C2D_TextBufDelete(textBuf);
        textBuf = NULL;
    }
}
