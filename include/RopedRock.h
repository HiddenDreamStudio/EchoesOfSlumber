#pragma once
#include "Entity.h"
#include "Physics.h"
#include <SDL3/SDL.h>

enum class RopedRockState { HANGING, FALLING, RESPAWNING };

class RopedRock : public Entity
{
public:
    RopedRock();
    ~RopedRock();

    bool Start()          override;
    bool Update(float dt) override;
    bool CleanUp()        override;

    void OnCollision(PhysBody* physA, PhysBody* physB) override;

    // Called by Map.cpp after spawning, before Start() — sets per-instance rope length
    void SetRopeLength(float length) { ropeLength_ = length; }

private:
    void SpawnBodies();
    void Draw();

    RopedRockState state_ = RopedRockState::HANGING;

    PhysBody* ropeSensor_ = nullptr;
    PhysBody* rockBody_   = nullptr;

    float anchorX_ = 0.0f;
    float anchorY_ = 0.0f;

    float ropeLength_ = 200.0f; // overridable per-instance via Tiled "ropeLength" property

    static constexpr int   ROPE_W         = 20;
    static constexpr int   ROCK_W         = 64;
    static constexpr int   ROCK_H         = 82;
    static constexpr float RESPAWN_DELAY  = 4000.0f;
    float respawnTimer_ = 0.0f;

    SDL_Texture* texRope_ = nullptr;
    SDL_Texture* texRock_ = nullptr;
};
