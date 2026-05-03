#include "VFX.h"
#include "Engine.h"
#include "Textures.h"
#include "Render.h"
#include "Window.h"
#include "Log.h"

VFX::VFX() : Entity(EntityType::VFX)
{
    name = "vfx";
}

VFX::~VFX() {}

bool VFX::Start() {
    return true;
}

void VFX::SetTexture(const char* path, int frames, int w, int h, float speed, float angle, float scale) {
    texture = Engine::GetInstance().textures->Load(path);
    frameW = w;
    frameH = h;
    this->angle = angle;
    this->drawScale = scale;
    
    for (int i = 0; i < frames; ++i) {
        anim.AddFrame({ i * frameW, 0, frameW, frameH }, (int)(speed * 1000));
    }
    anim.SetLoop(false);
}

bool VFX::Update(float dt) {
    if (!active) return true;

    anim.Update(dt);
    if (anim.HasFinishedOnce()) {
        Destroy();
        return true;
    }

    const SDL_Rect& rect = anim.GetCurrentFrame();
    
    // Center the effect based on the scaled dimensions
    int drawX = (int)(position.getX() - (float)frameW * drawScale / 2.0f);
    int drawY = (int)(position.getY() - (float)frameH * drawScale / 2.0f);

    Engine::GetInstance().render->DrawTexture(texture, 
        drawX, drawY, 
        &rect, 1.0f, (double)angle, INT_MAX, INT_MAX, SDL_FLIP_NONE, drawScale);

    return true;
}

bool VFX::CleanUp() {
    if (texture) Engine::GetInstance().textures->UnLoad(texture);
    return true;
}
