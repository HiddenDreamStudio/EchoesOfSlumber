#include "Checkpoint.h"
#include "Engine.h"
#include "Textures.h"
#include "Render.h"
#include "Physics.h"
#include "SaveSystem.h"
#include "Log.h"

Checkpoint::Checkpoint() : Entity(EntityType::CHECKPOINT)
{
    name = "checkpoint";
}

Checkpoint::~Checkpoint() {}

bool Checkpoint::Awake() {
    return true;
}

bool Checkpoint::Start() {
    // Assets are in assets/textures/AS_environment/AS_interactius_01.PNG
    // For now we load the texture, we might need a specific rect later
    texture = Engine::GetInstance().textures->Load("assets/textures/AS_environment/AS_interactius_01.PNG");
    
    // Create a sensor collider
    pbody = Engine::GetInstance().physics->CreateRectangleSensor(
        (int)position.getX(), (int)position.getY(), 64, 64, bodyType::STATIC);
    
    pbody->ctype = ColliderType::CHECKPOINT;
    pbody->listener = this;

    return true;
}

bool Checkpoint::Update(float dt) {
    if (!active) return true;

    floatTimer += dt * 0.002f;
    float offset = sinf(floatTimer) * 10.0f;

    int x, y;
    pbody->GetPosition(x, y);
    
    // Draw the checkpoint (crystal)
    // For now we use a placeholder rect or the whole texture if it's small
    SDL_Color tint = activated ? SDL_Color{100, 255, 100, 255} : SDL_Color{255, 255, 255, 255};
    
    // If it's a spritesheet, we should use a section. 
    // Since I don't know the exact rect, I'll draw it with a tint to show state.
    Engine::GetInstance().render->DrawTexture(texture, x - 32, y - 32 + (int)offset);

    return true;
}

bool Checkpoint::CleanUp() {
    Engine::GetInstance().textures->UnLoad(texture);
    Engine::GetInstance().physics->DeletePhysBody(pbody);
    return true;
}

void Checkpoint::OnCollision(PhysBody* physA, PhysBody* physB) {
    if (physB->ctype == ColliderType::PLAYER && !activated) {
        activated = true;
        LOG("Checkpoint activated! Saving game...");
        Engine::GetInstance().saveSystem->QuickSave();
    }
}
