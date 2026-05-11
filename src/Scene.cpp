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
#include "UIManager.h"
#include "SaveSystem.h"
#include "DiscordManager.h"
#include "Physics.h"
#include <cstdlib>
#include <cmath>

// â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
// Button IDs  (main menu)
// â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
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
	menuClickFxId = Engine::GetInstance().audio->LoadFx("assets/audio/fx/Menu-Selection-Click.wav");
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
	settingsAnimState_ = SettingsAnimState::NONE;
	settingsAnimTimer_ = 0.0f;
	settingsButtonsAlpha_ = 1.0f;
	settingsOptionsAlpha_ = 0.0f;
	musicVolume_ = 1.0f;
	sfxVolume_ = 1.0f;

	// Sync display mode index with actual window mode
	WindowMode wm = Engine::GetInstance().window->GetWindowMode();
	if (wm == WindowMode::FULLSCREEN) windowModeIndex_ = 1;
	else if (wm == WindowMode::BORDERLESS) windowModeIndex_ = 2;
	else windowModeIndex_ = 0;

	Engine::GetInstance().discord->UpdatePresence("In Main Menu", "Alpha Phase");
    Engine::GetInstance().render->SetCameraSway(false);

	Engine::GetInstance().audio->PlayMusic("assets/audio/music/Game-Menu.wav", 1.0f);

	/*Engine::GetInstance().audio->PlayMusic("assets/audio/music/level-iv-339695.wav", 1.0f);*/

	SDL_Texture* rawLogo = Engine::GetInstance().textures->Load("assets/textures/Menu/EchoesOfSlumber.png");
	texMenuLogo_ = Engine::GetInstance().render->RecolorTexture(rawLogo, 212, 218, 234);
	Engine::GetInstance().textures->UnLoad(rawLogo);
	texMenuChild_ = Engine::GetInstance().textures->Load("assets/textures/Menu/IL_NenFront_01.png");
	texMenuButton_ = Engine::GetInstance().textures->Load("assets/textures/Menu/UI_Pause_Menu_button_white.png");
	texButtonFragmented_ = Engine::GetInstance().textures->Load("assets/textures/Menu/UI_Button_white_fragmented.png");

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
	const int startY = logoY + logoH + 20;
	const int gap = btnH + 10;

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

	if (texButtonFragmented_) {
		btnPlay_->SetHoverTexture(texButtonFragmented_);
		btnSettings_->SetHoverTexture(texButtonFragmented_);
		btnExit_->SetHoverTexture(texButtonFragmented_);
	}

	// Back button for settings (same style as main buttons, hidden by default)
	SDL_Rect backPos = { btnX, startY + gap * 3, btnW, btnH };
	btnBack_ = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, BTN_SETTINGS_BACK, "back", backPos, this);
	if (btnBack_) {
		btnBack_->alphaMod = 0.0f;
		btnBack_->isVisible = false;
		btnBack_->state = UIElementState::DISABLED;
		if (texMenuButton_) btnBack_->SetTexture(texMenuButton_);
		if (texButtonFragmented_) btnBack_->SetHoverTexture(texButtonFragmented_);
	}
}

void Scene::UnloadMainMenu()
{
	Engine::GetInstance().uiManager->CleanUp();
	if (texMenuLogo_) { SDL_DestroyTexture(texMenuLogo_);                       texMenuLogo_ = nullptr; }
	if (texMenuChild_) { Engine::GetInstance().textures->UnLoad(texMenuChild_);  texMenuChild_ = nullptr; }
	if (texMenuButton_) { Engine::GetInstance().textures->UnLoad(texMenuButton_); texMenuButton_ = nullptr; }
	if (texButtonFragmented_) { Engine::GetInstance().textures->UnLoad(texButtonFragmented_); texButtonFragmented_ = nullptr; }
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

		auto& inp = *Engine::GetInstance().input;
		if (inp.GetKey(SDL_SCANCODE_SPACE) == KEY_DOWN ||
			inp.GetKey(SDL_SCANCODE_RETURN) == KEY_DOWN ||
			inp.GetKey(SDL_SCANCODE_ESCAPE) == KEY_DOWN ||
			inp.GetGamepadButton(SDL_GAMEPAD_BUTTON_SOUTH) == KEY_DOWN ||
			inp.GetGamepadButton(SDL_GAMEPAD_BUTTON_START) == KEY_DOWN)
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
		// â”€â”€ Settings in-place animation state machine â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
		const float FADE_DURATION = 300.0f;

		if (settingsAnimState_ != SettingsAnimState::NONE &&
			settingsAnimState_ != SettingsAnimState::OPTIONS_ACTIVE)
		{
			settingsAnimTimer_ += dt;
			float t = settingsAnimTimer_ / FADE_DURATION;
			if (t > 1.0f) t = 1.0f;

			switch (settingsAnimState_) {
			case SettingsAnimState::FADE_OUT_BUTTONS:
				settingsButtonsAlpha_ = 1.0f - t;
				if (btnPlay_) btnPlay_->alphaMod = settingsButtonsAlpha_;
				if (btnSettings_) btnSettings_->alphaMod = settingsButtonsAlpha_;
				if (btnExit_) btnExit_->alphaMod = settingsButtonsAlpha_;
				if (t >= 1.0f) {
					if (btnPlay_) { btnPlay_->isVisible = false; btnPlay_->state = UIElementState::DISABLED; }
					if (btnSettings_) { btnSettings_->isVisible = false; btnSettings_->state = UIElementState::DISABLED; }
					if (btnExit_) { btnExit_->isVisible = false; btnExit_->state = UIElementState::DISABLED; }
					settingsAnimState_ = SettingsAnimState::FADE_IN_OPTIONS;
					settingsAnimTimer_ = 0.0f;
					settingsOptionsAlpha_ = 0.0f;
					showSettings_ = true;
					// Show back button, start fading in
					if (btnBack_) { btnBack_->isVisible = true; btnBack_->state = UIElementState::NORMAL; btnBack_->alphaMod = 0.0f; }
				}
				break;
			case SettingsAnimState::FADE_IN_OPTIONS:
				settingsOptionsAlpha_ = t;
				if (btnBack_) btnBack_->alphaMod = t;
				if (t >= 1.0f) {
					settingsAnimState_ = SettingsAnimState::OPTIONS_ACTIVE;
					settingsOptionsAlpha_ = 1.0f;
					if (btnBack_) btnBack_->alphaMod = 1.0f;
				}
				break;
			case SettingsAnimState::FADE_OUT_OPTIONS:
				settingsOptionsAlpha_ = 1.0f - t;
				if (btnBack_) btnBack_->alphaMod = 1.0f - t;
				if (t >= 1.0f) {
					showSettings_ = false;
					settingsOptionsAlpha_ = 0.0f;
					if (btnBack_) { btnBack_->isVisible = false; btnBack_->state = UIElementState::DISABLED; btnBack_->alphaMod = 0.0f; }
					settingsAnimState_ = SettingsAnimState::FADE_IN_BUTTONS;
					settingsAnimTimer_ = 0.0f;
					if (btnPlay_) { btnPlay_->isVisible = true; btnPlay_->state = UIElementState::NORMAL; btnPlay_->alphaMod = 0.0f; }
					if (btnSettings_) { btnSettings_->isVisible = true; btnSettings_->state = UIElementState::NORMAL; btnSettings_->alphaMod = 0.0f; }
					if (btnExit_) { btnExit_->isVisible = true; btnExit_->state = UIElementState::NORMAL; btnExit_->alphaMod = 0.0f; }
				}
				break;
			case SettingsAnimState::FADE_IN_BUTTONS:
				settingsButtonsAlpha_ = t;
				if (btnPlay_) btnPlay_->alphaMod = settingsButtonsAlpha_;
				if (btnSettings_) btnSettings_->alphaMod = settingsButtonsAlpha_;
				if (btnExit_) btnExit_->alphaMod = settingsButtonsAlpha_;
				if (t >= 1.0f) {
					settingsAnimState_ = SettingsAnimState::NONE;
					settingsButtonsAlpha_ = 1.0f;
					Engine::GetInstance().discord->UpdatePresence("In Main Menu", "Alpha Phase");
				}
				break;
			default: break;
			}
		}

		// ESC / B to go back from options
		auto& inp2 = *Engine::GetInstance().input;
		if (settingsAnimState_ == SettingsAnimState::OPTIONS_ACTIVE &&
			(inp2.GetKey(SDL_SCANCODE_ESCAPE) == KEY_DOWN ||
			 inp2.GetGamepadButton(SDL_GAMEPAD_BUTTON_EAST) == KEY_DOWN))
		{
			settingsAnimState_ = SettingsAnimState::FADE_OUT_OPTIONS;
			settingsAnimTimer_ = 0.0f;
			Engine::GetInstance().audio->PlayFx(menuClickFxId);
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
	float renderLogoX = (float)logoDestX;
	float renderLogoY = (float)logoDestY;
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
			renderLogoX = (float)(winW / 2 - logoW / 2);
			renderLogoY = (float)(winH / 2 - logoH / 2);
			renderChildX = winW;
			if (btnPlay_) btnPlay_->isVisible = false;
			if (btnSettings_) btnSettings_->isVisible = false;
			if (btnExit_) btnExit_->isVisible = false;
			break;

		case MenuAnimState::LOGO_HOLD:
			renderLogoX = (float)(winW / 2 - logoW / 2);
			renderLogoY = (float)(winH / 2 - logoH / 2);
			renderChildX = winW;
			if (btnPlay_) btnPlay_->isVisible = false;
			if (btnSettings_) btnSettings_->isVisible = false;
			if (btnExit_) btnExit_->isVisible = false;
			break;

		case MenuAnimState::SLIDE_LOGO:
			t = menuAnimTimer_ / 1500.0f;
			if (t > 1.0f) t = 1.0f;
			t = 1.0f - (float)pow(1.0f - t, 3.0f);
			renderLogoX = ((float)winW / 2.0f - (float)logoW / 2.0f) + ((float)logoDestX - ((float)winW / 2.0f - (float)logoW / 2.0f)) * t;
			renderLogoY = ((float)winH / 2.0f - (float)logoH / 2.0f) + ((float)logoDestY - ((float)winH / 2.0f - (float)logoH / 2.0f)) * t;
			renderChildX = winW;
			if (btnPlay_) btnPlay_->isVisible = false;
			if (btnSettings_) btnSettings_->isVisible = false;
			if (btnExit_) btnExit_->isVisible = false;
			break;

		case MenuAnimState::SLIDE_CHILD:
			t = menuAnimTimer_ / 1500.0f;
			if (t > 1.0f) t = 1.0f;
			t = 1.0f - (float)pow(1.0f - t, 3.0f);
			renderChildX = (int)(winW + (childDestX - winW) * t);
			renderLogoX = (float)logoDestX;
			renderLogoY = (float)logoDestY;
			if (btnPlay_) btnPlay_->isVisible = false;
			if (btnSettings_) btnSettings_->isVisible = false;
			if (btnExit_) btnExit_->isVisible = false;
			break;

		case MenuAnimState::FADE_FRAGS_BTNS:
			renderChildX = childDestX;
			renderLogoX = (float)logoDestX;
			renderLogoY = (float)logoDestY;
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
		render.DrawTextureAlphaF(texMenuLogo_, renderLogoX, renderLogoY, (float)logoW, (float)logoH, 255);
	}

	if (showSettings_ || settingsOptionsAlpha_ > 0.0f)
		DrawSettingsInPlace(winW, winH);
}

