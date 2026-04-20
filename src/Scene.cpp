#include "Engine.h"
#include "Input.h"
#include "Textures.h"
#include "Audio.h"
#include "Render.h"
#include "Window.h"
#include "Scene.h"
#include "Cinematics.h"
#include "Log.h"
#include "Entity.h"
#include "EntityManager.h"
#include "Player.h"
#include "Map.h"
#include "Item.h"
#include "Enemy.h"
#include "UIManager.h"
#include "SaveSystem.h"
#include "Physics.h"
#include <cstdlib>
#include <cmath>

// ────────────────────────────────────────────────────────────────────────────
// Button IDs  (main menu)
// ────────────────────────────────────────────────────────────────────────────
static constexpr int BTN_PLAY = 1;
static constexpr int BTN_SETTINGS = 2;
static constexpr int BTN_EXIT = 3;
// Settings panel
static constexpr int BTN_SETTINGS_BACK = 10;
static constexpr int BTN_MUSIC_UP = 11;
static constexpr int BTN_MUSIC_DOWN = 12;
static constexpr int BTN_SFX_UP = 13;
static constexpr int BTN_SFX_DOWN = 14;

Scene::Scene() : Module()
{
	name = "scene";
}

Scene::~Scene() {}

bool Scene::Awake()
{
	LOG("Loading Scene");
	hasPendingSceneChange = true;
	pendingScene = currentScene;
	return true;
}

bool Scene::Start() { return true; }
bool Scene::PreUpdate() { return true; }

bool Scene::Update(float dt)
{
	if (waitingForFade_ && Engine::GetInstance().render->IsFadeComplete()) {
		waitingForFade_ = false;
		ChangeScene(fadeTargetScene_);
		Engine::GetInstance().render->StartFade(FadeDirection::FADE_IN, 800.0f);
	}

	if (hasPendingSceneChange) {
		hasPendingSceneChange = false;
		ChangeScene(pendingScene);
	}

	switch (currentScene)
	{
	case SceneID::INTRO:
		UpdateIntro(dt);
		break;
	case SceneID::MAIN_MENU:
		PostUpdateMainMenu();
		UpdateMainMenu(dt);
		break;
	case SceneID::INTRO_CINEMATIC:
		UpdateIntroCinematic(dt);
		break;
	case SceneID::GAMEPLAY:
		UpdateGameplay(dt);
		break;
	}
	return true;
}

bool Scene::PostUpdate()
{
	bool ret = true;

	switch (currentScene)
	{
	case SceneID::GAMEPLAY:
		PostUpdateGameplay();
		break;
	default:
		break;
	}

	return ret;
}

bool Scene::OnUIMouseClickEvent(UIElement* uiElement)
{
	switch (currentScene)
	{
	case SceneID::MAIN_MENU:
		HandleMainMenuUIEvents(uiElement);
		break;
	case SceneID::GAMEPLAY:
		HandlePauseMenuUIEvents(uiElement);
		break;
	default:
		break;
	}
	return true;
}

bool Scene::CleanUp()
{
	LOG("Freeing scene");
	UnloadCurrentScene();
	return true;
}

Vector2D Scene::GetPlayerPosition()
{
	if (player) return player->GetPosition();
	return Vector2D(0, 0);
}

void Scene::SetPlayerPosition(Vector2D pos)
{
	if (player) player->SetPosition(pos);
}

// ============================================================================
//  Scene routing
// ============================================================================

void Scene::LoadScene(SceneID s)
{
	switch (s)
	{
	case SceneID::INTRO:           LoadIntro();           break;
	case SceneID::MAIN_MENU:       LoadMainMenu();        break;
	case SceneID::INTRO_CINEMATIC: LoadIntroCinematic();  break;
	case SceneID::GAMEPLAY:        LoadGameplay();         break;
	}
}

void Scene::ChangeScene(SceneID s)
{
	UnloadCurrentScene();
	currentScene = s;
	LoadScene(currentScene);
}

void Scene::UnloadCurrentScene()
{
	switch (currentScene)
	{
	case SceneID::INTRO:           UnloadIntro();           break;
	case SceneID::MAIN_MENU:       UnloadMainMenu();        break;
	case SceneID::INTRO_CINEMATIC: UnloadIntroCinematic();  break;
	case SceneID::GAMEPLAY:        UnloadGameplay();         break;
	}
}

// ============================================================================
//  MAIN MENU
// ============================================================================

void Scene::LoadMainMenu()
{
	showSettings_ = false;
	settingsCooldown_ = 0;
	musicVolume_ = 0.8f;
	sfxVolume_ = 0.8f;

	SDL_Texture* rawLogo = Engine::GetInstance().textures->Load("assets/textures/menu/EchoesOfSlumber.png");
	texMenuLogo_ = Engine::GetInstance().render->RecolorTexture(rawLogo, 212, 218, 234);
	Engine::GetInstance().textures->UnLoad(rawLogo);
	texMenuChild_ = Engine::GetInstance().textures->Load("assets/textures/menu/IL_NenFront_01.png");
	texMenuButton_ = Engine::GetInstance().textures->Load("assets/textures/menu/UI_Pause_Menu_button_white.png");

	const char* fragPaths[NUM_FRAGMENTS] = {
		"assets/textures/Menu/UI_Fragment1.png",
		"assets/textures/Menu/UI_Fragment2.png",
		"assets/textures/Menu/UI_Fragment3.png",
		"assets/textures/Menu/UI_Fragment4.png",
		"assets/textures/Menu/UI_Fragment5.png"
	};
	for (int i = 0; i < NUM_FRAGMENTS; i++)
		fragments_[i].tex = Engine::GetInstance().textures->Load(fragPaths[i]);
	fragmentsInited_ = false;
	fragmentTime_ = 0.0f;

	int winW = 0, winH = 0;
	Engine::GetInstance().window->GetWindowSize(winW, winH);

	const int leftHalf = winW / 2;
	const int logoW = 385;
	const int logoH = static_cast<int>(static_cast<float>(logoW) * (569.0f / 1559.0f));
	const int logoX = (leftHalf - logoW) / 2;
	const int logoY = winH / 4 - logoH / 2;

	const int btnW = 315;
	const int btnH = static_cast<int>(static_cast<float>(btnW) * (130.0f / 456.0f));
	const int btnX = (leftHalf - btnW) / 2;
	const int startY = logoY + logoH + 50;
	const int gap = btnH + 15;

	menuAnimState_ = MenuAnimState::LOGO_FADE_IN;
	menuAnimTimer_ = 0.0f;

	SDL_Rect playPos = { btnX, startY,           btnW, btnH };
	SDL_Rect settingsPos = { btnX, startY + gap,     btnW, btnH };
	SDL_Rect exitPos = { btnX, startY + gap * 2, btnW, btnH };

	btnPlay_ = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, BTN_PLAY, "play", playPos, this);
	btnSettings_ = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, BTN_SETTINGS, "options", settingsPos, this);
	btnExit_ = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, BTN_EXIT, "exit", exitPos, this);

	if (btnPlay_) { btnPlay_->alphaMod = 0.0f; btnPlay_->isVisible = false; }
	if (btnSettings_) { btnSettings_->alphaMod = 0.0f; btnSettings_->isVisible = false; }
	if (btnExit_) { btnExit_->alphaMod = 0.0f; btnExit_->isVisible = false; }

	if (texMenuButton_) {
		btnPlay_->SetTexture(texMenuButton_);
		btnSettings_->SetTexture(texMenuButton_);
		btnExit_->SetTexture(texMenuButton_);
	}

	const int panelW = 340;
	const int panelX = winW / 2 - panelW / 2;
	const int panelY = 60;
	const int smallBtnW = 40;
	const int smallBtnH = 34;
	const int rowH = 52;

	SDL_Rect musicUpPos = { panelX + panelW - smallBtnW - 12, panelY + 60,                  smallBtnW, smallBtnH };
	SDL_Rect musicDownPos = { panelX + 12,                       panelY + 60,                  smallBtnW, smallBtnH };
	SDL_Rect sfxUpPos = { panelX + panelW - smallBtnW - 12, panelY + 60 + rowH,           smallBtnW, smallBtnH };
	SDL_Rect sfxDownPos = { panelX + 12,                       panelY + 60 + rowH,           smallBtnW, smallBtnH };
	SDL_Rect backPos = { panelX + panelW / 2 - 60,          panelY + 60 + rowH * 2 + 10, 120,       btnH };

	Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, BTN_MUSIC_UP, "+", musicUpPos, this);
	Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, BTN_MUSIC_DOWN, "-", musicDownPos, this);
	Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, BTN_SFX_UP, "+", sUpPos, this);
	Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, BTN_SFX_DOWN, "-", sDownPos, this);
	Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, BTN_SETTINGS_BACK, "back", backPos, this);

	SetSettingsPanelVisible(false);
}

void Scene::UnloadMainMenu()
{
	Engine::GetInstance().uiManager->CleanUp();
	if (texMenuLogo_) { SDL_DestroyTexture(texMenuLogo_);                       texMenuLogo_ = nullptr; }
	if (texMenuChild_) { Engine::GetInstance().textures->UnLoad(texMenuChild_);  texMenuChild_ = nullptr; }
	if (texMenuButton_) { Engine::GetInstance().textures->UnLoad(texMenuButton_); texMenuButton_ = nullptr; }
	for (int i = 0; i < NUM_FRAGMENTS; i++) {
		if (fragments_[i].tex) { Engine::GetInstance().textures->UnLoad(fragments_[i].tex); fragments_[i].tex = nullptr; }
	}
	fragmentsInited_ = false;
}

