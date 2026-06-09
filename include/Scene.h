#pragma once
#include "Module.h"
#include "Player.h"
#include "UIManager.h"
#include "UIElement.h"
#include "PuzzleManager.h"
#include "PuzzleManager3.h"
#include <map>
#include <set>
#include <utility>

class Boss; // forward declaration — full type in Scene.cpp

enum class SceneID {
    INTRO,
    MAIN_MENU,
    INTRO_CINEMATIC,
    TUTORIAL_TEXT_CARD,
    LOADING,
    GAMEPLAY
};

class Scene : public Module
{
public:
    friend class SaveSystem;
    Scene();
    ~Scene();

    bool Awake()          override;
    bool Start()          override;
    bool PreUpdate()      override;
    bool Update(float dt) override;
    bool PostUpdate()     override;
    bool CleanUp()        override;

    bool OnUIMouseClickEvent(UIElement* uiElement);

    Vector2D GetPlayerPosition();
    void     SetPlayerPosition(Vector2D pos);
    SceneID  GetCurrentScene() const { return currentScene; }
    int      GetCurrentLevelIndex() const { return currentLevelIndex_; }
    std::string GetTilePosDebug() const { return ""; }
    int GetMenuClickFxId() const { return menuClickFxId; }

    std::shared_ptr<Player> player = nullptr;

    bool pendingSubMapLoad_ = false;
    std::string subMapTarget_;
    std::string subMapSpawn_;
    std::string subMapSpawnId_;
    std::string previousMapFile_;
    std::string previousSpawnId_;
    float portalCooldown_ = 0.0f;


    void LoadSubMap(const std::string& tmxFile, const std::string& spawnId);
    void RequestSubMapTeleport(const std::string& tmxFile, const std::string& spawnId);
    void SealBossPortal(float x, float y, float w, float h);
    void CheckPortalCollisions(float dt);
    Vector2D GetSpawnPosition(const std::string& spawnId);
    void ExecuteSubMapLoad();
    bool RequestCheckpointActivation(const std::string& checkpointId, const Vector2D& spawnPosition);
    bool RequestCheckpointRespawn();
    bool IsCheckpointTransitionActive() const;
    const std::string& GetActiveCheckpointId() const { return activeCheckpointId_; }
    void SetActiveCheckpointId(const std::string& checkpointId);
    void SyncCheckpointEntities();



private:
    // ── Scene routing ─────────────────────────────────────────────────────────
    SceneID currentScene = SceneID::INTRO;
    bool    hasPendingSceneChange = false;
    SceneID pendingScene = SceneID::MAIN_MENU;

    void LoadScene(SceneID s);
    void ChangeScene(SceneID s);
    void UnloadCurrentScene();

    // ── Shared volume state (main menu + pause) ───────────────────────────────
    float musicVolume_ = 0.8f;
    float sfxVolume_ = 0.8f;
    int menuClickFxId = -1;
    bool isFullscreen_ = false;
    int checkpointFxId = -1;

    // =========================================================================
    //  MAIN MENU
    // =========================================================================
    bool showSettings_ = false;
    int  settingsCooldown_ = 0;

    // ── Settings in-place animation (main menu) ──────────────────────────────
    enum class SettingsAnimState {
        NONE,               // normal main menu buttons visible
        FADE_OUT_BUTTONS,   // fading out Play/Options/Exit
        FADE_IN_OPTIONS,    // fading in volume sliders + display mode + back
        OPTIONS_ACTIVE,     // options fully visible and interactive
        FADE_OUT_OPTIONS,   // fading out options
        FADE_IN_BUTTONS     // fading in Play/Options/Exit
    };
    SettingsAnimState settingsAnimState_ = SettingsAnimState::NONE;
    float settingsAnimTimer_ = 0.0f;
    float settingsButtonsAlpha_ = 1.0f;   // alpha for Play/Options/Exit
    float settingsOptionsAlpha_ = 0.0f;   // alpha for options controls
    int   windowModeIndex_ = 0;           // 0=Windowed, 1=Fullscreen, 2=Borderless

