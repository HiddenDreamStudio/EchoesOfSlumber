#include "Checkpoint.h"
#include "Engine.h"
#include "Textures.h"
#include "Render.h"
#include "Physics.h"
#include "SaveSystem.h"
#include "Log.h"
#include "Scene.h"
#include "Map.h"
#include "Input.h"
#include "Player.h"
#include <algorithm>
#include <cmath>

Checkpoint::Checkpoint() : Entity(EntityType::CHECKPOINT)
{
    name = "checkpoint";
}

Checkpoint::~Checkpoint() {}

bool Checkpoint::Awake() {
    return true;
}

bool Checkpoint::Start() {
    textureOff_ = Engine::GetInstance().textures->Load("assets/textures/UI/UI_Checkpoint_OFF.png");
    textureOn_ = Engine::GetInstance().textures->Load("assets/textures/UI/UI_Checkpoint_ON.png");

    SDL_Texture* sizeTexture = textureOff_ ? textureOff_ : textureOn_;
    if (sizeTexture) {
        Engine::GetInstance().textures->GetSize(sizeTexture, texW, texH);
    }

    if (checkpointId_.empty()) {
        checkpointId_ = Engine::GetInstance().map->mapFileName + "@"
            + std::to_string((int)position.getX()) + ","
            + std::to_string((int)position.getY());
    }

    activated = (checkpointId_ == Engine::GetInstance().saveSystem->GetActiveCheckpointId());
    
    float sensorW = std::max(objectW_, 96.0f);
    float sensorH = std::max(objectH_, 96.0f);
    pbody = Engine::GetInstance().physics->CreateRectangleSensor(
        (int)position.getX(), (int)position.getY(), (int)sensorW, (int)sensorH, bodyType::STATIC);
    
    pbody->ctype = ColliderType::CHECKPOINT;
    pbody->listener = this;

    return true;
}

void Checkpoint::DrawBehindMap() {
    SDL_Texture* texture = activated ? textureOn_ : textureOff_;

    if (texture && texW > 0 && texH > 0) {
        SDL_Rect section = { 0, 0, texW, texH };
        // Increase the target height multiplier to make the texture even larger (3.5f scale)
        float targetH = std::max(objectH_, 52.0f) * 3.5f;
        float drawScale = targetH / (float)texH;
        float targetW = (float)texW * drawScale;
        
        int drawX = (int)(position.getX() - targetW * 0.5f);
        // Align the bottom of the drawn texture to the bottom of the Tiled object
        // position.getY() + objectH_ * 0.5f is the bottom edge of the Tiled object
        int drawY = (int)(position.getY() + objectH_ * 0.5f - targetH);

        Engine::GetInstance().render->DrawTexture(texture, drawX, drawY, &section, 1.0f, -1.0f, 0, INT_MAX, INT_MAX, SDL_FLIP_NONE, drawScale);
    }
}

bool Checkpoint::Update(float dt) {
    if (!active) return true;

    auto& scene = Engine::GetInstance().scene;
    const bool playerCanInteract = scene && scene->player &&
        !scene->player->IsDead() &&
        !scene->player->isWakingUp &&
        !scene->player->isYoyoTrapped_;

    if (playerInRange_ && playerCanInteract && !scene->isPaused_ && !scene->isGameOver_ && !scene->IsCheckpointTransitionActive()) {
        if (!activated) {
            scene->RequestCheckpointActivation(checkpointId_, GetSpawnPosition());
            activated = true;
        }
    }

    return true;
}

bool Checkpoint::CleanUp() {
    if (textureOff_) {
        Engine::GetInstance().textures->UnLoad(textureOff_);
        textureOff_ = nullptr;
    }
    if (textureOn_) {
        Engine::GetInstance().textures->UnLoad(textureOn_);
        textureOn_ = nullptr;
    }
    if (pbody) {
        Engine::GetInstance().physics->DeletePhysBody(pbody);
        pbody = nullptr;
    }
    return true;
}

void Checkpoint::OnCollision(PhysBody* physA, PhysBody* physB) {
    if (physB->ctype == ColliderType::PLAYER) {
        playerInRange_ = true;
    }
}

void Checkpoint::OnCollisionEnd(PhysBody* physA, PhysBody* physB) {
    if (physB->ctype == ColliderType::PLAYER) {
        playerInRange_ = false;
    }
}

void Checkpoint::Configure(const std::string& id, float width, float height) {
    checkpointId_ = id;
    if (width > 0.0f) objectW_ = width;
    if (height > 0.0f) objectH_ = height;
}
