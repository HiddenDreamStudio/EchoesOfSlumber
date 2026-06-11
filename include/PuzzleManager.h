#pragma once
#include <SDL3/SDL.h>
#include <vector>
#include <string>

struct b2WorldId;
class Player;

struct PuzzleFigure {
    std::string name;
    SDL_FRect worldRect = { 0.0f, 0.0f, 0.0f, 0.0f };   
    bool collected = false;
};

class PuzzleManager {
public:
    PuzzleManager();
    ~PuzzleManager();

    void Init(SDL_Renderer* renderer);

    void LoadFiguresFromObjects(const std::vector<PuzzleFigure>& figures, SDL_FRect exitPortalRect);

    void Update(float dt, SDL_FRect playerRect, float playerScreenX, float playerScreenY);
    void Render(SDL_Renderer* renderer, float cameraX, float cameraY);

    void Reset();
    bool IsPuzzleActive() const { return puzzleActive_; }
    bool IsPortalOpen() const { return portalOpen_; }

    bool IsTimedOut() const { return timedOut_; }
    SDL_FRect GetExitPortalRect() const { return exitPortalRect_; }

private:
    void DrawVignette(SDL_Renderer* renderer, float playerScreenX, float playerScreenY);
    void DrawBlockedPortal(SDL_Renderer* renderer, float cameraX, float cameraY);
    void DrawFigures(SDL_Renderer* renderer, float cameraX, float cameraY);
    void DrawTimer(SDL_Renderer* renderer);

    std::vector<PuzzleFigure> figures_;
    int collectedCount_ = 0;

    float timer_ = 30.0f;
    bool timerStarted_ = false;
    bool puzzleActive_ = true; 
    bool portalOpen_ = false;
    bool timedOut_ = false;

    float vignetteAlpha_ = 200.0f;  
    float blinkTimer_ = 0.0f;
    bool blinking_ = false;

    float introTimer_ = 0.0f;
    float currentRadius_ = 2000.0f;

    float playerScrX_ = 0.0f;
    float playerScrY_ = 0.0f;

    SDL_FRect exitPortalRect_ = { 0,0,0,0 };
    float playerWorldX_ = 0.0f;
    float playerWorldY_ = 0.0f;
    SDL_Renderer* renderer_ = nullptr;
};