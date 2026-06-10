#include "PuzzleManager3.h"
#include "Engine.h"
#include "Render.h"
#include "Textures.h"
#include "Log.h"
#include <cmath>

PuzzleManager3::PuzzleManager3() {}

PuzzleManager3::~PuzzleManager3() {

    if (texLeverNormal_) { Engine::GetInstance().textures->UnLoad(texLeverNormal_);  texLeverNormal_ = nullptr; }
    if (texLeverActive_) { Engine::GetInstance().textures->UnLoad(texLeverActive_);  texLeverActive_ = nullptr; }
    if (texButtonNormal_) { Engine::GetInstance().textures->UnLoad(texButtonNormal_); texButtonNormal_ = nullptr; }
    if (texButtonPressed_) { Engine::GetInstance().textures->UnLoad(texButtonPressed_); texButtonPressed_ = nullptr; }
}

void PuzzleManager3::Init(SDL_Renderer* renderer) {
    renderer_ = renderer;

    if (!texLeverNormal_)
        texLeverNormal_ = Engine::GetInstance().textures->Load("assets/textures/UI/UI_Palanca_Normal.png");
    if (!texLeverActive_)
        texLeverActive_ = Engine::GetInstance().textures->Load("assets/textures/UI/UI_Palanca_Active.png");
    if (!texButtonNormal_)
        texButtonNormal_ = Engine::GetInstance().textures->Load("assets/textures/UI/UI_Boton_Normal.png");
    if (!texButtonPressed_)
        texButtonPressed_ = Engine::GetInstance().textures->Load("assets/textures/UI/UI_Boton_Pressed.png");
    LOG("PUZZLE3: texButtonNormal_ = %s", texButtonNormal_ ? "OK" : "NULL");
    LOG("PUZZLE3: texButtonPressed_ = %s", texButtonPressed_ ? "OK" : "NULL");
}


void PuzzleManager3::LoadLever(const LeverData3& lever, SDL_FRect blockedPortalRect) {
    lever_ = lever;
    blockedPortalRect_ = blockedPortalRect;
    isInLeverRoom = true;
    isInButtonRoom = false;
    LOG("PUZZLE3: LoadLever � portal bloqueado en (%.0f, %.0f) size %.0fx%.0f",
        blockedPortalRect.x, blockedPortalRect.y, blockedPortalRect.w, blockedPortalRect.h);
    LOG("PUZZLE3: Palanca worldRect (%.0f, %.0f) size %.0fx%.0f",
        lever.worldRect.x, lever.worldRect.y, lever.worldRect.w, lever.worldRect.h);
}

void PuzzleManager3::UpdateLever(float dt, SDL_FRect playerRect, bool keyP_pressed) {
    if (!isInLeverRoom) return;

    float px = playerRect.x + playerRect.w * 0.5f;
    float py = playerRect.y + playerRect.h * 0.5f;
    float lx = lever_.worldRect.x + lever_.worldRect.w * 0.5f;
    float ly = lever_.worldRect.y + lever_.worldRect.h * 0.5f;
    float dist = sqrtf((px - lx) * (px - lx) + (py - ly) * (py - ly));

    playerNearLever_ = (dist <= INTERACT_RADIUS);

    if (playerNearLever_ && keyP_pressed && !lever_.activated) {
        lever_.activated = true;
        LOG("PUZZLE3: Palanca activada! Portal desbloqueado.");
    }
}

