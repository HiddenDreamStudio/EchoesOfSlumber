#pragma once
#include "Entity.h"
#include <SDL3/SDL.h>
#include <memory>
#include <string>
#include <list>

class Platform;
class Lever;
class PushRock;

class PuzzleManager2 {
public:
    PuzzleManager2();
    ~PuzzleManager2();

    void Init(SDL_Renderer* renderer);
    void Update(float dt, const std::list<std::shared_ptr<Entity>>& entities);
    void Render(SDL_Renderer* renderer, float cameraX, float cameraY);
    void Reset();

    bool IsPuzzleActive() const { return puzzleActive_; }
    bool IsPortalOpen() const { return portalOpen_; }

private:
    std::shared_ptr<Entity> FindEntityByName(const std::list<std::shared_ptr<Entity>>& entities, const std::string& name);
    void ReplaceLeverTexture(std::shared_ptr<Lever> lever);

    SDL_Renderer* renderer_ = nullptr;

    bool puzzleActive_ = true;
    bool portalOpen_ = false;

    bool phase1Completed = false;
    bool phase2Completed = false;
    bool phase3LeftCompleted = false;
    bool phase3RightCompleted = false;

    std::shared_ptr<Lever> btnPhase1;
    std::shared_ptr<Lever> btnPhase2;
    std::shared_ptr<Platform> platElevator;
    std::shared_ptr<Platform> platButton;
    std::shared_ptr<Lever> btnLeftPhase3;
    std::shared_ptr<Lever> btnRightPhase3;
    std::shared_ptr<PushRock> dropBoxLeft;
    std::shared_ptr<PushRock> dropBoxRight;
    std::shared_ptr<Lever> btnStaticLeft;
    std::shared_ptr<Lever> btnStaticRight;
    std::shared_ptr<Entity> finalBlocker;
};