    // ── Cached button layout (set in LoadMainMenu, used in DrawSettingsInPlace)
    int   menuBtnX_ = 0;   // button X position
    int   menuBtnW_ = 0;   // button width

    void LoadMainMenu();
    void UnloadMainMenu();
    void UpdateMainMenu(float dt);
    void PostUpdateMainMenu();
    void HandleMainMenuUIEvents(UIElement* uiElement);
    void DrawSettingsInPlace(int winW, int winH);
    void SetSettingsPanelVisible(bool visible);

    enum class MenuAnimState {
        LOGO_FADE_IN,    // Background goes from black to blue, logo in center
        LOGO_HOLD,       // Hold logo in center briefly
        SLIDE_LOGO,      // Logo slides to top-left
        SLIDE_CHILD,     // Child slides in from right
        FADE_FRAGS_BTNS, // Fragments fade in, then buttons fade in
        IDLE             // Fully interactive
    };
    MenuAnimState menuAnimState_ = MenuAnimState::LOGO_FADE_IN;
    float         menuAnimTimer_ = 0.0f;

    // Button IDs — main menu
    static constexpr int BTN_PLAY = 1;
    static constexpr int BTN_SETTINGS = 2;
    static constexpr int BTN_EXIT = 3;
    static constexpr int BTN_SETTINGS_BACK = 10;
    static constexpr int BTN_MUSIC_UP = 11;
    static constexpr int BTN_MUSIC_DOWN = 12;
    static constexpr int BTN_SFX_UP = 13;
    static constexpr int BTN_SFX_DOWN = 14;
    static constexpr int BTN_FULLSCREEN = 15;

    // =========================================================================
    //  INTRO (Splash Logos)
    // =========================================================================
    enum class IntroPhase {
        CITM_FADEIN, CITM_HOLD, CITM_FADEOUT,
        STUDIO_FADEIN, STUDIO_HOLD, STUDIO_FADEOUT,
        DONE
    };
    IntroPhase   introPhase_ = IntroPhase::CITM_FADEIN;
    float        introTimer_ = 0.0f;
    SDL_Texture* texCitmLogo_ = nullptr;
    SDL_Texture* texStudioPlaceholder_ = nullptr;

    void LoadIntro();
    void UnloadIntro();
    void UpdateIntro(float dt);
    void DrawIntro();

    // =========================================================================
    //  INTRO CINEMATIC
    // =========================================================================
    enum class IntroCinState {
        PRE_VIDEO_LOADING,    // Pulsing "Loading..."
        FADING_OUT_TO_VIDEO,  // Fade to black, fixed "Loading..."
        PLAYING_VIDEO,        // Video starts with Fade In
        FADING_OUT_FROM_VIDEO,// Fade to black after video, fixed "Loading..."
        POST_VIDEO_LOADING    // Pulsing "Loading..." before title card
    };
    IntroCinState introCinState_ = IntroCinState::PRE_VIDEO_LOADING;

    void LoadIntroCinematic();
    void UnloadIntroCinematic();
    void UpdateIntroCinematic(float dt);
    void DrawLoadingText(bool pulsing, float timer);
    bool cinematicVideoStarted_ = false;
    float introLoadingDelay_ = 0.0f;
    bool introLoadingDelayActive_ = false;
    static constexpr float INTRO_PRE_VIDEO_DELAY = 1500.0f;
    static constexpr float INTRO_POST_VIDEO_DELAY = 2000.0f;

    // =========================================================================
    //  TUTORIAL TEXT CARD
    // =========================================================================
    void LoadTutorialTextCard();
    void UnloadTutorialTextCard();
    void UpdateTutorialTextCard(float dt);
    float tutorialTimer_ = 0.0f;
    SDL_Texture* texTutorialSeparator_ = nullptr;
    int fxTitleCardPt1_ = -1;
    int fxTitleCardPt2_ = -1;
    bool pt1Played_ = false;
    bool pt2Played_ = false;

