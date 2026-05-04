#pragma once

#include "Entity.h"
#include "Vector2D.h"
#include <vector>
#include <string>
#include <SDL3/SDL.h>

class Platform : public Entity {
public:
    Platform();
    virtual ~Platform();

    bool Awake() override;
    bool Start() override;
    bool Update(float dt) override;
    bool CleanUp() override;

    // Helper to parse a semicolon-separated string from Tiled (e.g., "100,200;300,200")
    void ParsePath(const std::string& pathStr);
    void SetSpeed(float s) { speed_ = s; }

public:
    int width = 64;
    int height = 16;

private:
    SDL_Texture* texture = nullptr;
    PhysBody* pbody = nullptr;

    std::vector<Vector2D> waypoints_;
    int currentWaypoint_ = 0;
    float speed_ = 2.0f;

    // Threshold to consider waypoint reached
    static constexpr float WAYPOINT_TOLERANCE = 2.0f;
};