void Scene::UpdateMainMenu(float dt)
{
	if (settingsCooldown_ > 0) settingsCooldown_--;

	fragmentTime_ += dt;

	if (menuAnimState_ != MenuAnimState::IDLE) {
		menuAnimTimer_ += dt;

		auto finishCinematic = [&]() {
			menuAnimState_ = MenuAnimState::IDLE;
			if (btnPlay_) { btnPlay_->alphaMod = 1.0f; btnPlay_->isVisible = true; }
			if (btnSettings_) { btnSettings_->alphaMod = 1.0f; btnSettings_->isVisible = true; }
			if (btnExit_) { btnExit_->alphaMod = 1.0f; btnExit_->isVisible = true; }
		};

		if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_SPACE) == KEY_DOWN ||
			Engine::GetInstance().input->GetKey(SDL_SCANCODE_RETURN) == KEY_DOWN ||
			Engine::GetInstance().input->GetKey(SDL_SCANCODE_ESCAPE) == KEY_DOWN)
		{
			finishCinematic();
			return;
		}

		if (menuAnimState_ == MenuAnimState::LOGO_FADE_IN && menuAnimTimer_ > 1500.0f) { menuAnimState_ = MenuAnimState::LOGO_HOLD;       menuAnimTimer_ = 0.0f; }
		else if (menuAnimState_ == MenuAnimState::LOGO_HOLD && menuAnimTimer_ > 1000.0f) { menuAnimState_ = MenuAnimState::SLIDE_LOGO;      menuAnimTimer_ = 0.0f; }
		else if (menuAnimState_ == MenuAnimState::SLIDE_LOGO && menuAnimTimer_ > 1500.0f) { menuAnimState_ = MenuAnimState::SLIDE_CHILD;     menuAnimTimer_ = 0.0f; }
		else if (menuAnimState_ == MenuAnimState::SLIDE_CHILD && menuAnimTimer_ > 1500.0f) { menuAnimState_ = MenuAnimState::FADE_FRAGS_BTNS; menuAnimTimer_ = 0.0f; }
		else if (menuAnimState_ == MenuAnimState::FADE_FRAGS_BTNS && menuAnimTimer_ > 3500.0f) { finishCinematic(); }
	}
	else
	{
		if (showSettings_ && Engine::GetInstance().input->GetKey(SDL_SCANCODE_ESCAPE) == KEY_DOWN)
		{
			showSettings_ = false;
			SetSettingsPanelVisible(false);
		}
	}
}

void Scene::PostUpdateMainMenu()
{
	auto& render = *Engine::GetInstance().render;
	int winW = 0, winH = 0;
	Engine::GetInstance().window->GetWindowSize(winW, winH);

	int childH = winH - 20;
	int childW = static_cast<int>(static_cast<float>(childH) * (1421.0f / 913.0f));
	int childDestX = winW - childW;

	const int leftHalf = winW / 2;
	int logoW = 385;
	int logoH = static_cast<int>(static_cast<float>(logoW) * (569.0f / 1559.0f));
	int logoDestX = (leftHalf - logoW) / 2;
	int logoDestY = winH / 4 - logoH / 2;

	if (!fragmentsInited_) {
		InitFragments(winW, winH, childDestX, childW);
		fragmentsInited_ = true;
	}

	int renderChildX = childDestX;
	int renderLogoX = logoDestX;
	int renderLogoY = logoDestY;
	int bgR = 21, bgG = 31, bgB = 32;

	if (menuAnimState_ != MenuAnimState::IDLE) {
		float t = 0.0f;

		switch (menuAnimState_) {
		case MenuAnimState::LOGO_FADE_IN:
			t = menuAnimTimer_ / 1500.0f;
			if (t > 1.0f) t = 1.0f;
			bgR = static_cast<int>(21.0f * t);
			bgG = static_cast<int>(31.0f * t);
			bgB = static_cast<int>(32.0f * t);
			renderLogoX = winW / 2 - logoW / 2;
			renderLogoY = winH / 2 - logoH / 2;
			renderChildX = winW;
			break;

		case MenuAnimState::LOGO_HOLD:
			renderLogoX = winW / 2 - logoW / 2;
			renderLogoY = winH / 2 - logoH / 2;
			renderChildX = winW;
			break;

		case MenuAnimState::SLIDE_LOGO:
			t = menuAnimTimer_ / 1500.0f;
			if (t > 1.0f) t = 1.0f;
			t = 1.0f - (float)pow(1.0f - t, 3.0f);
			renderLogoX = static_cast<int>((static_cast<float>(winW) / 2.0f - static_cast<float>(logoW) / 2.0f) + (static_cast<float>(logoDestX) - (static_cast<float>(winW) / 2.0f - static_cast<float>(logoW) / 2.0f)) * t);
			renderLogoY = static_cast<int>((static_cast<float>(winH) / 2.0f - static_cast<float>(logoH) / 2.0f) + (static_cast<float>(logoDestY) - (static_cast<float>(winH) / 2.0f - static_cast<float>(logoH) / 2.0f)) * t);
			renderChildX = winW;
			break;

		case MenuAnimState::SLIDE_CHILD:
			t = menuAnimTimer_ / 1500.0f;
			if (t > 1.0f) t = 1.0f;
			t = 1.0f - (float)pow(1.0f - t, 3.0f);
			renderChildX = (int)(winW + (childDestX - winW) * t);
			renderLogoX = logoDestX;
			renderLogoY = logoDestY;
			break;

		case MenuAnimState::FADE_FRAGS_BTNS:
			renderChildX = childDestX;
			renderLogoX = logoDestX;
			renderLogoY = logoDestY;
			if (btnPlay_) {
				float f0 = (menuAnimTimer_ - 0.0f) / 1000.0f;
				if (f0 < 0.0f) f0 = 0.0f; if (f0 > 1.0f) f0 = 1.0f;
				btnPlay_->alphaMod = f0;
				btnPlay_->isVisible = (f0 > 0.0f);
			}
			if (btnSettings_) {
				float f1 = (menuAnimTimer_ - 800.0f) / 1000.0f;
				if (f1 < 0.0f) f1 = 0.0f; if (f1 > 1.0f) f1 = 1.0f;
				btnSettings_->alphaMod = f1;
				btnSettings_->isVisible = (f1 > 0.0f);
			}
			if (btnExit_) {
				float f2 = (menuAnimTimer_ - 1600.0f) / 1000.0f;
				if (f2 < 0.0f) f2 = 0.0f; if (f2 > 1.0f) f2 = 1.0f;
				btnExit_->alphaMod = f2;
				btnExit_->isVisible = (f2 > 0.0f);
			}
			break;
		}

		if (menuAnimState_ < MenuAnimState::FADE_FRAGS_BTNS) {
			for (int i = 0; i < NUM_FRAGMENTS; i++) fragments_[i].alpha = 0;
		}
		else if (menuAnimState_ == MenuAnimState::FADE_FRAGS_BTNS) {
			for (int i = 0; i < NUM_FRAGMENTS; i++) {
				float f = (menuAnimTimer_ - (i * 400.0f)) / 1000.0f;
				if (f < 0.0f) f = 0.0f; if (f > 1.0f) f = 1.0f;
				fragments_[i].alpha = (Uint8)(255 * f);
			}
		}
	}
	else {
		for (int i = 0; i < NUM_FRAGMENTS; i++) fragments_[i].alpha = 255;
	}

	SDL_Rect bg = { 0, 0, winW, winH };
	render.DrawRectangle(bg, bgR, bgG, bgB, 255, true, false);

	DrawFragments(false, winW, winH);

	if (texMenuChild_) {
		int childY = 20;
		render.DrawTextureAlpha(texMenuChild_, renderChildX, childY, childW, childH, 255);
	}

	DrawFragments(true, winW, winH);

	if (texMenuLogo_) {
		render.DrawTextureAlpha(texMenuLogo_, renderLogoX, renderLogoY, logoW, logoH, 255);
	}

	if (showSettings_)
		DrawSettingsPanel(winW, winH);
}

