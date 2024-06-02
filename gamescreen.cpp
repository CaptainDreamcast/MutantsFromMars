#include "gamescreen.h"

#include <prism/soundeffect.h>

#include "storyscreen.h"

static struct {

    int mLevel = 0;
    int mScore = 0;
    int mTime = 0;

} gGameScreenData;


class GameScreen
{
    public:

    GameScreen() {
        setSoundEffectVolume(0.4);
        load();
    }

    void load() {
        loadFiles();
        loadBG();
        loadPlayer();
        loadShots();
        loadEnemies();
        loadTime();
        loadRoundStart();
    }

    int bigTextID;
    int mTotalTimeTextID = -1;
    bool isShowingRoundStart = false;
    int mRoundStartTimer = -1;
    void loadRoundStart()
    {
        std::string roundText = "ROUND " + std::to_string(gGameScreenData.mLevel + 1);
        if(gGameScreenData.mLevel == 2)
        {
            roundText = "FINAL ROUND";
        }

        bigTextID = addMugenTextMugenStyle(roundText.c_str(), Vector3D(160, 120, 30), Vector3DI(2, 0, 0));
        setMugenTextScale(bigTextID, 2);
        mRoundStartTimer = addTimerCB(180, nullptr, nullptr);
        isShowingRoundStart = true;
        pausePlayerAndEnemyAnimations();
        tryPlayMugenSound(&mSounds, 100, 1);
    }

    void pausePlayerAndEnemyAnimations()
    {
        pauseBlitzMugenAnimation(playerEntity);
        for(auto& e : mEnemies) {
            pauseMugenAnimation(e.second.animation);
        }
    }

    void unpausePlayerAndEnemyAnimations()
    {
        unpauseBlitzMugenAnimation(playerEntity);
        for(auto& e : mEnemies) {
            unpauseMugenAnimation(e.second.animation);
        }
    }

    MugenSpriteFile mSprites;
    MugenAnimations mAnimations;
    MugenSounds mSounds;

    void loadFiles()
    {
        mSprites = loadMugenSpriteFileWithoutPalette("game/GAME.sff");
        mAnimations = loadMugenAnimationFile("game/GAME.air");
        mSounds = loadMugenSoundFile("game/GAME.snd");
    }

    MugenAnimationHandlerElement* mBG;

    void loadBG()
    {
        mBG = addMugenAnimation(getMugenAnimation(&mAnimations, 1), &mSprites, Vector3D(0, 0, 1));
    }

    int playerEntity;
    void loadPlayer()
    {
        playerEntity = addBlitzEntity(Vector3D(160, 214, 2));
        addBlitzMugenAnimationComponent(playerEntity, &mSprites, &mAnimations, 10);
    }

    void update() {
        updatePlayer();
        updateEnemies();
        updateShots();
        updateUI();
    }

    void updateUI()
    {
        updateRoundStart();
        updateTime();
    }

    void updateRoundStart()
    {
        if(isShowingRoundStart) {
            if(hasTimerFinished(mRoundStartTimer)) {
                setMugenTextVisibility(bigTextID, 0);
                unpausePlayerAndEnemyAnimations();
                streamMusicFile("game/GAME.ogg");
                isShowingRoundStart = false;
            }
        }
    }

    void updatePlayer()
    {
        updatePlayerMovement();
        updatePlayerShooting();
        updatePlayerWin();
        updatePlayerDeath();
    }

