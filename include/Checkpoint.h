#pragma once

#include "Entity.h"
#include "Vector2D.h"
#include <SDL3/SDL.h>
#include <string>

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
    void OnCollisionEnd(PhysBody* physA, PhysBody* physB) override;

    void Configure(const std::string& id, float width, float height);
    void SetActivated(bool value) { activated = value; }
    bool IsActivated() const { return activated; }
    const std::string& GetCheckpointId() const { return checkpointId_; }
    Vector2D GetSpawnPosition() const { return position; }

private:
    SDL_Texture* textureOff_ = nullptr;
    SDL_Texture* textureOn_ = nullptr;
    int texW = 0, texH = 0;
    PhysBody* pbody = nullptr;
    bool activated = false;
    bool playerInRange_ = false;
    std::string checkpointId_;
    float objectW_ = 64.0f;
    float objectH_ = 64.0f;
    
    // Animation/Visuals
    float floatTimer = 0.0f;
};