void Scene::DrawSettingsPanel(int winW, int winH)
{
	auto& render = *Engine::GetInstance().render;

	const int panelW = 340;
	const int panelH = 240;
	const int panelX = winW / 2 - panelW / 2;
	const int panelY = 60;
	const int rowH = 52;

	SDL_Rect overlay = { 0, 0, winW, winH };
	render.DrawRectangle(overlay, 0, 0, 0, 160, true, false);

	SDL_Rect panel = { panelX, panelY, panelW, panelH };
	render.DrawRectangle(panel, 8, 12, 22, 250, true, false);
	render.DrawRectangle(panel, 60, 90, 150, 200, false, false);
	SDL_Rect topBar = { panelX, panelY, panelW, 4 };
	render.DrawRectangle(topBar, 80, 140, 200, 255, true, false);

	render.DrawMenuTextCentered("SETTINGS", { panelX, panelY + 8, panelW, 30 }, { 180, 210, 240, 255 });

	render.DrawMenuTextCentered("MUSIC", { panelX, panelY + 50, panelW / 2 - 10, 25 }, { 150, 180, 210, 220 });
	char volText[8];
	snprintf(volText, sizeof(volText), "%d%%", static_cast<int>(musicVolume_ * 100.0f));
	render.DrawMenuTextCentered(volText, { panelX + panelW / 2, panelY + 50, panelW / 2, 25 }, { 200, 220, 240, 255 });
	SDL_Rect barBg = { panelX + 20, panelY + 78, panelW - 40, 7 };
	render.DrawRectangle(barBg, 30, 40, 60, 200, true, false);
	int musicFill = static_cast<int>(static_cast<float>(panelW - 40) * musicVolume_);
	SDL_Rect barFill = { panelX + 20, panelY + 78, musicFill, 7 };
	render.DrawRectangle(barFill, 80, 160, 220, 255, true, false);

	render.DrawMenuTextCentered("SFX", { panelX, panelY + 50 + rowH, panelW / 2 - 10, 25 }, { 150, 180, 210, 220 });
	snprintf(volText, sizeof(volText), "%d%%", static_cast<int>(sfxVolume_ * 100.0f));
	render.DrawMenuTextCentered(volText, { panelX + panelW / 2, panelY + 50 + rowH, panelW / 2, 25 }, { 200, 220, 240, 255 });
	SDL_Rect sfxBarBg = { panelX + 20, panelY + 78 + rowH, panelW - 40, 7 };
	render.DrawRectangle(sfxBarBg, 30, 40, 60, 200, true, false);
	int sfxFill = static_cast<int>(static_cast<float>(panelW - 40) * sfxVolume_);
	SDL_Rect sfxBarFill = { panelX + 20, panelY + 78 + rowH, sfxFill, 7 };
	render.DrawRectangle(sfxBarFill, 80, 160, 220, 255, true, false);
}

void Scene::HandleMainMenuUIEvents(UIElement* uiElement)
{
	if (waitingForFade_)      return;
	if (settingsCooldown_ > 0) return;

	switch (uiElement->id)
	{
	case BTN_PLAY:
		LOG("Main Menu: Play");
		waitingForFade_ = true;
		fadeTargetScene_ = SceneID::INTRO_CINEMATIC;
		Engine::GetInstance().render->StartFade(FadeDirection::FADE_OUT, 1000.0f);
		break;
	case BTN_SETTINGS:
		showSettings_ = !showSettings_;
		SetSettingsPanelVisible(showSettings_);
		settingsCooldown_ = 4;
		break;
	case BTN_EXIT:
		LOG("Main Menu: Exit");
		SDL_Event quitEvent;
		quitEvent.type = SDL_EVENT_QUIT;
		SDL_PushEvent(&quitEvent);
		break;
	case BTN_MUSIC_UP:
		musicVolume_ = std::min(1.0f, musicVolume_ + 0.1f);
		Engine::GetInstance().audio->SetMusicVolume(musicVolume_);
		break;
	case BTN_MUSIC_DOWN:
		musicVolume_ = std::max(0.0f, musicVolume_ - 0.1f);
		Engine::GetInstance().audio->SetMusicVolume(musicVolume_);
		break;
	case BTN_SFX_UP:
		sfxVolume_ = std::min(1.0f, sfxVolume_ + 0.1f);
		Engine::GetInstance().audio->SetSFXVolume(sfxVolume_);
		break;
	case BTN_SFX_DOWN:
		sfxVolume_ = std::max(0.0f, sfxVolume_ - 0.1f);
		Engine::GetInstance().audio->SetSFXVolume(sfxVolume_);
		break;
	case BTN_SETTINGS_BACK:
		showSettings_ = false;
		SetSettingsPanelVisible(false);
		break;
	default:
		break;
	}
}

void Scene::SetSettingsPanelVisible(bool visible)
{
	auto& list = Engine::GetInstance().uiManager->UIElementsList;
	for (auto& el : list)
	{
		bool isSettingsBtn = (el->id >= BTN_SETTINGS_BACK && el->id <= BTN_SFX_DOWN) || el->id == BTN_SETTINGS_BACK;
		bool isMainBtn = (el->id == BTN_PLAY || el->id == BTN_SETTINGS || el->id == BTN_EXIT);

		if (isSettingsBtn) {
			el->state = visible ? UIElementState::NORMAL : UIElementState::DISABLED;
			el->isVisible = visible;
		}

		if (isMainBtn) {
			el->state = visible ? UIElementState::DISABLED : UIElementState::NORMAL;
			el->isVisible = !visible;
		}
	}
}

// ============================================================================
//  INTRO (Splash Logos)
// ============================================================================

static constexpr float INTRO_FADE_MS = 800.0f;
static constexpr float INTRO_HOLD_MS = 1500.0f;
static constexpr float INTRO_TOTAL_MS = INTRO_FADE_MS * 2.0f + INTRO_HOLD_MS;

void Scene::LoadIntro()
{
	LOG("Loading Intro splash screen");
	introPhase_ = IntroPhase::CITM_FADEIN;
	introTimer_ = 0.0f;

	Engine::GetInstance().render->StartFade(FadeDirection::FADE_IN, 800.0f);

	texCitmLogo_ = Engine::GetInstance().textures->Load("assets/textures/icons/logo-citm.png");
	texStudioPlaceholder_ = Engine::GetInstance().render->CreateMenuTextTexture("HIDDEN DREAM STUDIO", { 255, 255, 255, 255 });
}

void Scene::UnloadIntro()
{
	if (texCitmLogo_) { Engine::GetInstance().textures->UnLoad(texCitmLogo_); texCitmLogo_ = nullptr; }
	if (texStudioPlaceholder_) { SDL_DestroyTexture(texStudioPlaceholder_);            texStudioPlaceholder_ = nullptr; }
}

void Scene::UpdateIntro(float dt)
{
	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_SPACE) == KEY_DOWN) {
		if (!waitingForFade_) {
			waitingForFade_ = true;
			fadeTargetScene_ = SceneID::MAIN_MENU;
			Engine::GetInstance().render->StartFade(FadeDirection::FADE_OUT, 300.0f);
		}
	}

	if (waitingForFade_) { DrawIntro(); return; }

	introTimer_ += dt;

	switch (introPhase_) {
	case IntroPhase::CITM_FADEIN:   if (introTimer_ >= INTRO_FADE_MS) { introTimer_ = 0; introPhase_ = IntroPhase::CITM_HOLD; } break;
	case IntroPhase::CITM_HOLD:     if (introTimer_ >= INTRO_HOLD_MS) { introTimer_ = 0; introPhase_ = IntroPhase::CITM_FADEOUT; } break;
	case IntroPhase::CITM_FADEOUT:  if (introTimer_ >= INTRO_FADE_MS) { introTimer_ = 0; introPhase_ = IntroPhase::STUDIO_FADEIN; } break;
	case IntroPhase::STUDIO_FADEIN: if (introTimer_ >= INTRO_FADE_MS) { introTimer_ = 0; introPhase_ = IntroPhase::STUDIO_HOLD; } break;
	case IntroPhase::STUDIO_HOLD:   if (introTimer_ >= INTRO_HOLD_MS) { introTimer_ = 0; introPhase_ = IntroPhase::STUDIO_FADEOUT; } break;
	case IntroPhase::STUDIO_FADEOUT:
		if (introTimer_ >= INTRO_FADE_MS) {
			introPhase_ = IntroPhase::DONE;
			waitingForFade_ = true;
			fadeTargetScene_ = SceneID::MAIN_MENU;
			Engine::GetInstance().render->StartFade(FadeDirection::FADE_OUT, 500.0f);
		}
		break;
	case IntroPhase::DONE: break;
	}

	DrawIntro();
}

void Scene::DrawIntro()
{
	auto& render = *Engine::GetInstance().render;
	int winW = 0, winH = 0;
	Engine::GetInstance().window->GetWindowSize(winW, winH);

	SDL_Rect bg = { 0, 0, winW, winH };
	render.DrawRectangle(bg, 0, 0, 0, 255, true, false);

	SDL_Texture* logo = nullptr;
	Uint8  alpha = 0;
	float  cumTime = 0.0f;

	bool isCitm = (introPhase_ == IntroPhase::CITM_FADEIN || introPhase_ == IntroPhase::CITM_HOLD || introPhase_ == IntroPhase::CITM_FADEOUT);
	bool isStudio = (introPhase_ == IntroPhase::STUDIO_FADEIN || introPhase_ == IntroPhase::STUDIO_HOLD || introPhase_ == IntroPhase::STUDIO_FADEOUT);

	if (isCitm) {
		logo = texCitmLogo_;
		if (introPhase_ == IntroPhase::CITM_FADEIN) { alpha = (Uint8)(255.0f * (introTimer_ / INTRO_FADE_MS));               cumTime = introTimer_; }
		else if (introPhase_ == IntroPhase::CITM_HOLD) { alpha = 255;                                                             cumTime = INTRO_FADE_MS + introTimer_; }
		else { alpha = (Uint8)(255.0f * (1.0f - introTimer_ / INTRO_FADE_MS));         cumTime = INTRO_FADE_MS + INTRO_HOLD_MS + introTimer_; }
	}
	else if (isStudio) {
		logo = texStudioPlaceholder_;
		if (introPhase_ == IntroPhase::STUDIO_FADEIN) { alpha = (Uint8)(255.0f * (introTimer_ / INTRO_FADE_MS));             cumTime = introTimer_; }
		else if (introPhase_ == IntroPhase::STUDIO_HOLD) { alpha = 255;                                                           cumTime = INTRO_FADE_MS + introTimer_; }
		else { alpha = (Uint8)(255.0f * (1.0f - introTimer_ / INTRO_FADE_MS));       cumTime = INTRO_FADE_MS + INTRO_HOLD_MS + introTimer_; }
	}

	if (logo && alpha > 0) {
		float zoomProgress = cumTime / INTRO_TOTAL_MS;
		if (zoomProgress > 1.0f) zoomProgress = 1.0f;
		float easeT = zoomProgress * (2.0f - zoomProgress);
		float zoom = 1.0f + 0.05f * easeT;

		float tw = 0, th = 0;
		SDL_GetTextureSize(logo, &tw, &th);

		float targetW = winW * 0.45f;
		float logoScale = targetW / tw;
		if (logoScale > 3.0f) logoScale = 3.0f;

		float drawW = tw * logoScale * zoom;
		float drawH = th * logoScale * zoom;
		float drawX = (winW - drawW) / 2.0f;
		float drawY = (winH - drawH) / 2.0f;

		render.DrawTextureAlphaF(logo, drawX, drawY, drawW, drawH, alpha);
	}
}