    bool isShowingRoundWin = false;
    int roundWinTimer = -1;
    void updatePlayerWin()
    {
        if(isPlayerDying) return;

        if(mEnemies.empty() && !isShowingRoundWin) {
            removeAllEnemyShots();
            isShowingRoundWin = true;
            roundWinTimer = addTimerCB(180, nullptr, nullptr);
            pausePlayerAndEnemyAnimations();

            stopStreamingMusicFile();
            tryPlayMugenSound(&mSounds, 100, 2);

            std::string roundText = "ROUND " + std::to_string(gGameScreenData.mLevel + 1) + " CLEARED";
            if(gGameScreenData.mLevel == 2)
            {
                roundText = "FINAL ROUND CLEARED";
                mTotalTimeTextID = addMugenTextMugenStyle("TOTAL TIME: 0:00", Vector3D(160, 140, 30), Vector3DI(2, 0, 0));
                setMugenTextScale(mTotalTimeTextID, 2);
                int minutes = gGameScreenData.mTime / 60 / 60;
                int seconds = gGameScreenData.mTime / 60;
                char text[100];
                sprintf(text, "TOTAL TIME: %d:%02d", minutes, seconds);
                changeMugenText(mTotalTimeTextID, text);
            }

            changeMugenText(bigTextID, roundText.c_str());
            setMugenTextVisibility(bigTextID, 1);
        }
        else if(isShowingRoundWin) {
            if(hasTimerFinished(roundWinTimer)) {
                if(gGameScreenData.mLevel == 2)
                {
                    resetGame();
                    setCurrentStoryDefinitionFile("game/OUTRO.def", 0);
                    setNewScreen(getStoryScreen());
                }
                else
                {
                    gGameScreenData.mLevel++;
                    setNewScreen(getGameScreen());
                }
            }
        }
    }

    void removeAllEnemyShots()
    {
        for(auto& e : mShots) {
            if(e.second.dy > 0) {
                e.second.isScheduledForDeletion = 1;
            }
        }
    }

    bool isPlayerDying = false;
    bool isShowingGameOver = false;
    int playerDeathTimerId = -1;
    void updatePlayerDeath()
    {
        if(isShowingRoundWin) return;

        if(!isPlayerDying)
        {
            if(isPlayerCollidingWithEnemies() || isPlayerCollidingWithShots() || hasEnemyMovedPastScreen()) {
                isPlayerDying = true;
                MugenAnimationHandlerElement* explosion = addMugenAnimation(getMugenAnimation(&mAnimations, 30), &mSprites, getBlitzEntityPosition(playerEntity));
                setMugenAnimationNoLoop(explosion);
                tryPlayMugenSound(&mSounds, 1, 2);
                setBlitzMugenAnimationVisibility(playerEntity, 0);
                playerDeathTimerId = addTimerCB(60, nullptr, this);
                removeAllPlayerShots();
            }
        }
        else if(!isShowingGameOver)
        {
            if(hasTimerFinished(playerDeathTimerId)) {
                isShowingGameOver = true;
                pausePlayerAndEnemyAnimations();
                changeMugenText(bigTextID, "GAME OVER");
                setMugenTextVisibility(bigTextID, 1);

                stopStreamingMusicFile();
                tryPlayMugenSound(&mSounds, 100, 0);
            }
        }
        else{
            if(hasPressedAFlank() || hasPressedStartFlank()) {
                resetGame();
                setNewScreen(getGameScreen());
            }
        }
    }

    void removeAllPlayerShots()
    {
        for(auto& e : mShots) {
            if(e.second.dy < 0) {
                e.second.isScheduledForDeletion = 1;
            }
        }
    }

    bool hasEnemyMovedPastScreen()
    {
        for(auto& e : mEnemies) {
            if(getMugenAnimationPosition(e.second.animation).y > 240) {
                return 1;
            }
        }
        return 0;
    }

    bool isPlayerCollidingWithEnemies()
    {
        for(auto& e : mEnemies) {
            if(isOverlapping(getBlitzEntityPosition(playerEntity).xy(), getMugenAnimationPosition(e.second.animation).xy(), 15)) {
                return 1;
            }
        }
        return 0;
    }

    bool isPlayerCollidingWithShots()
    {
        for(auto& e : mShots) {
            if(e.second.dy > 0 && isOverlapping(getBlitzEntityPosition(playerEntity).xy(), getMugenAnimationPosition(e.second.animation).xy(), 2)) {
                return 1;
            }
        }
        return 0;
    }

    int playerShootingCooldown = 0;
    void updatePlayerShooting()
    {
        if(isPlayerDying) return;
        if(isShowingRoundStart) return;
        if(isShowingRoundWin) return;

        if(playerShootingCooldown) playerShootingCooldown--;

        if(hasPressedA() && !playerShootingCooldown) {
            addShot(getBlitzEntityPosition(playerEntity).xy(), 1);
            tryPlayMugenSoundAdvanced(&mSounds, 1, 3, 0.05);
            playerShootingCooldown = 10;
        }
    }