void Scene::DrawSettingsInPlace(int winW, int winH)
{
	auto& render = *Engine::GetInstance().render;
	Uint8 alpha = (Uint8)(255.0f * settingsOptionsAlpha_);
	if (alpha == 0) return;

	// Reuse the exact same positions as the main menu buttons
	const int leftHalf = winW / 2;
	const int btnW = 315;
	const int btnH = static_cast<int>(static_cast<float>(btnW) * (130.0f / 456.0f));
	const int logoW = 385;
	const int logoH = static_cast<int>(static_cast<float>(logoW) * (569.0f / 1559.0f));
	const int logoY = winH / 4 - logoH / 2;
	const int startY = logoY + logoH + 20;
	const int gap = btnH + 10;
	const int panelX = (leftHalf - btnW) / 2;

	int trackX = panelX + 20;
	int trackW = btnW - 40;

	SDL_Color labelColor = { 180, 200, 220, alpha };
	SDL_Color valColor   = { 255, 255, 255, alpha };

	// Row 0: Music slider (same Y as Play button)
	int row0Y = startY + 10;
	render.DrawMenuTextCentered("MUSIC", { panelX, row0Y, btnW / 2 - 10, 20 }, labelColor);
	char vol[8];
	snprintf(vol, sizeof(vol), "%d%%", static_cast<int>(musicVolume_ * 100.0f));
	render.DrawMenuTextCentered(vol, { panelX + btnW / 2, row0Y, btnW / 2, 20 }, valColor);

	SDL_Rect mBarBg = { trackX, row0Y + 32, trackW, 8 };
	render.DrawRectangle(mBarBg, 10, 15, 25, alpha, true, false);
	int mFill = static_cast<int>(static_cast<float>(trackW) * musicVolume_);
	SDL_Rect mBarFill = { trackX, row0Y + 32, mFill, 8 };
	render.DrawRectangle(mBarFill, 100, 180, 255, alpha, true, false);
	SDL_Rect mKnob = { trackX + mFill - 5, row0Y + 28, 10, 16 };
	render.DrawRectangle(mKnob, 200, 220, 255, alpha, true, false);

	// Row 1: SFX slider (same Y as Options button)
	int row1Y = startY + gap + 10;
	render.DrawMenuTextCentered("SFX", { panelX, row1Y, btnW / 2 - 10, 20 }, labelColor);
	snprintf(vol, sizeof(vol), "%d%%", static_cast<int>(sfxVolume_ * 100.0f));
	render.DrawMenuTextCentered(vol, { panelX + btnW / 2, row1Y, btnW / 2, 20 }, valColor);

	SDL_Rect sBarBg = { trackX, row1Y + 32, trackW, 8 };
	render.DrawRectangle(sBarBg, 10, 15, 25, alpha, true, false);
	int sFill = static_cast<int>(static_cast<float>(trackW) * sfxVolume_);
	SDL_Rect sBarFill = { trackX, row1Y + 32, sFill, 8 };
	render.DrawRectangle(sBarFill, 100, 180, 255, alpha, true, false);
	SDL_Rect sKnob = { trackX + sFill - 5, row1Y + 28, 10, 16 };
	render.DrawRectangle(sKnob, 200, 220, 255, alpha, true, false);

	// Row 2: Display mode selector (same Y as Exit button)
	int row2Y = startY + gap * 2 + 15;
	render.DrawMenuTextCentered("DISPLAY", { panelX, row2Y, btnW, 20 }, labelColor);

	const char* modeNames[] = { "WINDOWED", "FULLSCREEN", "BORDERLESS" };
	int arrowW = 30;
	SDL_Rect leftArrowArea  = { panelX + 20, row2Y + 28, arrowW, 30 };
	SDL_Rect modeArea       = { panelX + 20 + arrowW, row2Y + 28, btnW - 40 - arrowW * 2, 30 };
	SDL_Rect rightArrowArea = { panelX + btnW - 20 - arrowW, row2Y + 28, arrowW, 30 };

	SDL_Color arrowColor = { 100, 180, 255, alpha };
	render.DrawMenuTextCentered("<", leftArrowArea, arrowColor);
	render.DrawMenuTextCentered(modeNames[windowModeIndex_], modeArea, valColor);
	render.DrawMenuTextCentered(">", rightArrowArea, arrowColor);

	// Back button (row 3) is rendered by the UIElement system (btnBack_)

	// Handle mouse input for sliders and display mode
	if (settingsAnimState_ == SettingsAnimState::OPTIONS_ACTIVE)
	{
		auto& input = *Engine::GetInstance().input;

		if (input.GetMouseButtonDown(SDL_BUTTON_LEFT) == KEY_DOWN ||
			input.GetMouseButtonDown(SDL_BUTTON_LEFT) == KEY_REPEAT)
		{
			Vector2D mousePos = input.GetMousePosition();
			int mx = (int)mousePos.getX();
			int my = (int)mousePos.getY();

			SDL_Rect mHit = { trackX - 10, row0Y + 18, trackW + 20, 32 };
			if (mx >= mHit.x && mx <= mHit.x + mHit.w && my >= mHit.y && my <= mHit.y + mHit.h) {
				musicVolume_ = (float)(mx - trackX) / (float)trackW;
				if (musicVolume_ < 0.0f) musicVolume_ = 0.0f;
				if (musicVolume_ > 1.0f) musicVolume_ = 1.0f;
				Engine::GetInstance().audio->SetMusicVolume(musicVolume_);
			}

			SDL_Rect sHit = { trackX - 10, row1Y + 18, trackW + 20, 32 };
			if (mx >= sHit.x && mx <= sHit.x + sHit.w && my >= sHit.y && my <= sHit.y + sHit.h) {
				sfxVolume_ = (float)(mx - trackX) / (float)trackW;
				if (sfxVolume_ < 0.0f) sfxVolume_ = 0.0f;
				if (sfxVolume_ > 1.0f) sfxVolume_ = 1.0f;
				Engine::GetInstance().audio->SetSFXVolume(sfxVolume_);
			}
		}

		if (input.GetMouseButtonDown(SDL_BUTTON_LEFT) == KEY_DOWN) {
			Vector2D mousePos = input.GetMousePosition();
			int mx = (int)mousePos.getX();
			int my = (int)mousePos.getY();

			if (mx >= leftArrowArea.x && mx <= leftArrowArea.x + leftArrowArea.w &&
				my >= leftArrowArea.y && my <= leftArrowArea.y + leftArrowArea.h) {
				windowModeIndex_ = (windowModeIndex_ + 2) % 3;
				WindowMode modes[] = { WindowMode::WINDOWED, WindowMode::FULLSCREEN, WindowMode::BORDERLESS };
				Engine::GetInstance().window->SetWindowMode(modes[windowModeIndex_]);
				Engine::GetInstance().audio->PlayFx(menuClickFxId);
			}
			if (mx >= rightArrowArea.x && mx <= rightArrowArea.x + rightArrowArea.w &&
				my >= rightArrowArea.y && my <= rightArrowArea.y + rightArrowArea.h) {
				windowModeIndex_ = (windowModeIndex_ + 1) % 3;
				WindowMode modes[] = { WindowMode::WINDOWED, WindowMode::FULLSCREEN, WindowMode::BORDERLESS };
				Engine::GetInstance().window->SetWindowMode(modes[windowModeIndex_]);
				Engine::GetInstance().audio->PlayFx(menuClickFxId);
			}
		}

		// ── Gamepad navigation ──────────────────────────────────────
		auto& gpInput = *Engine::GetInstance().input;
		float dt = Engine::GetInstance().GetDt();

		if (gpInput.GetGamepadButton(SDL_GAMEPAD_BUTTON_DPAD_UP) == KEY_DOWN ||
			(gpInput.GetLeftStickY() < -0.5f && sliderRepeatTimer_ <= 0.0f)) {
			optionsSliderSel_ = std::max(0, optionsSliderSel_ - 1);
			sliderRepeatTimer_ = 250.0f;
		}
		if (gpInput.GetGamepadButton(SDL_GAMEPAD_BUTTON_DPAD_DOWN) == KEY_DOWN ||
			(gpInput.GetLeftStickY() > 0.5f && sliderRepeatTimer_ <= 0.0f)) {
			optionsSliderSel_ = std::min(2, optionsSliderSel_ + 1);
			sliderRepeatTimer_ = 250.0f;
		}

		float volStep = 0.05f;
		bool stepLeft = gpInput.GetGamepadButton(SDL_GAMEPAD_BUTTON_DPAD_LEFT) == KEY_DOWN;
		bool stepRight = gpInput.GetGamepadButton(SDL_GAMEPAD_BUTTON_DPAD_RIGHT) == KEY_DOWN;

		if (optionsSliderSel_ == 0) {
			if (stepRight)  musicVolume_ = std::min(1.0f, musicVolume_ + volStep);
			if (stepLeft)   musicVolume_ = std::max(0.0f, musicVolume_ - volStep);
			if (stepLeft || stepRight) Engine::GetInstance().audio->SetMusicVolume(musicVolume_);
		}
		else if (optionsSliderSel_ == 1) {
			if (stepRight)  sfxVolume_ = std::min(1.0f, sfxVolume_ + volStep);
			if (stepLeft)   sfxVolume_ = std::max(0.0f, sfxVolume_ - volStep);
			if (stepLeft || stepRight) Engine::GetInstance().audio->SetSFXVolume(sfxVolume_);
		}
		else if (optionsSliderSel_ == 2) {
			if (stepLeft || stepRight) {
				windowModeIndex_ = (windowModeIndex_ + (stepRight ? 1 : 2)) % 3;
				WindowMode modes[] = { WindowMode::WINDOWED, WindowMode::FULLSCREEN, WindowMode::BORDERLESS };
				Engine::GetInstance().window->SetWindowMode(modes[windowModeIndex_]);
				Engine::GetInstance().audio->PlayFx(menuClickFxId);
			}
		}

		if (sliderRepeatTimer_ > 0.0f) sliderRepeatTimer_ -= dt;

		if (gpInput.IsGamepadConnected()) {
			int selY = row0Y;
			if (optionsSliderSel_ == 1) selY = row1Y;
			else if (optionsSliderSel_ == 2) selY = row2Y;
			SDL_Rect selHighlight = { panelX, selY - 4, btnW, 50 };
			render.DrawRectangle(selHighlight, 60, 100, 180, 50, true, false);
		}
	}
}

