#pragma once
#include "Entity.h"
#include "Animation.h"
#include <SDL3/SDL.h>

enum class AntagonistState { IDLE, DISAPPEARING };

class Antagonist : public Entity
{
public:
    Antagonist();
    ~Antagonist();

    bool Start()          override;
    bool Update(float dt) override;
    bool CleanUp()        override;

    void SetAppearRange(float r) { appearRange_ = r; }
    void SetAlpha(int a)         { alpha_ = (Uint8)a; }
    void SetScale(float s)       { scale_ = s; }

private:
    void Draw();

    AntagonistState state_ = AntagonistState::IDLE;

    SDL_Texture* texIdle_      = nullptr;
    SDL_Texture* texDisappear_ = nullptr;
    Animation    animIdle_;
    Animation    animDisappear_;

    float  appearRange_ = 450.0f;
    Uint8  alpha_       = 230;
    // Scale applied to the disappear animation (256px frames).
    // The idle animation (512px frames) uses scale_*0.5 internally so both
    // animations render at the same on-screen size.
    float  scale_       = 0.6f;

    static constexpr int   IDLE_FRAMES    = 9;
    static constexpr int   IDLE_FRAME_W   = 512;
    static constexpr int   IDLE_FRAME_H   = 512;
    static constexpr int   DISAPP_FRAMES  = 13;
    static constexpr int   DISAPP_FRAME_W = 256;
    static constexpr int   DISAPP_FRAME_H = 256;
    static constexpr float MS_PER_FRAME   = 100.0f;
    // Pixels to push the sprite down after the ground snap — compensates for
    // transparent space at the bottom of the sprite frames.  Increase if it
    // still floats; decrease if it sinks into the floor.
    static constexpr float FOOT_OFFSET   = 25.0f;
};