void PuzzleManager3::RenderLever(SDL_Renderer* renderer, float cameraX, float cameraY) {
    if (!isInLeverRoom) return;

    auto& render = *Engine::GetInstance().render;

    float portalSx = blockedPortalRect_.x + cameraX;
    float portalSy = blockedPortalRect_.y + cameraY;
    float leverSx = lever_.worldRect.x + cameraX;
    float leverSy = lever_.worldRect.y + cameraY;

    SDL_Texture* leverTex = lever_.activated ? texLeverActive_ : texLeverNormal_;
    if (leverTex) {
        float tw, th;
        SDL_GetTextureSize(leverTex, &tw, &th);
        SDL_FRect dst = {
            leverSx,
            leverSy,
            lever_.worldRect.w,
            lever_.worldRect.h
        };
        SDL_RenderTexture(renderer, leverTex, nullptr, &dst);
    }
    else {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, lever_.activated ? 50 : 150,
            lever_.activated ? 200 : 150,
            50, 200);
        SDL_FRect leverRect = { leverSx, leverSy, lever_.worldRect.w, lever_.worldRect.h };
        SDL_RenderFillRect(renderer, &leverRect);
    }

    SDL_FRect screenPortal = { portalSx, portalSy, blockedPortalRect_.w, blockedPortalRect_.h };
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    if (!lever_.activated) {
        SDL_SetRenderDrawColor(renderer, 200, 50, 50, 100);
        SDL_RenderFillRect(renderer, &screenPortal);

        SDL_Rect textArea = {
            (int)portalSx,
            (int)(portalSy - 24),
            (int)blockedPortalRect_.w,
            20
        };
        SDL_Color red = { 220, 80, 80, 255 };
        render.DrawMenuTextCentered("locked", textArea, red, 0.35f);
    }
    else {
        SDL_SetRenderDrawColor(renderer, 50, 200, 50, 60);
        SDL_RenderFillRect(renderer, &screenPortal);

        SDL_Rect textArea = {
            (int)portalSx,
            (int)(portalSy - 24),
            (int)blockedPortalRect_.w,
            20
        };
        SDL_Color green = { 80, 220, 80, 255 };
        render.DrawMenuTextCentered("open", textArea, green, 0.35f);
    }

    if (!lever_.activated && playerNearLever_) {
        SDL_Rect hintArea = {
            (int)leverSx,
            (int)(leverSy - 30),
            (int)lever_.worldRect.w,
            20
        };
        SDL_Color yellow = { 255, 220, 50, 255 };
        render.DrawMenuTextCentered("press_p", hintArea, yellow, 0.35f);
    }

    auto drawExtraPortal = [&](SDL_FRect rect, bool unlocked) {
        float psx = rect.x + cameraX;
        float psy = rect.y + cameraY;
        SDL_FRect sr = { psx, psy, rect.w, rect.h };

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        if (unlocked) {
            SDL_SetRenderDrawColor(renderer, 0, 200, 80, 120);
            SDL_RenderFillRect(renderer, &sr);
            SDL_SetRenderDrawColor(renderer, 0, 255, 100, 255);
            SDL_RenderRect(renderer, &sr);
            SDL_Rect ta = { (int)psx, (int)(psy - 24), (int)rect.w, 20 };
            SDL_Color green = { 80, 220, 80, 255 };
            render.DrawMenuTextCentered("open", ta, green, 0.35f);
        }
        else {
            SDL_SetRenderDrawColor(renderer, 200, 50, 50, 120);
            SDL_RenderFillRect(renderer, &sr);
            SDL_SetRenderDrawColor(renderer, 255, 50, 50, 255);
            SDL_RenderRect(renderer, &sr);
            SDL_Rect ta = { (int)psx, (int)(psy - 24), (int)rect.w, 20 };
            SDL_Color red = { 220, 80, 80, 255 };
            render.DrawMenuTextCentered("locked", ta, red, 0.35f);
        }
        };

    if (blockedPortalA_.w > 0) drawExtraPortal(blockedPortalA_, bothButtonsDone_);
    if (blockedPortalB_.w > 0) drawExtraPortal(blockedPortalB_, bothButtonsDone_);
}

void PuzzleManager3::LoadButtons(const std::vector<ButtonData3>& buttons) {
    buttons_ = buttons;
    bothButtonsDone_ = false;
    isInLeverRoom = false;
    isInButtonRoom = true;
    for (int i = 0; i < 2; i++) {
        inactivityTimer_[i] = 0.0f;
        buttonInProgress_[i] = false;
    }
    LOG("PUZZLE3: LoadButtons FINAL: isInButtonRoom=%d, count=%d", (int)isInButtonRoom, (int)buttons_.size());
}

void PuzzleManager3::UpdateButtons(float dt, SDL_FRect playerRect,float mouseWorldX, float mouseWorldY, bool mouseClicked)
{

    if (openTextTimer_ > 0.0f) {
        openTextTimer_ -= dt;
    }

    if (flashActive_) {
        flashTimer_ -= dt;
        if (flashTimer_ <= 0.0f) {
            flashActive_ = false;
            flashTimer_ = 0.0f;
        }
    }

    if (!isInButtonRoom || bothButtonsDone_) return;

    for (int i = 0; i < (int)buttons_.size() && i < 2; i++) {
        ButtonData3& btn = buttons_[i];
        if (btn.completed) continue;

        bool overBtn =
            mouseWorldX >= btn.worldRect.x &&
            mouseWorldX <= btn.worldRect.x + btn.worldRect.w &&
            mouseWorldY >= btn.worldRect.y &&
            mouseWorldY <= btn.worldRect.y + btn.worldRect.h;

        if (overBtn && mouseClicked) {
            btn.currentClicks++;
            buttonInProgress_[i] = true;
            inactivityTimer_[i] = 0.0f;

            if (btn.currentClicks > btn.requiredClicks) {
                btn.currentClicks = 0;
                buttonInProgress_[i] = false;
                flashActive_ = true;
                flashRed_ = true;
                flashTimer_ = FLASH_DURATION;
            }
            else if (btn.currentClicks == btn.requiredClicks) {
                btn.completed = true;
            }
        }

        if (buttonInProgress_[i] && !btn.completed) {
            inactivityTimer_[i] += dt;
            if (inactivityTimer_[i] >= INACTIVITY_LIMIT) {
                btn.currentClicks = 0;
                buttonInProgress_[i] = false;
                inactivityTimer_[i] = 0.0f;
                flashActive_ = true;
                flashRed_ = true;
                flashTimer_ = FLASH_DURATION;
                LOG("PUZZLE3: Boton %d � inactividad, reset.", i);
            }
        }
    }

    if (!bothButtonsDone_ && !buttons_.empty()) {
        bool allDone = true;
        for (int i = 0; i < (int)buttons_.size() && i < 2; i++) {
            if (!buttons_[i].completed) { allDone = false; break; }
        }
        if (allDone) {
            bothButtonsDone_ = true;
            openTextTimer_ = OPEN_TEXT_DURATION;   
            flashActive_ = true;
            flashRed_ = false;
            flashTimer_ = FLASH_DURATION;
            LOG("PUZZLE3: Ambos botones completados!");
        }
    }
}