void Scene::HandleMainMenuUIEvents(UIElement* uiElement)
{
	if (waitingForFade_)      return;
	if (settingsCooldown_ > 0) return;

	Engine::GetInstance().audio->PlayFx(menuClickFxId);

	switch (uiElement->id)
	{
	case BTN_PLAY:
		LOG("Main Menu: Play");
		waitingForFade_ = true;
		fadeTargetScene_ = SceneID::GAMEPLAY;
		Engine::GetInstance().render->StartFade(FadeDirection::FADE_OUT, 800.0f);
		break;
	case BTN_SETTINGS:
		if (settingsAnimState_ == SettingsAnimState::NONE) {
			settingsAnimState_ = SettingsAnimState::FADE_OUT_BUTTONS;
			settingsAnimTimer_ = 0.0f;
			optionsSliderSel_ = 0;
			Engine::GetInstance().discord->UpdatePresence("In Options", "Alpha Phase");
		}
		break;
	case BTN_EXIT:
		LOG("Main Menu: Exit");
		SDL_Event quitEvent;
		quitEvent.type = SDL_EVENT_QUIT;
		SDL_PushEvent(&quitEvent);
		break;
	case BTN_SETTINGS_BACK:
		if (settingsAnimState_ == SettingsAnimState::OPTIONS_ACTIVE) {
			settingsAnimState_ = SettingsAnimState::FADE_OUT_OPTIONS;
			settingsAnimTimer_ = 0.0f;
		}
		break;
	default:
		break;
	}
}

