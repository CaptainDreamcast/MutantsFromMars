#include "titlescreen.h"

#include <prism/blitz.h>

#include "storyscreen.h"

class TitleScreen
{
public:
    MugenSpriteFile mSprites;
    MugenAnimations mAnimations;
    MugenSounds mSounds;

    MugenAnimationHandlerElement* titleAnimation;

    static void startStoryScreen(void* tCaller) {
        (void)tCaller;
        setCurrentStoryDefinitionFile("game/INTRO.def", 0);
        setNewScreen(getStoryScreen());
    }

    TitleScreen() {
        mSprites = loadMugenSpriteFileWithoutPalette("game/TITLE.sff");
        mAnimations = loadMugenAnimationFile("game/TITLE.air");
        mSounds = loadMugenSoundFile("game/TITLE.snd");

        titleAnimation = addMugenAnimation(getMugenAnimation(&mAnimations, 1), &mSprites, Vector3D(0, 0, 1));
        addFadeIn(30, NULL, NULL);
        streamMusicFile("game/GAME.ogg");
    }
    void update() {
        updateInput();
    }

    void updateInput()
    {
            if(hasPressedAFlank() || hasPressedStartFlank())
            {
                tryPlayMugenSound(&mSounds, 1, 0);
                addFadeOut(30, startStoryScreen, NULL);
            }
    }
};

EXPORT_SCREEN_CLASS(TitleScreen);