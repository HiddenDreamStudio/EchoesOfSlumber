#include "Platform.h"
#include "Engine.h"
#include "Textures.h"
#include "Render.h"
#include "Physics.h"
#include "Log.h"

Platform::Platform() : Entity(EntityType::PLATFORM) {
    name = "Platform";
}

Platform::~Platform() {}

bool Platform::Awake() {
    return true;
}

bool Platform::Start() {
    // Load the platform texture
    LOG("Platform loading texture: %s", texturePath.c_str());
    texture = Engine::GetInstance().textures->Load(texturePath.c_str());

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

    if (triggerOnPlayer && !playerOnTop) {
        Engine::GetInstance().physics->SetLinearVelocity(pbody, 0.0f, 0.0f);
        Engine::GetInstance().render->DrawTexture(texture, bx - texW / 2, by - texH / 2);
        return true;
    }

    if (requireLever && !activatedByLever) {
        Engine::GetInstance().physics->SetLinearVelocity(pbody, 0.0f, 0.0f);
        Engine::GetInstance().render->DrawTexture(texture, bx - texW / 2, by - texH / 2);
        return true;
    }

    Vector2D currentPos((float)bx, (float)by);
    Vector2D target = waypoints[currentWaypoint];

    float dist = currentPos.distanceEuclidean(target);
    float moveDistThisFrame = (speed * 50.0f) * (dt / 1000.0f);

    if (dist <= moveDistThisFrame || dist <= 3.0f) {
        pbody->SetPosition((int)target.getX(), (int)target.getY());
        currentWaypoint += direction;

        // Reverse direction if it reaches the end or the beginning of the path
        if (currentWaypoint >= waypoints.size() || currentWaypoint < 0) {
            direction *= -1;
            currentWaypoint += direction * 2;
        }
        target = waypoints[currentWaypoint];
    }
    else {
        // Calculate the movement direction and apply velocity
        Vector2D dir = (target - currentPos).normalized();
        Engine::GetInstance().physics->SetLinearVelocity(pbody, dir.getX() * speed, dir.getY() * speed);
    }

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

void Platform::OnCollision(PhysBody* physA, PhysBody* physB) {
    if (physB->ctype == ColliderType::PLAYER) {
        int platX, platY, pX, pY;
        physA->GetPosition(platX, platY);
        physB->GetPosition(pX, pY);
        if (pY < platY) {
            playerOnTop = true;
        }
    }
}

void Platform::OnCollisionEnd(PhysBody* physA, PhysBody* physB) {
    if (physB->ctype == ColliderType::PLAYER)
        playerOnTop = false;
}