// ============================================================================
//  INTRO CINEMATIC
// ============================================================================

void Scene::LoadIntroCinematic()
{
	LOG("Playing intro cinematic...");
	Engine::GetInstance().cinematics->PlayVideo("assets/video/test.mp4");
}

void Scene::UnloadIntroCinematic()
{
	Engine::GetInstance().cinematics->StopVideo();
}

void Scene::UpdateIntroCinematic(float dt)
{
	if (waitingForFade_) return;

	bool skipRequested = Engine::GetInstance().input->GetKey(SDL_SCANCODE_SPACE) == KEY_DOWN ||
		Engine::GetInstance().input->GetKey(SDL_SCANCODE_ESCAPE) == KEY_DOWN;

	if (skipRequested) {
		waitingForFade_ = true;
		fadeTargetScene_ = SceneID::GAMEPLAY;
		Engine::GetInstance().render->StartFade(FadeDirection::FADE_OUT, 1000.0f);
	}
	else if (!Engine::GetInstance().cinematics->IsPlaying()) {
		waitingForFade_ = true;
		fadeTargetScene_ = SceneID::GAMEPLAY;
		Engine::GetInstance().render->StartFade(FadeDirection::FADE_OUT, 0.0f);
	}
}

// ============================================================================
//  GAMEPLAY
// ============================================================================

void Scene::LoadGameplay()
{
	isPaused_ = false;
	showPauseOptions_ = false;
	showMapViewer_ = false;

	Engine::GetInstance().map->Load("assets/maps/", "MapTemplate.tmx");
	Engine::GetInstance().map->LoadEntities(player);

	if (player == nullptr) {
		player = std::dynamic_pointer_cast<Player>(
			Engine::GetInstance().entityManager->CreateEntity(EntityType::PLAYER));
		player->position = Vector2D(96.0f, 672.0f);
		player->Start();
	}

	LoadPauseMenuButtons();

	Engine::GetInstance().render->SetAmbientTint(140, 155, 190);


	auto setupAnimFromTSX = [](const char* tsxPath, Animation& anim, SDL_Texture*& tex) {
		anim = Animation();
		pugi::xml_document doc;
		if (!doc.load_file(tsxPath)) {
			LOG("Error: Failed to load TSX: %s", tsxPath);
			return;
		}

		pugi::xml_node tileset = doc.child("tileset");
		int tw = tileset.attribute("tilewidth").as_int();
		int th = tileset.attribute("tileheight").as_int();
		int count = tileset.attribute("tilecount").as_int();
		int columns = tileset.attribute("columns").as_int();
		std::string imgPath = tileset.child("image").attribute("source").as_string();
		
		std::string finalPath = "assets/textures/spritesheets/SS Healthbar/";
		size_t lastSlash = imgPath.find_last_of('/');
		if (lastSlash != std::string::npos) {
			finalPath += imgPath.substr(lastSlash + 1);
		} else {
			finalPath += imgPath;
		}

		tex = Engine::GetInstance().textures->Load(finalPath.c_str());
		if (!tex) LOG("Error: Failed to load texture from TSX: %s", finalPath.c_str());

		LOG("HUD TSX loaded: %s, %dx%d, %d frames", tsxPath, tw, th, count);

		for (int i = 0; i < count; i++) {
			SDL_Rect r;
			r.x = (i % columns) * tw;
			r.y = (i / columns) * th;
			r.w = tw;
			r.h = th;
			anim.AddFrame(r, 100);
		}
		anim.SetLoop(false);
	};

	setupAnimFromTSX("assets/textures/animations/HealthAnimations/SS_Healthbar-1.tsx", animHealth1_, texHealth1_);
	setupAnimFromTSX("assets/textures/animations/HealthAnimations/SS_Healthbar-2.tsx", animHealth2_, texHealth2_);
	setupAnimFromTSX("assets/textures/animations/HealthAnimations/SS_Healthbar-3.tsx", animHealth3_, texHealth3_);

	currentHealthUI_ = 3;
	activeHealthAnim_ = 0;
	isGameOver_ = false;
	texGameOver_ = Engine::GetInstance().render->CreateMenuTextTexture("GAME OVER, YOU DIED", { 255, 0, 0, 255 });

	// Game Over Button
	int winW = 0, winH = 0;
	Engine::GetInstance().window->GetWindowSize(winW, winH);
	
	// Load Game Over textures
	texGameOverBg_ = nullptr;
	texGameOverBtn_ = Engine::GetInstance().textures->Load("assets/textures/Menu/UI_Pause_Menu_button_white.png");
	texGameOverKid_ = Engine::GetInstance().textures->Load("assets/textures/Menu/UI_Kid_Fragmented2.png");
	texGameOverFrag1_ = Engine::GetInstance().textures->Load("assets/textures/Menu/UI_Fragment1.png");
	texGameOverFrag3_ = Engine::GetInstance().textures->Load("assets/textures/Menu/UI_Fragment3.png");
	texGameOverFrag4_ = Engine::GetInstance().textures->Load("assets/textures/Menu/UI_Fragment4.png");
	texGameOverFrag5_ = Engine::GetInstance().textures->Load("assets/textures/Menu/UI_Fragment5.png");

	SDL_Rect contBtnPos = { winW / 2 - 230, winH / 2 - 20, 460, 84 };
	SDL_Rect mainBtnPos = { winW / 2 - 230, winH / 2 + 84, 460, 84 };
	
	auto contBtn = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, BTN_GAMEOVER_CONTINUE, "CONTINUE", contBtnPos, this);
	auto goBtn = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, BTN_GAMEOVER_MAINMENU, "MAIN MENU", mainBtnPos, this);

	// Hide Game Over buttons by default
	contBtn->isVisible = false;
	goBtn->isVisible = false;

	if (texGameOverBtn_) {
		contBtn->SetTexture(texGameOverBtn_);
		goBtn->SetTexture(texGameOverBtn_);
	}
	if (goBtn) {
		goBtn->state = UIElementState::DISABLED;
	}
}

void Scene::UpdateGameplay(float dt)
{
	// Toggle pause with ESC
	if (!isGameOver_ && Engine::GetInstance().input->GetKey(SDL_SCANCODE_ESCAPE) == KEY_DOWN)
	{
		if (showMapViewer_) {
			showMapViewer_ = false;
			SetPauseMenuVisible(true);
		}
		else if (showPauseOptions_) {
			showPauseOptions_ = false;
			SetPauseOptionsPanelVisible(false);
		}
		else {
			isPaused_ = !isPaused_;
			SetPauseMenuVisible(isPaused_);
		}
	}

	if (showMapViewer_)
	{
		auto& input = *Engine::GetInstance().input;

		if (input.GetKey(SDL_SCANCODE_KP_PLUS) == KEY_DOWN || input.GetKey(SDL_SCANCODE_EQUALS) == KEY_DOWN)
			mapViewZoom_ = std::min(mapViewZoom_ + 0.05f, 2.0f);
		if (input.GetKey(SDL_SCANCODE_KP_MINUS) == KEY_DOWN || input.GetKey(SDL_SCANCODE_MINUS) == KEY_DOWN)
			mapViewZoom_ = std::max(mapViewZoom_ - 0.05f, 0.05f);

		Vector2D mousePos = input.GetMousePosition();
		if (input.GetMouseButtonDown(SDL_BUTTON_LEFT) == KEY_DOWN) {
			mapViewDragging_ = true;
			mapViewDragStartX_ = mousePos.getX();
			mapViewDragStartY_ = mousePos.getY();
			mapViewDragOriginX_ = mapViewOffsetX_;
			mapViewDragOriginY_ = mapViewOffsetY_;
		}
		if (input.GetMouseButtonDown(SDL_BUTTON_LEFT) == KEY_UP)
			mapViewDragging_ = false;

		if (mapViewDragging_) {
			mapViewOffsetX_ = mapViewDragOriginX_ + (mousePos.getX() - mapViewDragStartX_);
			mapViewOffsetY_ = mapViewDragOriginY_ + (mousePos.getY() - mapViewDragStartY_);
		}

		int winW = 0, winH = 0;
		Engine::GetInstance().window->GetWindowSize(winW, winH);
		DrawMapViewer(winW, winH);
		return;
	}

	if (isPaused_) DrawPauseMenu();

	if (isPaused_) return;

	if (player) {
		// Fall death check
		Vector2D mapSize = Engine::GetInstance().map->GetMapSizeInPixels();
		if (player->position.getY() > mapSize.getY() + 100.0f && player->health > 0) {
			player->TakeDamage(player->health);
			currentHealthUI_ = 0;
			activeHealthAnim_ = 0;
			isGameOver_ = true;
		}

		// HUD Logic
		if (player->health < currentHealthUI_) {
			if (currentHealthUI_ == 3) {
				activeHealthAnim_ = 1;
				animHealth1_.Reset();
			} else if (currentHealthUI_ == 2) {
				activeHealthAnim_ = 2;
				animHealth2_.Reset();
			} else if (currentHealthUI_ == 1) {
				activeHealthAnim_ = 3;
				animHealth3_.Reset();
			}
			currentHealthUI_ = player->health;
		}

		if (activeHealthAnim_ == 1) {
			animHealth1_.Update(dt);
			if (animHealth1_.HasFinishedOnce()) activeHealthAnim_ = 0;
		} else if (activeHealthAnim_ == 2) {
			animHealth2_.Update(dt);
			if (animHealth2_.HasFinishedOnce()) activeHealthAnim_ = 0;
		} else if (activeHealthAnim_ == 3) {
			animHealth3_.Update(dt);
			if (animHealth3_.HasFinishedOnce()) activeHealthAnim_ = 0;
		}

		if (player->health <= 0 && activeHealthAnim_ == 0 && !isGameOver_) {
			isGameOver_ = true;
			// Enable Game Over buttons
			auto& list = Engine::GetInstance().uiManager->UIElementsList;
			for (auto& el : list) {
				if (el->id == BTN_GAMEOVER_MAINMENU || el->id == BTN_GAMEOVER_CONTINUE) {
					el->isVisible = true;
					el->state = UIElementState::NORMAL;
				}
				
				if (el->id == BTN_GAMEOVER_CONTINUE) {
					if (!Engine::GetInstance().saveSystem->HasValidSave())
						el->state = UIElementState::DISABLED;
				}
			}
		}
	}
}

