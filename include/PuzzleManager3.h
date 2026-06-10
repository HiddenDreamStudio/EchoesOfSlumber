#pragma once
#include <SDL3/SDL.h>
#include <vector>
#include <string>

struct LeverData3 {
    SDL_FRect worldRect = { 0.0f, 0.0f, 0.0f, 0.0f };
    bool activated = false;
};

struct ButtonData3 {
    SDL_FRect worldRect = { 0.0f, 0.0f, 0.0f, 0.0f };
    int requiredClicks = 1;
    int currentClicks = 0;
    bool completed = false;
};

class PuzzleManager3 {
public:
    PuzzleManager3();
    ~PuzzleManager3();

    void Init(SDL_Renderer* renderer);

    void LoadLever(const LeverData3& lever, SDL_FRect blockedPortalRect);

    void LoadButtons(const std::vector<ButtonData3>& buttons);

    void UpdateLever(float dt, SDL_FRect playerRect, bool keyP_pressed);
    void UpdateButtons(float dt, SDL_FRect playerRect,
        float mouseWorldX, float mouseWorldY, bool mouseClicked);

    void RenderLever(SDL_Renderer* renderer, float cameraX, float cameraY);
    void RenderButtons(SDL_Renderer* renderer, float cameraX, float cameraY);
    void RenderFlash(SDL_Renderer* renderer, int winW, int winH);

    bool IsLeverActivated()   const { return lever_.activated; }
    bool AreBothButtonsDone() const { return bothButtonsDone_; }

    bool isInLeverRoom = false;
    bool isInButtonRoom = false;

    void Reset();
    SDL_FRect GetBlockedPortalRect() const { return blockedPortalRect_; }
    void LoadExtraPortals(SDL_FRect portalA, SDL_FRect portalB);
    SDL_FRect GetPortalARect() const { return blockedPortalA_; }
    SDL_FRect GetPortalBRect() const { return blockedPortalB_; }

private:
    SDL_Renderer* renderer_ = nullptr;

    LeverData3    lever_;
    SDL_FRect     blockedPortalRect_ = { 0, 0, 0, 0 };
    SDL_FRect blockedPortalA_ = { 0, 0, 0, 0 };
    SDL_FRect blockedPortalB_ = { 0, 0, 0, 0 };
    bool portalAUnlocked_ = false;
    bool portalBUnlocked_ = false;

    std::vector<ButtonData3> buttons_;
    bool bothButtonsDone_ = false;

    float flashTimer_ = 0.0f;
    bool  flashRed_ = false;
    bool  flashActive_ = false;

    float inactivityTimer_[2] = { 0.0f, 0.0f };
    bool  buttonInProgress_[2] = { false, false };

    bool playerNearLever_ = false;

    float openTextTimer_ = 0.0f;
    static constexpr float OPEN_TEXT_DURATION = 2000.0f;

    SDL_Texture* texLeverNormal_ = nullptr;
    SDL_Texture* texLeverActive_ = nullptr;
    SDL_Texture* texButtonNormal_ = nullptr;
    SDL_Texture* texButtonPressed_ = nullptr;

    static constexpr float INTERACT_RADIUS = 80.0f;
    static constexpr float FLASH_DURATION = 1000.0f;
    static constexpr float INACTIVITY_LIMIT = 3000.0f;
};