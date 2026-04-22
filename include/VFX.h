#pragma once

#include "Entity.h"
#include "Animation.h"
#include <SDL3/SDL.h>

class VFX : public Entity
{
public:
    VFX();
    virtual ~VFX();

    bool Start() override;
    bool Update(float dt) override;
    bool CleanUp() override;

    void SetTexture(const char* path, int frames, int frameW, int frameH, float speed, float angle = 0.0f, float scale = 1.0f);

private:
    SDL_Texture* texture = nullptr;
    Animation anim;
    int frameW = 0, frameH = 0;
    float angle = 0.0f;
    float drawScale = 1.0f;
};