    // ── Loading Screen ────────────────────────────────────────────────────────
    void LoadLoading();
    void UnloadLoading();
    void UpdateLoading(float dt);
    void DrawLoading();
    float loadingTimer_ = 0.0f;
    bool  mapLoadingFinished_ = false;
    int   targetLevelIndex_ = -1;

    // ── Level Data ────────────────────────────────────────────────────────────
    struct LevelInfo {
        std::string number;
        std::string name;
        std::string file;
    };
    std::vector<LevelInfo> levels_;
    int currentLevelIndex_ = 0;

    PuzzleManager* puzzleManager_ = nullptr;
    bool isPuzzleMap_ = false;
    bool puzzleTimeoutPending_ = false;

    PuzzleManager3* puzzleManager3_ = nullptr;
    bool isLvl3Map_ = false;   
    bool isLvl3Puzzle_ = false;
    bool isPuzzleMap3Lever_ = false;  
    bool isPuzzleMap3Buttons_ = false;

    // Automatic entry movement (Level 2+)
    bool  isAutoEntering_ = false;
    float autoEntryStartX_ = 0.0f;
    float autoEntryTargetX_ = 0.0f;
    float autoEntryProgress_ = 0.0f;

    // In-game intro sequence variables (Silksong-style phased transition)
    float inGameIntroTimer_ = 0.0f;
    bool inGameIntroActive_ = false;
    float introEntryDelay_ = 0.0f;          // Phase B: brief pause after zoom before hero wakes
    bool introEntryDelayActive_ = false;
    static constexpr float IN_GAME_INTRO_DURATION = 4500.0f;  // Phase A: cinematic zoom
    static constexpr float INTRO_ENTRY_DELAY = 800.0f;     // Phase B: Silksong-style entryDelay

    // =========================================================================
    //  GAMEPLAY + PAUSE MENU
    // =========================================================================
public:
    bool isPaused_ = false;
    bool showPauseOptions_ = false;
    bool isGameOver_ = false;
    float gameOverFadeTimer_ = 0.0f;

private:
    void LoadGameplay();
    void UnloadGameplay();
    void UpdateGameplay(float dt);
    void PostUpdateGameplay();
    bool UpdateCheckpointTransition(float dt);
    void ResolveCheckpointTransition();

    // ── Map switching (F1 / F2 / F3 / F4) ──────────────────────────────────────
    std::string currentMapFile_ = "MapTemplate.tmx";
    void LoadMap(int index);
    void LoadMap1();   // loads MapTemplate.tmx
    void LoadMap2();   // loads Map2.tmx
    void LoadMap3();   // loads Map3.tmx
    void LoadMap4();   // loads Map4.tmx

    void LoadPauseMenuButtons();
    void SetPauseMenuVisible(bool visible);
    void SetPauseOptionsPanelVisible(bool visible);
    void DrawPauseMenu();
    void DrawPauseOptionsPanel(int winW, int winH);
    void HandlePauseMenuUIEvents(UIElement* uiElement);
    bool HandleVolumeSliderInput(int trackX, int trackW, int row0Y, int row1Y, int row2Y);

    // Gamepad slider navigation (0 = music, 1 = sfx)
    int   optionsSliderSel_ = 0;
    float sliderRepeatTimer_ = 0.0f;

