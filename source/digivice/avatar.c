#include "digivice/avatar.h"

#include "digiapi.h"
#include "enums.h"

#include "globals.h"
#include "texture.h"

#include <ctype.h>
#include <stdlib.h>
#include <time.h>

int initAvatar(Avatar* ret) {
    int statusInit = DIGI_init(SAVE_FILE) == DIGI_RET_OK;

    if (statusInit) {
        ret->infoApi = DIGI_playingDigimon();
        strncpy(ret->name, ret->infoApi.pstCurrentDigimon->szName,
                sizeof(ret->name));

        char spriteSheetFile[270] = {0};
        int i;
        snprintf(spriteSheetFile, sizeof(spriteSheetFile), "resource/%s.gif",
                 ret->name);
        for (i = 0; i < strlen(spriteSheetFile); i++) {
            spriteSheetFile[i] = tolower(spriteSheetFile[i]);
        }

        ret->spriteSheet = loadTexture(spriteSheetFile);
        if (ret->spriteSheet == NULL)
            return 0;

        ret->transform.x = WIDTH_SCREEN / 2 - WIDTH_SPRITE / 2;
        ret->transform.y = HEIGHT_SCREEN / 2 - HEIGHT_SPRITE / 2;
        ret->transform.w = WIDTH_SPRITE;
        ret->transform.h = HEIGHT_SPRITE;

        addAnimation(&ret->animationController, "hatching", 2,
                     createRect(0, 0, 16, 16), 1.f,
                     createRect(NORMAL_SIZE_SPRITE, 0, NORMAL_SIZE_SPRITE,
                                NORMAL_SIZE_SPRITE),
                     1.f);
        addAnimation(&ret->animationController, "beingBorn", 3,
                     createRect(0, 0, NORMAL_SIZE_SPRITE, NORMAL_SIZE_SPRITE),
                     1.f,
                     createRect(NORMAL_SIZE_SPRITE, 0, NORMAL_SIZE_SPRITE,
                                NORMAL_SIZE_SPRITE),
                     1.f,
                     createRect(NORMAL_SIZE_SPRITE * 2, 0, NORMAL_SIZE_SPRITE,
                                NORMAL_SIZE_SPRITE),
                     1.f);
        addAnimation(
            &ret->animationController, "walking", 9, createRect(0, 0, 16, 16),
            1.f,
            createRect(NORMAL_SIZE_SPRITE, 0, NORMAL_SIZE_SPRITE,
                       NORMAL_SIZE_SPRITE),
            1.f, createRect(0, 0, NORMAL_SIZE_SPRITE, NORMAL_SIZE_SPRITE), 1.f,
            createRect(NORMAL_SIZE_SPRITE, 0, NORMAL_SIZE_SPRITE,
                       NORMAL_SIZE_SPRITE),
            1.f, createRect(0, 0, NORMAL_SIZE_SPRITE, NORMAL_SIZE_SPRITE), 1.f,
            createRect(NORMAL_SIZE_SPRITE, 0, NORMAL_SIZE_SPRITE,
                       NORMAL_SIZE_SPRITE),
            1.f, createRect(0, 0, NORMAL_SIZE_SPRITE, NORMAL_SIZE_SPRITE), 1.f,
            createRect(NORMAL_SIZE_SPRITE, 0, NORMAL_SIZE_SPRITE,
                       NORMAL_SIZE_SPRITE),
            1.f,
            createRect(NORMAL_SIZE_SPRITE * 2, 0, NORMAL_SIZE_SPRITE,
                       NORMAL_SIZE_SPRITE),
            1.f);

        if (ret->infoApi.pstCurrentDigimon->uiStage == DIGI_STAGE_EGG) {
            ret->currentAction = HATCHING;
            setCurrentAnimation(&ret->animationController, "hatching");
        } else {
            ret->currentAction = WALKING;
            setCurrentAnimation(&ret->animationController, "walking");
        }
        ret->initiated = 1;
    }

    srand(time(NULL));
    return statusInit;
}

void updateAvatar(Avatar* avatar, const float deltaTime) {
    if (!avatar->initiated)
        return;

    avatar->timePassed += deltaTime;
    if (avatar->timePassed >= 1.f) {
        avatar->secondsPassed++;
        if (avatar->secondsPassed >= 60) {
            unsigned char events;
            DIGI_updateEventsDeltaTime(1, &events);
            avatar->infoApi = DIGI_playingDigimon();

            handleEvents(avatar, events);
            avatar->secondsPassed = 0;
        }

        if (avatar->currentAction == WALKING &&
            avatar->infoApi.pstCurrentDigimon->uiStage > DIGI_STAGE_EGG) {
            if (rand() % 100 < 50) {
                avatar->renderFlags = avatar->renderFlags == SDL_FLIP_HORIZONTAL
                                          ? SDL_FLIP_NONE
                                          : SDL_FLIP_HORIZONTAL;
            }
            const int direction =
                avatar->renderFlags & SDL_FLIP_HORIZONTAL ? -1 : 1;

            avatar->transform.x += STEP_SPRITE * direction;
        } else if (avatar->currentAction == EVOLVING) {
            if (finishedCurrentAnimation(&avatar->animationController)) {
                char spriteSheetFile[270] = {0};
                int i;
                snprintf(spriteSheetFile, sizeof(spriteSheetFile),
                         "resource/%s.gif",
                         avatar->infoApi.pstCurrentDigimon->szName);
                for (i = 0; i < strlen(spriteSheetFile); i++) {
                    spriteSheetFile[i] = tolower(spriteSheetFile[i]);
                }

                freeTexture(avatar->spriteSheet);
                avatar->spriteSheet = loadTexture(spriteSheetFile);
                setCurrentAnimation(&avatar->animationController, "walking");
                avatar->currentAction = WALKING;
            }
        }

        avatar->timePassed = 0.f;
    }

    updateAnimation(&avatar->animationController, deltaTime);
}

void handleEvents(Avatar* avatar, const unsigned char events) {
    if (events & DIGI_EVENT_MASK_EVOLVE) {
        avatar->currentAction = EVOLVING;

        SDL_Log("Evolving to %s", avatar->infoApi.pstCurrentDigimon->szName);
        if (avatar->infoApi.pstCurrentDigimon->uiStage == DIGI_STAGE_BABY_1) {
            setCurrentAnimation(&avatar->animationController, "beingBorn");
            SDL_Log("It's a birth");
        }
    }
}

void drawAvatar(SDL_Renderer* render, const Avatar* avatar) {
    if (!avatar->initiated)
        return;

    const SDL_Rect* currentSpriteRect =
        getAnimationFrameClip(&avatar->animationController);

    SDL_RenderCopyEx(render, avatar->spriteSheet, currentSpriteRect,
                     &avatar->transform, 0.f, NULL, avatar->renderFlags);
}

void freeAvatar(Avatar* avatar) {
    freeAnimationController(&avatar->animationController);
    if (avatar->spriteSheet)
        freeTexture(avatar->spriteSheet);
}