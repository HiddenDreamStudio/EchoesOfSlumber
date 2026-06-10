#pragma once
#include "Entity.h"
#include "Platform.h"
#include <string>
#include <memory>
#include <SDL3/SDL.h>

class Lever : public Entity {
public:
    Lever();
    virtual ~Lever();

    bool Awake() override;
    bool Start() override;
    bool Update(float dt) override;
    bool CleanUp() override;

    void OnCollision(PhysBody* physA, PhysBody* physB) override;
    void OnCollisionEnd(PhysBody* physA, PhysBody* physB) override;

    std::string targetPlatformName = "";
    bool isActivated = false;

    // Puzzle lock
    bool isLockedByPuzzle = false;
    bool isPressurePlate = false;

    // Visual size variables (Must be public for Map.cpp)
    int texW = 64;
    int texH = 64;

    SDL_Texture* texNormal = nullptr;
    SDL_Texture* texActive = nullptr;

    PhysBody* pbody = nullptr;

private:

    bool playerInRange = false;
    int objectsOnTop = 0;
    std::shared_ptr<Platform> targetPlatform = nullptr;
};