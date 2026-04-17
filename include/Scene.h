#pragma once
#include "Module.h"
#include "Player.h"
#include "UIManager.h"
#include "UIElement.h"

enum class SceneID {
    INTRO,
    MAIN_MENU,
    INTRO_CINEMATIC,
    GAMEPLAY
};

class Scene : public Module
{
public:
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
    std::string GetTilePosDebug() const { return ""; }

    std::shared_ptr<Player> player = nullptr;

private:
    // ?? Scene routing ?????????????????????????????????????????????????????????
    SceneID currentScene = SceneID::INTRO;
    bool    hasPendingSceneChange = false;
    SceneID pendingScene = SceneID::MAIN_MENU;

    void LoadScene(SceneID s);
    void ChangeScene(SceneID s);
    void UnloadCurrentScene();

    // ?? Shared volume state (main menu + pause) ????????????????????????????????
    float musicVolume_ = 0.8f;
    float sfxVolume_ = 0.8f;

    // ????????????????????????????????????????????????????????????????????????
    //  MAIN MENU
    // ????????????????????????????????????????????????????????????????????????
    bool showSettings_ = false;
    int  settingsCooldown_ = 0;

    void LoadMainMenu();
    void UnloadMainMenu();
    void UpdateMainMenu(float dt);
    void PostUpdateMainMenu();
    void HandleMainMenuUIEvents(UIElement* uiElement);
    void DrawSettingsPanel(int winW, int winH);
    void SetSettingsPanelVisible(bool visible);

    // Button IDs � main menu
    static constexpr int BTN_PLAY = 1;
    static constexpr int BTN_SETTINGS = 2;
    static constexpr int BTN_EXIT = 3;
    static constexpr int BTN_SETTINGS_BACK = 10;
    static constexpr int BTN_MUSIC_UP = 11;
    static constexpr int BTN_MUSIC_DOWN = 12;
    static constexpr int BTN_SFX_UP = 13;
    static constexpr int BTN_SFX_DOWN = 14;

    // ????????????????????????????????????????????????????????????????????????
    //  INTRO CINEMATIC
    // ????????????????????????????????????????????????????????????????????????
    // INTRO (Splash Logos)
    enum class IntroPhase {
        CITM_FADEIN, CITM_HOLD, CITM_FADEOUT,
        STUDIO_FADEIN, STUDIO_HOLD, STUDIO_FADEOUT,
        DONE
    };
    IntroPhase introPhase_ = IntroPhase::CITM_FADEIN;
    float introTimer_ = 0.0f;
    SDL_Texture* texCitmLogo_ = nullptr;
    SDL_Texture* texStudioPlaceholder_ = nullptr;

    void LoadIntro();
    void UnloadIntro();
    void UpdateIntro(float dt);
    void DrawIntro();

    // INTRO CINEMATIC
    void LoadIntroCinematic();
    void UnloadIntroCinematic();
    void UpdateIntroCinematic(float dt);

    // ????????????????????????????????????????????????????????????????????????
    //  GAMEPLAY + PAUSE MENU
    // ????????????????????????????????????????????????????????????????????????
    public:
    bool isPaused_ = false;
    bool showPauseOptions_ = false;
private:

    void LoadGameplay();
    void UnloadGameplay();
    void UpdateGameplay(float dt);
    void PostUpdateGameplay();

    void LoadPauseMenuButtons();
    void SetPauseMenuVisible(bool visible);
    void SetPauseOptionsPanelVisible(bool visible);
    void DrawPauseMenu();
    void DrawPauseOptionsPanel(int winW, int winH);
    void HandlePauseMenuUIEvents(UIElement* uiElement);

    // Button IDs � pause menu
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

    // Main menu textures
    SDL_Texture* texMenuLogo_ = nullptr;
    SDL_Texture* texMenuChild_ = nullptr;
    SDL_Texture* texMenuButton_ = nullptr;

    // Fade orchestration
    bool waitingForFade_ = false;
    SceneID fadeTargetScene_ = SceneID::MAIN_MENU;

    // ── Floating fragments decoration ────────────────────────────────────────
    static constexpr int NUM_FRAGMENTS = 5;

    struct MenuFragment {
        SDL_Texture* tex = nullptr;
        float x, y;               // base position (randomized)
        float w, h;               // drawn size
        float floatSpeed;          // oscillation speed (rad/s)
        float floatAmplitude;      // oscillation range (px)
        float floatPhase;          // initial phase offset
        float driftX;              // horizontal micro-drift speed
        float driftPhase;          // horizontal drift phase
        float rotation;            // current rotation angle
        float rotSpeed;            // degrees per second
        bool  inFront;             // drawn in front of the character?
        Uint8 alpha;               // alpha (lower for in-front = blur/ghostly)
    };

    MenuFragment fragments_[NUM_FRAGMENTS];
    float fragmentTime_ = 0.0f;
    bool  fragmentsInited_ = false;

    void InitFragments(int winW, int winH, int childX, int childW);
    void DrawFragments(bool front, int winW, int winH);
};