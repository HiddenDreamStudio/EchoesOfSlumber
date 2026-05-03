#include "Box.h"
#include "Engine.h"
#include "Textures.h"
#include "Render.h"
#include "Physics.h"

Box::Box() : Entity(EntityType::BOX)
{
    name = "box";
}

Box::~Box() {}

bool Box::Awake() {
    return true;
}

bool Box::Start() {
    texture = Engine::GetInstance().textures->Load("assets/textures/AS_environment/AS_interactius_01.PNG");
    
    // Create a dynamic physics body so it can be pushed
    pbody = Engine::GetInstance().physics->CreateRectangle(
        (int)position.getX(), (int)position.getY(), 64, 64, bodyType::DYNAMIC);
    
    pbody->ctype = ColliderType::BOX;
    pbody->listener = this;

    return true;
}

bool Box::Update(float dt) {
    if (!active) return true;

    int x, y;
    pbody->GetPosition(x, y);
    position.setX((float)x);
    position.setY((float)y);

    // Draw the box
    // For now we use the texture, ideally we'd use a specific rect for the box from AS_interactius_01.PNG
    Engine::GetInstance().render->DrawTexture(texture, x - 32, y - 32);

    return true;
}

bool Box::CleanUp() {
    Engine::GetInstance().textures->UnLoad(texture);
    Engine::GetInstance().physics->DeletePhysBody(pbody);
    return true;
}
