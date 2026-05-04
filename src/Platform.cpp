#include "Platform.h"
#include "Engine.h"
#include "Textures.h"
#include "Render.h"
#include "Physics.h"
#include "Scene.h"
#include "Log.h"
#include <sstream>

Platform::Platform() : Entity(EntityType::PLATFORM) {
    name = "Platform";
}

Platform::~Platform() {}

bool Platform::Awake() { return true; }

bool Platform::Start() {
    // You can adjust this path to your actual platform texture
    texture = Engine::GetInstance().textures->Load("assets/maps/Platform.png");

    // Create a KINEMATIC body so it moves via velocity and carries dynamic bodies (like the player) via friction
    pbody = Engine::GetInstance().physics->CreateRectangle((int)position.getX(), (int)position.getY(), width, height, bodyType::KINEMATIC, 1.0f);
    pbody->ctype = ColliderType::PLATFORM;
    pbody->listener = this;

    // If no path was provided, use the initial position as a single stationary waypoint
    if (waypoints_.empty()) {
        waypoints_.push_back(position);
    }

    return true;
}

bool Platform::Update(float dt) {
    if (!active) return true;
    if (Engine::GetInstance().scene->isPaused_) return true;

    if (!waypoints_.empty()) {
        Vector2D target = waypoints_[currentWaypoint_];

        int bodyX, bodyY;
        pbody->GetPosition(bodyX, bodyY);

        float dx = target.getX() - (float)bodyX;
        float dy = target.getY() - (float)bodyY;
        float dist = std::sqrt(dx * dx + dy * dy);

        if (dist <= WAYPOINT_TOLERANCE) {
            // Snap to target to prevent drifting and target the next waypoint
            pbody->SetPosition((int)target.getX(), (int)target.getY());
            Engine::GetInstance().physics->SetLinearVelocity(pbody, 0.0f, 0.0f);

            // Loop back to the first waypoint if at the end
            currentWaypoint_ = (currentWaypoint_ + 1) % waypoints_.size();
        }
        else {
            // Move towards the target waypoint
            float vx = (dx / dist) * speed_;
            float vy = (dy / dist) * speed_;
            Engine::GetInstance().physics->SetLinearVelocity(pbody, vx, vy);
        }

        // Update Entity position for rendering
        pbody->GetPosition(bodyX, bodyY);
        position.setX((float)bodyX);
        position.setY((float)bodyY);
    }

    // Draw the platform
    Engine::GetInstance().render->DrawTexture(texture, (int)position.getX() - width / 2, (int)position.getY() - height / 2);

    return true;
}

bool Platform::CleanUp() {
    if (texture) {
        Engine::GetInstance().textures->UnLoad(texture);
        texture = nullptr;
    }
    if (pbody) {
        Engine::GetInstance().physics->DeletePhysBody(pbody);
        pbody = nullptr;
    }
    return true;
}

void Platform::ParsePath(const std::string& pathStr) {
    // Parses strings like "100,200;300,200;400,500"
    std::stringstream ss(pathStr);
    std::string pointStr;

    while (std::getline(ss, pointStr, ';')) {
        std::stringstream ssPoint(pointStr);
        std::string xStr, yStr;

        if (std::getline(ssPoint, xStr, ',') && std::getline(ssPoint, yStr, ',')) {
            waypoints_.push_back(Vector2D(std::stof(xStr), std::stof(yStr)));
        }
    }
}