#include "scene_manager.h"
#include "scene_mainmenu.h"
#include "scene_intro.h"
#include "sprites.h"

bool debug;
static SceneType currentScene = SCENE_NONE;

void sceneManagerInit(SceneType initialScene) {
    sceneManagerSwitchTo(initialScene);
}

void sceneManagerUpdate(uint32_t kDown, uint32_t kHeld) {
    switch (currentScene) {
        case SCENE_MAIN_MENU: sceneMainMenuUpdate(kDown, kHeld); break;
        case SCENE_INTRO: sceneIntroUpdate(kDown, kHeld); break;
        default: break;
    }
}

void sceneManagerRender(void) {
    switch (currentScene) {
        case SCENE_MAIN_MENU: sceneMainMenuRender(); break;
        case SCENE_INTRO: sceneIntroRender(); break;
        default: break;
    }
}

void sceneManagerSwitchTo(SceneType nextScene) {
    
    switch (currentScene) {
        case SCENE_MAIN_MENU: sceneMainMenuExit(); break;
        case SCENE_INTRO: sceneIntroExit(); break;
        default: break;
    }


    currentScene = nextScene;
    switch (currentScene) {
        case SCENE_MAIN_MENU: sceneMainMenuInit(); break;
        case SCENE_INTRO: sceneIntroInit(); break;
        default: break;
    }
}

void sceneManagerExit(void) {
    sceneManagerSwitchTo(SCENE_NONE);
}