void Scene::UnloadGameplay()
{
	Engine::GetInstance().uiManager->CleanUp();
	player.reset();
	Engine::GetInstance().map->CleanUp();
	Engine::GetInstance().entityManager->CleanUp();
	isPaused_ = false;
	showPauseOptions_ = false;
	showMapViewer_ = false;

	if (texHealth1_) { Engine::GetInstance().textures->UnLoad(texHealth1_); texHealth1_ = nullptr; }
	if (texHealth2_) { Engine::GetInstance().textures->UnLoad(texHealth2_); texHealth2_ = nullptr; }
	if (texHealth3_) { Engine::GetInstance().textures->UnLoad(texHealth3_); texHealth3_ = nullptr; }
	if (texGameOver_) { SDL_DestroyTexture(texGameOver_); texGameOver_ = nullptr; }
	
	if (texGameOverBg_) { Engine::GetInstance().textures->UnLoad(texGameOverBg_); texGameOverBg_ = nullptr; }
	if (texGameOverBtn_) { Engine::GetInstance().textures->UnLoad(texGameOverBtn_); texGameOverBtn_ = nullptr; }
	if (texGameOverKid_) { Engine::GetInstance().textures->UnLoad(texGameOverKid_); texGameOverKid_ = nullptr; }
	if (texGameOverFrag1_) { Engine::GetInstance().textures->UnLoad(texGameOverFrag1_); texGameOverFrag1_ = nullptr; }
	if (texGameOverFrag3_) { Engine::GetInstance().textures->UnLoad(texGameOverFrag3_); texGameOverFrag3_ = nullptr; }
	if (texGameOverFrag5_) { Engine::GetInstance().textures->UnLoad(texGameOverFrag5_); texGameOverFrag5_ = nullptr; }

	if (texPauseBackground_) { Engine::GetInstance().textures->UnLoad(texPauseBackground_);  texPauseBackground_ = nullptr; }
	if (texPauseButtonWhite_) { Engine::GetInstance().textures->UnLoad(texPauseButtonWhite_); texPauseButtonWhite_ = nullptr; }
	if (texPauseButtonBlack_) { Engine::GetInstance().textures->UnLoad(texPauseButtonBlack_); texPauseButtonBlack_ = nullptr; }
}

void Scene::PostUpdateGameplay()
{
	// Quick save/load shortcuts (only when not paused)
	if (!isPaused_ && !isGameOver_) {
		if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_F5) == KEY_DOWN)
			Engine::GetInstance().saveSystem->QuickLoad();
		if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_F6) == KEY_DOWN)
			Engine::GetInstance().saveSystem->QuickSave();
	}

	// --- Draw Health HUD ---
	SDL_Rect r;
	const SDL_Rect* frame = nullptr;
	SDL_Texture* texToDraw = nullptr;

	if (activeHealthAnim_ == 1) {
		texToDraw = texHealth1_;
		frame = &animHealth1_.GetCurrentFrame();
	} else if (activeHealthAnim_ == 2) {
		texToDraw = texHealth2_;
		frame = &animHealth2_.GetCurrentFrame();
	} else if (activeHealthAnim_ == 3) {
		texToDraw = texHealth3_;
		frame = &animHealth3_.GetCurrentFrame();
	} else {
		// Draw static frame based on health
		if (currentHealthUI_ == 3) {
			texToDraw = texHealth1_;
			if (texHealth1_) {
				r = animHealth1_.GetCurrentFrame(); // Use frame 0 (Reset ensures it's at 0)
				frame = &r;
			}
		} else if (currentHealthUI_ == 2) {
			texToDraw = texHealth2_;
			if (texHealth2_) {
				r = animHealth2_.GetCurrentFrame();
				frame = &r;
			}
		} else if (currentHealthUI_ == 1) {
			texToDraw = texHealth3_;
			if (texHealth3_) {
				r = animHealth3_.GetCurrentFrame();
				frame = &r;
			}
		} else if (currentHealthUI_ <= 0) {
			texToDraw = texHealth3_;
			if (texHealth3_) {
				r = animHealth3_.GetCurrentFrame();
				frame = &r;
			}
		}
	}

	if (texToDraw && frame) {
		// Draw HUD at top left. Using speed = 0.0f makes it static to camera
		// Scale reduced to 0.5f as requested
		Engine::GetInstance().render->DrawTexture(texToDraw, 40, 40, frame, 0.0f, 0, INT_MAX, INT_MAX, SDL_FLIP_NONE, 0.5f);
	}

	// --- Game Over Screen ---
	if (isGameOver_) {
		int winW = 0, winH = 0;
		Engine::GetInstance().window->GetWindowSize(winW, winH);

		// Dark overlay removed as requested

		// "GAME OVER" Text
		if (texGameOver_) {
			float tw, th;
			SDL_GetTextureSize(texGameOver_, &tw, &th);
			float drawX = ((float)winW - tw) / 2.0f;
			float drawY = ((float)winH - th) / 2.0f - 200.0f; 
			
			float scale = 1.5f;
			float sw = tw * scale;
			float sh = th * scale;
			float sx = ((float)winW - sw) / 2.0f;
			float sy = drawY - (sh - th) / 2.0f;

			// Enable linear filtering for smoother scaling
			SDL_SetTextureScaleMode(texGameOver_, SDL_SCALEMODE_LINEAR);

			// Directional Shadow: Draw once with a fixed offset and low alpha
			float shadowOffset = 6.0f;
			SDL_SetTextureColorMod(texGameOver_, 255, 100, 100); // Shadow color (light red)
			Engine::GetInstance().render->DrawTextureAlphaF(texGameOver_, sx + shadowOffset, sy + shadowOffset, sw, sh, (Uint8)130);
			
			SDL_SetTextureColorMod(texGameOver_, 255, 255, 255); // Reset color mod

			// Main text draw
			Engine::GetInstance().render->DrawTextureAlphaF(texGameOver_, sx, sy, sw, sh, (Uint8)255);
		}

		// Draw Fragments and Kid
		if (texGameOverKid_) {
			float kw, kh;
			SDL_GetTextureSize(texGameOverKid_, &kw, &kh);
			float kScale = 0.22f; // Reduced size as requested
			Engine::GetInstance().render->DrawTextureAlphaF(texGameOverKid_, (float)winW - kw * kScale - 20.0f, (float)winH - kh * kScale - 20.0f, kw * kScale, kh * kScale, (Uint8)255);
		}
		
		// Distributed fragments
		if (texGameOverFrag1_) Engine::GetInstance().render->DrawTextureAlphaF(texGameOverFrag1_, 100.0f, 150.0f, 160.0f, 160.0f, 180);
		if (texGameOverFrag3_) Engine::GetInstance().render->DrawTextureAlphaF(texGameOverFrag3_, (float)winW - 350.0f, 80.0f, 200.0f, 200.0f, 150);
		if (texGameOverFrag5_) Engine::GetInstance().render->DrawTextureAlphaF(texGameOverFrag5_, 200.0f, (float)winH - 300.0f, 140.0f, 140.0f, 160);
		if (texGameOverFrag1_) Engine::GetInstance().render->DrawTextureAlphaF(texGameOverFrag1_, (float)winW / 2.0f + 250.0f, (float)winH / 2.0f + 120.0f, 120.0f, 120.0f, 130);
		
		// New Fragment 4 in bottom-left
		if (texGameOverFrag4_) Engine::GetInstance().render->DrawTextureAlphaF(texGameOverFrag4_, 50.0f, (float)winH - 220.0f, 180.0f, 180.0f, 200);
	}
}

