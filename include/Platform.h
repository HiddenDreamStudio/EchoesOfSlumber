#pragma once
#include "Entity.h"
#include <vector>
#include <SDL3/SDL.h>

class Platform : public Entity
{
public:
    Platform();
    virtual ~Platform();

    bool Awake() override;
    bool Start() override;
    bool Update(float dt) override;
    bool CleanUp() override;

    void AddWaypoint(Vector2D point);

    float speed = 2.0f;
    bool triggerOnPlayer = false;
    bool playerOnTop = false;
    bool requireLever = false;
    bool activatedByLever = false;

    void OnCollision(PhysBody* physA, PhysBody* physB) override;
    void OnCollisionEnd(PhysBody* physA, PhysBody* physB) override;

    std::vector<Vector2D> waypoints;

    std::string texturePath = "EchoesOfSlumber/assets/textures/TL_environment/plat_4tiles.png";
    int texW = 192;
    int texH = 64;

    std::string platformName = "";

private:
    int currentWaypoint = 0;
    int direction = 1;
    int bx = 0, by = 0;

    SDL_Texture* texture = nullptr;
    PhysBody* pbody = nullptr;
};