    void updatePlayerMovement()
    {
        if(isPlayerDying) return;
        if(isShowingRoundStart) return;
        if(isShowingRoundWin) return;

        int dx = 0;
        if(hasPressedLeft()) {
            dx = -1;
        } else if(hasPressedRight()) {
            dx = 1;
        }

        const auto speed = 2.0;
        auto p = getBlitzEntityPositionReference(playerEntity);
        p->x = clamp(p->x + dx * speed, 16.0, 303.0);
    }

    struct Shot {
        MugenAnimationHandlerElement* animation;
        int dy;
        int isScheduledForDeletion = 0;
    };
    std::map<int, Shot> mShots;

    void loadShots()
    {
        mShots.clear();
    }

    int mNextShotID = 0;
    void addShot(const Vector2D& position, int tIsPlayerShot)
    {
        auto& shot = mShots[mNextShotID++];
        shot.animation = addMugenAnimation(getMugenAnimation(&mAnimations, 40), &mSprites, Vector3D(position.x, position.y, 10));
        shot.dy = tIsPlayerShot ? -1 : 1;
    }

    void updateShots()
    {
        updateShotsMovement();
        updateShotRemoval();
    }

    void updateShotsMovement()
    {
        if(isShowingRoundWin) return;
        if(isShowingGameOver) return;

        for(auto& e : mShots) {
            auto p = getMugenAnimationPositionReference(e.second.animation);
            p->y += e.second.dy;

            if(p->y < 15 || p->y > 224) {
                e.second.isScheduledForDeletion = 1;
            }
        }
    }

    void updateShotRemoval()
    {
        for(auto& e : mShots) {
            if(e.second.isScheduledForDeletion) {
                removeMugenAnimation(e.second.animation);
            }
        }

        for(auto iter = mShots.begin(); iter != mShots.end(); ) {
            if (iter->second.isScheduledForDeletion) {
                iter = mShots.erase(iter);
            } else {
                ++iter;
            }
        }
    }

    struct Enemy {
        MugenAnimationHandlerElement* animation;
        int isScheduledForDeletion = 0;
    };
    std::map<int, Enemy> mEnemies;

    int enemyLifeTime = 0;
    void loadEnemies()
    {
        mEnemies.clear();
        enemyLifeTime = 0;
        spawnEnemies();
    }

    void spawnEnemies()
    {
        for(int i = 0; i < 5; i++) {
            addEnemy(Vector2D(80 + i * 40, 20));
            addEnemy(Vector2D(80 + i * 40, 40));
            addEnemy(Vector2D(80 + i * 40, 60));
        }

        const auto speedFactor = levelToEnemySpeedFactor();
        const auto inverseSpeedFactor = 1.0 / speedFactor;
        enemyLifeTime += 60 * inverseSpeedFactor;

    }

    int mNextEnemyID = 0;
    void addEnemy(const Vector2D& position)
    {
        auto& enemy = mEnemies[mNextEnemyID++];
        enemy.animation = addMugenAnimation(getMugenAnimation(&mAnimations, 20), &mSprites, Vector3D(position.x, position.y, 10));
    }

    void updateEnemies()
    {
        if(isShowingRoundStart) return;

        updateEnemiesMovement();
        updateEnemiesShooting();
        updateEnemiesDeath();
    }

    int levelToShotProbability()
    {
        return 5 + gGameScreenData.mLevel * 5;
    }

    void updateEnemiesShooting()
    {
        if(mEnemies.empty()) return;
        if(isShowingGameOver) return;

        bool doesFireShot = randfromInteger(0, 100) < levelToShotProbability();
        if(doesFireShot)
        {
            int firingEnemy = randfromInteger(0, mEnemies.size() - 1);
            auto iter = mEnemies.begin();
            std::advance(iter, firingEnemy);
            addShot(getMugenAnimationPosition(iter->second.animation).xy(), 0);
            tryPlayMugenSound(&mSounds, 1, 1);
        }
    }

    double levelToEnemySpeedFactor()
    {
        return 0.5 + gGameScreenData.mLevel * 0.25;
    }