void PuzzleManager3::RenderButtons(SDL_Renderer* renderer, float cameraX, float cameraY) {

    if (!isInButtonRoom) return;

    auto& render = *Engine::GetInstance().render;

    for (int i = 0; i < (int)buttons_.size() && i < 2; i++) {
        const ButtonData3& btn = buttons_[i];
        float sx = btn.worldRect.x + cameraX;
        float sy = btn.worldRect.y + cameraY;

        SDL_Texture* btnTex = btn.completed ? texButtonPressed_ : texButtonNormal_;
        if (btnTex) {
            SDL_FRect dst = { sx, sy, btn.worldRect.w, btn.worldRect.h };
            SDL_RenderTexture(renderer, btnTex, nullptr, &dst);
        }
        else {
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer,
                btn.completed ? 50 : 100,
                btn.completed ? 200 : 100,
                btn.completed ? 50 : 200,
                180);
            SDL_FRect btnRect = { sx, sy, btn.worldRect.w, btn.worldRect.h };
            SDL_RenderFillRect(renderer, &btnRect);
        }

        char buf[32];
        SDL_snprintf(buf, sizeof(buf), "%d / %d", btn.currentClicks, btn.requiredClicks);
        SDL_Color white = { 255, 255, 255, 255 };
        SDL_Rect textArea = {
            (int)sx,
            (int)(sy - 22),
            (int)btn.worldRect.w,
            18
        };
        render.DrawMenuTextCentered(buf, textArea, white, 0.32f);
    }

    if (bothButtonsDone_ && openTextTimer_ > 0.0f && !buttons_.empty()) {
        int winW, winH;
        SDL_GetRenderOutputSize(renderer, &winW, &winH);
        float alpha = (openTextTimer_ < 1000.0f) ? (openTextTimer_ / 1000.0f) : 1.0f;
        SDL_Color green = { 50, 220, 80, (Uint8)(255 * alpha) };
        SDL_Rect area = { 0, winH / 2 - 30, winW, 60 };
        render.DrawMenuTextCentered("open", area, green, 1.5f);
    }
}

void PuzzleManager3::RenderFlash(SDL_Renderer* renderer, int winW, int winH) {
    if (!flashActive_) return;

    float ratio = flashTimer_ / FLASH_DURATION;
    Uint8 alpha = (Uint8)(ratio * 100.0f);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    if (flashRed_)
        SDL_SetRenderDrawColor(renderer, 200, 30, 30, alpha);
    else
        SDL_SetRenderDrawColor(renderer, 30, 200, 30, alpha);

    SDL_FRect full = { 0.0f, 0.0f, (float)winW, (float)winH };
    SDL_RenderFillRect(renderer, &full);
}

void PuzzleManager3::Reset() {
    lever_.activated = false;
    buttons_.clear();
    bothButtonsDone_ = false;
    flashActive_ = false;
    flashTimer_ = 0.0f;
    isInLeverRoom = false;
    isInButtonRoom = false;
    playerNearLever_ = false;
    openTextTimer_ = 0.0f;

    for (int i = 0; i < 2; i++) {
        inactivityTimer_[i] = 0.0f;
        buttonInProgress_[i] = false;
    }
}

void PuzzleManager3::LoadExtraPortals(SDL_FRect portalA, SDL_FRect portalB) {
    blockedPortalA_ = portalA;
    blockedPortalB_ = portalB;
    portalAUnlocked_ = false;
    portalBUnlocked_ = false;
}
