#pragma once

#include "Entity.h"
#include <SDL3/SDL.h>

class Box : public Entity
{
public:
    Box();
    virtual ~Box();

    bool Awake() override;
    bool Start() override;
    bool Update(float dt) override;
    bool CleanUp() override;

private:
    SDL_Texture* texture = nullptr;
    int texW = 0, texH = 0;
    PhysBody* pbody = nullptr;
};
