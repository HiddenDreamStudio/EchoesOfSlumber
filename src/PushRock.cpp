#include "PushRock.h"
#include "Engine.h"
#include "Textures.h"
#include "Render.h"
#include "Physics.h"
#include "Log.h"
#include "Window.h"
#include <box2d/box2d.h>

PushRock::PushRock() : Entity(EntityType::PUSH_ROCK)
{
    name = "push_rock";
}

PushRock::~PushRock() {}

bool PushRock::Awake() {
    return true;
}

bool PushRock::Start() {
    texture = Engine::GetInstance().textures->Load("assets/textures/AS_environment/Push_Rock.PNG");

    if (texture) {
        float tw = 0, th = 0;
        SDL_GetTextureSize(texture, &tw, &th);
        texW = (int)tw;
        texH = (int)th;
    }

    // Create a DYNAMIC rectangle body — high density makes it heavy and hard to push
    int collW = (int)rockWidth;
    int collH = (int)rockHeight;

    pbody = Engine::GetInstance().physics->CreateRectangle(
        (int)position.getX(), (int)position.getY(),
        collW, collH, bodyType::DYNAMIC, 0.95f);

    pbody->ctype = ColliderType::PUSH_ROCK;
    pbody->listener = this;

    // Lock rotation so the rock doesn't tumble
    b2Body_SetFixedRotation(pbody->body, true);

    // Increase mass by setting high density on the existing shape
    int shapeCount = b2Body_GetShapeCount(pbody->body);
    if (shapeCount > 0) {
        b2ShapeId shapes[4];
        b2Body_GetShapes(pbody->body, shapes, shapeCount);
        for (int i = 0; i < shapeCount; i++) {
            b2Shape_SetDensity(shapes[i], 12.0f, true);
        }
    }

    LOG("PushRock started at: %f, %f (size: %dx%d)", position.getX(), position.getY(), collW, collH);

    return true;
}

bool PushRock::Update(float dt) {
    if (!active) return true;

    int x, y;
    pbody->GetPosition(x, y);
    position.setX((float)x);
    position.setY((float)y);

    int halfW = (int)(rockWidth / 2.0f);
    int halfH = (int)(rockHeight / 2.0f);

    // Draw the texture scaled to match the rock collision size
    SDL_Rect srcRect = { 0, 0, texW, texH };

    auto& render = Engine::GetInstance().render;
    int scale = Engine::GetInstance().window->GetScale();

    SDL_FRect dst;
    dst.x = (float)(render->camera.x + (x - halfW) * scale);
    // Add 14 pixels downward offset because the player's sprite is drawn 14 pixels
    // below their physics capsule, so the visual ground is lower than the physics ground
    dst.y = (float)(render->camera.y + (y - halfH + 14) * scale);
    dst.w = rockWidth * scale;
    dst.h = rockHeight * scale;

    SDL_FRect src;
    src.x = 0;
    src.y = 0;
    src.w = (float)texW;
    src.h = (float)texH;

    render->ApplyAmbientTint(texture);
    SDL_RenderTexture(render->renderer, texture, &src, &dst);
    render->ResetAmbientTint(texture);

    return true;
}

bool PushRock::CleanUp() {
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

void PushRock::OnCollision(PhysBody* physA, PhysBody* physB) {
    // Landing on top acts as a platform
    if (physB->ctype == ColliderType::PLAYER) {
        int rockX, rockY, playerX, playerY;
        physA->GetPosition(rockX, rockY);
        physB->GetPosition(playerX, playerY);

        // If player is above the rock, treat it like a platform for landing
        if (playerY < rockY - (int)(rockHeight * 0.3f)) {
            // Player landed on top — handled by Player's OnCollision with PLATFORM-like behavior
        }
    }
}

void PushRock::OnCollisionEnd(PhysBody* physA, PhysBody* physB) {
}