// ── Map viewer ────────────────────────────────────────────────────────────────

void Scene::DrawMapViewer(int winW, int winH)
{
	auto& render = *Engine::GetInstance().render;
	auto& map = *Engine::GetInstance().map;
	int   scale = Engine::GetInstance().window->GetScale();

	SDL_Rect fullBg = { 0, 0, winW, winH };
	render.DrawRectangle(fullBg, 5, 8, 15, 240, true, false);

	SDL_Rect titleBar = { 0, 0, winW, 36 };
	render.DrawRectangle(titleBar, 10, 16, 30, 255, true, false);
	SDL_Rect titleAccent = { 0, 34, winW, 2 };
	render.DrawRectangle(titleAccent, 80, 140, 200, 255, true, false);
	render.DrawMenuTextCentered("MAP", { 0, 2, winW, 30 }, { 180, 210, 240, 255 });

	SDL_Rect hintBar = { 0, winH - 28, winW, 28 };
	render.DrawRectangle(hintBar, 10, 16, 30, 220, true, false);
	render.DrawText("Left click + drag: pan   |   +/-: zoom   |   ESC: back",
		winW / 2 - 230, winH - 22, 0, 0, { 120, 160, 200, 200 });

	const int viewX = 10;
	const int viewY = 42;
	const int viewW = winW - 20;
	const int viewH = winH - 42 - 30;

	SDL_Rect clipRect = { viewX * scale, viewY * scale, viewW * scale, viewH * scale };
	SDL_SetRenderClipRect(render.renderer, &clipRect);

	int tileW = map.GetTileWidth();
	int tileH = map.GetTileHeight();

	// Mouse wheel zoom
	{
		SDL_PumpEvents();
		SDL_Event events[16];
		int count = SDL_PeepEvents(events, 16, SDL_PEEKEVENT,
			SDL_EVENT_MOUSE_WHEEL, SDL_EVENT_MOUSE_WHEEL);
		for (int i = 0; i < count; i++) {
			float wheelY = events[i].wheel.y;
			float newZoom = mapViewZoom_ + wheelY * 0.05f;
			if (newZoom < 0.05f) newZoom = 0.05f;
			if (newZoom > 2.0f)  newZoom = 2.0f;
			Vector2D mousePos = Engine::GetInstance().input->GetMousePosition();
			float mx = mousePos.getX() - (float)viewX;
			float my = mousePos.getY() - (float)viewY;
			mapViewOffsetX_ = mx - (mx - mapViewOffsetX_) * (newZoom / mapViewZoom_);
			mapViewOffsetY_ = my - (my - mapViewOffsetY_) * (newZoom / mapViewZoom_);
			mapViewZoom_ = newZoom;
		}
		SDL_PeepEvents(events, count, SDL_GETEVENT,
			SDL_EVENT_MOUSE_WHEEL, SDL_EVENT_MOUSE_WHEEL);
	}

	float invZoom = 1.0f / mapViewZoom_;
	float camLeft = -mapViewOffsetX_ * invZoom;
	float camTop = -mapViewOffsetY_ * invZoom;
	float camRight = camLeft + (float)viewW * invZoom;
	float camBottom = camTop + (float)viewH * invZoom;

	int startTileX = std::max(0, (int)(camLeft / (float)tileW) - 1);
	int startTileY = std::max(0, (int)(camTop / (float)tileH) - 1);
	int endTileX = std::min((int)map.GetMapSizeInTiles().getX(), (int)(camRight / (float)tileW) + 2);
	int endTileY = std::min((int)map.GetMapSizeInTiles().getY(), (int)(camBottom / (float)tileH) + 2);

	// ── Step 1: Image Layers ─────────────────────────────────────────────────
	for (const auto& imgLayer : map.mapData.imageLayers)
	{
		if (!imgLayer->texture) continue;
		float tw, th;
		SDL_GetTextureSize(imgLayer->texture, &tw, &th);
		float screenX = (float)viewX + mapViewOffsetX_ + imgLayer->offsetX * mapViewZoom_;
		float screenY = (float)viewY + mapViewOffsetY_ + imgLayer->offsetY * mapViewZoom_;
		float screenW = tw * mapViewZoom_;
		float screenH = th * mapViewZoom_;
		if (screenX + screenW < (float)viewX || screenX > (float)viewX + (float)viewW) continue;
		if (screenY + screenH < (float)viewY || screenY > (float)viewY + (float)viewH) continue;
		SDL_FRect dst = { screenX * (float)scale, screenY * (float)scale, screenW * (float)scale, screenH * (float)scale };
		SDL_SetTextureColorMod(imgLayer->texture, 255, 255, 255);
		SDL_SetTextureAlphaMod(imgLayer->texture, 255);
		SDL_RenderTexture(render.renderer, imgLayer->texture, nullptr, &dst);
	}

	// ── Step 2: Decoration Objects ───────────────────────────────────────────
	for (const auto& deco : map.mapData.decorationObjects)
	{
		if (!deco->texture) continue;
		float worldX = deco->x;
		float worldY = deco->y - deco->height;
		float screenX = (float)viewX + mapViewOffsetX_ + worldX * mapViewZoom_;
		float screenY = (float)viewY + mapViewOffsetY_ + worldY * mapViewZoom_;
		float screenW = deco->width * mapViewZoom_;
		float screenH = deco->height * mapViewZoom_;
		if (screenX + screenW < (float)viewX || screenX > (float)viewX + (float)viewW) continue;
		if (screenY + screenH < (float)viewY || screenY > (float)viewY + (float)viewH) continue;
		SDL_FRect dst = { screenX * (float)scale, screenY * (float)scale, screenW * (float)scale, screenH * (float)scale };
		SDL_SetTextureColorMod(deco->texture, 255, 255, 255);
		SDL_SetTextureAlphaMod(deco->texture, 255);
		SDL_RenderTexture(render.renderer, deco->texture, nullptr, &dst);
	}

	// ── Step 3: Tile Layers ──────────────────────────────────────────────────
	for (const auto& layer : map.mapData.layers)
	{
		auto* drawProp = layer->properties.GetProperty("Draw");
		if (drawProp && !drawProp->value) continue;

		for (int ty = startTileY; ty < endTileY; ty++)
		{
			for (int tx = startTileX; tx < endTileX; tx++)
			{
				int gid = layer->Get(tx, ty);
				if (gid == 0) continue;
				TileSet* ts = map.GetTilesetFromTileId(gid);
				if (!ts || !ts->texture) continue;
				SDL_Rect src = ts->GetRect(gid);
				float worldX = (float)(tx * tileW);
				float worldY = (float)(ty * tileH);
				float screenX = (float)viewX + mapViewOffsetX_ + worldX * mapViewZoom_;
				float screenY = (float)viewY + mapViewOffsetY_ + worldY * mapViewZoom_;
				float screenW = (float)tileW * mapViewZoom_;
				float screenH = (float)tileH * mapViewZoom_;
				if (screenX + screenW < (float)viewX || screenX > (float)viewX + (float)viewW) continue;
				if (screenY + screenH < (float)viewY || screenY > (float)viewY + (float)viewH) continue;
				SDL_FRect dst = { screenX * (float)scale, screenY * (float)scale, screenW * (float)scale, screenH * (float)scale };
				SDL_FRect srcF = { (float)src.x, (float)src.y, (float)src.w, (float)src.h };
				SDL_SetTextureColorMod(ts->texture, 255, 255, 255);
				SDL_SetTextureAlphaMod(ts->texture, 255);
				SDL_RenderTexture(render.renderer, ts->texture, &srcF, &dst);
			}
		}
	}

	// ── Step 4: Player sprite + marcador de posicion ─────────────────────────
	if (player && player->texture)
	{
		// Centro del player en coordenadas mundo
		Vector2D playerPos = player->GetPosition();
		float worldX = playerPos.getX() + (float)player->texW / 2.0f;
		float worldY = playerPos.getY() + (float)player->texH / 2.0f;

		// Centro del player en pantalla (espacio de la ventana, sin escala SDL)
		float screenX = (float)viewX + mapViewOffsetX_ + worldX * mapViewZoom_;
		float screenY = (float)viewY + mapViewOffsetY_ + worldY * mapViewZoom_;

		// ── Sprite del player ────────────────────────────────────────────────
		SDL_Rect src = player->GetCurrentAnimationRect();
		SDL_FRect srcF = { (float)src.x, (float)src.y, (float)src.w, (float)src.h };

		float drawW = 128.0f * mapViewZoom_;
		float drawH = 128.0f * mapViewZoom_;

		SDL_FRect dst = {
			(screenX - drawW / 2.0f) * (float)scale,
			(screenY - drawH / 2.0f) * (float)scale,
			drawW * (float)scale,
			drawH * (float)scale
		};

		bool spriteNativeRight = (src.y / 128 == 3 || src.y / 128 == 1);
		bool facingRight = player->IsFacingRight();
		SDL_FlipMode flip;
		if (spriteNativeRight)
			flip = facingRight ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
		else
			flip = facingRight ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL;

		SDL_SetTextureColorMod(player->texture, 255, 255, 255);
		SDL_SetTextureAlphaMod(player->texture, 200);
		SDL_RenderTextureRotated(render.renderer, player->texture, &srcF, &dst, 0, nullptr, flip);
		SDL_SetTextureAlphaMod(player->texture, 255);

		// ── Marcador "YOU ARE HERE" encima del player ────────────────────────
		// Radio del circulo escalado con el zoom (minimo 4px, maximo razonable)
		int r = (int)(9.0f * mapViewZoom_ * (float)scale);
		if (r < 4) r = 4;

		// Posicion del marcador: encima del sprite
		int markerCX = (int)(screenX * (float)scale);
		int markerCY = (int)((screenY - drawH / 2.0f - 6.0f) * (float)scale);

		// Sombra negra (cuadrado ligeramente mas grande)
		SDL_Rect shadowRect = {
			markerCX - r - 2,
			markerCY - r - 2,
			(r + 2) * 2,
			(r + 2) * 2
		};
		render.DrawRectangle(shadowRect, 0, 0, 0, 180, true, false);

		// Circulo amarillo solido (representado como rect; si tu render tiene DrawCircle, usalo)
		SDL_Rect dotRect = {
			markerCX - r,
			markerCY - r,
			r * 2,
			r * 2
		};
		render.DrawRectangle(dotRect, 255, 215, 0, 255, true, false);

		// Punto blanco interior para contraste
		int innerR = std::max(2, r / 3);
		SDL_Rect innerDot = {
			markerCX - innerR,
			markerCY - innerR,
			innerR * 2,
			innerR * 2
		};
		render.DrawRectangle(innerDot, 255, 255, 255, 255, true, false);

		// Etiqueta "YOU" encima del marcador (coordenadas sin escala SDL para DrawText)
		int labelX = (int)(screenX)-10;
		int labelY = (int)(screenY - drawH / 2.0f - 6.0f) - (int)((float)r / (float)scale) - 14;
		render.DrawText("YOU", labelX, labelY, 0, 0, { 255, 215, 0, 255 });
	}

	SDL_SetRenderClipRect(render.renderer, nullptr);

	SDL_Rect viewBorder = { viewX, viewY, viewW, viewH };
	render.DrawRectangle(viewBorder, 60, 90, 150, 180, false, false);

	char zoomText[16];
	snprintf(zoomText, sizeof(zoomText), "%.0f%%", mapViewZoom_ * 100.0f);
	render.DrawText(zoomText, winW - 60, winH - 22, 0, 0, { 120, 160, 200, 200 });
}

