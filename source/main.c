#include <citro2d.h>
#include <3ds.h>
#include <3ds/synchronization.h>
#include <cwav.h>
#include <ncsnd.h>
#include <jansson.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <malloc.h>
#include "logs.h"
#include "buttons.h"
#include "cwav_shit.h"
#include "scene_manager.h"
#include "main.h"
#include "sprites.h"
#include "request.h"
#define MAX_SPRITES   1
#define SCREEN_WIDTH  400
#define SCREEN_HEIGHT 240
#define fmin(a, b) ((a) < (b) ? (a) : (b))
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define SOC_ALIGN       0x1000
#define SOC_BUFFERSIZE  0x300000
struct memory {
    char *response;
    size_t size;
};
bool cpu_debug = false;
extern bool logplz, readingoferta, json_done, citra_machen;
extern float text_w, text_h, max_scroll;
extern CWAVInfo cwavList[8];
extern int cwavCount;
extern Button buttonsy[100];

bool pausedForSleep = false;
int Scene;
int selectioncodelol;
bool przycskmachen = true;
bool generatingQR = false;
char combinedText[128];
static u32 *SOC_buffer = NULL;

C2D_TextBuf totpBuf;

C2D_TextBuf g_staticBuf, kupon_text_Buf, themeBuf;
C2D_Text g_staticText[100];
C2D_Text themeText[100];
C2D_Text g_totpText[5];

C3D_RenderTarget* top;
C3D_RenderTarget* bottom;



void executeButtonFunction(int buttonIndex) {
    if (buttonIndex >= 0 && buttonIndex < 100 && buttonsy[buttonIndex].onClick) {
        buttonsy[buttonIndex].onClick();
    } else {
        log_to_file("Invalid button index or function not assigned!\n");
    }
}

int main(int argc, char* argv[]) {
    cwavUseEnvironment(CWAV_ENV_DSP);
    romfsInit();
    cfguInit();
    gfxInitDefault();
    PrintConsole topConsole;
    consoleInit(GFX_TOP, &topConsole);
    ndspInit();
    init_logger();

    SOC_buffer = (u32*)memalign(SOC_ALIGN, SOC_BUFFERSIZE);
    if (!SOC_buffer || socInit(SOC_buffer, SOC_BUFFERSIZE)) {
        printf("SOC init failed\n");
    }
	debug = false;
    totpBuf = C2D_TextBufNew(128);
    g_staticBuf = C2D_TextBufNew(256);
    kupon_text_Buf = C2D_TextBufNew(512);
    themeBuf = C2D_TextBufNew(512);
	C2D_TextBuf memBuf = C2D_TextBufNew(128);
	C2D_Text memtext[100];
    consoleClear();

    gfxExit();
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();
    top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
	populateCwavList();
    Scene = 0;
	spritesInit();
	sceneManagerSwitchTo(SCENE_INTRO);
    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        u32 kHeld = hidKeysHeld();
        if (kDown & KEY_START) break;

        sceneManagerUpdate(kDown, kHeld);

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        sceneManagerRender();
		if (debug) {
			C2D_SceneBegin(bottom);
			float cpuStartX = 20.0f;
			float cpuStartY = 20.0f;
			float cpuMaxLength = 260.0f;

			float cpuUsage = C3D_GetProcessingTime() * 6.0f;
			if (cpuUsage > 100.0f) cpuUsage = 100.0f;
			float cpuLineLength = (cpuUsage / 100.0f) * cpuMaxLength;
			C2D_DrawLine(cpuStartX, cpuStartY, C2D_Color32(255, 255, 0, 255),
						 cpuStartX + cpuLineLength, cpuStartY, C2D_Color32(255, 255, 0, 255), 5.0f, 0.4f);

			C2D_TextBufClear(memBuf);
			char memeText[64];
			snprintf(memeText, sizeof(memeText), "CPU: %.2f%%", cpuUsage);
			C2D_TextParse(&memtext[0], memBuf, memeText);
			C2D_TextOptimize(&memtext[0]);


			C2D_DrawText(&memtext[0], C2D_AlignLeft | C2D_WithColor, 20, 25, 0.4f, 0.4f, 0.4f, C2D_Color32(0, 0, 0, 255));

			float drawUsage = C3D_GetDrawingTime() * 6.0f;
			if (drawUsage > 100.0f) drawUsage = 100.0f;
			float drawLineLength = (drawUsage / 100.0f) * cpuMaxLength;
			C2D_DrawLine(cpuStartX, cpuStartY + 30, C2D_Color32(255, 255, 0, 255),
						 cpuStartX + drawLineLength, cpuStartY + 30, C2D_Color32(255, 255, 0, 255), 5.0f, 0.4f);

			C2D_TextBufClear(memBuf);
			snprintf(memeText, sizeof(memeText), "GPU: %.2f%%", drawUsage);
			C2D_TextParse(&memtext[1], memBuf, memeText);
			C2D_TextOptimize(&memtext[1]);


			C2D_DrawText(&memtext[1], C2D_AlignLeft | C2D_WithColor, 20, 55, 1.0f, 0.4f, 0.4f, C2D_Color32(0, 0, 0, 255));
		}

        C3D_FrameEnd(0);
    }

    close_logger();


    C2D_TextBufDelete(g_staticBuf);
    C2D_TextBufDelete(kupon_text_Buf);
    C2D_TextBufDelete(themeBuf);
    C2D_TextBufDelete(totpBuf);

    C2D_Fini();
    C3D_Fini();
    ndspExit();
    cfguExit();
    romfsExit();
    gfxExit();

    return 0;
}
