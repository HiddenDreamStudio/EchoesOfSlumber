#include "RopedRock.h"
#include "Engine.h"
#include "Physics.h"
#include "Render.h"
#include "Textures.h"
#include "Window.h"
#include "Scene.h"
#include "Log.h"

RopedRock::RopedRock() : Entity(EntityType::ROPE_ROCK)
{
    name = "RopedRock";
}

RopedRock::~RopedRock() {}

bool RopedRock::Start()
{
    auto& tex = *Engine::GetInstance().textures;
    texRope_ = tex.Load("assets/textures/AS_props/AS_Cuerda.png");
    texRock_ = tex.Load("assets/textures/AS_props/AS_roca.png");

    anchorX_ = position.getX();
    anchorY_ = position.getY();

    SpawnBodies();
    return true;
}

void RopedRock::SpawnBodies()
{
    int ax = (int)anchorX_;
    int ay = (int)anchorY_;

    int ropeSensorCY = ay + (int)(ROPE_LENGTH / 2);
    ropeSensor_ = Engine::GetInstance().physics->CreateRectangleSensor(
        ax, ropeSensorCY, ROPE_W, (int)ROPE_LENGTH, bodyType::STATIC);
    ropeSensor_->listener = this;
    ropeSensor_->ctype    = ColliderType::ROPE;

    int rockCY = ay + (int)ROPE_LENGTH + ROCK_H / 2;
    rockBody_ = Engine::GetInstance().physics->CreateRectangle(
        ax, rockCY, ROCK_W, ROCK_H, bodyType::STATIC, 0.3f);
    rockBody_->listener = this;
    rockBody_->ctype    = ColliderType::ENEMY;
}

bool RopedRock::CleanUp()
{
    if (ropeSensor_) {
        Engine::GetInstance().physics->DeletePhysBody(ropeSensor_);
        ropeSensor_ = nullptr;
    }
    if (rockBody_) {
        Engine::GetInstance().physics->DeletePhysBody(rockBody_);
        rockBody_ = nullptr;
    }
    return true;
}

bool RopedRock::Update(float dt)
{
    if (state_ == RopedRockState::FALLING && rockBody_)
    {
        int bx, by;
        rockBody_->GetPosition(bx, by);
        position.setX((float)bx);
        position.setY((float)(by - (int)ROPE_LENGTH - ROCK_H / 2));
    }

    if (state_ == RopedRockState::RESPAWNING)
    {
        respawnTimer_ -= dt;
        if (respawnTimer_ <= 0.0f)
        {
            CleanUp();
            position.setX(anchorX_);
            position.setY(anchorY_);
            SpawnBodies();
            state_ = RopedRockState::HANGING;
        }
    }

    Draw();
    return true;
}

void RopedRock::OnCollision(PhysBody* physA, PhysBody* physB)
{
    if (physA == ropeSensor_
        && (physB->ctype == ColliderType::SLINGSHOT_PROJ || physB->ctype == ColliderType::ATTACK)
        && state_ == RopedRockState::HANGING)
    {
        Engine::GetInstance().physics->DeletePhysBody(ropeSensor_);
        ropeSensor_ = nullptr;

        if (rockBody_)
        {
            int bx, by;
            rockBody_->GetPosition(bx, by);
            Engine::GetInstance().physics->DeletePhysBody(rockBody_);
            rockBody_ = Engine::GetInstance().physics->CreateRectangle(
                bx, by, ROCK_W, ROCK_H, bodyType::DYNAMIC, 0.3f);
            rockBody_->listener = this;
            rockBody_->ctype    = ColliderType::ENEMY;
        }
        state_ = RopedRockState::FALLING;
    }
    else if (physA == rockBody_ && state_ == RopedRockState::FALLING)
    {
        if (physB->listener)
            physB->listener->TakeDamage(1);

        state_        = RopedRockState::RESPAWNING;
        respawnTimer_ = RESPAWN_DELAY;
    }
}

void RopedRock::Draw()
{
    auto& render = *Engine::GetInstance().render;
    int sc = Engine::GetInstance().window->GetScale();

    auto toScreen = [&](int wx, int wy, int w, int h) -> SDL_FRect {
        return { (float)(render.camera.x + wx * sc),
                 (float)(render.camera.y + wy * sc),
                 (float)(w * sc), (float)(h * sc) };
    };

    int ax = (int)anchorX_;
    int ay = (int)anchorY_;

    if (state_ == RopedRockState::HANGING)
    {
        if (texRope_)
        {
            SDL_FRect dst = toScreen(ax - ROPE_W / 2, ay, ROPE_W, (int)ROPE_LENGTH);
            SDL_RenderTexture(render.renderer, texRope_, nullptr, &dst);
        }
        if (texRock_)
        {
            SDL_FRect dst = toScreen(ax - ROCK_W / 2, ay + (int)ROPE_LENGTH, ROCK_W, ROCK_H);
            SDL_RenderTexture(render.renderer, texRock_, nullptr, &dst);
        }
    }
    else if (state_ == RopedRockState::FALLING && rockBody_)
    {
        int bx, by;
        rockBody_->GetPosition(bx, by);
        if (texRock_)
        {
            SDL_FRect dst = toScreen(bx - ROCK_W / 2, by - ROCK_H / 2, ROCK_W, ROCK_H);
            SDL_RenderTexture(render.renderer, texRock_, nullptr, &dst);
        }
    }
    // RESPAWNING: nothing drawn
}