// ── Pause menu helpers ────────────────────────────────────────────────────────

void Scene::LoadPauseMenuButtons()
{
	texPauseBackground_ = Engine::GetInstance().textures->Load("assets/textures/menu/UI_Pause_Menu_base+background.png");
	texPauseButtonWhite_ = Engine::GetInstance().textures->Load("assets/textures/menu/UI_Pause_Menu_button_white.png");
	texPauseButtonBlack_ = Engine::GetInstance().textures->Load("assets/textures/menu/UI_Pause_Menu_button_black.png");

	int winW = 0, winH = 0;
	Engine::GetInstance().window->GetWindowSize(winW, winH);

	const int btnW = 315;
	const int btnH = static_cast<int>(static_cast<float>(btnW) * (130.0f / 456.0f));
	const int btnGap = 15;
	const int totalH = btnH * 4 + btnGap * 3;
	const int leftHalf = winW / 2;
	const int btnX = (leftHalf - btnW) / 2;
	const int startY = (winH - totalH) / 2;

	SDL_Rect optPos = { btnX, startY,                       btnW, btnH };
	SDL_Rect mapPos = { btnX, startY + (btnH + btnGap),     btnW, btnH };
	SDL_Rect menuPos = { btnX, startY + (btnH + btnGap) * 2, btnW, btnH };
	SDL_Rect contPos = { btnX, startY + (btnH + btnGap) * 3, btnW, btnH };

	auto btnOpt = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, BTN_PAUSE_OPTIONS, "options", optPos, this);
	auto btnMap = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, BTN_PAUSE_MAP, "map", mapPos, this);
	auto btnMenu = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, BTN_PAUSE_MAINMENU, "menu", menuPos, this);
	auto btnCont = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, BTN_PAUSE_CONTINUE, "resume", contPos, this);

	if (texPauseButtonWhite_) {
		btnOpt->SetTexture(texPauseButtonWhite_);
		btnMap->SetTexture(texPauseButtonWhite_);
		btnMenu->SetTexture(texPauseButtonWhite_);
		btnCont->SetTexture(texPauseButtonWhite_);
	}
	if (texPauseButtonBlack_) {
		btnOpt->SetHoverTexture(texPauseButtonBlack_);
		btnMap->SetHoverTexture(texPauseButtonBlack_);
		btnMenu->SetHoverTexture(texPauseButtonBlack_);
		btnCont->SetHoverTexture(texPauseButtonBlack_);
	}

	const int panelW = 340;
	const int panelX = winW / 2 - panelW / 2;
	const int panelY = winH / 2 - 100;
	const int smallBtnW = 40;
	const int smallBtnH = 34;
	const int rowH = 52;

	SDL_Rect mUpPos = { panelX + panelW - smallBtnW - 12, panelY + 60,                  smallBtnW, smallBtnH };
	SDL_Rect mDownPos = { panelX + 12,                       panelY + 60,                  smallBtnW, smallBtnH };
	SDL_Rect sUpPos = { panelX + panelW - smallBtnW - 12, panelY + 60 + rowH,           smallBtnW, smallBtnH };
	SDL_Rect sDownPos = { panelX + 12,                       panelY + 60 + rowH,           smallBtnW, smallBtnH };
	SDL_Rect backPos = { panelX + panelW / 2 - 60,          panelY + 60 + rowH * 2 + 10, 120,       btnH };

	Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, BTN_PAUSE_OPT_MUSIC_UP, "+", mUpPos, this);
	Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, BTN_PAUSE_OPT_MUSIC_DOWN, "-", mDownPos, this);
	Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, BTN_PAUSE_OPT_SFX_UP, "+", sUpPos, this);
	Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, BTN_PAUSE_OPT_SFX_DOWN, "-", sDownPos, this);
	Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, BTN_PAUSE_OPT_BACK, "back", backPos, this);

	SetPauseMenuVisible(false);
}

void Scene::SetPauseMenuVisible(bool visible)
{
	auto& list = Engine::GetInstance().uiManager->UIElementsList;
	for (auto& el : list) {
		if (el->id == BTN_PAUSE_CONTINUE || el->id == BTN_PAUSE_OPTIONS || el->id == BTN_PAUSE_SAVE || el->id == BTN_PAUSE_MAINMENU || el->id == BTN_PAUSE_QUIT || el->id == BTN_PAUSE_MAP) {
			el->isVisible = visible;
			el->state = visible ? UIElementState::NORMAL : UIElementState::DISABLED;
		}
		
		bool isOpt = (el->id >= BTN_PAUSE_OPT_MUSIC_UP && el->id <= BTN_PAUSE_OPT_BACK);
		if (isOpt) {
			el->state = UIElementState::DISABLED;
			el->isVisible = false;
		}
	}
}

void Scene::SetPauseOptionsPanelVisible(bool visible)
{
	auto& list = Engine::GetInstance().uiManager->UIElementsList;
	for (auto& el : list)
	{
		bool isPauseMain = (el->id == BTN_PAUSE_CONTINUE || el->id == BTN_PAUSE_OPTIONS ||
			el->id == BTN_PAUSE_MAP || el->id == BTN_PAUSE_MAINMENU ||
			el->id == BTN_PAUSE_QUIT);
		bool isOpt = (el->id >= BTN_PAUSE_OPT_MUSIC_UP && el->id <= BTN_PAUSE_OPT_BACK);

		if (isPauseMain) {
			el->state = visible ? UIElementState::DISABLED : UIElementState::NORMAL;
			el->isVisible = !visible;
		}
		if (isOpt) {
			el->state = visible ? UIElementState::NORMAL : UIElementState::DISABLED;
			el->isVisible = visible;
		}
	}
}

void Scene::SetGameOverVisible(bool visible)
{
	isGameOver_ = visible;
	auto& list = Engine::GetInstance().uiManager->UIElementsList;
	for (auto& el : list) {
		if (el->id == BTN_GAMEOVER_CONTINUE || el->id == BTN_GAMEOVER_MAINMENU) {
			el->isVisible = visible;
		}
	}
}

void Scene::ResetHealthUI(int health)
{
	currentHealthUI_ = health;
	activeHealthAnim_ = 0;
	animHealth1_.Reset();
	animHealth2_.Reset();
	animHealth3_.Reset();
}

void Scene::DrawPauseMenu()
{
	auto& render = *Engine::GetInstance().render;
	int winW = 0, winH = 0;
	Engine::GetInstance().window->GetWindowSize(winW, winH);

	if (showPauseOptions_)
	{
		SDL_Rect overlay = { 0, 0, winW, winH };
		render.DrawRectangle(overlay, 0, 0, 0, 180, true, false);
		DrawPauseOptionsPanel(winW, winH);
		return;
	}

	if (texPauseBackground_)
		render.DrawTextureAlpha(texPauseBackground_, 0, 0, winW, winH, 255);
	else {
		SDL_Rect overlay = { 0, 0, winW, winH };
		render.DrawRectangle(overlay, 0, 0, 0, 180, true, false);
	}
}