    std::string pendingLevelSpawnId_;
    bool hasPendingLevelSpawn_ = false;
    enum class CheckpointTransitionMode {
        NONE,
        ACTIVATE,
        RESPAWN
    };
    enum class CheckpointTransitionPhase {
        NONE,
        FADE_OUT,
        HOLD_BLACK,
        FADE_IN
    };
    CheckpointTransitionMode checkpointTransitionMode_ = CheckpointTransitionMode::NONE;
    CheckpointTransitionPhase checkpointTransitionPhase_ = CheckpointTransitionPhase::NONE;
    std::string activeCheckpointId_;
    std::string pendingCheckpointId_;
    Vector2D pendingCheckpointSpawn_;
    float checkpointBlackHoldTimer_ = 0.0f;
    bool checkpointNotifyAfterFade_ = false;

public:
    void SetGameOverVisible(bool visible);
    void TriggerEndGameCinematic();
    void ResetHealthUI(int health);
    bool IsEndGameTriggered() const { return endGameTriggered_; }
    float wakeUpNotifTimer_ = 0.0f;
    static constexpr float WAKEUP_NOTIF_DURATION = 4000.0f;
    float checkpointSaveTimer_ = 0.0f;
    float noCapeNotifTimer_ = 0.0f;
    static constexpr float CAPA_NOTIF_DURATION = 3000.0f;
    void ShowNoCapeNotification();

    float noBearNotifTimer_ = 0.0f;
    bool bearNotifHasStuffed_ = false;
    void ShowNoBearNotification(bool hasStuffedAnimal);

    float screenDamageTimer_ = 0.0f;
    void TriggerScreenDamage() { screenDamageTimer_ = 1000.0f; }

private:
    // Button IDs — pause menu
    static constexpr int BTN_PAUSE_CONTINUE = 20;
    static constexpr int BTN_PAUSE_OPTIONS = 21;
    static constexpr int BTN_PAUSE_SAVE = 22;
    static constexpr int BTN_PAUSE_MAINMENU = 23;
    static constexpr int BTN_PAUSE_QUIT = 24;
    static constexpr int BTN_PAUSE_OPT_MUSIC_UP = 25;
    static constexpr int BTN_PAUSE_OPT_MUSIC_DOWN = 26;
    static constexpr int BTN_PAUSE_OPT_SFX_UP = 27;
    static constexpr int BTN_PAUSE_OPT_SFX_DOWN = 28;
    static constexpr int BTN_PAUSE_OPT_BACK = 29;
    static constexpr int BTN_PAUSE_OPT_FULLSCREEN = 33;
    static constexpr int BTN_PAUSE_MAP = 32;
    static constexpr int BTN_GAMEOVER_MAINMENU = 30;
    static constexpr int BTN_GAMEOVER_CONTINUE = 31;

private:

    // ── Boss fight ────────────────────────────────────────────────────────────
    std::weak_ptr<Boss> activeBoss_;
    bool  isBossFightActive_   = false;

    // Boss death video — plays before the post-death map transition / music change
    bool        bossDeathVideoActive_  = false;
    bool        endGameVideoActive_    = false;
    bool        endGameFading_         = false;  // fade-out before final cinematic
    bool        endGameTriggered_      = false;  // true once end sequence fires; never reset
    bool        bossDeathFading_       = false;
    bool        bossDeathNeedsMapLoad_ = false;
    std::string bossDeathVideoPath_;
    std::string bossDeathMapTarget_;
    std::string bossDeathSpawnId_;
    float bossHealthDisplay_   = 1.0f;
    SDL_Texture* texBossBarEmpty_     = nullptr;
    SDL_Texture* texBossBarFull_      = nullptr;
    SDL_Texture* texBossBarIndicator_ = nullptr;

    void UpdateBossFight(float dt);
    void DrawBossHUD(int winW, int winH);

    // Health HUD (supports up to 6 health slots depending on level/fase)
    static constexpr int MAX_HEALTH_SLOTS = 6;
    SDL_Texture* texHealth_[MAX_HEALTH_SLOTS] = {};
    Animation    animHealth_[MAX_HEALTH_SLOTS];
    int          healthSlotCount_ = 3; // number of slots loaded for current level
    int currentHealthUI_ = 3;
    int activeHealthAnim_ = 0; // 0=None, otherwise legacy (unused)
    SDL_Texture* texGameOver_ = nullptr;

    // Blanket ability HUD icon
    SDL_Texture* texBlanketActive_ = nullptr;   // full opacity — shown while hiding
    SDL_Texture* texBlanketInactive_ = nullptr;  // low opacity — shown when not hiding

