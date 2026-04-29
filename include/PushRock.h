#pragma once

#include "Entity.h"
#include <SDL3/SDL.h>

class PushRock : public Entity
{
public:
    PushRock();
    virtual ~PushRock();

    bool Awake() override;
    bool Start() override;
    bool Update(float dt) override;
    bool CleanUp() override;

    void OnCollision(PhysBody* physA, PhysBody* physB) override;
    void OnCollisionEnd(PhysBody* physA, PhysBody* physB) override;

    // Rock dimensions set from Tiled object
    float rockWidth = 64.0f;
    float rockHeight = 64.0f;

private:
    SDL_Texture* texture = nullptr;
    int texW = 0, texH = 0;
    PhysBody* pbody = nullptr;
};
