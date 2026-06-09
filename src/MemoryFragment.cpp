#include "MemoryFragment.h"
#include "Engine.h"
#include "Textures.h"
#include "Render.h"
#include "Physics.h"
#include "Scene.h"
#include "Cinematics.h"
#include "Log.h"
#include <cmath>

MemoryFragment::MemoryFragment() : Entity(EntityType::MEMORY_FRAGMENT)
{
    name = "MemoryFragment";
}

MemoryFragment::~MemoryFragment() {}

bool MemoryFragment::Start()
{
    texture_ = Engine::GetInstance().textures->Load(
        "assets/textures/spritesheets/SS_Fragment_Collectible.png");

    for (int row = 0; row < ROWS; ++row)
        for (int col = 0; col < COLS; ++col)
            anim_.AddFrame({ col * FRAME_W, row * FRAME_H, FRAME_W, FRAME_H }, (int)MS_PER_FRAME);
    anim_.SetLoop(true);

    return true;
}

bool MemoryFragment::CleanUp()
{
    if (texture_) { Engine::GetInstance().textures->UnLoad(texture_); texture_ = nullptr; }
    return true;
}

bool MemoryFragment::Update(float dt)
{
    if (pendingToDelete) return true;

    // Video playing — wait for it to finish, then delete
    if (collected_)
    {
        if (!Engine::GetInstance().cinematics->IsPlaying())
            pendingToDelete = true;
        return true;
    }

    // Fading to black — wait for fade to complete, then launch video
    if (fading_)
    {
        if (!Engine::GetInstance().render->IsFadingOut())
        {
            fading_    = false;
            collected_ = true;
            if (!videoPath_.empty())
            {
                Engine::GetInstance().cinematics->PlayVideo(videoPath_.c_str());
                Engine::GetInstance().render->StartFade(FadeDirection::FADE_IN, 1.0f);
            }
            else
                LOG("MemoryFragment: no video_path set — fragment collected silently");
        }
        return true;
    }

    anim_.Update(dt);

    // Proximity check — start fade to black before playing video
    auto& scn = *Engine::GetInstance().scene;
    if (scn.player && scn.player->pbody)
    {
        int px, py;
        scn.player->pbody->GetPosition(px, py);
        float dx = (float)px - position.getX();
        float dy = (float)py - position.getY();
        if (sqrtf(dx * dx + dy * dy) < collectRange_)
        {
            fading_ = true;
            Engine::GetInstance().render->StartCameraShake(600.0f, 15.0f);
            Engine::GetInstance().render->StartFade(FadeDirection::FADE_OUT, 600.0f);
            return true;
        }
    }

    Draw();
    return true;
}

void MemoryFragment::Draw()
{
    if (!texture_) return;
    auto& render = *Engine::GetInstance().render;

    const SDL_Rect& frame = anim_.GetCurrentFrame();
    int drawX = (int)(position.getX() - frame.w * scale_ * 0.5f);
    int drawY = (int)(position.getY() - frame.h * scale_ * 0.5f);

    render.DrawTexture(texture_, drawX, drawY, &frame, 1.0f, 0.0, INT_MAX, INT_MAX, SDL_FLIP_NONE, scale_);
}
