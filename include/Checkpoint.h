#pragma once

#include "Entity.h"
#include <SDL3/SDL.h>

class Checkpoint : public Entity
{
public:
    Checkpoint();
    virtual ~Checkpoint();

    bool Awake() override;
    bool Start() override;
    bool Update(float dt) override;
    bool CleanUp() override;

    void OnCollision(PhysBody* physA, PhysBody* physB) override;

private:
    SDL_Texture* texture = nullptr;
    int texW = 0, texH = 0;
    PhysBody* pbody = nullptr;
    bool activated = false;
    
    // Animation/Visuals
    float floatTimer = 0.0f;
};