    void updateEnemiesMovement()
    {
        if(isShowingGameOver) return;
        if(isShowingRoundWin) return;

        enemyLifeTime++;

        const auto speedFactor = levelToEnemySpeedFactor();
        const auto inverseSpeedFactor = 1.0 / speedFactor;

        int dx = 0;
        int dy = 0;
        if(enemyLifeTime % int(280 * inverseSpeedFactor) < int(120 * inverseSpeedFactor)) {
            dx = 1;
        }
        else if(enemyLifeTime % int(280 * inverseSpeedFactor) < int((120 + 20) * inverseSpeedFactor)) {
            dy = 1;
        }
        else if(enemyLifeTime % int(280 * inverseSpeedFactor) < int((120 + 20 + 120) * inverseSpeedFactor)) {
            dx = -1;
        }
        else
        {
            dy = 1;
        }

        if(enemyLifeTime % int(280 * inverseSpeedFactor) == int(120 * inverseSpeedFactor))
        {
            tryPlayMugenSound(&mSounds, 1, 4);
        }
        else if(enemyLifeTime % int(280 * inverseSpeedFactor) == int((120 + 20 + 120) * inverseSpeedFactor))
        {
            tryPlayMugenSound(&mSounds, 1, 4);
        }
       
        for(auto& e : mEnemies) {
            auto p = getMugenAnimationPositionReference(e.second.animation);
            p->x += dx * speedFactor;
            p->y += dy * speedFactor;
        }
    }

    void updateEnemiesDeath()
    {
        bool hasEnemyDied = false;
        for(auto& e : mEnemies) {
            if(isShotCollidingWithEnemy(e.second.animation)) {
                e.second.isScheduledForDeletion = 1;
                MugenAnimationHandlerElement* explosion = addMugenAnimation(getMugenAnimation(&mAnimations, 30), &mSprites, getMugenAnimationPosition(e.second.animation));
                setMugenAnimationNoLoop(explosion);
                hasEnemyDied = true;
            }
        }
        if(hasEnemyDied) {
            tryPlayMugenSound(&mSounds, 1, 0);
        }


        for(auto& e : mEnemies) {
            if(e.second.isScheduledForDeletion) {
                removeMugenAnimation(e.second.animation);
            }
        }

        for(auto iter = mEnemies.begin(); iter != mEnemies.end(); ) {
            if (iter->second.isScheduledForDeletion) {
                iter = mEnemies.erase(iter);
            } else {
                ++iter;
            }
        }
    }

    bool isOverlapping(const Vector2D& tFirst, const Vector2D& tSecond, double tRadius)
    {
        return (tFirst.x - tSecond.x) * (tFirst.x - tSecond.x) + (tFirst.y - tSecond.y) * (tFirst.y - tSecond.y) < tRadius * tRadius;
    }

    bool isShotCollidingWithEnemy(MugenAnimationHandlerElement* tEnemy)
    {
        for(auto& e : mShots) {
            if(e.second.dy < 0 && isOverlapping(getMugenAnimationPosition(e.second.animation).xy(), getMugenAnimationPosition(tEnemy).xy(), 10)) {
                e.second.isScheduledForDeletion = 1;
                return 1;
            }
        }
        return 0;
    }

    int mTimeTextID;
    void loadTime()
    {
        mTimeTextID = addMugenTextMugenStyle("TIME: 0", Vector3D(160, 11, 30), Vector3DI(2, 0, 0));
        updateTimerText();
    }

    void updateTime()
    {
        if(getMugenTextVisibility(bigTextID) != 0) return;

        gGameScreenData.mTime++;
        updateTimerText();       
    }

    void updateTimerText()
    {
        int minutes = gGameScreenData.mTime / 60 / 60;
        int seconds = gGameScreenData.mTime / 60;
        char text[100];
        sprintf(text, "TIME: %d:%02d", minutes, seconds);
        changeMugenText(mTimeTextID, text);
    }
};

EXPORT_SCREEN_CLASS(GameScreen); 

void resetGame()
{
    gGameScreenData.mLevel = 0;
    gGameScreenData.mScore = 0;
    gGameScreenData.mTime = 0;
}