    // Cape collectible (AS_capa.png)
    SDL_Texture* texCapaCollectible_ = nullptr;
    PhysBody* capaBody_ = nullptr;
    float capaX_ = 300.0f;
    float capaY_ = 600.0f;
    bool  capaCollected_ = false;
    float capaFloatTimer_ = 0.0f; // floating animation timer
    float capaNotifTimer_ = 0.0f; // cape pickup notification timer

    // Slingshot (tirachinas) collectible
    SDL_Texture* texSlingshotCollectible_ = nullptr;
    float slingshotX_ = 300.0f;
    float slingshotY_ = 600.0f;
    bool  slingshotCollected_ = false;
    float slingshotFloatTimer_ = 0.0f;
    float slingshotNotifTimer_ = 0.0f;
    static constexpr float SLINGSHOT_NOTIF_DURATION = 3000.0f;

    // Stuffed Animal (oso) collectible
    SDL_Texture* texStuffedAnimalCollectible_ = nullptr;
    float stuffedAnimalX_ = 300.0f;
    float stuffedAnimalY_ = 600.0f;
    bool  stuffedAnimalCollected_ = false;
    float stuffedAnimalFloatTimer_ = 0.0f;
    float stuffedAnimalNotifTimer_ = 0.0f;
    static constexpr float STUFFED_ANIMAL_NOTIF_DURATION = 3000.0f;

    // Minimap Ornate Frame Border
    SDL_Texture* texMinimapFrame_ = nullptr;

    // Game Over Menu Assets
    SDL_Texture* texGameOverBg_ = nullptr;
    SDL_Texture* texGameOverBtn_ = nullptr;
    SDL_Texture* texGameOverKid_ = nullptr;
    SDL_Texture* texGameOverFrag1_ = nullptr;
    SDL_Texture* texGameOverFrag3_ = nullptr;
    SDL_Texture* texGameOverFrag4_ = nullptr;
    SDL_Texture* texGameOverFrag5_ = nullptr;
    SDL_Texture* texGameOverScreenBase_ = nullptr;
    SDL_Texture* texGameOverText_ = nullptr;

    // Checkpoint notification
    SDL_Texture* texCheckpointSaved_ = nullptr;
    SDL_Texture* texNoCapeNotif_ = nullptr;
    SDL_Texture* texDamageVignette_ = nullptr;

    // Pause menu textures
    SDL_Texture* texPauseBackground_ = nullptr;
    SDL_Texture* texPauseButtonWhite_ = nullptr;
    SDL_Texture* texPauseButtonBlack_ = nullptr;
    SDL_Texture* texButtonFragmented_ = nullptr;

    // ── Map viewer ────────────────────────────────────────────────────────────
    bool  showMapViewer_ = false;
    float mapViewOffsetX_ = 0.0f;
    float mapViewOffsetY_ = 0.0f;
    float mapViewZoom_ = 0.2f;
    bool  mapViewDragging_ = false;
    float mapViewDragStartX_ = 0.0f;
    float mapViewDragStartY_ = 0.0f;
    float mapViewDragOriginX_ = 0.0f;
    float mapViewDragOriginY_ = 0.0f;

    std::map<std::string, std::set<std::pair<int, int>>> visitedCells_;
    std::map<std::string, bool> mapRevealed_;

    void DrawMapViewer(int winW, int winH);
    void DrawBottomFog(int winW, int winH);

    // ── Inventory ─────────────────────────────────────────────────────────────
    bool  showInventory_ = false;
    int   inventorySel_ = -1;
    void DrawInventory(int winW, int winH);

    SDL_Texture* texInventoryBg_ = nullptr;
    SDL_Texture* texAbilitiesLocked_ = nullptr;
    SDL_Texture* texAbilitiesBlanket_ = nullptr;
    SDL_Texture* texAbilitiesBlanketSling_ = nullptr;
    SDL_Texture* texAbilitiesAll_ = nullptr;