void Scene::SetSettingsPanelVisible(bool visible)
{
	// No-op for main menu â€” settings are now drawn in-place without separate UI elements.
	// Kept for compatibility with pause menu code paths.
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
	Engine::GetInstance().discord->UpdatePresence("", "");

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
	auto& introInput = *Engine::GetInstance().input;
	if (introInput.GetKey(SDL_SCANCODE_SPACE) == KEY_DOWN ||
		introInput.GetGamepadButton(SDL_GAMEPAD_BUTTON_SOUTH) == KEY_DOWN ||
		introInput.GetGamepadButton(SDL_GAMEPAD_BUTTON_START) == KEY_DOWN) {
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
	Engine::GetInstance().audio->PlayMusic(nullptr);
	Engine::GetInstance().cinematics->PlayVideo("assets/video/test.mp4");
}

void Scene::UnloadIntroCinematic()
{
	Engine::GetInstance().cinematics->StopVideo();
}

void Scene::UpdateIntroCinematic(float dt)
{
	if (waitingForFade_) return;

	auto& cinInput = *Engine::GetInstance().input;
	bool skipRequested = cinInput.GetKey(SDL_SCANCODE_SPACE) == KEY_DOWN ||
		cinInput.GetKey(SDL_SCANCODE_ESCAPE) == KEY_DOWN ||
		cinInput.GetGamepadButton(SDL_GAMEPAD_BUTTON_SOUTH) == KEY_DOWN ||
		cinInput.GetGamepadButton(SDL_GAMEPAD_BUTTON_START) == KEY_DOWN;

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

	Engine::GetInstance().discord->UpdatePresence("Playing: Level 1", "Alpha Phase");
    Engine::GetInstance().render->SetCameraSway(true);

	Engine::GetInstance().audio->PlayMusic("assets/audio/music/backgroundmusic.wav", 1.0f);

	Engine::GetInstance().map->Load("assets/maps/", "MapTemplate.tmx");
	Engine::GetInstance().map->LoadEntities(player);

	if (player == nullptr) {
		player = std::dynamic_pointer_cast<Player>(
			Engine::GetInstance().entityManager->CreateEntity(EntityType::PLAYER));
		player->position = Vector2D(96.0f, 672.0f);
		player->Start();
	}

	// FIX: Set camera position immediately to avoid the "initial slide" bug
	Engine::GetInstance().render->SetCameraPosition(player->position.getX(), player->position.getY());

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
		}
		else {
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
	texCheckpointSaved_ = Engine::GetInstance().render->CreateMenuTextTexture("GAME SAVED", { 255, 255, 255, 255 });
	checkpointSaveTimer_ = 0.0f;

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

	// Blanket ability HUD icons
	texBlanketActive_ = Engine::GetInstance().textures->Load("assets/textures/UI/UI_Blanket_Ability.png");
	texBlanketInactive_ = Engine::GetInstance().textures->Load("assets/textures/UI/UI_Blanket_Ability_low_opacity.png");

	// Cape collectible (AS_capa.png)
	texCapaCollectible_ = Engine::GetInstance().textures->Load("assets/textures/AS_props/AS_capa.png");
	if (texCapaCollectible_) {
		SDL_SetTextureColorMod(texCapaCollectible_, 100, 100, 120);
	}
	capaCollected_ = false;
	capaFloatTimer_ = 0.0f;

	// Read cape position from TMX Entities layer instead of hardcoding
	if (!Engine::GetInstance().map->GetCapePosition(capaX_, capaY_)) {
		LOG("WARNING: No Cape entity found in TMX Entities layer, cape will not spawn");
		capaCollected_ = true;
	}

	capaBody_ = nullptr;
}

void Scene::UpdateGameplay(float dt)
{
	// Toggle pause with ESC
	auto& gpInput = *Engine::GetInstance().input;
	bool pauseToggle = gpInput.GetKey(SDL_SCANCODE_ESCAPE) == KEY_DOWN ||
		gpInput.GetGamepadButton(SDL_GAMEPAD_BUTTON_START) == KEY_DOWN;
	bool backBtn = gpInput.GetGamepadButton(SDL_GAMEPAD_BUTTON_EAST) == KEY_DOWN;

	if (!isGameOver_ && (pauseToggle || (backBtn && (showMapViewer_ || showPauseOptions_ || isPaused_))))
	{
		if (showMapViewer_) {
			showMapViewer_ = false;
			// If no pause menu buttons are visible, map was opened via touchpad â€” unpause
			isPaused_ = false;
			SetPauseMenuVisible(false);
		}
		else if (showPauseOptions_) {
			showPauseOptions_ = false;
			SetPauseOptionsPanelVisible(false);
		}
		else {
			isPaused_ = !isPaused_;
			SetPauseMenuVisible(isPaused_);
			Engine::GetInstance().uiManager->ResetFocus();
		}
	}

	// â”€â”€ Touchpad opens/closes map directly during gameplay â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
	if (!isGameOver_ && !isPaused_ && !showMapViewer_ &&
		gpInput.GetTouchpadPressed() == KEY_DOWN)
	{
		showMapViewer_ = true;
		isPaused_ = true;
		SetPauseMenuVisible(false);

		int winW = 0, winH = 0;
		Engine::GetInstance().window->GetWindowSize(winW, winH);
		const int viewW = winW - 20;
		const int viewH = winH - 42 - 30;
		mapViewZoom_ = 1.0f;
		Vector2D playerPos = player ? player->GetPosition() : Vector2D(0, 0);
		mapViewOffsetX_ = (float)viewW / 2.0f - (playerPos.getX() * mapViewZoom_);
		mapViewOffsetY_ = (float)viewH / 2.0f - (playerPos.getY() * mapViewZoom_);
	}

	if (showMapViewer_)
	{
		auto& input = *Engine::GetInstance().input;

		// Close map with touchpad
		if (input.GetTouchpadPressed() == KEY_DOWN) {
			showMapViewer_ = false;
			isPaused_ = false;
			SetPauseMenuVisible(false);
			return;
		}

		// Clamp min zoom to not allow zooming out past the map dimensions
		int winW = 0, winH = 0;
		Engine::GetInstance().window->GetWindowSize(winW, winH);
		const int viewW = winW - 20;
		const int viewH = winH - 42 - 30;

		Vector2D mapSize = Engine::GetInstance().map->GetMapSizeInPixels();
		float minZoomX = (float)viewW / mapSize.getX();
		float minZoomY = (float)viewH / mapSize.getY();
		float minZoom = std::max(std::max(minZoomX, minZoomY), 0.05f); // Don't allow zooming out smaller than the map requires to fit the screen

		if (input.GetKey(SDL_SCANCODE_KP_PLUS) == KEY_DOWN || input.GetKey(SDL_SCANCODE_EQUALS) == KEY_DOWN)
			mapViewZoom_ = std::min(mapViewZoom_ + 0.05f, 2.0f);
		if (input.GetKey(SDL_SCANCODE_KP_MINUS) == KEY_DOWN || input.GetKey(SDL_SCANCODE_MINUS) == KEY_DOWN)
			mapViewZoom_ = std::max(mapViewZoom_ - 0.05f, minZoom);

		// â”€â”€ Gamepad: R2 zoom in, L2 zoom out â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
		float rt = input.GetRightTrigger();
		float lt = input.GetLeftTrigger();
		if (rt > 0.0f)
			mapViewZoom_ = std::min(mapViewZoom_ + rt * 0.002f * dt, 2.0f);
		if (lt > 0.0f)
			mapViewZoom_ = std::max(mapViewZoom_ - lt * 0.002f * dt, minZoom);

		// Ensure zoom stays within limits
		if (mapViewZoom_ < minZoom) mapViewZoom_ = minZoom;

		// â”€â”€ Gamepad: Left stick to pan the map â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
		float lsx = input.GetLeftStickX();
		float lsy = input.GetLeftStickY();
		if (lsx != 0.0f || lsy != 0.0f) {
			float panSpeed = 0.4f * dt;
			mapViewOffsetX_ -= lsx * panSpeed;
			mapViewOffsetY_ -= lsy * panSpeed;
		}

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


		Engine::GetInstance().window->GetWindowSize(winW, winH);
		DrawMapViewer(winW, winH);
		return;
	}

	if (isPaused_) {
		DrawPauseMenu();
		return;
	}

	if (player) {
		// Wake-up notification trigger
		if (!player->isWakingUp && wakeUpNotifTimer_ == 0.0f) {
			wakeUpNotifTimer_ = WAKEUP_NOTIF_DURATION;
		}

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
			}
			else if (currentHealthUI_ == 2) {
				activeHealthAnim_ = 2;
				animHealth2_.Reset();
			}
			else if (currentHealthUI_ == 1) {
				activeHealthAnim_ = 3;
				animHealth3_.Reset();
			}
			currentHealthUI_ = player->health;
		}

		if (activeHealthAnim_ == 1) {
			animHealth1_.Update(dt);
			if (animHealth1_.HasFinishedOnce()) activeHealthAnim_ = 0;
		}
		else if (activeHealthAnim_ == 2) {
			animHealth2_.Update(dt);
			if (animHealth2_.HasFinishedOnce()) activeHealthAnim_ = 0;
		}
		else if (activeHealthAnim_ == 3) {
			animHealth3_.Update(dt);
			if (animHealth3_.HasFinishedOnce()) activeHealthAnim_ = 0;
		}

		if (player->health <= 0 && activeHealthAnim_ == 0 && !isGameOver_) {
			isGameOver_ = true;

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

		// Cape collectible pickup (proximity check)
		if (!capaCollected_ && player)
		{
			capaFloatTimer_ += dt;

			float dx = player->position.getX() - capaX_;
			float dy = player->position.getY() - (capaY_ - 50.0f);
			float distSq = dx * dx + dy * dy;
			float pickupRadius = 50.0f;

			if (distSq < pickupRadius * pickupRadius)
			{
				capaCollected_ = true;
				player->SetHasBlanket(true);
				Engine::GetInstance().audio->PlayFx(player->pickCoinFxId);
				capaNotifTimer_ = CAPA_NOTIF_DURATION;
				LOG("Cape collected - blanket ability unlocked!");
			}
		}
	}

	// Draw cape collectible in-world
	if (!capaCollected_ && texCapaCollectible_)
	{
		int capaTexW = 0, capaTexH = 0;
		Engine::GetInstance().textures->GetSize(texCapaCollectible_, capaTexW, capaTexH);
		float floatOffset = 6.0f * sinf(capaFloatTimer_ * 0.003f);
		int drawX = (int)(capaX_ - (float)capaTexW * 0.5f / 2.0f);
		int drawY = (int)(capaY_ - (float)capaTexH * 0.5f / 2.0f + floatOffset);

		SDL_Rect section = { 0, 0, capaTexW, capaTexH };
		Engine::GetInstance().render->DrawTexture(texCapaCollectible_, drawX, drawY, &section, 1.0f, 0, INT_MAX, INT_MAX, SDL_FLIP_NONE, 0.5f);
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
	if (texButtonFragmented_) { Engine::GetInstance().textures->UnLoad(texButtonFragmented_); texButtonFragmented_ = nullptr; }

	if (texBlanketActive_) { Engine::GetInstance().textures->UnLoad(texBlanketActive_);   texBlanketActive_ = nullptr; }

	if (texBlanketInactive_) { Engine::GetInstance().textures->UnLoad(texBlanketInactive_); texBlanketInactive_ = nullptr; }
	if (texCapaCollectible_) { Engine::GetInstance().textures->UnLoad(texCapaCollectible_); texCapaCollectible_ = nullptr; }
	if (capaBody_) { Engine::GetInstance().physics->DeletePhysBody(capaBody_); capaBody_ = nullptr; }
	capaCollected_ = false;
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
	if (!isPaused_ && !showMapViewer_ && player && !player->isWakingUp) {
		SDL_Rect r;
		const SDL_Rect* frame = nullptr;
		SDL_Texture* texToDraw = nullptr;

		if (activeHealthAnim_ == 1) {
			texToDraw = texHealth1_;
			frame = &animHealth1_.GetCurrentFrame();
		}
		else if (activeHealthAnim_ == 2) {
			texToDraw = texHealth2_;
			frame = &animHealth2_.GetCurrentFrame();
		}
		else if (activeHealthAnim_ == 3) {
			texToDraw = texHealth3_;
			frame = &animHealth3_.GetCurrentFrame();
		}
		else {

			if (currentHealthUI_ == 3) {
				texToDraw = texHealth1_;
				if (texHealth1_) {
					r = animHealth1_.GetCurrentFrame();
					frame = &r;
				}
			}
			else if (currentHealthUI_ == 2) {
				texToDraw = texHealth2_;
				if (texHealth2_) {
					r = animHealth2_.GetCurrentFrame();
					frame = &r;
				}
			}
			else if (currentHealthUI_ == 1) {
				texToDraw = texHealth3_;
				if (texHealth3_) {
					r = animHealth3_.GetCurrentFrame();
					frame = &r;
				}
			}
			else if (currentHealthUI_ <= 0) {
				texToDraw = texHealth3_;
				if (texHealth3_) {
					r = animHealth3_.GetCurrentFrame();
					frame = &r;
				}
			}
		}

		if (texToDraw && frame) {
			Engine::GetInstance().render->DrawTexture(texToDraw, 40, 40, frame, 0.0f, 0, INT_MAX, INT_MAX, SDL_FLIP_NONE, 0.5f);
		}


		if (player->HasBlanket())
		{
			// --- Blanket Ability HUD Icon ---
			SDL_Texture* blanketTex = player->IsHiding() ? texBlanketActive_ : texBlanketInactive_;
			if (blanketTex)
			{
				Engine::GetInstance().render->DrawTextureAlpha(blanketTex, 220, 72, 64, 64, 255);
			}
		}
	}

	// --- Game Over Screen ---
	if (isGameOver_) {
		int winW = 0, winH = 0;
		Engine::GetInstance().window->GetWindowSize(winW, winH);

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


			SDL_SetTextureScaleMode(texGameOver_, SDL_SCALEMODE_LINEAR);


			float shadowOffset = 6.0f;
			SDL_SetTextureColorMod(texGameOver_, 255, 100, 100);
			Engine::GetInstance().render->DrawTextureAlphaF(texGameOver_, sx + shadowOffset, sy + shadowOffset, sw, sh, (Uint8)130);

			SDL_SetTextureColorMod(texGameOver_, 255, 255, 255);
			Engine::GetInstance().render->DrawTextureAlphaF(texGameOver_, sx, sy, sw, sh, (Uint8)255);
		}


		if (texGameOverKid_) {
			float kw, kh;
			SDL_GetTextureSize(texGameOverKid_, &kw, &kh);
			float kScale = 0.22f;
			Engine::GetInstance().render->DrawTextureAlphaF(texGameOverKid_, (float)winW - kw * kScale - 20.0f, (float)winH - kh * kScale - 20.0f, kw * kScale, kh * kScale, (Uint8)255);
		}


		if (texGameOverFrag1_) Engine::GetInstance().render->DrawTextureAlphaF(texGameOverFrag1_, 100.0f, 150.0f, 160.0f, 160.0f, 180);
		if (texGameOverFrag3_) Engine::GetInstance().render->DrawTextureAlphaF(texGameOverFrag3_, (float)winW - 350.0f, 80.0f, 200.0f, 200.0f, 150);
		if (texGameOverFrag5_) Engine::GetInstance().render->DrawTextureAlphaF(texGameOverFrag5_, 200.0f, (float)winH - 300.0f, 140.0f, 140.0f, 160);
		if (texGameOverFrag1_) Engine::GetInstance().render->DrawTextureAlphaF(texGameOverFrag1_, (float)winW / 2.0f + 250.0f, (float)winH / 2.0f + 120.0f, 120.0f, 120.0f, 130);


		if (texGameOverFrag4_) Engine::GetInstance().render->DrawTextureAlphaF(texGameOverFrag4_, 50.0f, (float)winH - 220.0f, 180.0f, 180.0f, 200);
	}

	// --- Checkpoint Save Notification ---
	if (!isPaused_ && checkpointSaveTimer_ > 0.0f) {
		checkpointSaveTimer_ -= Engine::GetInstance().GetDt();
		if (texCheckpointSaved_) {
			float tw, th;
			SDL_GetTextureSize(texCheckpointSaved_, &tw, &th);
			int winW = 0, winH = 0;
			Engine::GetInstance().window->GetWindowSize(winW, winH);

			float drawX = (float)winW - tw - 40.0f;
			float drawY = (float)winH - th - 40.0f;

			Uint8 alpha = 255;
			if (checkpointSaveTimer_ < 1000.0f) {
				alpha = (Uint8)(255.0f * (checkpointSaveTimer_ / 1000.0f));
			}

			Engine::GetInstance().render->DrawTextureAlphaF(texCheckpointSaved_, drawX, drawY, tw, th, alpha);
		}
	}

	// --- Wake-up notification ---
	if (!isPaused_ && wakeUpNotifTimer_ > 0.0f) {
		wakeUpNotifTimer_ -= Engine::GetInstance().GetDt();

		int winW = 0, winH = 0;
		Engine::GetInstance().window->GetWindowSize(winW, winH);
		auto& render = *Engine::GetInstance().render;

		Uint8 alpha = 255;
		if (wakeUpNotifTimer_ < 800.0f)
			alpha = (Uint8)(255.0f * (wakeUpNotifTimer_ / 800.0f));

		// Panel dimensions
		const int panelW = 270;
		const int panelH = 36;
		const int panelX = (winW - panelW) / 2;
		const int panelY = 40;

		SDL_Rect panel = { panelX, panelY, panelW, panelH };
		render.DrawRectangle(panel, 0, 0, 0, alpha, true, false);
		SDL_Rect border = { panelX - 2, panelY - 2, panelW + 4, panelH + 4 };
		render.DrawRectangle(border, 255, 255, 255, alpha, false, false);

		SDL_Color textColor = { 255, 255, 255, alpha };
		render.DrawMenuTextCentered("What is this... Where am I...", { panelX, panelY, panelW, panelH }, textColor, 0.35f);
	}

	// --- Cape pickup notification ---
	if (!isPaused_ && capaNotifTimer_ > 0.0f) {
		capaNotifTimer_ -= Engine::GetInstance().GetDt();

		int winW2 = 0, winH2 = 0;
		Engine::GetInstance().window->GetWindowSize(winW2, winH2);
		auto& render2 = *Engine::GetInstance().render;

		Uint8 alpha2 = 255;
		if (capaNotifTimer_ < 800.0f)
			alpha2 = (Uint8)(255.0f * (capaNotifTimer_ / 800.0f));

		const int cpW = 280;
		const int cpH = 36;
		const int cpX = (winW2 - cpW) / 2;
		const int cpY = 40;

		SDL_Rect cpPanel = { cpX, cpY, cpW, cpH };
		render2.DrawRectangle(cpPanel, 0, 0, 0, alpha2, true, false);
		SDL_Rect cpBorder = { cpX - 2, cpY - 2, cpW + 4, cpH + 4 };
		render2.DrawRectangle(cpBorder, 255, 255, 255, alpha2, false, false);

		SDL_Color cpColor = { 255, 255, 255, alpha2 };
		render2.DrawMenuTextCentered("Cape collected! Press H to hide", { cpX, cpY, cpW, cpH }, cpColor, 0.35f);
	}

	// --- No cape notification ---
	if (!isPaused_ && noCapeNotifTimer_ > 0.0f) {
		noCapeNotifTimer_ -= Engine::GetInstance().GetDt();

		int winW3 = 0, winH3 = 0;
		Engine::GetInstance().window->GetWindowSize(winW3, winH3);
		auto& render3 = *Engine::GetInstance().render;

		Uint8 alpha3 = 255;
		if (noCapeNotifTimer_ < 800.0f)
			alpha3 = (Uint8)(255.0f * (noCapeNotifTimer_ / 800.0f));

		const int ncpW = 340;
		const int ncpH = 36;
		const int ncpX = (winW3 - ncpW) / 2;
		const int ncpY = 40;

		SDL_Rect ncpPanel = { ncpX, ncpY, ncpW, ncpH };
		render3.DrawRectangle(ncpPanel, 0, 0, 0, alpha3, true, false);
		SDL_Rect ncpBorder = { ncpX - 2, ncpY - 2, ncpW + 4, ncpH + 4 };
		render3.DrawRectangle(ncpBorder, 255, 255, 255, alpha3, false, false);

		SDL_Color ncpColor = { 255, 255, 255, alpha3 };
		render3.DrawMenuTextCentered("You cant do this you need an object", { ncpX, ncpY, ncpW, ncpH }, ncpColor, 0.35f);
	}

	// â”€â”€ Final rendering passes (CRITICAL: Must be on top of EVERYTHING) â”€â”€â”€â”€
	if (showMapViewer_) {
		int winW = 0, winH = 0;
		Engine::GetInstance().window->GetWindowSize(winW, winH);
		DrawMapViewer(winW, winH);
	}
	else if (isPaused_) {
		DrawPauseMenu();
	}
}// â”€â”€ Map viewer â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

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
	if (Engine::GetInstance().input->IsGamepadConnected())
		render.DrawText("L-Stick: pan  |  R2: zoom in  |  L2: zoom out  |  Touchpad: back",
			winW / 2 - 270, winH - 22, 0, 0, { 120, 160, 200, 200 });
	else
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

		Vector2D mapSize = map.GetMapSizeInPixels();
		float minZoomX = (float)viewW / mapSize.getX();
		float minZoomY = (float)viewH / mapSize.getY();
		float minZoom = std::max(std::max(minZoomX, minZoomY), 0.05f);

		for (int i = 0; i < count; i++) {
			float wheelY = events[i].wheel.y;
			float newZoom = mapViewZoom_ + wheelY * 0.05f;
			if (newZoom < minZoom) newZoom = minZoom;
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

	// â”€â”€ Clamp map offset to map boundaries â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
	Vector2D mapSize = map.GetMapSizeInPixels();
	float maxOffsetX = 0.0f;
	float maxOffsetY = 0.0f;
	float minOffsetX = (float)viewW - mapSize.getX() * mapViewZoom_;
	float minOffsetY = (float)viewH - mapSize.getY() * mapViewZoom_;

	if (mapViewOffsetX_ > maxOffsetX) mapViewOffsetX_ = maxOffsetX;
	if (mapViewOffsetX_ < minOffsetX) mapViewOffsetX_ = minOffsetX;
	if (mapViewOffsetY_ > maxOffsetY) mapViewOffsetY_ = maxOffsetY;
	if (mapViewOffsetY_ < minOffsetY) mapViewOffsetY_ = minOffsetY;

	float invZoom = 1.0f / mapViewZoom_;
	float camLeft = -mapViewOffsetX_ * invZoom;
	float camTop = -mapViewOffsetY_ * invZoom;
	float camRight = camLeft + (float)viewW * invZoom;
	float camBottom = camTop + (float)viewH * invZoom;

	int startTileX = std::max(0, (int)(camLeft / (float)tileW) - 1);
	int startTileY = std::max(0, (int)(camTop / (float)tileH) - 1);
	int endTileX = std::min((int)map.GetMapSizeInTiles().getX(), (int)(camRight / (float)tileW) + 2);
	int endTileY = std::min((int)map.GetMapSizeInTiles().getY(), (int)(camBottom / (float)tileH) + 2);

	// Step 1: Image Layers
	for (const auto& imgLayer : map.mapData.imageLayers)
	{
		if (!imgLayer->texture) continue;
		float tw, th;
		SDL_GetTextureSize(imgLayer->texture, &tw, &th);
		float screenX = (float)viewX + mapViewOffsetX_ + imgLayer->offsetX * mapViewZoom_;
		float screenY = (float)viewY + mapViewOffsetY_ + imgLayer->offsetY * mapViewZoom_;
		float screenW = tw * mapViewZoom_;
		float screenH = th * mapViewZoom_;
		if (screenX + screenW < (float)viewX || screenX >(float)viewX + (float)viewW) continue;
		if (screenY + screenH < (float)viewY || screenY >(float)viewY + (float)viewH) continue;
		SDL_FRect dst = { screenX * (float)scale, screenY * (float)scale, screenW * (float)scale, screenH * (float)scale };
		SDL_SetTextureColorMod(imgLayer->texture, 255, 255, 255);
		SDL_SetTextureAlphaMod(imgLayer->texture, 255);
		SDL_RenderTexture(render.renderer, imgLayer->texture, nullptr, &dst);
	}

	// Step 2: Decoration Objects
	for (const auto& deco : map.mapData.decorationObjects)
	{
		if (!deco->texture) continue;
		float worldX = deco->x;
		float worldY = deco->y - deco->height;
		float screenX = (float)viewX + mapViewOffsetX_ + worldX * mapViewZoom_;
		float screenY = (float)viewY + mapViewOffsetY_ + worldY * mapViewZoom_;
		float screenW = deco->width * mapViewZoom_;
		float screenH = deco->height * mapViewZoom_;
		if (screenX + screenW < (float)viewX || screenX >(float)viewX + (float)viewW) continue;
		if (screenY + screenH < (float)viewY || screenY >(float)viewY + (float)viewH) continue;
		SDL_FRect dst = { screenX * (float)scale, screenY * (float)scale, screenW * (float)scale, screenH * (float)scale };
		SDL_SetTextureColorMod(deco->texture, 255, 255, 255);
		SDL_SetTextureAlphaMod(deco->texture, 255);
		SDL_RenderTexture(render.renderer, deco->texture, nullptr, &dst);
	}

	// Step 3: Tile Layers
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
				if (screenX + screenW < (float)viewX || screenX >(float)viewX + (float)viewW) continue;
				if (screenY + screenH < (float)viewY || screenY >(float)viewY + (float)viewH) continue;
				SDL_FRect dst = { screenX * (float)scale, screenY * (float)scale, screenW * (float)scale, screenH * (float)scale };
				SDL_FRect srcF = { (float)src.x, (float)src.y, (float)src.w, (float)src.h };
				SDL_SetTextureColorMod(ts->texture, 255, 255, 255);
				SDL_SetTextureAlphaMod(ts->texture, 255);
				SDL_RenderTexture(render.renderer, ts->texture, &srcF, &dst);
			}
		}
	}

	// Step 4: Player sprite + position marker
	if (player && player->texture)
	{

		Vector2D playerPos = player->GetPosition();
		float worldX = playerPos.getX() + (float)player->texW / 2.0f;
		float worldY = playerPos.getY() + (float)player->texH / 2.0f;


		float screenX = (float)viewX + mapViewOffsetX_ + worldX * mapViewZoom_;
		float screenY = (float)viewY + mapViewOffsetY_ + worldY * mapViewZoom_;


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



		int r = (int)(9.0f * mapViewZoom_ * (float)scale);
		if (r < 4) r = 4;


		int markerCX = (int)(screenX * (float)scale);
		int markerCY = (int)((screenY - drawH / 2.0f - 6.0f) * (float)scale);


		SDL_Rect shadowRect = {
			markerCX - r - 2,
			markerCY - r - 2,
			(r + 2) * 2,
			(r + 2) * 2
		};
		render.DrawRectangle(shadowRect, 0, 0, 0, 180, true, false);


		SDL_Rect dotRect = {
			markerCX - r,
			markerCY - r,
			r * 2,
			r * 2
		};
		render.DrawRectangle(dotRect, 255, 215, 0, 255, true, false);


		int innerR = std::max(2, r / 3);
		SDL_Rect innerDot = {
			markerCX - innerR,
			markerCY - innerR,
			innerR * 2,
			innerR * 2
		};
		render.DrawRectangle(innerDot, 255, 255, 255, 255, true, false);


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

// â”€â”€ Pause menu helpers â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

void Scene::LoadPauseMenuButtons()
{
	texPauseBackground_ = Engine::GetInstance().textures->Load("assets/textures/Menu/UI_Pause_Menu_base+background.png");
	texPauseButtonWhite_ = Engine::GetInstance().textures->Load("assets/textures/Menu/UI_Pause_Menu_button_white.png");
	texPauseButtonBlack_ = Engine::GetInstance().textures->Load("assets/textures/Menu/UI_Pause_Menu_button_black.png");
	texButtonFragmented_ = Engine::GetInstance().textures->Load("assets/textures/Menu/UI_Button_white_fragmented.png");

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

	if (texButtonFragmented_) {
		btnOpt->SetHoverTexture(texButtonFragmented_);
		btnMap->SetHoverTexture(texButtonFragmented_);
		btnMenu->SetHoverTexture(texButtonFragmented_);
		btnCont->SetHoverTexture(texButtonFragmented_);
	}







	const int panelW = 340;
	const int panelX = winW / 2 - panelW / 2;
	const int panelY = winH / 2 - 100;


	const int rowH = 52;













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

void Scene::ShowNoCapeNotification()
{
	noCapeNotifTimer_ = 3000.0f;
}

void Scene::DrawPauseMenu()
{
	auto& render = *Engine::GetInstance().render;
	int winW = 0, winH = 0;
	Engine::GetInstance().window->GetWindowSize(winW, winH);

	if (showPauseOptions_)
	{
		DrawPauseOptionsPanel(winW, winH);
		return;
	}

	if (texPauseBackground_)
		render.DrawTextureAlpha(texPauseBackground_, 0, 0, winW, winH, 255);
}

void Scene::DrawPauseOptionsPanel(int winW, int winH)
{
	auto& render = *Engine::GetInstance().render;

	const int panelW = 340;
	const int panelH = 240;
	const int panelX = winW / 2 - panelW / 2;
	const int panelY = winH / 2 - 100;
	const int rowH = 52;

	HandleVolumeSliderInput(panelX, panelY, panelW, rowH);

	// Prettier panel background with a modern gradient-like style
	SDL_Rect panel = { panelX, panelY, panelW, panelH };
	render.DrawRectangle(panel, 15, 20, 35, 245, true, false); // Dark teal base
	SDL_Rect innerPanel = { panelX + 2, panelY + 2, panelW - 4, panelH - 4 };
	render.DrawRectangle(innerPanel, 25, 32, 50, 250, false, false); // subtle inner border

	SDL_Rect topBar = { panelX, panelY, panelW, 36 };
	render.DrawRectangle(topBar, 35, 45, 75, 255, true, false);
	render.DrawMenuTextCentered("OPTIONS", { panelX, panelY + 4, panelW, 28 }, { 220, 240, 255, 255 });

	// Music slider
	render.DrawMenuTextCentered("MUSIC", { panelX, panelY + 45, panelW / 2 - 10, 20 }, { 180, 200, 220, 255 });

	char vol[8];
	snprintf(vol, sizeof(vol), "%d%%", static_cast<int>(musicVolume_ * 100.0f));
	render.DrawMenuTextCentered(vol, { panelX + panelW / 2, panelY + 45, panelW / 2, 20 }, { 255, 255, 255, 255 });

	SDL_Rect mBarBg = { panelX + 20, panelY + 74, panelW - 40, 8 };
	render.DrawRectangle(mBarBg, 10, 15, 25, 255, true, false); // Track background
	int mFill = static_cast<int>(static_cast<float>(panelW - 40) * musicVolume_);
	SDL_Rect mBarFill = { panelX + 20, panelY + 74, mFill, 8 };
	render.DrawRectangle(mBarFill, 100, 180, 255, 255, true, false); // vibrant cyan fill

	// Knob
	SDL_Rect mKnob = { panelX + 20 + mFill - 5, panelY + 70, 10, 16 };
	render.DrawRectangle(mKnob, 200, 220, 255, 255, true, false);

	// SFX slider
	render.DrawMenuTextCentered("SFX", { panelX, panelY + 45 + rowH, panelW / 2 - 10, 20 }, { 180, 200, 220, 255 });
	snprintf(vol, sizeof(vol), "%d%%", static_cast<int>(sfxVolume_ * 100.0f));
	render.DrawMenuTextCentered(vol, { panelX + panelW / 2, panelY + 45 + rowH, panelW / 2, 20 }, { 255, 255, 255, 255 });

	SDL_Rect sBarBg = { panelX + 20, panelY + 74 + rowH, panelW - 40, 8 };
	render.DrawRectangle(sBarBg, 10, 15, 25, 255, true, false);
	int sFill = static_cast<int>(static_cast<float>(panelW - 40) * sfxVolume_);
	SDL_Rect sBarFill = { panelX + 20, panelY + 74 + rowH, sFill, 8 };
	render.DrawRectangle(sBarFill, 100, 180, 255, 255, true, false);

	// Knob
	SDL_Rect sKnob = { panelX + 20 + sFill - 5, panelY + 70 + rowH, 10, 16 };
	render.DrawRectangle(sKnob, 200, 220, 255, 255, true, false);

	// Back Button rendering
	SDL_Rect backBtnBg = { panelX + panelW / 2 - 60, panelY + 60 + rowH * 2 + 10, 120, 36 };
	render.DrawRectangle(backBtnBg, 40, 50, 70, 255, true, false);
	render.DrawMenuTextCentered("BACK", backBtnBg, { 200, 220, 255, 255 });

	// â”€â”€ Gamepad selection indicator (â–º arrow next to selected slider) â”€â”€â”€â”€
	if (Engine::GetInstance().input->IsGamepadConnected()) {
		int selY = panelY + 45;
		if (optionsSliderSel_ == 1) selY += rowH;
		else if (optionsSliderSel_ == 2) selY = panelY + 60 + rowH * 2 + 10;

		// Highlight bar behind the selected row
		SDL_Rect selHighlight = { panelX + 4, selY - 2, panelW - 8, 42 };
		if (optionsSliderSel_ == 2) selHighlight = { backBtnBg.x - 4, backBtnBg.y - 2, backBtnBg.w + 8, backBtnBg.h + 4 };
		render.DrawRectangle(selHighlight, 60, 100, 180, 50, true, false);

		// Small arrow indicator
		render.DrawMenuTextCentered(">", { selHighlight.x, selHighlight.y + selHighlight.h / 2 - 10, 16, 20 }, { 100, 200, 255, 255 });
	}
}


void Scene::HandlePauseMenuUIEvents(UIElement* uiElement)
{
	if (waitingForFade_) return;

	Engine::GetInstance().audio->PlayFx(menuClickFxId);

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
	{
		showMapViewer_ = true;
		SetPauseMenuVisible(false);

		int winW = 0, winH = 0;
		Engine::GetInstance().window->GetWindowSize(winW, winH);
		Vector2D mapSize = Engine::GetInstance().map->GetMapSizeInPixels();

		const int viewW = winW - 20;
		const int viewH = winH - 42 - 30;

		mapViewZoom_ = 1.0f;

		Vector2D playerPos = player ? player->GetPosition() : Vector2D(0, 0);
		mapViewOffsetX_ = (float)viewW / 2.0f - (playerPos.getX() * mapViewZoom_);
		mapViewOffsetY_ = (float)viewH / 2.0f - (playerPos.getY() * mapViewZoom_);

		break;
	}
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

		float sc = RandF(0.30f, 0.42f);
		f.w = (float)winW * sc;
		f.h = f.w * (th / tw);

		f.inFront = (i < 3);

		// Logical distribution to AVOID the face (upper center-right part of the illustration)
		// We push them towards the edges of the right half or the bottom
		if (i % 2 == 0) {
			// Prefer bottom area
			f.x = RandF(halfW - 50.0f, (float)winW - f.w * 0.5f);
			f.y = RandF(halfH, (float)winH - f.h - 10.0f);
		}
		else {
			// Prefer side areas (far right or closer to center but not top-center)
			if (i == 1) f.x = RandF(halfW - 30.0f, halfW + 100.0f);
			else        f.x = RandF((float)winW - f.w - 20.0f, (float)winW - 10.0f);
			
			f.y = RandF(10.0f, halfH);
		}

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
		SDL_SetTextureScaleMode(f.tex, SDL_SCALEMODE_LINEAR);

		// Use logical coordinates directly — SDL_SetRenderLogicalPresentation handles scaling
		SDL_FRect dst = { f.x + xOff, f.y + yOff, f.w, f.h };

		SDL_RenderTextureRotated(render.renderer, f.tex, nullptr, &dst,
			(double)angle, nullptr, SDL_FLIP_NONE);

		SDL_SetTextureAlphaMod(f.tex, 255);
	}
}

bool Scene::HandleVolumeSliderInput(int panelX, int panelY, int panelW, int rowH)
{
	auto& input = *Engine::GetInstance().input;
	float dt = Engine::GetInstance().GetDt();

	// Back Button hit box (shared between Main Menu settings and Pause options)
	SDL_Rect backHit = { panelX + panelW / 2 - 60, panelY + 60 + rowH * 2 + 10, 120, 36 };

	// â”€â”€ Mouse slider dragging / clicks â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
	if (input.GetMouseButtonDown(SDL_BUTTON_LEFT) == KEY_DOWN ||
		input.GetMouseButtonDown(SDL_BUTTON_LEFT) == KEY_REPEAT)
	{
		Vector2D mousePos = input.GetMousePosition();
		int mouseX = (int)mousePos.getX();
		int mouseY = (int)mousePos.getY();

		// Check Back button click
		if (input.GetMouseButtonDown(SDL_BUTTON_LEFT) == KEY_DOWN) {
			if (mouseX >= backHit.x && mouseX <= backHit.x + backHit.w &&
				mouseY >= backHit.y && mouseY <= backHit.y + backHit.h) {
				return true; // Go back
			}
		}

		int trackX = panelX + 20;
		int trackW = panelW - 40;

		// Music slider hit box
		SDL_Rect mSliderHit = { trackX - 10, panelY + 60, trackW + 20, 30 };
		if (mouseX >= mSliderHit.x && mouseX <= mSliderHit.x + mSliderHit.w &&
			mouseY >= mSliderHit.y && mouseY <= mSliderHit.y + mSliderHit.h) {

			musicVolume_ = (float)(mouseX - trackX) / (float)trackW;
			if (musicVolume_ < 0.0f) musicVolume_ = 0.0f;
			if (musicVolume_ > 1.0f) musicVolume_ = 1.0f;
			Engine::GetInstance().audio->SetMusicVolume(musicVolume_);
		}

		// SFX slider hit box
		SDL_Rect sSliderHit = { trackX - 10, panelY + 60 + rowH, trackW + 20, 30 };
		if (mouseX >= sSliderHit.x && mouseX <= sSliderHit.x + sSliderHit.w &&
			mouseY >= sSliderHit.y && mouseY <= sSliderHit.y + sSliderHit.h) {

			sfxVolume_ = (float)(mouseX - trackX) / (float)trackW;
			if (sfxVolume_ < 0.0f) sfxVolume_ = 0.0f;
			if (sfxVolume_ > 1.0f) sfxVolume_ = 1.0f;
			Engine::GetInstance().audio->SetSFXVolume(sfxVolume_);
		}
	}

	// â”€â”€ Gamepad D-pad / stick slider navigation â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
	// Back button shortcuts (B / East)
	if (input.GetGamepadButton(SDL_GAMEPAD_BUTTON_EAST) == KEY_DOWN) {
		return true; // Back
	}

	// D-pad Up/Down or Left Stick Y: switch between Music (0), SFX (1), Back (2)
	if (input.GetGamepadButton(SDL_GAMEPAD_BUTTON_DPAD_UP) == KEY_DOWN ||
		(input.GetLeftStickY() < -0.5f && sliderRepeatTimer_ <= 0.0f))
	{
		optionsSliderSel_ = std::max(0, optionsSliderSel_ - 1);
		sliderRepeatTimer_ = 250.0f;
	}
	if (input.GetGamepadButton(SDL_GAMEPAD_BUTTON_DPAD_DOWN) == KEY_DOWN ||
		(input.GetLeftStickY() > 0.5f && sliderRepeatTimer_ <= 0.0f))
	{
		optionsSliderSel_ = std::min(2, optionsSliderSel_ + 1);
		sliderRepeatTimer_ = 250.0f;
	}

	// If Back is selected and A (SOUTH) is pressed, go back
	if (optionsSliderSel_ == 2 && input.GetGamepadButton(SDL_GAMEPAD_BUTTON_SOUTH) == KEY_DOWN) {
		return true;
	}

	// D-pad Left/Right: step volume by 5%
	float volStep = 0.05f;
	bool stepLeft = input.GetGamepadButton(SDL_GAMEPAD_BUTTON_DPAD_LEFT) == KEY_DOWN;
	bool stepRight = input.GetGamepadButton(SDL_GAMEPAD_BUTTON_DPAD_RIGHT) == KEY_DOWN;

	// Left stick X: continuous adjust (smooth)
	float stickX = input.GetLeftStickX();
	float continuousAdj = 0.0f;
	if (std::fabs(stickX) > 0.3f)
		continuousAdj = stickX * 0.0005f * dt;

	if (optionsSliderSel_ == 0) {
		if (stepRight)  musicVolume_ = std::min(1.0f, musicVolume_ + volStep);
		if (stepLeft)   musicVolume_ = std::max(0.0f, musicVolume_ - volStep);
		if (continuousAdj != 0.0f) {
			musicVolume_ += continuousAdj;
			if (musicVolume_ < 0.0f) musicVolume_ = 0.0f;
			if (musicVolume_ > 1.0f) musicVolume_ = 1.0f;
		}
		if (stepLeft || stepRight || continuousAdj != 0.0f)
			Engine::GetInstance().audio->SetMusicVolume(musicVolume_);
	}
	else if (optionsSliderSel_ == 1) {
		if (stepRight)  sfxVolume_ = std::min(1.0f, sfxVolume_ + volStep);
		if (stepLeft)   sfxVolume_ = std::max(0.0f, sfxVolume_ - volStep);
		if (continuousAdj != 0.0f) {
			sfxVolume_ += continuousAdj;
			if (sfxVolume_ < 0.0f) sfxVolume_ = 0.0f;
			if (sfxVolume_ > 1.0f) sfxVolume_ = 1.0f;
		}
		if (stepLeft || stepRight || continuousAdj != 0.0f)
			Engine::GetInstance().audio->SetSFXVolume(sfxVolume_);
	}

	if (sliderRepeatTimer_ > 0.0f)
		sliderRepeatTimer_ -= dt;

	return false;
}
