#pragma once
#include "Entity.h"
#include "Animation.h"
#include <SDL3/SDL.h>
#include <string>

class MemoryFragment : public Entity
{
public:
    MemoryFragment();
    ~MemoryFragment();

    bool Start()          override;
    bool Update(float dt) override;
    bool CleanUp()        override;

    void SetVideoPath(const std::string& path) { videoPath_ = path; }
    void SetCollectRange(float r)              { collectRange_ = r; }
    void SetScale(float s)                     { scale_ = s; }
    void SetFragmentId(int id)                 { fragmentId_ = id; }

private:
    void Draw();

    bool collected_ = false;
    bool fading_    = false;

    SDL_Texture* texture_ = nullptr;
    Animation    anim_;

    std::string videoPath_    = "";
    float       collectRange_ = 100.0f;
    float       scale_        = 0.5f;   // 256 * 0.5 = 128px on screen
    int         fragmentId_   = -1;

    // SS_Fragment_Collectible.png: 1536x1024 → 6 cols × 4 rows of 256×256 frames
    static constexpr int   COLS         = 6;
    static constexpr int   ROWS         = 4;
    static constexpr int   FRAME_W      = 256;
    static constexpr int   FRAME_H      = 256;
    static constexpr float MS_PER_FRAME = 80.0f;
};