    SDL_Texture* texAbilityBlanketIcon_ = nullptr;
    SDL_Texture* texAbilitySlingshotIcon_ = nullptr;
    SDL_Texture* texAbilityStuffedAnimalIcon_ = nullptr;
    SDL_Texture* texAbilityAnchorIcon_ = nullptr;

    // ── Memories UI ───────────────────────────────────────────────────────────
    SDL_Texture* texMemoria1Base_ = nullptr;
    SDL_Texture* texMemoria1N1_ = nullptr;
    SDL_Texture* texMemoria1N2_ = nullptr;

    SDL_Texture* texMemoria2Base_ = nullptr;
    SDL_Texture* texMemoria2N1_ = nullptr;
    SDL_Texture* texMemoria2N2_ = nullptr;

    SDL_Texture* texMemoria3Base_ = nullptr;
    SDL_Texture* texMemoria3N1_ = nullptr;
    SDL_Texture* texMemoria3N2_ = nullptr;
    SDL_Texture* texMemoria3N3_ = nullptr;

    // Fullscreen memories
    SDL_Texture* texMemoriaFrameFullScreen_ = nullptr;
    SDL_Texture* texMemoria1Full1_ = nullptr;
    SDL_Texture* texMemoria1Full2_ = nullptr;
    SDL_Texture* texMemoria2Full1_ = nullptr;
    SDL_Texture* texMemoria2Full2_ = nullptr;
    SDL_Texture* texMemoria3Full1_ = nullptr;
    SDL_Texture* texMemoria3Full2_ = nullptr;
    SDL_Texture* texMemoria3Full3_ = nullptr;

    // Hover fade states
    float memoryHoverTimers_[3] = { 0.0f, 0.0f, 0.0f };

    // Fullscreen state
    bool showMemoryViewer_ = false;
    int activeMemoryIndex_ = -1; // 0, 1, or 2
    int activeMemoryPage_ = 0;   // index of active page

    // ── Main menu textures ────────────────────────────────────────────────────
    SDL_Texture* texMenuLogo_ = nullptr;
    SDL_Texture* texMenuChild_ = nullptr;
    SDL_Texture* texMenuButton_ = nullptr;
    SDL_Texture* texSettingsBase_ = nullptr;

    std::shared_ptr<UIElement> btnPlay_;
    std::shared_ptr<UIElement> btnSettings_;
    std::shared_ptr<UIElement> btnExit_;
    std::shared_ptr<UIElement> btnBack_;

    // ── Fade orchestration ────────────────────────────────────────────────────
    bool    waitingForFade_ = false;
    SceneID fadeTargetScene_ = SceneID::MAIN_MENU;

    // ── Floating fragments decoration ─────────────────────────────────────────
    static constexpr int NUM_FRAGMENTS = 5;

    struct MenuFragment {
        SDL_Texture* tex = nullptr;
        float x, y;            // base position (randomized)
        float w, h;            // drawn size
        float floatSpeed;       // oscillation speed (rad/s)
        float floatAmplitude;   // oscillation range (px)
        float floatPhase;       // initial phase offset
        float driftX;           // horizontal micro-drift speed
        float driftPhase;       // horizontal drift phase
        float rotation;         // current rotation angle
        float rotSpeed;         // degrees per second
        bool  inFront;          // drawn in front of the character?
        Uint8 alpha;            // alpha
    };

    MenuFragment fragments_[NUM_FRAGMENTS];
    float fragmentTime_ = 0.0f;
    bool  fragmentsInited_ = false;

    void InitFragments(int winW, int winH, int childX, int childW);
    void DrawFragments(bool front, int winW, int winH);

private:

    int konamiIndex = 0;
    bool isKonamiActive = false;

    const int konamiSequence[10] = {
        SDL_SCANCODE_UP, SDL_SCANCODE_UP,
        SDL_SCANCODE_DOWN, SDL_SCANCODE_DOWN,
        SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT,
        SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT,
        SDL_SCANCODE_B, SDL_SCANCODE_A
    };
};