void Scene::DrawPauseOptionsPanel(int winW, int winH)
{
	auto& render = *Engine::GetInstance().render;

	const int panelW = 340;
	const int panelH = 240;
	const int panelX = winW / 2 - panelW / 2;
	const int panelY = winH / 2 - 100;
	const int rowH = 52;

	SDL_Rect panel = { panelX, panelY, panelW, panelH };
	render.DrawRectangle(panel, 8, 12, 22, 250, true, false);
	render.DrawRectangle(panel, 60, 90, 150, 200, false, false);
	SDL_Rect topBar = { panelX, panelY, panelW, 4 };
	render.DrawRectangle(topBar, 80, 140, 200, 255, true, false);

	render.DrawMenuTextCentered("OPTIONS", { panelX, panelY + 8, panelW, 30 }, { 180, 210, 240, 255 });

	render.DrawMenuTextCentered("MUSIC", { panelX, panelY + 50, panelW / 2 - 10, 25 }, { 150, 180, 210, 220 });
	char vol[8];
	snprintf(vol, sizeof(vol), "%d%%", static_cast<int>(musicVolume_ * 100.0f));
	render.DrawMenuTextCentered(vol, { panelX + panelW / 2, panelY + 50, panelW / 2, 25 }, { 200, 220, 240, 255 });
	SDL_Rect mBarBg = { panelX + 20, panelY + 78, panelW - 40, 7 };
	render.DrawRectangle(mBarBg, 30, 40, 60, 200, true, false);
	int mFill = static_cast<int>(static_cast<float>(panelW - 40) * musicVolume_);
	SDL_Rect mBarFill = { panelX + 20, panelY + 78, mFill, 7 };
	render.DrawRectangle(mBarFill, 80, 160, 220, 255, true, false);

	render.DrawMenuTextCentered("SFX", { panelX, panelY + 50 + rowH, panelW / 2 - 10, 25 }, { 150, 180, 210, 220 });
	snprintf(vol, sizeof(vol), "%d%%", static_cast<int>(sfxVolume_ * 100.0f));
	render.DrawMenuTextCentered(vol, { panelX + panelW / 2, panelY + 50 + rowH, panelW / 2, 25 }, { 200, 220, 240, 255 });
	SDL_Rect sBarBg = { panelX + 20, panelY + 78 + rowH, panelW - 40, 7 };
	render.DrawRectangle(sBarBg, 30, 40, 60, 200, true, false);
	int sFill = static_cast<int>(static_cast<float>(panelW - 40) * sfxVolume_);
	SDL_Rect sBarFill = { panelX + 20, panelY + 78 + rowH, sFill, 7 };
	render.DrawRectangle(sBarFill, 80, 160, 220, 255, true, false);
}

void Scene::HandlePauseMenuUIEvents(UIElement* uiElement)
{
	if (waitingForFade_) return;

	switch (uiElement->id)
	{
	case BTN_PAUSE_CONTINUE:
		isPaused_ = false;
		SetPauseMenuVisible(false);
		break;
	case BTN_PAUSE_OPTIONS:
		showPauseOptions_ = true;
		SetPauseOptionsPanelVisible(true);
		break;
	case BTN_PAUSE_MAP:
		showMapViewer_ = true;
		SetPauseMenuVisible(false);
		{
			int winW = 0, winH = 0;
			Engine::GetInstance().window->GetWindowSize(winW, winH);
			const int viewW = winW - 20;
			const int viewH = winH - 42 - 30;
			Vector2D mapSizePx = Engine::GetInstance().map->GetMapSizeInPixels();
			float zoomX = (float)viewW / mapSizePx.getX();
			float zoomY = (float)viewH / mapSizePx.getY();
			mapViewZoom_ = std::min(zoomX, zoomY) * 0.95f;
			if (mapViewZoom_ < 0.05f) mapViewZoom_ = 0.05f;
			if (mapViewZoom_ > 2.0f)  mapViewZoom_ = 2.0f;
			mapViewOffsetX_ = ((float)viewW - mapSizePx.getX() * mapViewZoom_) / 2.0f;
			mapViewOffsetY_ = ((float)viewH - mapSizePx.getY() * mapViewZoom_) / 2.0f;
		}
		break;
	case BTN_PAUSE_SAVE:
		Engine::GetInstance().saveSystem->QuickSave();
		LOG("Game saved from pause menu");
		break;
	case BTN_PAUSE_MAINMENU:
		isPaused_ = false;
		waitingForFade_ = true;
		fadeTargetScene_ = SceneID::MAIN_MENU;
		Engine::GetInstance().render->StartFade(FadeDirection::FADE_OUT, 500.0f);
		break;
	case BTN_PAUSE_QUIT:
	{
		SDL_Event quitEvent;
		quitEvent.type = SDL_EVENT_QUIT;
		SDL_PushEvent(&quitEvent);
		break;
	}
	case BTN_PAUSE_OPT_MUSIC_UP:
		musicVolume_ = std::min(1.0f, musicVolume_ + 0.1f);
		Engine::GetInstance().audio->SetMusicVolume(musicVolume_);
		break;
	case BTN_PAUSE_OPT_MUSIC_DOWN:
		musicVolume_ = std::max(0.0f, musicVolume_ - 0.1f);
		Engine::GetInstance().audio->SetMusicVolume(musicVolume_);
		break;
	case BTN_PAUSE_OPT_SFX_UP:
		sfxVolume_ = std::min(1.0f, sfxVolume_ + 0.1f);
		Engine::GetInstance().audio->SetSFXVolume(sfxVolume_);
		break;
	case BTN_PAUSE_OPT_SFX_DOWN:
		sfxVolume_ = std::max(0.0f, sfxVolume_ - 0.1f);
		Engine::GetInstance().audio->SetSFXVolume(sfxVolume_);
		break;
	case BTN_PAUSE_OPT_BACK:
		showPauseOptions_ = false;
		SetPauseOptionsPanelVisible(false);
		break;
	
	case BTN_GAMEOVER_MAINMENU:
		isGameOver_ = false;
		waitingForFade_ = true;
		fadeTargetScene_ = SceneID::MAIN_MENU;
		Engine::GetInstance().render->StartFade(FadeDirection::FADE_OUT, 500.0f);
		break;
	
	case BTN_GAMEOVER_CONTINUE:
	{
		SetGameOverVisible(false);
		Engine::GetInstance().saveSystem->QuickLoad();
		break;
	}

	default:
		break;
	}
}

// ============================================================================
//  FLOATING FRAGMENTS  (Main Menu Decoration)
// ============================================================================

static float RandF(float lo, float hi) {
	return lo + (float)rand() / (float)RAND_MAX * (hi - lo);
}

void Scene::InitFragments(int winW, int winH, int childX, int childW)
{
	srand((unsigned)time(nullptr));

	float faceLeft = (float)childX + (float)childW * 0.20f;
	float faceRight = (float)childX + (float)childW * 0.80f;
	float faceTop = 0.0f;
	float faceBottom = (float)winH * 0.45f;
	float halfW = (float)winW * 0.5f;
	float halfH = (float)winH * 0.5f;

	for (int i = 0; i < NUM_FRAGMENTS; i++) {
		auto& f = fragments_[i];
		if (!f.tex) continue;

		float tw = 0, th = 0;
		SDL_GetTextureSize(f.tex, &tw, &th);

		float sc = RandF(0.25f, 0.30f);
		f.w = (float)winW * sc;
		f.h = f.w * (th / tw);

		f.inFront = (i < 3);

		float padX = 10.0f, padY = 15.0f;
		if (i == 0) { f.x = halfW + padX;                                  f.y = (float)winH - f.h - padY; }
		else if (i == 1) { f.x = halfW + ((float)winW - halfW) / 2.0f - (f.w / 2.0f); f.y = (float)winH - f.h - padY; }
		else if (i == 2) { f.x = (float)winW - f.w - padX;                             f.y = (float)winH - f.h - padY; }
		else if (i == 3) { f.x = halfW + padX;                                  f.y = padY + 10.0f; }
		else if (i == 4) { f.x = (float)winW - f.w - padX;                             f.y = padY + 10.0f; }

		f.floatSpeed = RandF(0.4f, 0.9f);
		f.floatAmplitude = RandF(8.0f, 22.0f);
		f.floatPhase = RandF(0.0f, 6.2831f);
		f.driftX = RandF(0.15f, 0.45f);
		f.driftPhase = RandF(0.0f, 6.2831f);
		f.rotSpeed = RandF(-6.0f, 6.0f);
		f.rotation = RandF(0.0f, 360.0f);
		f.alpha = 255;
	}
}

void Scene::DrawFragments(bool front, int winW, int winH)
{
	auto& render = *Engine::GetInstance().render;
	float t = fragmentTime_ / 1000.0f;

	for (int i = 0; i < NUM_FRAGMENTS; i++) {
		auto& f = fragments_[i];
		if (!f.tex || f.inFront != front) continue;

		float yOff = f.floatAmplitude * sinf(t * f.floatSpeed + f.floatPhase);
		float xOff = (f.floatAmplitude * 0.3f) * sinf(t * f.driftX + f.driftPhase);
		float angle = f.rotation + f.rotSpeed * t;

		SDL_SetTextureAlphaMod(f.tex, f.alpha);
		SDL_SetTextureBlendMode(f.tex, SDL_BLENDMODE_BLEND);

		int scale = Engine::GetInstance().window->GetScale();
		SDL_FRect dst = { (f.x + xOff) * (float)scale, (f.y + yOff) * (float)scale, f.w * (float)scale, f.h * (float)scale };

		SDL_RenderTextureRotated(render.renderer, f.tex, nullptr, &dst,
			(double)angle, nullptr, SDL_FLIP_NONE);

		SDL_SetTextureAlphaMod(f.tex, 255);
	}
}
