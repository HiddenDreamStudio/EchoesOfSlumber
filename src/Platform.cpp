#include "Platform.h"
#include "Engine.h"
#include "Textures.h"
#include "Render.h"
#include "Physics.h"

Platform::Platform() : Entity(EntityType::PLATFORM) {
    name = "Platform";
}

Platform::~Platform() {}

bool Platform::Awake() { 
    return true; 
}

bool Platform::Start() {
    // Load the platform texture
    texture = Engine::GetInstance().textures->Load("assets/textures/AS_environment/AS_interactius_01.PNG");
    
    // Create a kinematic body so it can move and carry other objects
    pbody = Engine::GetInstance().physics->CreateRectangle((int)position.getX(), (int)position.getY(), texW, texH, bodyType::KINEMATIC);
    pbody->ctype = ColliderType::PLATFORM; 
    pbody->listener = this;

    return true;
}

bool Platform::Update(float dt) {
    if (!active || waypoints.empty() || !pbody) return true;

    int bx, by;
    pbody->GetPosition(bx, by);
    Vector2D currentPos((float)bx, (float)by);
    Vector2D target = waypoints[currentWaypoint];

    // Check if the platform has reached the current waypoint
    if (currentPos.distanceEuclidean(target) < 5.0f) {
        currentWaypoint += direction;
        
        // Reverse direction if it reaches the end or the beginning of the path
        if (currentWaypoint >= waypoints.size() || currentWaypoint < 0) {
            direction *= -1;
            currentWaypoint += direction * 2;
        }
        target = waypoints[currentWaypoint];
    }

    // Calculate the movement direction and apply velocity
    Vector2D dir = (target - currentPos).normalized();
    Engine::GetInstance().physics->SetLinearVelocity(pbody, dir.getX() * speed, dir.getY() * speed);

    // Render the platform at its updated physical position
    pbody->GetPosition(bx, by);
    Engine::GetInstance().render->DrawTexture(texture, bx - texW / 2, by - texH / 2);

    return true;
}

bool Platform::CleanUp() {
    Engine::GetInstance().textures->UnLoad(texture);
    Engine::GetInstance().physics->DeletePhysBody(pbody);
    return true;
}

void Platform::AddWaypoint(Vector2D point) {
    waypoints.push_back(point);
}