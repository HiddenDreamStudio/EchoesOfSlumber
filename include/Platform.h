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

    std::vector<Vector2D> waypoints;

    std::string texturePath = "assets/textures/TL_environment/plat_4tiles.png";
    int texW = 256;
    int texH = 64;

private:
    int currentWaypoint = 0;
    int direction = 1;

    SDL_Texture* texture = nullptr;
    PhysBody* pbody = nullptr;
};