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
#include "Boss.h"
#include "Boss1.h"
#include "Checkpoint.h"
#include "Boss2.h"
#include <cstdlib>
#include <cmath>
#include <algorithm>
#include <filesystem>

// ----------------------------------------------------------------------------
// Button IDs  (main menu)
// ----------------------------------------------------------------------------
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

	levels_ = {
		{"LEVEL 1", "Rock Bottom", "MapTemplate.tmx"},
		{"LEVEL 2", "Shattered Ruins", "Map2.tmx"},
		{"LEVEL 3", "Forest Borderlines", "Map3.tmx"},
		{"LEVEL 4", "Forgotten Playground", "Map4.tmx"}
	};
}

Scene::~Scene() {}

bool Scene::Awake()
{
	LOG("Loading Scene");
	menuClickFxId = Engine::GetInstance().audio->LoadFx("assets/audio/fx/Menu-Selection-Click.wav");
	checkpointFxId = Engine::GetInstance().audio->LoadFx("assets/audio/fx/checkpoint2.wav");
	hasPendingSceneChange = true;
	pendingScene = currentScene;
	return true;
}

bool Scene::Start() { return true; }
bool Scene::PreUpdate() { return true; }

bool Scene::Update(float dt)
{
	if (waitingForFade_) {
		float fadeAlpha = Engine::GetInstance().render->GetFadeAlpha();
		// If FADE_OUT, fadeAlpha goes 0 -> 255.
		// Scale the master volumes down based on the user's settings.
		if (Engine::GetInstance().render->IsFadingOut()) {
			float fadeMultiplier = 1.0f - (fadeAlpha / 255.0f);
			Engine::GetInstance().audio->SetMusicVolume(musicVolume_ * fadeMultiplier);
			Engine::GetInstance().audio->SetSFXVolume(sfxVolume_ * fadeMultiplier);
		}

		if (Engine::GetInstance().render->IsFadeComplete()) {
			waitingForFade_ = false;
			
			// Restore the standard volumes for the next scene
			Engine::GetInstance().audio->SetMusicVolume(musicVolume_);
			Engine::GetInstance().audio->SetSFXVolume(sfxVolume_);
			
			if (pendingSubMapLoad_) {
				pendingSubMapLoad_ = false;
				ExecuteSubMapLoad();
				Engine::GetInstance().render->StartFade(FadeDirection::FADE_IN, 400.0f);
			}
			else {
				ChangeScene(fadeTargetScene_);
				Engine::GetInstance().render->StartFade(FadeDirection::FADE_IN, 800.0f);
			}
		}
	}

	if (hasPendingSceneChange) {
		hasPendingSceneChange = false;
		ChangeScene(pendingScene);
	}

	if (Engine::GetInstance().input->GetKey(konamiSequence[konamiIndex]) == KEY_DOWN) {

		konamiIndex++;

		if (konamiIndex >= 10) {
			isKonamiActive = !isKonamiActive;

			if (isKonamiActive) {
				LOG("CODIGO KONAMI COMPLETADO Recompensa activada.");

				if (player != nullptr) {
					player->health = 999999; 
					ResetHealthUI(3);
				}
			}
			else {
				LOG("Codigo Konami desactivado.");
			}

			konamiIndex = 0; 
		}
	}

	switch (currentScene)
	{
	case SceneID::INTRO:
		UpdateIntro(dt);
		break;
	case SceneID::MAIN_MENU:
		UpdateMainMenu(dt);
		PostUpdateMainMenu();
		break;
	case SceneID::INTRO_CINEMATIC:
		UpdateIntroCinematic(dt);
		break;
	case SceneID::TUTORIAL_TEXT_CARD:
		UpdateTutorialTextCard(dt);
		break;
	case SceneID::LOADING:
		UpdateLoading(dt);
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
	case SceneID::TUTORIAL_TEXT_CARD: LoadTutorialTextCard();  break;
	case SceneID::LOADING:         LoadLoading();          break;
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
	case SceneID::TUTORIAL_TEXT_CARD: UnloadTutorialTextCard();  break;
	case SceneID::LOADING:         UnloadLoading();          break;
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
	playSubState_ = PlaySubState::NONE;
	playSubAnimTimer_ = 0.0f;
	playSubAlpha_ = 0.0f;
	playSlotsAlpha_ = 0.0f;
	isSlotSelectionForNewGame_ = false;
	loadingFromSaveSlot_ = false;
	loadedFromSave_ = false;

	Engine::GetInstance().audio->SetMusicVolume(musicVolume_);
	Engine::GetInstance().audio->SetSFXVolume(sfxVolume_);

	// Sync display mode index with actual window mode
	WindowMode wm = Engine::GetInstance().window->GetWindowMode();
	if (wm == WindowMode::FULLSCREEN) windowModeIndex_ = 1;
	else if (wm == WindowMode::BORDERLESS) windowModeIndex_ = 2;
	else windowModeIndex_ = 0;

	Engine::GetInstance().discord->UpdatePresence("In Main Menu", "Release v1.0.0");
    Engine::GetInstance().render->SetCameraSway(false);

	Engine::GetInstance().audio->PlayMusic("assets/audio/music/Echoes_of_Slumber_Main_Menu.wav", 1.0f);

	SDL_Texture* rawLogo = Engine::GetInstance().textures->Load("assets/textures/Menu/EchoesOfSlumber.png");
	texMenuLogo_ = Engine::GetInstance().render->RecolorTexture(rawLogo, 212, 218, 234);
	Engine::GetInstance().textures->UnLoad(rawLogo);
	texMenuChild_ = Engine::GetInstance().textures->Load("assets/textures/Menu/IL_NenFront_01.png");
	texMenuButton_ = Engine::GetInstance().textures->Load("assets/textures/Menu/UI_Pause_Menu_button_white.png");
	texButtonFragmented_ = Engine::GetInstance().textures->Load("assets/textures/Menu/UI_Button_white_fragmented.png");
	texSettingsBase_ = Engine::GetInstance().textures->Load("assets/textures/UI/UI_Settings_Main_Menu_FIXED.png");
	texSettingsPause_ = Engine::GetInstance().textures->Load("assets/textures/UI/UI_Settings_Main_Menu_FIXED.png");
	texMenuHiddenLogo_ = Engine::GetInstance().textures->Load("assets/textures/UI/UI_HiddenLogo.png");

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

	menuBtnX_ = btnX;
	menuBtnW_ = btnW;

	// --- Play sub-screen buttons (New Game / Continue / Back) ---
	btnNewGame_ = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, BTN_PLAY_NEWGAME, "new game", playPos, this);
	btnContinue_ = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, BTN_PLAY_CONTINUE, "continue", settingsPos, this);
	btnPlayBack_ = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, BTN_PLAY_BACK, "back", exitPos, this);

	// --- Save slot buttons (Slot 1 / Slot 2 / Slot 3 / Back) ---
	btnSlot1_ = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, BTN_SLOT_1, "slot 1", playPos, this);
	btnSlot2_ = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, BTN_SLOT_2, "slot 2", settingsPos, this);
	btnSlot3_ = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, BTN_SLOT_3, "slot 3", exitPos, this);
	btnSlotsBack_ = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, BTN_SLOTS_BACK, "back", backPos, this);

	// Setup textures and hide play sub-screen buttons initially
	std::vector<std::shared_ptr<UIElement>> playSubBtns = {
		btnNewGame_, btnContinue_, btnPlayBack_, btnSlot1_, btnSlot2_, btnSlot3_, btnSlotsBack_
	};
	for (auto& btn : playSubBtns) {
		if (btn) {
			btn->alphaMod = 0.0f;
			btn->isVisible = false;
			btn->state = UIElementState::DISABLED;
			if (texMenuButton_) btn->SetTexture(texMenuButton_);
			if (texButtonFragmented_) btn->SetHoverTexture(texButtonFragmented_);
		}
	}
}

void Scene::UnloadMainMenu()
{
	Engine::GetInstance().uiManager->CleanUp();
	btnPlay_ = nullptr;
	btnSettings_ = nullptr;
	btnExit_ = nullptr;
	btnBack_ = nullptr;
	btnNewGame_ = nullptr;
	btnContinue_ = nullptr;
	btnPlayBack_ = nullptr;
	btnSlot1_ = nullptr;
	btnSlot2_ = nullptr;
	btnSlot3_ = nullptr;
	btnSlotsBack_ = nullptr;

	if (texMenuLogo_) { SDL_DestroyTexture(texMenuLogo_);                       texMenuLogo_ = nullptr; }
	if (texMenuHiddenLogo_) { Engine::GetInstance().textures->UnLoad(texMenuHiddenLogo_); texMenuHiddenLogo_ = nullptr; }
	if (texMenuChild_) { Engine::GetInstance().textures->UnLoad(texMenuChild_);  texMenuChild_ = nullptr; }
	if (texMenuButton_) { Engine::GetInstance().textures->UnLoad(texMenuButton_); texMenuButton_ = nullptr; }
	if (texButtonFragmented_) { Engine::GetInstance().textures->UnLoad(texButtonFragmented_); texButtonFragmented_ = nullptr; }
	if (texSettingsBase_) { Engine::GetInstance().textures->UnLoad(texSettingsBase_); texSettingsBase_ = nullptr; }
	if (texSettingsPause_) { Engine::GetInstance().textures->UnLoad(texSettingsPause_); texSettingsPause_ = nullptr; }
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
		// -- Settings in-place animation state machine --------------------
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
					Engine::GetInstance().discord->UpdatePresence("In Main Menu", "Release v1.0.0");
				}
				break;
			default: break;
			}
		}

		// -- Play sub-screen animation state machine ----------------------
		if (playSubState_ != PlaySubState::NONE &&
			playSubState_ != PlaySubState::OPTS_ACTIVE &&
			playSubState_ != PlaySubState::SLOTS_ACTIVE)
		{
			playSubAnimTimer_ += dt;
			float t = playSubAnimTimer_ / FADE_DURATION;
			if (t > 1.0f) t = 1.0f;

			switch (playSubState_) {
			case PlaySubState::FADE_OUT_BUTTONS:
				settingsButtonsAlpha_ = 1.0f - t;
				if (btnPlay_) btnPlay_->alphaMod = settingsButtonsAlpha_;
				if (btnSettings_) btnSettings_->alphaMod = settingsButtonsAlpha_;
				if (btnExit_) btnExit_->alphaMod = settingsButtonsAlpha_;
				if (t >= 1.0f) {
					if (btnPlay_) { btnPlay_->isVisible = false; btnPlay_->state = UIElementState::DISABLED; }
					if (btnSettings_) { btnSettings_->isVisible = false; btnSettings_->state = UIElementState::DISABLED; }
					if (btnExit_) { btnExit_->isVisible = false; btnExit_->state = UIElementState::DISABLED; }

					playSubState_ = PlaySubState::FADE_IN_OPTS;
					playSubAnimTimer_ = 0.0f;
					playSubAlpha_ = 0.0f;

					if (btnNewGame_) { btnNewGame_->isVisible = true; btnNewGame_->state = UIElementState::NORMAL; btnNewGame_->alphaMod = 0.0f; }
					if (btnContinue_) {
						btnContinue_->isVisible = true;
						auto& saveSys = Engine::GetInstance().saveSystem;
						bool anySave = saveSys->SlotHasValidSave(0) || saveSys->SlotHasValidSave(1) || saveSys->SlotHasValidSave(2);
						btnContinue_->state = anySave ? UIElementState::NORMAL : UIElementState::DISABLED;
						btnContinue_->alphaMod = 0.0f;
					}
					if (btnPlayBack_) { btnPlayBack_->isVisible = true; btnPlayBack_->state = UIElementState::NORMAL; btnPlayBack_->alphaMod = 0.0f; }
				}
				break;

			case PlaySubState::FADE_IN_OPTS:
				playSubAlpha_ = t;
				if (btnNewGame_) btnNewGame_->alphaMod = t;
				if (btnContinue_ && btnContinue_->state != UIElementState::DISABLED) btnContinue_->alphaMod = t;
				else if (btnContinue_) btnContinue_->alphaMod = t * 0.5f;
				if (btnPlayBack_) btnPlayBack_->alphaMod = t;
				if (t >= 1.0f) {
					playSubState_ = PlaySubState::OPTS_ACTIVE;
					playSubAlpha_ = 1.0f;
					if (btnNewGame_) btnNewGame_->alphaMod = 1.0f;
					if (btnContinue_ && btnContinue_->state != UIElementState::DISABLED) btnContinue_->alphaMod = 1.0f;
					else if (btnContinue_) btnContinue_->alphaMod = 0.5f;
					if (btnPlayBack_) btnPlayBack_->alphaMod = 1.0f;
				}
				break;

			case PlaySubState::FADE_OUT_OPTS_TO_SLOTS:
				playSubAlpha_ = 1.0f - t;
				if (btnNewGame_) btnNewGame_->alphaMod = playSubAlpha_;
				if (btnContinue_ && btnContinue_->state != UIElementState::DISABLED) btnContinue_->alphaMod = playSubAlpha_;
				else if (btnContinue_) btnContinue_->alphaMod = playSubAlpha_ * 0.5f;
				if (btnPlayBack_) btnPlayBack_->alphaMod = playSubAlpha_;
				if (t >= 1.0f) {
					if (btnNewGame_) { btnNewGame_->isVisible = false; btnNewGame_->state = UIElementState::DISABLED; }
					if (btnContinue_) { btnContinue_->isVisible = false; btnContinue_->state = UIElementState::DISABLED; }
					if (btnPlayBack_) { btnPlayBack_->isVisible = false; btnPlayBack_->state = UIElementState::DISABLED; }

					playSubState_ = PlaySubState::FADE_IN_SLOTS;
					playSubAnimTimer_ = 0.0f;
					playSlotsAlpha_ = 0.0f;

					auto& saveSys = Engine::GetInstance().saveSystem;
					if (btnSlot1_) {
						btnSlot1_->text = saveSys->GetSlotInfoString(0);
						btnSlot1_->isVisible = true;
						btnSlot1_->alphaMod = 0.0f;
						if (isSlotSelectionForNewGame_) {
							btnSlot1_->state = UIElementState::NORMAL;
						} else {
							btnSlot1_->state = saveSys->SlotHasValidSave(0) ? UIElementState::NORMAL : UIElementState::DISABLED;
						}
					}
					if (btnSlot2_) {
						btnSlot2_->text = saveSys->GetSlotInfoString(1);
						btnSlot2_->isVisible = true;
						btnSlot2_->alphaMod = 0.0f;
						if (isSlotSelectionForNewGame_) {
							btnSlot2_->state = UIElementState::NORMAL;
						} else {
							btnSlot2_->state = saveSys->SlotHasValidSave(1) ? UIElementState::NORMAL : UIElementState::DISABLED;
						}
					}
					if (btnSlot3_) {
						btnSlot3_->text = saveSys->GetSlotInfoString(2);
						btnSlot3_->isVisible = true;
						btnSlot3_->alphaMod = 0.0f;
						if (isSlotSelectionForNewGame_) {
							btnSlot3_->state = UIElementState::NORMAL;
						} else {
							btnSlot3_->state = saveSys->SlotHasValidSave(2) ? UIElementState::NORMAL : UIElementState::DISABLED;
						}
					}
					if (btnSlotsBack_) { btnSlotsBack_->isVisible = true; btnSlotsBack_->state = UIElementState::NORMAL; btnSlotsBack_->alphaMod = 0.0f; }
				}
				break;

			case PlaySubState::FADE_IN_SLOTS:
				playSlotsAlpha_ = t;
				if (btnSlot1_) btnSlot1_->alphaMod = (btnSlot1_->state == UIElementState::DISABLED) ? t * 0.5f : t;
				if (btnSlot2_) btnSlot2_->alphaMod = (btnSlot2_->state == UIElementState::DISABLED) ? t * 0.5f : t;
				if (btnSlot3_) btnSlot3_->alphaMod = (btnSlot3_->state == UIElementState::DISABLED) ? t * 0.5f : t;
				if (btnSlotsBack_) btnSlotsBack_->alphaMod = t;
				if (t >= 1.0f) {
					playSubState_ = PlaySubState::SLOTS_ACTIVE;
					playSlotsAlpha_ = 1.0f;
					if (btnSlot1_) btnSlot1_->alphaMod = (btnSlot1_->state == UIElementState::DISABLED) ? 0.5f : 1.0f;
					if (btnSlot2_) btnSlot2_->alphaMod = (btnSlot2_->state == UIElementState::DISABLED) ? 0.5f : 1.0f;
					if (btnSlot3_) btnSlot3_->alphaMod = (btnSlot3_->state == UIElementState::DISABLED) ? 0.5f : 1.0f;
					if (btnSlotsBack_) btnSlotsBack_->alphaMod = 1.0f;
				}
				break;

			case PlaySubState::FADE_OUT_SLOTS:
				playSlotsAlpha_ = 1.0f - t;
				if (btnSlot1_) btnSlot1_->alphaMod = (btnSlot1_->state == UIElementState::DISABLED) ? playSlotsAlpha_ * 0.5f : playSlotsAlpha_;
				if (btnSlot2_) btnSlot2_->alphaMod = (btnSlot2_->state == UIElementState::DISABLED) ? playSlotsAlpha_ * 0.5f : playSlotsAlpha_;
				if (btnSlot3_) btnSlot3_->alphaMod = (btnSlot3_->state == UIElementState::DISABLED) ? playSlotsAlpha_ * 0.5f : playSlotsAlpha_;
				if (btnSlotsBack_) btnSlotsBack_->alphaMod = playSlotsAlpha_;
				if (t >= 1.0f) {
					if (btnSlot1_) { btnSlot1_->isVisible = false; btnSlot1_->state = UIElementState::DISABLED; }
					if (btnSlot2_) { btnSlot2_->isVisible = false; btnSlot2_->state = UIElementState::DISABLED; }
					if (btnSlot3_) { btnSlot3_->isVisible = false; btnSlot3_->state = UIElementState::DISABLED; }
					if (btnSlotsBack_) { btnSlotsBack_->isVisible = false; btnSlotsBack_->state = UIElementState::DISABLED; }

					playSubState_ = PlaySubState::FADE_IN_OPTS_FROM_SLOTS;
					playSubAnimTimer_ = 0.0f;
					playSubAlpha_ = 0.0f;

					if (btnNewGame_) { btnNewGame_->isVisible = true; btnNewGame_->state = UIElementState::NORMAL; btnNewGame_->alphaMod = 0.0f; }
					if (btnContinue_) {
						btnContinue_->isVisible = true;
						auto& saveSys = Engine::GetInstance().saveSystem;
						bool anySave = saveSys->SlotHasValidSave(0) || saveSys->SlotHasValidSave(1) || saveSys->SlotHasValidSave(2);
						btnContinue_->state = anySave ? UIElementState::NORMAL : UIElementState::DISABLED;
						btnContinue_->alphaMod = 0.0f;
					}
					if (btnPlayBack_) { btnPlayBack_->isVisible = true; btnPlayBack_->state = UIElementState::NORMAL; btnPlayBack_->alphaMod = 0.0f; }
				}
				break;

			case PlaySubState::FADE_IN_OPTS_FROM_SLOTS:
				playSubAlpha_ = t;
				if (btnNewGame_) btnNewGame_->alphaMod = t;
				if (btnContinue_ && btnContinue_->state != UIElementState::DISABLED) btnContinue_->alphaMod = t;
				else if (btnContinue_) btnContinue_->alphaMod = t * 0.5f;
				if (btnPlayBack_) btnPlayBack_->alphaMod = t;
				if (t >= 1.0f) {
					playSubState_ = PlaySubState::OPTS_ACTIVE;
					playSubAlpha_ = 1.0f;
					if (btnNewGame_) btnNewGame_->alphaMod = 1.0f;
					if (btnContinue_ && btnContinue_->state != UIElementState::DISABLED) btnContinue_->alphaMod = 1.0f;
					else if (btnContinue_) btnContinue_->alphaMod = 0.5f;
					if (btnPlayBack_) btnPlayBack_->alphaMod = 1.0f;
				}
				break;

			case PlaySubState::FADE_OUT_OPTS:
				playSubAlpha_ = 1.0f - t;
				if (btnNewGame_) btnNewGame_->alphaMod = playSubAlpha_;
				if (btnContinue_ && btnContinue_->state != UIElementState::DISABLED) btnContinue_->alphaMod = playSubAlpha_;
				else if (btnContinue_) btnContinue_->alphaMod = playSubAlpha_ * 0.5f;
				if (btnPlayBack_) btnPlayBack_->alphaMod = playSubAlpha_;
				if (t >= 1.0f) {
					if (btnNewGame_) { btnNewGame_->isVisible = false; btnNewGame_->state = UIElementState::DISABLED; }
					if (btnContinue_) { btnContinue_->isVisible = false; btnContinue_->state = UIElementState::DISABLED; }
					if (btnPlayBack_) { btnPlayBack_->isVisible = false; btnPlayBack_->state = UIElementState::DISABLED; }

					playSubState_ = PlaySubState::FADE_IN_BUTTONS;
					playSubAnimTimer_ = 0.0f;
					settingsButtonsAlpha_ = 0.0f;

					if (btnPlay_) { btnPlay_->isVisible = true; btnPlay_->state = UIElementState::NORMAL; btnPlay_->alphaMod = 0.0f; }
					if (btnSettings_) { btnSettings_->isVisible = true; btnSettings_->state = UIElementState::NORMAL; btnSettings_->alphaMod = 0.0f; }
					if (btnExit_) { btnExit_->isVisible = true; btnExit_->state = UIElementState::NORMAL; btnExit_->alphaMod = 0.0f; }
				}
				break;

			case PlaySubState::FADE_IN_BUTTONS:
				settingsButtonsAlpha_ = t;
				if (btnPlay_) btnPlay_->alphaMod = settingsButtonsAlpha_;
				if (btnSettings_) btnSettings_->alphaMod = settingsButtonsAlpha_;
				if (btnExit_) btnExit_->alphaMod = settingsButtonsAlpha_;
				if (t >= 1.0f) {
					playSubState_ = PlaySubState::NONE;
					settingsButtonsAlpha_ = 1.0f;
					Engine::GetInstance().discord->UpdatePresence("In Main Menu", "Release v1.0.0");
				}
				break;
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
			Engine::GetInstance().audio->PlayFx(menuClickFxId, 0, true);
		}

		// ESC / B to go back from play sub-screens
		if (inp2.GetKey(SDL_SCANCODE_ESCAPE) == KEY_DOWN ||
			inp2.GetGamepadButton(SDL_GAMEPAD_BUTTON_EAST) == KEY_DOWN)
		{
			if (playSubState_ == PlaySubState::OPTS_ACTIVE) {
				playSubState_ = PlaySubState::FADE_OUT_OPTS;
				playSubAnimTimer_ = 0.0f;
				Engine::GetInstance().uiManager->ResetFocus();
				Engine::GetInstance().audio->PlayFx(menuClickFxId, 0, true);
			}
			else if (playSubState_ == PlaySubState::SLOTS_ACTIVE) {
				playSubState_ = PlaySubState::FADE_OUT_SLOTS;
				playSubAnimTimer_ = 0.0f;
				Engine::GetInstance().uiManager->ResetFocus();
				Engine::GetInstance().audio->PlayFx(menuClickFxId, 0, true);
			}
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

	// Render studio logo bottom-left and version text bottom-right
	Uint8 bottomAlpha = 0;
	if (menuAnimState_ == MenuAnimState::IDLE) {
		bottomAlpha = 255;
	} else if (menuAnimState_ == MenuAnimState::FADE_FRAGS_BTNS) {
		float f = menuAnimTimer_ / 2000.0f;
		if (f > 1.0f) f = 1.0f;
		bottomAlpha = (Uint8)(255 * f);
	}
	bottomAlpha = (Uint8)((float)bottomAlpha * settingsButtonsAlpha_);

	if (bottomAlpha > 0) {
		// 1. Bottom-left: UI_HiddenLogo.png
		if (texMenuHiddenLogo_) {
			float hW = 0, hH = 0;
			SDL_GetTextureSize(texMenuHiddenLogo_, &hW, &hH);
			float targetH = 100.0f;
			float scale = targetH / hH;
			float targetW = hW * scale;

			float centerBaselineY = (float)winH - 20.0f - 50.0f;
			float logoY = centerBaselineY - targetH / 2.0f;
			float logoX = 20.0f;

			render.DrawTextureAlphaF(texMenuHiddenLogo_, logoX, logoY, targetW, targetH, bottomAlpha);
		}

		// 2. Bottom-right: "v1.0.0"
		SDL_Color versionColor = { 255, 255, 255, bottomAlpha };
		SDL_Texture* txVersion = render.CreateMenuTextTexture("v1.0.0", versionColor);
		if (txVersion) {
			float vW = 0, vH = 0;
			SDL_GetTextureSize(txVersion, &vW, &vH);
			float textScale = 0.4f;
			float drawW = vW * textScale;
			float drawH = vH * textScale;

			float centerBaselineY = (float)winH - 20.0f - 50.0f;
			float versionY = centerBaselineY - drawH / 2.0f;
			float versionX = (float)winW - drawW - 20.0f;

			render.DrawTextureAlphaF(txVersion, versionX, versionY, drawW, drawH, bottomAlpha);
			SDL_DestroyTexture(txVersion);
		}
	}

	if (showSettings_ || settingsOptionsAlpha_ > 0.0f)
		DrawSettingsInPlace(winW, winH);
}

void Scene::DrawPlaySubScreen(int winW, int winH)
{
	// Headers removed as requested
}

void Scene::DrawSettingsInPlace(int winW, int winH)
{
	auto& render = *Engine::GetInstance().render;
	Uint8 alpha = (Uint8)(255.0f * settingsOptionsAlpha_);
	if (alpha == 0) return;

	// Draw the base panel image centered in the left half
	const int leftHalf = winW / 2;

	// Render texture using the original 840.0f width, but make it significantly taller (780.0f height)
	const float panelScale = (float)leftHalf / 840.0f * 0.85f;
	const int panelImgW = (int)(840.0f * panelScale);
	const int panelImgH = (int)(780.0f * panelScale);
	const int panelImgX = (leftHalf - panelImgW) / 2;
	const int panelImgY = (winH - panelImgH) / 2;

	// Shift the background texture up slightly so texts sit better in the white space
	const int panelRenderY = panelImgY - (int)(100.0f * panelScale);

	if (texSettingsBase_) {
		render.DrawTextureAlpha(texSettingsBase_, panelImgX, panelRenderY, panelImgW, panelImgH, alpha);
	}

	// Track area — aligned with the panel's icon positions
	const int trackX = panelImgX + (int)(panelImgW * 0.44f);
	const int trackW = (int)(panelImgW * 0.42f);

	// Better-spaced rows to avoid overlap with panel icons
	const int row0Y = panelImgY + (int)(panelImgH * 0.32f); // MUSIC
	const int row1Y = panelImgY + (int)(panelImgH * 0.48f); // SOUNDS
	const int row2Y = panelImgY + (int)(panelImgH * 0.62f); // DISPLAY

	SDL_Color labelColor = { 40, 55, 70, alpha };
	SDL_Color valColor   = { 30, 45, 60, alpha };

	// --- Row 0: MUSIC ---
	render.DrawMenuTextCentered("MUSIC", { trackX, row0Y, trackW / 2, 20 }, labelColor);
	char vol[8];
	snprintf(vol, sizeof(vol), "%d", static_cast<int>(musicVolume_ * 100.0f));
	render.DrawMenuTextCentered(vol, { trackX + trackW / 2, row0Y, trackW / 2, 20 }, valColor);

	SDL_Rect mBarBg = { trackX, row0Y + 25, trackW, 6 };
	render.DrawRectangle(mBarBg, 10, 15, 25, alpha, true, false);
	int mFill = static_cast<int>(static_cast<float>(trackW) * musicVolume_);
	SDL_Rect mBarFill = { trackX, row0Y + 25, mFill, 6 };
	render.DrawRectangle(mBarFill, 100, 180, 255, alpha, true, false);
	SDL_Rect mKnob = { trackX + mFill - 4, row0Y + 22, 8, 12 };
	render.DrawRectangle(mKnob, 200, 220, 255, alpha, true, false);

	// --- Row 1: SOUNDS ---
	render.DrawMenuTextCentered("SOUNDS", { trackX, row1Y, trackW / 2, 20 }, labelColor);
	snprintf(vol, sizeof(vol), "%d", static_cast<int>(sfxVolume_ * 100.0f));
	render.DrawMenuTextCentered(vol, { trackX + trackW / 2, row1Y, trackW / 2, 20 }, valColor);

	SDL_Rect sBarBg = { trackX, row1Y + 25, trackW, 6 };
	render.DrawRectangle(sBarBg, 10, 15, 25, alpha, true, false);
	int sFill = static_cast<int>(static_cast<float>(trackW) * sfxVolume_);
	SDL_Rect sBarFill = { trackX, row1Y + 25, sFill, 6 };
	render.DrawRectangle(sBarFill, 100, 180, 255, alpha, true, false);
	SDL_Rect sKnob = { trackX + sFill - 4, row1Y + 22, 8, 12 };
	render.DrawRectangle(sKnob, 200, 220, 255, alpha, true, false);

	// --- Row 2: DISPLAY ---
	render.DrawMenuTextCentered("DISPLAY", { trackX, row2Y, trackW, 20 }, labelColor);

	const char* modeNames[] = { "WINDOWED", "FULLSCREEN", "BORDERLESS" };
	int arrowW = 30;
	SDL_Rect leftArrowArea  = { trackX - 10, row2Y + 24, arrowW, 26 }; // Moved further left
	SDL_Rect modeArea       = { trackX + arrowW, row2Y + 24, trackW - arrowW * 2, 26 };
	SDL_Rect rightArrowArea = { trackX + trackW - arrowW + 10, row2Y + 24, arrowW, 26 }; // Moved further right

	SDL_Color arrowColor = { 100, 180, 255, alpha };
	render.DrawMenuTextCentered("<", leftArrowArea, arrowColor);
	render.DrawMenuTextCentered(modeNames[windowModeIndex_], modeArea, valColor);
	render.DrawMenuTextCentered(">", rightArrowArea, arrowColor);

	// Handle input for sliders and display mode
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
				Engine::GetInstance().audio->PlayFx(menuClickFxId, 0, true);
			}
			if (mx >= rightArrowArea.x && mx <= rightArrowArea.x + rightArrowArea.w &&
				my >= rightArrowArea.y && my <= rightArrowArea.y + rightArrowArea.h) {
				windowModeIndex_ = (windowModeIndex_ + 1) % 3;
				WindowMode modes[] = { WindowMode::WINDOWED, WindowMode::FULLSCREEN, WindowMode::BORDERLESS };
				Engine::GetInstance().window->SetWindowMode(modes[windowModeIndex_]);
				Engine::GetInstance().audio->PlayFx(menuClickFxId, 0, true);
			}
		}

		// -- Gamepad navigation --------------------------------------
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
				Engine::GetInstance().audio->PlayFx(menuClickFxId, 0, true);
			}
		}

		if (sliderRepeatTimer_ > 0.0f) sliderRepeatTimer_ -= dt;

		if (gpInput.IsGamepadConnected()) {
			int selY = row0Y;
			if (optionsSliderSel_ == 1) selY = row1Y;
			else if (optionsSliderSel_ == 2) selY = row2Y;
			SDL_Rect selHighlight = { trackX - 10, selY - 4, trackW + 20, 50 };
			render.DrawRectangle(selHighlight, 60, 100, 180, 50, true, false);
		}
	}
	}


void Scene::HandleMainMenuUIEvents(UIElement* uiElement)
{
	if (waitingForFade_)      return;
	if (settingsCooldown_ > 0) return;

	Engine::GetInstance().audio->PlayFx(menuClickFxId, 0, true);

	switch (uiElement->id)
	{
	case BTN_PLAY:
		if (playSubState_ == PlaySubState::NONE && settingsAnimState_ == SettingsAnimState::NONE) {
			LOG("Main Menu: Play clicked (opening sub-menu)");
			playSubState_ = PlaySubState::FADE_OUT_BUTTONS;
			playSubAnimTimer_ = 0.0f;
			Engine::GetInstance().uiManager->ResetFocus();
			Engine::GetInstance().discord->UpdatePresence("In Play Sub-menu", "Release v1.0.0");
		}
		break;
	case BTN_PLAY_NEWGAME:
		if (playSubState_ == PlaySubState::OPTS_ACTIVE) {
			LOG("Play Sub-menu: New Game selected");
			isSlotSelectionForNewGame_ = true;
			playSubState_ = PlaySubState::FADE_OUT_OPTS_TO_SLOTS;
			playSubAnimTimer_ = 0.0f;
			Engine::GetInstance().uiManager->ResetFocus();
		}
		break;
	case BTN_PLAY_CONTINUE:
		if (playSubState_ == PlaySubState::OPTS_ACTIVE) {
			LOG("Play Sub-menu: Continue selected");
			isSlotSelectionForNewGame_ = false;
			playSubState_ = PlaySubState::FADE_OUT_OPTS_TO_SLOTS;
			playSubAnimTimer_ = 0.0f;
			Engine::GetInstance().uiManager->ResetFocus();
		}
		break;
	case BTN_PLAY_BACK:
		if (playSubState_ == PlaySubState::OPTS_ACTIVE) {
			LOG("Play Sub-menu: Back to Main Menu");
			playSubState_ = PlaySubState::FADE_OUT_OPTS;
			playSubAnimTimer_ = 0.0f;
			Engine::GetInstance().uiManager->ResetFocus();
		}
		break;
	case BTN_SLOT_1:
	case BTN_SLOT_2:
	case BTN_SLOT_3:
		if (playSubState_ == PlaySubState::SLOTS_ACTIVE) {
			int slotIdx = uiElement->id - BTN_SLOT_1;
			auto& saveSys = Engine::GetInstance().saveSystem;
			if (isSlotSelectionForNewGame_) {
				LOG("Slot Selection: Starting New Game in Slot %d", slotIdx + 1);
				saveSys->SetActiveSlot(slotIdx);

				// Delete any existing save file for this slot to start fresh
				std::string savePath = saveSys->GetSlotFilename(slotIdx);
				std::error_code ec;
				std::filesystem::remove(savePath, ec);

				waitingForFade_ = true;
				fadeTargetScene_ = SceneID::INTRO_CINEMATIC;
				Engine::GetInstance().render->StartFade(FadeDirection::FADE_OUT, 800.0f);
			} else {
				LOG("Slot Selection: Continuing Game from Slot %d", slotIdx + 1);
				if (saveSys->SlotHasValidSave(slotIdx)) {
					saveSys->SetActiveSlot(slotIdx);
					loadingFromSaveSlot_ = true;
					loadedFromSave_ = true;
					waitingForFade_ = true;
					fadeTargetScene_ = SceneID::LOADING;
					Engine::GetInstance().render->StartFade(FadeDirection::FADE_OUT, 800.0f);
				}
			}
		}
		break;
	case BTN_SLOTS_BACK:
		if (playSubState_ == PlaySubState::SLOTS_ACTIVE) {
			LOG("Slot Selection: Back to Play Sub-menu");
			playSubState_ = PlaySubState::FADE_OUT_SLOTS;
			playSubAnimTimer_ = 0.0f;
			Engine::GetInstance().uiManager->ResetFocus();
		}
		break;
	case BTN_SETTINGS:
		if (settingsAnimState_ == SettingsAnimState::NONE && playSubState_ == PlaySubState::NONE) {
			settingsAnimState_ = SettingsAnimState::FADE_OUT_BUTTONS;
			settingsAnimTimer_ = 0.0f;
			optionsSliderSel_ = 0;
			Engine::GetInstance().discord->UpdatePresence("In Options", "Release v1.0.0");
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
	// No-op for main menu -- settings are now drawn in-place without separate UI elements.
	// Pause menu settings use SetPauseOptionsPanelVisible() instead.
	// Kept as stub so callers don't need #ifdef guards.
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
	texStudioPlaceholder_ = Engine::GetInstance().textures->Load("assets/textures/UI/UI_HiddenLogo.png");
}

void Scene::UnloadIntro()
{
	if (texCitmLogo_) { Engine::GetInstance().textures->UnLoad(texCitmLogo_); texCitmLogo_ = nullptr; }
	if (texStudioPlaceholder_) { Engine::GetInstance().textures->UnLoad(texStudioPlaceholder_); texStudioPlaceholder_ = nullptr; }
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
	LOG("Loading intro cinematic...");
	tutorialTimer_ = 0.0f;
	cinematicVideoStarted_ = false;
	introLoadingDelayActive_ = false;
	introLoadingDelay_ = 0.0f;

	// Load running kid sprite for loading animation
	texLoadingKid_ = Engine::GetInstance().textures->Load("assets/textures/spritesheets/SS_Loading_Kid.png");

	// Start in Pre-Video Loading
	introCinState_ = IntroCinState::PRE_VIDEO_LOADING;
	Engine::GetInstance().audio->PlayMusic(nullptr);
}

void Scene::UnloadIntroCinematic()
{
	Engine::GetInstance().cinematics->StopVideo();
	if (texLoadingKid_) {
		Engine::GetInstance().textures->UnLoad(texLoadingKid_);
		texLoadingKid_ = nullptr;
	}
}

void Scene::DrawLoadingText(bool pulsing, float timer)
{
	auto& render = *Engine::GetInstance().render;
	int winW = 0, winH = 0;
	Engine::GetInstance().window->GetWindowSize(winW, winH);

	Uint8 alpha = 255;
	if (pulsing) {
		float pulse = 0.65f + 0.35f * sinf(timer * 0.004f);
		alpha = (Uint8)(255 * pulse);
	}

	SDL_Color textColor = { 255, 255, 255, alpha };

	const int textY = winH - 80;
	const int textH = 40;

	SDL_Texture* tx = render.CreateMenuTextTexture("loading...", textColor);
	if (tx) {
		float tw = 0, th = 0;
		SDL_GetTextureSize(tx, &tw, &th);
		float textScaledW = tw * 0.5f;
		float textScaledH = th * 0.5f;

		// Align text right edge at winW - 50
		const int targetRight = winW - 50;
		int textX = targetRight - (int)textScaledW;
		int textYPos = textY + (textH - (int)textScaledH) / 2;

		// Draw text
		render.DrawTexture(tx, textX, textYPos, nullptr, 0.0f, 0.0f, 0, INT_MAX, INT_MAX, SDL_FLIP_NONE, 0.5f);

		// Draw running kid sprite to the left of "Loading..."
		if (texLoadingKid_) {
			const int kidSize = textH;  // square, same height as the text row
			const int gap = 2; // closer gap as requested
			int frame = (int)(timer / (1000.0f / LOADING_KID_FPS)) % LOADING_KID_FRAMES;
			SDL_Rect src = { frame * LOADING_KID_FRAME_W, 0, LOADING_KID_FRAME_W, LOADING_KID_FRAME_H };
			int kidX = textX - kidSize - gap;
			int kidY = textY + (textH - kidSize) / 2;
			SDL_SetTextureAlphaMod(texLoadingKid_, alpha);
			render.DrawTexture(texLoadingKid_, kidX, kidY, &src, 0.0f, 0.0f, 0, INT_MAX, INT_MAX, SDL_FLIP_HORIZONTAL, (float)kidSize / (float)LOADING_KID_FRAME_W);
			SDL_SetTextureAlphaMod(texLoadingKid_, 255);
		}

		SDL_DestroyTexture(tx);
	}
}

void Scene::UpdateIntroCinematic(float dt)
{
	if (waitingForFade_) return;

	auto& render = *Engine::GetInstance().render;
	int winW = 0, winH = 0;
	Engine::GetInstance().window->GetWindowSize(winW, winH);

	// Ensure black background in all cinematic states to prevent flickering
	SDL_Rect fullBg = { 0, 0, winW, winH };
	render.DrawRectangle(fullBg, 0, 0, 0, 255, true, false);

	switch (introCinState_)
	{
	case IntroCinState::PRE_VIDEO_LOADING:
	{
		introLoadingDelay_ += dt;
		DrawLoadingText(true, introLoadingDelay_);

		if (introLoadingDelay_ > INTRO_PRE_VIDEO_DELAY) {
			LOG("SCENE: INTRO - PRE_VIDEO_LOADING done. Fading out to video...");
			introCinState_ = IntroCinState::FADING_OUT_TO_VIDEO;
			render.StartFade(FadeDirection::FADE_OUT, 800.0f);
		}
		break;
	}

	case IntroCinState::FADING_OUT_TO_VIDEO:
	{
		// Draw Fixed "Loading..." during fade
		DrawLoadingText(false, 0.0f);

		if (render.IsFadeComplete()) {
			LOG("SCENE: INTRO - Fading to video complete. Starting video with fade-in.");
			introCinState_ = IntroCinState::PLAYING_VIDEO;
			Engine::GetInstance().cinematics->PlayVideo("assets/video/animatica_inici.MOV");
			render.StartFade(FadeDirection::FADE_IN, 1200.0f);
		}
		break;
	}

	case IntroCinState::PLAYING_VIDEO:
	{
		auto& cinInput = *Engine::GetInstance().input;
		bool skipRequested = cinInput.GetKey(SDL_SCANCODE_SPACE) == KEY_DOWN ||
			cinInput.GetKey(SDL_SCANCODE_ESCAPE) == KEY_DOWN ||
			cinInput.GetGamepadButton(SDL_GAMEPAD_BUTTON_SOUTH) == KEY_DOWN ||
			cinInput.GetGamepadButton(SDL_GAMEPAD_BUTTON_START) == KEY_DOWN ||
			Engine::GetInstance().cinematics->HasSkipBeenRequested();

		if (skipRequested || !Engine::GetInstance().cinematics->IsPlaying()) {
			LOG("SCENE: INTRO - Video ended or skipped. FADING OUT from video...");
			introCinState_ = IntroCinState::FADING_OUT_FROM_VIDEO;
			render.StartFade(FadeDirection::FADE_OUT, 800.0f);
			// Do NOT stop video yet to allow it to fade out smoothly if still playing
		} else {
			SDL_Color skipColor = { 255, 255, 255, 120 };
			render.DrawMenuTextCentered("Press SPACE to Skip", { winW - 300, winH - 60, 280, 30 }, skipColor, 0.4f);
		}
		break;
	}

	case IntroCinState::FADING_OUT_FROM_VIDEO:
	{
		// Draw black overlay manually as well to ensure video disappears
		SDL_Rect bg = { 0, 0, winW, winH };
		render.DrawRectangle(bg, 0, 0, 0, render.GetFadeAlpha(), true, false);

		// Draw fixed loading text while fading to total black
		DrawLoadingText(false, 0.0f);

		if (render.IsFadeComplete()) {
			LOG("SCENE: INTRO - Fading from video complete. Entering post-video loading screen.");
			Engine::GetInstance().cinematics->StopVideo(); // Now stop it
			introCinState_ = IntroCinState::POST_VIDEO_LOADING;
			introLoadingDelay_ = 0.0f;
			render.StartFade(FadeDirection::FADE_IN, 500.0f); // Reveal the pulsing loading screen
		}
		break;
	}

	case IntroCinState::POST_VIDEO_LOADING:
	{
		introLoadingDelay_ += dt;
		DrawLoadingText(true, introLoadingDelay_);

		// Final transition to LOADING screen
		if (introLoadingDelay_ > INTRO_POST_VIDEO_DELAY) {
			LOG("SCENE: INTRO - Post-video loading complete. Transitioning to LOADING.");
			if (!waitingForFade_) {
				waitingForFade_ = true;
				targetLevelIndex_ = 0; // Level 1
				fadeTargetScene_ = SceneID::LOADING;
				render.StartFade(FadeDirection::FADE_OUT, 1000.0f);
			}
		}
		break;
	}
	}
}
// ============================================================================
//  TUTORIAL TEXT CARD
// ============================================================================

void Scene::LoadTutorialTextCard()
{
	LOG("Loading Tutorial Video Card for Level %d...", currentLevelIndex_ + 1);
	tutorialTimer_ = 0.0f;
	Engine::GetInstance().audio->PlayMusic(nullptr);

	// Construct dynamic video path for the level
	std::string videoPath = "assets/video/UI_Level" + std::to_string(currentLevelIndex_ + 1) + "_Intro.mp4";
	Engine::GetInstance().cinematics->PlayVideo(videoPath.c_str());

	// FADE IN from black
	Engine::GetInstance().render->StartFade(FadeDirection::FADE_IN, 1000.0f);
}

void Scene::UnloadTutorialTextCard()
{
	LOG("Unloading Tutorial Video Card");
	Engine::GetInstance().cinematics->StopVideo();
}

void Scene::UpdateTutorialTextCard(float dt)
{
	tutorialTimer_ += dt;

	auto& input = *Engine::GetInstance().input;
	auto& cin = *Engine::GetInstance().cinematics;

	// Skip requested if user presses Space, Escape, or Gamepad South/Start
	bool skipRequested = input.GetKey(SDL_SCANCODE_SPACE) == KEY_DOWN ||
	                     input.GetKey(SDL_SCANCODE_ESCAPE) == KEY_DOWN ||
	                     input.GetGamepadButton(SDL_GAMEPAD_BUTTON_SOUTH) == KEY_DOWN ||
	                     input.GetGamepadButton(SDL_GAMEPAD_BUTTON_START) == KEY_DOWN ||
	                     cin.HasSkipBeenRequested();

	// Transition if user skipped or the video has ended
	if ((skipRequested || !cin.IsPlaying()) && !waitingForFade_) {
		waitingForFade_ = true;
		fadeTargetScene_ = SceneID::GAMEPLAY;
		Engine::GetInstance().render->StartFade(FadeDirection::FADE_OUT, 1000.0f);
	}

	auto& render = *Engine::GetInstance().render;
	int winW = 0, winH = 0;
	Engine::GetInstance().window->GetWindowSize(winW, winH);

	// Draw black background manually under/around video
	SDL_Rect bg = { 0, 0, winW, winH };
	render.DrawRectangle(bg, 0, 0, 0, 255, true, false);

	// Render the "Press SPACE to Skip" overlay
	if (!waitingForFade_ && cin.IsPlaying()) {
		SDL_Color skipColor = { 255, 255, 255, 120 };
		render.DrawMenuTextCentered("Press SPACE to Skip", { winW - 300, winH - 60, 280, 30 }, skipColor, 0.4f);
	}
}

// ============================================================================
//  LOADING SCREEN
// ============================================================================

void Scene::LoadLoading()
{
	LOG("SCENE: Loading screen active for Level %d", targetLevelIndex_ + 1);
	loadingTimer_ = 0.0f;
	mapLoadingFinished_ = false;
	Engine::GetInstance().audio->PlayMusic(nullptr);
	texLoadingKid_ = Engine::GetInstance().textures->Load("assets/textures/spritesheets/SS_Loading_Kid.png");
}

void Scene::UnloadLoading()
{
	if (texLoadingKid_) {
		Engine::GetInstance().textures->UnLoad(texLoadingKid_);
		texLoadingKid_ = nullptr;
	}
}

void Scene::UpdateLoading(float dt)
{
	loadingTimer_ += dt;

	if (loadingTimer_ > 800.0f && !mapLoadingFinished_) {
		if (loadingFromSaveSlot_) {
			if (Engine::GetInstance().saveSystem->QuickLoadImmediate()) {
				inGameIntroActive_ = false;
				isAutoEntering_ = false;
			}
			loadingFromSaveSlot_ = false;
			mapLoadingFinished_ = true;
		}
		else {
			int index = targetLevelIndex_;
			if (index >= 0 && (size_t)index < levels_.size()) {
				bool savedHasBlanket = false, savedHasSlingshot = false, savedHasStuffedAnimal = false;
				Player::EquippedItem eqItem = Player::EquippedItem::NONE;
				int savedHealth = 0;
				if (player) {
					savedHasBlanket = player->HasBlanket();
					savedHasSlingshot = player->HasSlingshot();
					savedHasStuffedAnimal = player->HasStuffedAnimal();
					eqItem = player->GetEquippedItem();
					savedHealth = player->health;
				}

			if (puzzleManager2) {
				puzzleManager2->Reset();
			}

			currentMapFile_ = levels_[index].file;
			currentLevelIndex_ = index;
				player.reset();
				Engine::GetInstance().entityManager->CleanUp();
				Engine::GetInstance().physics->FlushPendingDeletes();
				Engine::GetInstance().map->CleanUp();
				Engine::GetInstance().physics->FlushPendingDeletes();

				currentMapFile_ = levels_[index].file;
				currentLevelIndex_ = index;

				Engine::GetInstance().map->Load("assets/maps/", currentMapFile_);
				Engine::GetInstance().map->LoadEntities(player);

				if (player == nullptr) {
					player = std::dynamic_pointer_cast<Player>(
						Engine::GetInstance().entityManager->CreateEntity(EntityType::PLAYER));

					// LEVEL 1: Start at bed
					// LEVEL 2+: Start off-screen right
					if (currentLevelIndex_ == 0)
						player->position = Vector2D(96.0f, 672.0f);
					else
						player->position = Vector2D(1280.0f + 100.0f, 672.0f); 

				player->Start();
			}
			healthSlotCount_ = currentLevelIndex_ + 3;
			if (healthSlotCount_ > MAX_HEALTH_SLOTS) healthSlotCount_ = MAX_HEALTH_SLOTS;
			if (player) {
				player->SetHasBlanket(savedHasBlanket);
				player->SetHasSlingshot(savedHasSlingshot);
				player->SetHasStuffedAnimal(savedHasStuffedAnimal);
				player->SetEquippedItem(eqItem);
				player->maxHealth = healthSlotCount_;
				// For level transitions (not first load), preserve health up to the new max
				if (currentLevelIndex_ > 0 && savedHealth > 0) {
					player->health = std::min(savedHealth, healthSlotCount_);
				} else {
					player->health = healthSlotCount_;
				}
			}
			if (savedHasBlanket) capaCollected_ = true;
			if (savedHasSlingshot) slingshotCollected_ = true;
			if (savedHasStuffedAnimal) stuffedAnimalCollected_ = true;
			currentHealthUI_ = player ? player->health : healthSlotCount_;
			activeHealthAnim_ = 0;
			isGameOver_ = false;
			inGameIntroActive_ = false;

				if (hasPendingLevelSpawn_ && player)
				{
					float spawnX = 0.0f, spawnY = 0.0f;
					if (Engine::GetInstance().map->GetSpawnById(pendingLevelSpawnId_, spawnX, spawnY))
					{
						player->SetPosition(Vector2D(spawnX - player->texW / 2.0f, spawnY - player->texH / 2.0f));
						player->position = Vector2D(spawnX, spawnY);
						LOG("PORTAL: Level spawn applied at '%s' (%.1f, %.1f)", pendingLevelSpawnId_.c_str(), spawnX, spawnY);
					}
					hasPendingLevelSpawn_ = false;
					pendingLevelSpawnId_ = "";
				}

				
				isLvl3Map_ = (currentMapFile_ == "Map3.tmx");
				isLvl3Puzzle_ = false;
				isPuzzleMap3Lever_ = isLvl3Map_;
				isPuzzleMap3Buttons_ = false;

				if (isLvl3Map_) {
					if (!puzzleManager3_) puzzleManager3_ = new PuzzleManager3();
					puzzleManager3_->Init(Engine::GetInstance().render->renderer);

					LeverData3 lever;
					SDL_FRect blockedPortal = { 0, 0, 48, 96 };

					for (auto& obj : Engine::GetInstance().map->GetPuzzleObjects()) {
						if (obj.name == "Lever") lever.worldRect = obj.rect;
					}
					for (auto& portal : Engine::GetInstance().map->GetPortals()) {
						if (portal.spawnId == "PuzzleMain") {
							blockedPortal = { portal.x, portal.y, portal.w, portal.h };
							break;
						}
					}
					
					SDL_FRect portalA = { 0,0,0,0 }, portalB = { 0,0,0,0 };
					for (auto& portal : Engine::GetInstance().map->GetPortals()) {
						if (portal.spawnId == "A") portalA = { portal.x, portal.y, portal.w, portal.h };
						if (portal.spawnId == "B") portalB = { portal.x, portal.y, portal.w, portal.h };
					}
					puzzleManager3_->LoadExtraPortals(portalA, portalB);
					puzzleManager3_->LoadLever(lever, blockedPortal);
					LOG("PUZZLE3: Init en LoadMap. Palanca (%.0f,%.0f) Portal (%.0f,%.0f)",
						lever.worldRect.x, lever.worldRect.y, blockedPortal.x, blockedPortal.y);
				}
				else {
					if (puzzleManager3_) { delete puzzleManager3_; puzzleManager3_ = nullptr; }
				}

				mapLoadingFinished_ = true;
				LOG("SCENE: Map %s loaded", currentMapFile_.c_str());
			} else {
				mapLoadingFinished_ = true;
			}
		}
	}

	if (mapLoadingFinished_ && loadingTimer_ > 2000.0f) {
		if (loadedFromSave_) {
			ChangeScene(SceneID::GAMEPLAY);
		} else {
			ChangeScene(SceneID::TUTORIAL_TEXT_CARD);
		}
	}

	DrawLoading();
}
void Scene::DrawLoading()
{
	auto& render = *Engine::GetInstance().render;
	int winW, winH;
	Engine::GetInstance().window->GetWindowSize(winW, winH);

	SDL_Rect bg = { 0, 0, winW, winH };
	render.DrawRectangle(bg, 0, 0, 0, 255, true, false);

	DrawLoadingText(true, loadingTimer_);
}

// ============================================================================
//  GAMEPLAY
// ============================================================================

void Scene::LoadGameplay()
{
	isPaused_ = false;
	showPauseOptions_ = false;
	showMapViewer_ = false;
	showInventory_ = false;
	isBossFightActive_ = false;
	activeBoss_.reset();
	endGameTriggered_ = false;
	endGameVideoActive_ = false;

	if (currentLevelIndex_ < 0 || (size_t)currentLevelIndex_ >= levels_.size()) currentLevelIndex_ = 0;
	std::string presenceStr = "Playing: " + levels_[currentLevelIndex_].name;
	Engine::GetInstance().discord->UpdatePresence(presenceStr.c_str(), "Release v1.0.0");

	if (player) {
		if (currentLevelIndex_ == 0) {
			// LEVEL 1: Trigger Wake Up animation and cinematic effects
			player->isWakingUp = true;
			player->wakeUpAnimStarted = false;
			inGameIntroActive_ = true;
			inGameIntroTimer_ = 0.0f;
			isAutoEntering_ = false;
		}
		else {
			// LEVEL 2+: Skip wake up, trigger automatic entry from right
			player->isWakingUp = false;
			player->wakeUpAnimStarted = true;
			inGameIntroActive_ = false;
			
			isAutoEntering_ = true;
			autoEntryProgress_ = 0.0f;
			autoEntryStartX_ = player->position.getX();
			autoEntryTargetX_ = 960.0f; 
		}
	}

	auto* render = Engine::GetInstance().render.get();
	
	if (inGameIntroActive_) {
		render->cameraZoom = 0.45f;
		render->blurIntensity = 2.5f;
		render->SetCameraSway(false);
		render->SetCameraClamping(false);
		render->SetCameraMovement(false);
	}
	else {
		render->cameraZoom = 1.0f;
		render->blurIntensity = 0.0f;
		render->SetCameraSway(true);
		render->SetCameraClamping(true);
		render->SetCameraMovement(true);
	}

	// Snap camera to hero position using the fixed logic centering (640, 360)
	render->SetCameraPosition(player->position.getX(), player->position.getY());
	
	// Pre-calculate logical screen position for zoom pivot
	float pX = player->position.getX() + 64.0f;
	float pY = player->position.getY() + 64.0f;
	render->zoomCenterX = (float)render->camera.x + pX;
	render->zoomCenterY = (float)render->camera.y + pY;

	LOG("SCENE: LoadGameplay - Player World: (%.2f, %.2f). Camera Start: (%d, %d). Zoom: %.2f. Pivot: (%.2f, %.2f)", 
		player->position.getX(), player->position.getY(), render->camera.x, render->camera.y, render->cameraZoom, render->zoomCenterX, render->zoomCenterY);

	// Silksong: actorSnapshotPaused → music starts silent, fades in during entry
	Engine::GetInstance().audio->PlayMusic("assets/audio/music/Echoes_of_Slumber_In_Game.wav", 1.0f);
	Engine::GetInstance().audio->SetMusicVolume(0.0f);
	Engine::GetInstance().audio->SetSFXVolume(sfxVolume_); // Use user configured SFX volume

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

		// Resolve relative path dynamically from the TSX file location
		std::string finalPath = tsxPath;
		size_t lastSlashTsx = finalPath.find_last_of("/\\");
		if (lastSlashTsx != std::string::npos) {
			finalPath = finalPath.substr(0, lastSlashTsx + 1) + imgPath;
		}
		else {
			finalPath = imgPath;
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
			anim.AddFrame(r, 80); // 80ms per frame for a smooth loop
		}
		anim.SetLoop(false);
		anim.Reset();
	};

	// Load health bar animations based on current level (Fase)
	// Level 1 = Fase1 (3 HP), Level 2 = Fase2 (4 HP), Level 3 = Fase3 (5 HP), Level 4 = Fase4 (6 HP)
	{
		int fase = currentLevelIndex_ + 1;
		healthSlotCount_ = currentLevelIndex_ + 3; // 3, 4, 5, or 6
		if (healthSlotCount_ > MAX_HEALTH_SLOTS) healthSlotCount_ = MAX_HEALTH_SLOTS;

		// Clear all slots first
		for (int i = 0; i < MAX_HEALTH_SLOTS; i++) {
			texHealth_[i] = nullptr;
			animHealth_[i] = Animation();
		}

		if (fase == 1) {
			// Fase1 uses the original TSX naming: SS_Healthbar-1.tsx, SS_Healthbar-2.tsx, SS_Healthbar-3.tsx
			for (int i = 0; i < healthSlotCount_; i++) {
				std::string tsxPath = "assets/textures/animations/HealthAnimations/SS_Healthbar-" + std::to_string(i + 1) + ".tsx";
				setupAnimFromTSX(tsxPath.c_str(), animHealth_[i], texHealth_[i]);
			}
		}
		else {
			// Fase2/3/4 use naming: SS_Healthbar_FaseN-M.tsx
			for (int i = 0; i < healthSlotCount_; i++) {
				std::string tsxPath = "assets/textures/animations/HealthAnimations/SS_Healthbar_Fase" + std::to_string(fase) + "-" + std::to_string(i + 1) + ".tsx";
				setupAnimFromTSX(tsxPath.c_str(), animHealth_[i], texHealth_[i]);
			}
		}

		// Set player max health and health for this level
		if (player) {
			player->maxHealth = healthSlotCount_;
			player->health = healthSlotCount_;
		}
		LOG("Health HUD loaded: Fase %d, %d slots", fase, healthSlotCount_);
	}

	currentHealthUI_ = healthSlotCount_;
	activeHealthAnim_ = 0;
	isGameOver_ = false;
	texGameOver_ = nullptr; // No longer using code-generated text
	texCheckpointSaved_ = Engine::GetInstance().render->CreateMenuTextTexture("GAME SAVED", { 255, 255, 255, 255 });
	texDamageVignette_ = Engine::GetInstance().textures->Load("assets/textures/UI/UI_Damage_Vignette.png");
	checkpointSaveTimer_ = 0.0f;

	// Game Over Button
	int winW = 0, winH = 0;
	Engine::GetInstance().window->GetWindowSize(winW, winH);

	// Load new Game Over screen assets with built-in text and soft background blur
	texGameOverScreenBase_ = Engine::GetInstance().textures->Load("assets/textures/UI/UI_GameOver_Screen_Base_Blur.png");
	texGameOverText_ = nullptr; // Not needed since the text is perfectly integrated in the base!
	texGameOverBg_ = nullptr;
	texGameOverBtn_ = Engine::GetInstance().textures->Load("assets/textures/Menu/UI_Pause_Menu_button_white.png");
	texGameOverKid_ = nullptr;
	texGameOverFrag1_ = nullptr;
	texGameOverFrag3_ = nullptr;
	texGameOverFrag4_ = nullptr;
	texGameOverFrag5_ = nullptr;

	// Shift Y coordinates down (+35 and +135) to align beautifully under the built-in "GAME OVER" text
	SDL_Rect contBtnPos = { winW / 2 - 230, winH / 2 + 35, 460, 84 };
	SDL_Rect mainBtnPos = { winW / 2 - 230, winH / 2 + 135, 460, 84 };

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

	// Cape collectible (Colectible manta.png - high quality folded blue blanket)
	texCapaCollectible_ = Engine::GetInstance().textures->Load("assets/textures/AS_props/Colectible manta.png");
	capaCollected_ = false;
	capaFloatTimer_ = 0.0f;

	// Read cape position from TMX Entities layer. If not found, mark as collected to avoid spawning hardcoded one
	if (!Engine::GetInstance().map->GetCapePosition(capaX_, capaY_)) {
		LOG("WARNING: No Cape entity found in TMX Entities layer, will not spawn");
		capaCollected_ = true;
	}

	capaBody_ = nullptr;

	// Slingshot (Tirachinas) collectible
	texSlingshotCollectible_ = Engine::GetInstance().textures->Load("assets/textures/AS_props/Colectible tirachinas.png");

	// Inventory textures
	texInventoryBg_ = Engine::GetInstance().textures->Load("assets/textures/UI/UI_Inventory_Menu_Base.png");
	texAbilitiesLocked_ = Engine::GetInstance().textures->Load("assets/textures/UI/UI_Abilities_Locked.png");
	texAbilitiesBlanket_ = Engine::GetInstance().textures->Load("assets/textures/UI/UI_Abilities_Blanket_Unlocked.png");
	texAbilitiesBlanketSling_ = Engine::GetInstance().textures->Load("assets/textures/UI/UI_Abilities_Blanket_Slingshot_Unlocked.png");
	texAbilitiesAll_ = Engine::GetInstance().textures->Load("assets/textures/UI/UI_Abilities_ALL_Unlocked.png");

	texAbilityBlanketIcon_ = Engine::GetInstance().textures->Load("assets/textures/UI/UI_Ability_Blanket.png");
	texAbilitySlingshotIcon_ = Engine::GetInstance().textures->Load("assets/textures/UI/UI_Ability_Slingshot.png");
	texAbilityStuffedAnimalIcon_ = Engine::GetInstance().textures->Load("assets/textures/UI/UI_Ability_Stuffed_Animal.png");
	texAbilityAnchorIcon_ = Engine::GetInstance().textures->Load("assets/textures/UI/UI_Ability_Anchor.png");

	// Memories UI textures
	texMemoria1Base_ = Engine::GetInstance().textures->Load("assets/textures/UI/UI_Memoria_1_Base.png");
	texMemoria1N1_ = Engine::GetInstance().textures->Load("assets/textures/UI/UI_Memoria_1_N1.png");
	texMemoria1N2_ = Engine::GetInstance().textures->Load("assets/textures/UI/UI_Memoria_1_N2.png");

	texMemoria2Base_ = Engine::GetInstance().textures->Load("assets/textures/UI/UI_Memoria_2_Base.png");
	texMemoria2N1_ = Engine::GetInstance().textures->Load("assets/textures/UI/UI_Memoria_2_N1.png");
	texMemoria2N2_ = Engine::GetInstance().textures->Load("assets/textures/UI/UI_Memoria_2_N2.png");

	texMemoria3Base_ = Engine::GetInstance().textures->Load("assets/textures/UI/UI_Memoria_3_Base.png");
	texMemoria3N1_ = Engine::GetInstance().textures->Load("assets/textures/UI/UI_Memoria_3_N1.png");
	texMemoria3N2_ = Engine::GetInstance().textures->Load("assets/textures/UI/UI_Memoria_3_N2.png");
	texMemoria3N3_ = Engine::GetInstance().textures->Load("assets/textures/UI/UI_Memoria_3_N3.png");

	// Fullscreen memories
	texMemoriaFrameFullScreen_ = Engine::GetInstance().textures->Load("assets/textures/UI/UI_Frame_Memoria_FullScreen.png");
	texMemoria1Full1_ = Engine::GetInstance().textures->Load("assets/textures/Memorias_Full_Screen/Memoria1_1.png");
	texMemoria1Full2_ = Engine::GetInstance().textures->Load("assets/textures/Memorias_Full_Screen/Memoria1_2.png");

	texMemoria2Full1_ = Engine::GetInstance().textures->Load("assets/textures/Memorias_Full_Screen/memoria_2.1.jpg");
	texMemoria2Full2_ = Engine::GetInstance().textures->Load("assets/textures/Memorias_Full_Screen/memoria_2.2.png");

	texMemoria3Full1_ = Engine::GetInstance().textures->Load("assets/textures/Memorias_Full_Screen/memoria_3.1.jpg");
	texMemoria3Full2_ = Engine::GetInstance().textures->Load("assets/textures/Memorias_Full_Screen/memoria_3.2.jpg");
	texMemoria3Full3_ = Engine::GetInstance().textures->Load("assets/textures/Memorias_Full_Screen/memoria_3.3.jpg");

	// Reset hover and fullscreen states
	for (int i = 0; i < 3; i++) {
		memoryHoverTimers_[i] = 0.0f;
	}
	showMemoryViewer_ = false;
	activeMemoryIndex_ = -1;
	activeMemoryPage_ = 0;
	slingshotCollected_ = false;
	slingshotFloatTimer_ = 0.0f;

	// Read slingshot position from TMX. If not found, mark as collected to avoid spawning hardcoded one
	if (!Engine::GetInstance().map->GetSlingshotPosition(slingshotX_, slingshotY_)) {
		LOG("WARNING: No Tirachinas entity found in TMX, will not spawn");
		slingshotCollected_ = true;
	}

	// Stuffed Animal (Oso) collectible
	texStuffedAnimalCollectible_ = Engine::GetInstance().textures->Load("assets/textures/AS_props/Colectible oso.png");
	stuffedAnimalCollected_ = false;
	stuffedAnimalFloatTimer_ = 0.0f;

	// Read stuffed animal position from TMX. If not found, mark as collected to avoid spawning hardcoded one
	if (!Engine::GetInstance().map->GetStuffedAnimalPosition(stuffedAnimalX_, stuffedAnimalY_)) {
		LOG("WARNING: No Oso/Peluche/StuffedAnimal entity found in TMX, will not spawn");
		stuffedAnimalCollected_ = true;
	}

	// Load Minimap Ornate Frame Texture
	texMinimapFrame_ = Engine::GetInstance().textures->Load("assets/textures/UI/UI_Minimap.png");
	// Pre-simulate physics to settle all dynamic bodies
	Engine::GetInstance().physics->PreSimulateScene(3.0f);

	puzzleManager2 = std::make_shared<PuzzleManager2>();
	puzzleManager2->Init(Engine::GetInstance().render->renderer);
}

void Scene::UpdateGameplay(float dt)
{
	if (UpdateCheckpointTransition(dt))
		return;

	if (isGameOver_) {
		gameOverFadeTimer_ += dt * 0.001f; // Convert ms to seconds
		if (gameOverFadeTimer_ > 1.5f) {
			gameOverFadeTimer_ = 1.5f;
		}

		// Enable and show buttons only after the fade-in is mostly complete (after 1.0 second)
		auto& list = Engine::GetInstance().uiManager->UIElementsList;
		for (auto& el : list) {
			if (el->id == BTN_GAMEOVER_MAINMENU || el->id == BTN_GAMEOVER_CONTINUE) {
				if (!el->isVisible && gameOverFadeTimer_ >= 1.0f) {
					el->isVisible = true;
					el->state = UIElementState::NORMAL;
				}
				if (el->id == BTN_GAMEOVER_CONTINUE && !Engine::GetInstance().saveSystem->HasValidSave()) {
					el->state = UIElementState::DISABLED;
				}
			}
		}
	}

	if (screenDamageTimer_ > 0.0f) {
		screenDamageTimer_ -= dt;
	}

	// ── Automatic Entry Movement (Levels 2, 3, 4) ───────────────────────────
	if (isAutoEntering_ && player) {
		autoEntryProgress_ += dt * 0.001f; // ~1 second entry
		if (autoEntryProgress_ >= 1.0f) {
			autoEntryProgress_ = 1.0f;
			isAutoEntering_ = false;
		}
		
		float currentX = autoEntryStartX_ + (autoEntryTargetX_ - autoEntryStartX_) * autoEntryProgress_;
		player->position.setX(currentX);
		
		if (!isAutoEntering_) LOG("SCENE: Auto Entry Finished.");
		return; // Lock input during auto-entry
	}

	// =========================================================================
	// SILKSONG-STYLE SCENE ENTRY (from BeginSceneTransitionRoutine)
	//
	// Team Cherry does NOT use camera zoom or post-process blur during
	// scene transitions. The actual flow is:
	//   1. Screen fades in from black (handled by Scene::Update FADE_IN)
	//   2. Brief entry delay while hero is frozen (entryDelay)
	//   3. Hero wake-up animation plays, hero regains control
	//   4. Camera sway re-enabled, music at full volume
	// =========================================================================
	if (inGameIntroActive_) {

		inGameIntroTimer_ += dt;
		float progress = inGameIntroTimer_ / IN_GAME_INTRO_DURATION;

		if (progress >= 1.0f) {
			inGameIntroActive_ = false;
			auto& render = *Engine::GetInstance().render;
			render.cameraZoom = 1.0f;
			render.blurIntensity = 0.0f;
			render.SetCameraSway(true);
			render.SetCameraMovement(true); // CRITICAL: Unlock follow
			render.SetCameraClamping(true);
			Engine::GetInstance().audio->SetMusicVolume(musicVolume_);
			if (player) player->wakeUpAnimStarted = true;
			LOG("SCENE: Intro Cinematic Finished. CAMERA UNLOCKED.");
		}
		else {
			float smoothT = progress * progress * (3.0f - 2.0f * progress);

			Engine::GetInstance().render->cameraZoom = 0.45f + (1.0f - 0.45f) * smoothT;
			Engine::GetInstance().render->blurIntensity = 2.5f * (1.0f - smoothT);
			Engine::GetInstance().audio->SetMusicVolume(musicVolume_ * smoothT);

			if (player) {
				float pX = player->GetPosition().getX() + 64.0f;
				float pY = player->GetPosition().getY() + 64.0f;
				// Pivot zoom exactly on the player's current logical screen position
				Engine::GetInstance().render->zoomCenterX = (float)Engine::GetInstance().render->camera.x + pX;
				Engine::GetInstance().render->zoomCenterY = (float)Engine::GetInstance().render->camera.y + pY;

				LOG("SCENE: Intro Update - Progress: %.2f. Zoom: %.2f. Camera: (%d, %d). Player World: (%.2f, %.2f). Pivot: (%.2f, %.2f)", 
					progress, Engine::GetInstance().render->cameraZoom, Engine::GetInstance().render->camera.x, Engine::GetInstance().render->camera.y, 
					pX - 64.0f, pY - 64.0f, Engine::GetInstance().render->zoomCenterX, Engine::GetInstance().render->zoomCenterY);

				float pulse = 1.0f + 0.06f * sinf(inGameIntroTimer_ * 0.005f);
				float radiusScale = 0.85f * pulse;
				Uint8 alpha = (Uint8)(230.0f * (1.0f - smoothT));
				if (alpha > 0) {
					Engine::GetInstance().render->DrawWhiteGlow((int)pX, (int)pY, radiusScale, alpha);
				}
			}
		}
		return;
	}

	// Track player position to uncover map cells (320x180 px cell size)
	if (player)
	{
		Vector2D playerPos = player->GetPosition();
		int cellX = (int)(playerPos.getX() / 320.0f);
		int cellY = (int)(playerPos.getY() / 180.0f);
		visitedCells_[currentMapFile_].insert({ cellX, cellY });
	}

	// Toggle pause with ESC
	auto& gpInput = *Engine::GetInstance().input;
	bool pauseToggle = gpInput.GetKey(SDL_SCANCODE_ESCAPE) == KEY_DOWN ||
		gpInput.GetGamepadButton(SDL_GAMEPAD_BUTTON_START) == KEY_DOWN;
	bool backBtn = gpInput.GetGamepadButton(SDL_GAMEPAD_BUTTON_EAST) == KEY_DOWN;

	// Toggle Inventory with 'I' or Gamepad D-pad UP
	bool inventoryToggle = gpInput.GetKey(SDL_SCANCODE_I) == KEY_DOWN ||
		gpInput.GetGamepadButton(SDL_GAMEPAD_BUTTON_DPAD_UP) == KEY_DOWN;

	if (!isGameOver_ && inventoryToggle)
	{
		if (showInventory_) {
			showInventory_ = false;
			isPaused_ = false;
			SetPauseMenuVisible(false);
			Engine::GetInstance().audio->PlayFx(menuClickFxId, 0, true);
		}
		else if (!isPaused_ && !showMapViewer_) {
			showInventory_ = true;
			isPaused_ = true;
			SetPauseMenuVisible(false);

			// Initialize gamepad selection to currently equipped item
			Player::EquippedItem eq = player ? player->GetEquippedItem() : Player::EquippedItem::NONE;
			bool blanket = player && player->HasBlanket();
			bool slingshot = player && player->HasSlingshot();
			bool stuffed = player && player->HasStuffedAnimal();

			if (eq == Player::EquippedItem::BLANKET && blanket) inventorySel_ = 0;
			else if (eq == Player::EquippedItem::SLINGSHOT && slingshot) inventorySel_ = 1;
			else if (eq == Player::EquippedItem::STUFFED_ANIMAL && stuffed) inventorySel_ = 2;
			else if (blanket) inventorySel_ = 0;
			else if (slingshot) inventorySel_ = 1;
			else if (stuffed) inventorySel_ = 2;
			else inventorySel_ = -1;

			Engine::GetInstance().audio->PlayFx(menuClickFxId, 0, true);
		}
	}

	if (!isGameOver_ && (pauseToggle || (backBtn && (showMapViewer_ || showInventory_ || showPauseOptions_ || isPaused_))))
	{
		if (showMapViewer_) {
			showMapViewer_ = false;
			// If no pause menu buttons are visible, map was opened via touchpad -- unpause
			isPaused_ = false;
			SetPauseMenuVisible(false);
		}
		else if (showInventory_) {
			showInventory_ = false;
			isPaused_ = false;
			SetPauseMenuVisible(false);
			Engine::GetInstance().audio->PlayFx(menuClickFxId, 0, true);
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

	// -- Touchpad opens/closes map directly during gameplay ---------------
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

	if (showInventory_)
	{
		int winW = 0, winH = 0;
		Engine::GetInstance().window->GetWindowSize(winW, winH);

		if (showMemoryViewer_)
		{
			// Handle Fullscreen Memory Viewer Input
			auto& input = *Engine::GetInstance().input;
			int pageCount = (activeMemoryIndex_ == 2) ? 3 : 2;

			if (input.GetMouseButtonDown(SDL_BUTTON_LEFT) == KEY_DOWN)
			{
				Vector2D mousePos = input.GetMousePosition();
				float mx = mousePos.getX();
				float my = mousePos.getY();

				// Calculate scaled frame boundaries
				float scaleX = (float)winW / 1920.0f;
				float scaleY = (float)winH / 1080.0f;
				float fScale = std::min(scaleX, scaleY) * 0.95f;
				float frameW = 1920.0f * fScale;
				float frameH = 1080.0f * fScale;
				float frameX = (winW - frameW) / 2.0f;
				float frameY = (winH - frameH) / 2.0f;

				// Check if clicked outside the frame to close
				if (mx < frameX || mx > frameX + frameW || my < frameY || my > frameY + frameH)
				{
					showMemoryViewer_ = false;
					activeMemoryIndex_ = -1;
					Engine::GetInstance().audio->PlayFx(menuClickFxId, 0, true);
				}
				else
				{
					// Clicked inside the frame
					// Left 30% goes to previous page, right 30% goes to next page, middle closes
					if (mx < frameX + frameW * 0.3f)
					{
						activeMemoryPage_ = (activeMemoryPage_ - 1 + pageCount) % pageCount;
						Engine::GetInstance().audio->PlayFx(menuClickFxId, 0, true);
					}
					else if (mx > frameX + frameW * 0.7f)
					{
						activeMemoryPage_ = (activeMemoryPage_ + 1) % pageCount;
						Engine::GetInstance().audio->PlayFx(menuClickFxId, 0, true);
					}
					else
					{
						showMemoryViewer_ = false;
						activeMemoryIndex_ = -1;
						Engine::GetInstance().audio->PlayFx(menuClickFxId, 0, true);
					}
				}
			}

			// Key shortcuts to close
			if (input.GetKey(SDL_SCANCODE_ESCAPE) == KEY_DOWN ||
				input.GetKey(SDL_SCANCODE_I) == KEY_DOWN ||
				input.GetGamepadButton(SDL_GAMEPAD_BUTTON_EAST) == KEY_DOWN ||
				input.GetGamepadButton(SDL_GAMEPAD_BUTTON_SOUTH) == KEY_DOWN)
			{
				showMemoryViewer_ = false;
				activeMemoryIndex_ = -1;
				Engine::GetInstance().audio->PlayFx(menuClickFxId, 0, true);
			}

			// Arrow keys or D-pad to change pages
			if (input.GetKey(SDL_SCANCODE_LEFT) == KEY_DOWN ||
				input.GetGamepadButton(SDL_GAMEPAD_BUTTON_DPAD_LEFT) == KEY_DOWN)
			{
				activeMemoryPage_ = (activeMemoryPage_ - 1 + pageCount) % pageCount;
				Engine::GetInstance().audio->PlayFx(menuClickFxId, 0, true);
			}
			if (input.GetKey(SDL_SCANCODE_RIGHT) == KEY_DOWN ||
				input.GetGamepadButton(SDL_GAMEPAD_BUTTON_DPAD_RIGHT) == KEY_DOWN)
			{
				activeMemoryPage_ = (activeMemoryPage_ + 1) % pageCount;
				Engine::GetInstance().audio->PlayFx(menuClickFxId, 0, true);
			}
		}

		DrawInventory(winW, winH);
		return;
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

		// -- Gamepad: R2 zoom in, L2 zoom out -----------------------------
		float rt = input.GetRightTrigger();
		float lt = input.GetLeftTrigger();
		if (rt > 0.0f)
			mapViewZoom_ = std::min(mapViewZoom_ + rt * 0.002f * dt, 2.0f);
		if (lt > 0.0f)
			mapViewZoom_ = std::max(mapViewZoom_ - lt * 0.002f * dt, minZoom);

		// Ensure zoom stays within limits
		if (mapViewZoom_ < minZoom) mapViewZoom_ = minZoom;

		// -- Gamepad: Left stick to pan the map ---------------------------
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

	if (!isPaused_ && !isGameOver_ && !inGameIntroActive_ && !isAutoEntering_)
		CheckPortalCollisions(dt);

	if (isPaused_) {
		// Pause menu is drawn in PostUpdateGameplay (after all HUD layers)
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
			if (!RequestCheckpointRespawn()) {
				isGameOver_ = true;
			}
		}

		// HUD Logic

		if (player->health != currentHealthUI_) {
			currentHealthUI_ = player->health;
			// Reset the animation for the slot that just became active
			int diff = player->maxHealth - currentHealthUI_;
			if (diff > 0 && diff <= healthSlotCount_) {
				animHealth_[diff - 1].Reset();
			}
			else if (diff == 0) {
				animHealth_[0].Reset();
			}
			activeHealthAnim_ = 0;
		}

		{
			int diff = player->maxHealth - currentHealthUI_;
			if (diff == 0) {
				// Full health: keep static (first frame)
				animHealth_[0].Reset();
			}
			else if (diff > 0 && diff <= healthSlotCount_) {
				animHealth_[diff - 1].Update(dt);
			}
		}

		if (player->health <= 0 && activeHealthAnim_ == 0 && !isGameOver_) {
			if (!RequestCheckpointRespawn())
			{
				isGameOver_ = true;
				gameOverFadeTimer_ = 0.0f; // Initialize fade timer

				auto& list = Engine::GetInstance().uiManager->UIElementsList;
				for (auto& el : list) {
					if (el->id == BTN_GAMEOVER_MAINMENU || el->id == BTN_GAMEOVER_CONTINUE) {
						el->isVisible = false; // Hide initially so they pop/fade in smoothly later
						el->state = UIElementState::NORMAL;
					}

					if (el->id == BTN_GAMEOVER_CONTINUE) {
						if (!Engine::GetInstance().saveSystem->HasValidSave())
							el->state = UIElementState::DISABLED;
					}
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

		// Slingshot collectible pickup (proximity check)
		if (!slingshotCollected_ && player)
		{
			slingshotFloatTimer_ += dt;

			float sdx = player->position.getX() - slingshotX_;
			float sdy = player->position.getY() - (slingshotY_ - 50.0f);
			float sdistSq = sdx * sdx + sdy * sdy;
			float spickupRadius = 60.0f;

			if (sdistSq < spickupRadius * spickupRadius)
			{
				slingshotCollected_ = true;
				player->SetHasSlingshot(true);
				Engine::GetInstance().audio->PlayFx(player->pickCoinFxId);
				slingshotNotifTimer_ = SLINGSHOT_NOTIF_DURATION;
				LOG("Slingshot collected! Ranged attack unlocked.");
			}
		}

		// Stuffed Animal collectible pickup (proximity check)
		if (!stuffedAnimalCollected_ && player)
		{
			stuffedAnimalFloatTimer_ += dt;

			float sdx = player->position.getX() - stuffedAnimalX_;
			float sdy = player->position.getY() - (stuffedAnimalY_ - 50.0f);
			float sdistSq = sdx * sdx + sdy * sdy;
			float spickupRadius = 60.0f;

			if (sdistSq < spickupRadius * spickupRadius)
			{
				stuffedAnimalCollected_ = true;
				player->SetHasStuffedAnimal(true);
				Engine::GetInstance().audio->PlayFx(player->pickCoinFxId);
				stuffedAnimalNotifTimer_ = STUFFED_ANIMAL_NOTIF_DURATION;
				LOG("Stuffed Animal collected! Bear transformation unlocked.");
			}
		}
	}

	// Draw slingshot collectible in-world
	if (!slingshotCollected_ && texSlingshotCollectible_)
	{
		int slTexW = 0, slTexH = 0;
		Engine::GetInstance().textures->GetSize(texSlingshotCollectible_, slTexW, slTexH);
		float slFloatOffset = 6.0f * sinf(slingshotFloatTimer_ * 0.003f);
		float slScale = 0.05f;
		int slDrawX = (int)(slingshotX_ - (float)slTexW * slScale / 2.0f);
		int slDrawY = (int)(slingshotY_ - (float)slTexH * slScale / 2.0f + slFloatOffset);

		SDL_Rect slSection = { 0, 0, slTexW, slTexH };
		Engine::GetInstance().render->DrawTexture(texSlingshotCollectible_, slDrawX, slDrawY, &slSection, 1.0f, -1.0f, 0, INT_MAX, INT_MAX, SDL_FLIP_NONE, slScale);
	}

	// Draw stuffed animal collectible in-world
	if (!stuffedAnimalCollected_ && texStuffedAnimalCollectible_)
	{
		int slTexW = 0, slTexH = 0;
		Engine::GetInstance().textures->GetSize(texStuffedAnimalCollectible_, slTexW, slTexH);
		float slFloatOffset = 6.0f * sinf(stuffedAnimalFloatTimer_ * 0.003f);
		float slScale = 0.05f; 
		int slDrawX = (int)(stuffedAnimalX_ - (float)slTexW * slScale / 2.0f);
		int slDrawY = (int)(stuffedAnimalY_ - (float)slTexH * slScale / 2.0f + slFloatOffset);

		SDL_Rect slSection = { 0, 0, slTexW, slTexH };
		Engine::GetInstance().render->DrawTexture(texStuffedAnimalCollectible_, slDrawX, slDrawY, &slSection, 1.0f, -1.0f, 0, INT_MAX, INT_MAX, SDL_FLIP_NONE, slScale);
	}
if (!isPaused_ && !isGameOver_) UpdateBossFight(dt);

if (isPuzzleMap_ && puzzleManager_ && player && !isPaused_ && !isGameOver_) {
    SDL_FRect pRect = {
        player->position.getX(),
        player->position.getY(),
        48.0f,
        80.0f
    };
    float camX = Engine::GetInstance().render->camera.x;
    float camY = Engine::GetInstance().render->camera.y;
    float pScrX = player->position.getX() + camX + 24.0f;
    float pScrY = player->position.getY() + camY + 40.0f;
    puzzleManager_->Update(dt, pRect, pScrX, pScrY);

    if (puzzleManager_->IsTimedOut() && !puzzleTimeoutPending_) {
        puzzleTimeoutPending_ = true;
        waitingForFade_ = true;
        pendingSubMapLoad_ = true;
        subMapTarget_ = currentMapFile_;
        Engine::GetInstance().render->StartFade(FadeDirection::FADE_OUT, 600.0f);
    }
}

if (puzzleManager3_ && player && !isPaused_ && !isGameOver_) {
	SDL_FRect pRect = {
		player->position.getX(),
		player->position.getY(),
		48.0f, 80.0f
	};

	if (isLvl3Map_) {
		bool keyP = Engine::GetInstance().input->GetKey(SDL_SCANCODE_P) == KEY_DOWN;
		puzzleManager3_->UpdateLever(dt, pRect, keyP);
	}

	if (isLvl3Puzzle_) {
		// Mouse en world space
		Vector2D mousePos = Engine::GetInstance().input->GetMousePosition();
		float mwx = mousePos.getX() - Engine::GetInstance().render->camera.x;
		float mwy = mousePos.getY() - Engine::GetInstance().render->camera.y;
		bool click = Engine::GetInstance().input->GetMouseButtonDown(SDL_BUTTON_LEFT) == KEY_DOWN;
		puzzleManager3_->UpdateButtons(dt, pRect, mwx, mwy, click);
	}
}
// --- FIN PUZZLE3 UPDATE ---

	if (!isPaused_ && !isGameOver_ && puzzleManager2) {
		puzzleManager2->Update(dt, Engine::GetInstance().entityManager->entities);
	}

	// Draw cape collectible in-world
	if (!capaCollected_ && texCapaCollectible_)
	{
		int capaTexW = 0, capaTexH = 0;
		Engine::GetInstance().textures->GetSize(texCapaCollectible_, capaTexW, capaTexH);
		float floatOffset = 6.0f * sinf(capaFloatTimer_ * 0.003f);
		float capaScale = 0.06f; // Perfect floating scale for the high-res blue blanket!
		int drawX = (int)(capaX_ - (float)capaTexW * capaScale / 2.0f);
		int drawY = (int)(capaY_ - (float)capaTexH * capaScale / 2.0f + floatOffset);

		SDL_Rect section = { 0, 0, capaTexW, capaTexH };
		Engine::GetInstance().render->DrawTexture(texCapaCollectible_, drawX, drawY, &section, 1.0f, -1.0f, 0, INT_MAX, INT_MAX, SDL_FLIP_NONE, capaScale);
	}
}

// ============================================================================
//  BOSS FIGHT
// ============================================================================

void Scene::UpdateBossFight(float dt)
{
    if (videoSkipCooldown_ > 0.0f) {
        videoSkipCooldown_ -= dt;
    }
    // ── End-game final cinematic: fade to black, play video, go to main menu ──
    if (endGameFading_)
    {
        if (!Engine::GetInstance().render->IsFadingOut())
        {
            endGameFading_ = false;
            endGameVideoActive_ = true;
            Engine::GetInstance().cinematics->PlayVideo("assets/video/animatica_final.MOV");
            Engine::GetInstance().render->StartFade(FadeDirection::FADE_IN, 800.0f);
        }
        return;
    }

    if (endGameVideoActive_)
    {
        auto& input = *Engine::GetInstance().input;
        bool skipRequested = false;
        if (videoSkipCooldown_ <= 0.0f) {
            skipRequested = input.GetKey(SDL_SCANCODE_SPACE) == KEY_DOWN ||
                            input.GetGamepadButton(SDL_GAMEPAD_BUTTON_SOUTH) == KEY_DOWN ||
                            input.GetGamepadButton(SDL_GAMEPAD_BUTTON_START) == KEY_DOWN ||
                            Engine::GetInstance().cinematics->HasSkipBeenRequested();
        }

        if (skipRequested || !Engine::GetInstance().cinematics->IsPlaying())
        {
            if (skipRequested) {
                Engine::GetInstance().cinematics->StopVideo();
            }
            endGameVideoActive_ = false;
            
            // Queue next video: Credits
            creditsVideoActive_ = true;
            videoSkipCooldown_ = 1000.0f; // 1 second cooldown
            Engine::GetInstance().cinematics->PlayVideo("assets/video/Credits_Videos.mp4");
            Engine::GetInstance().render->StartFade(FadeDirection::FADE_IN, 800.0f);
        }
        else 
        {
            // Draw "Press SPACE to Skip" overlay while playing
            int winW = 0, winH = 0;
            Engine::GetInstance().window->GetWindowSize(winW, winH);
            SDL_Color skipColor = { 255, 255, 255, 120 };
            Engine::GetInstance().render->DrawMenuTextCentered("Press SPACE to Skip", { winW - 300, winH - 60, 280, 30 }, skipColor, 0.4f);
        }
        return;
    }

    if (creditsVideoActive_)
    {
        auto& input = *Engine::GetInstance().input;
        bool skipRequested = false;
        if (videoSkipCooldown_ <= 0.0f) {
            skipRequested = input.GetKey(SDL_SCANCODE_SPACE) == KEY_DOWN ||
                            input.GetGamepadButton(SDL_GAMEPAD_BUTTON_SOUTH) == KEY_DOWN ||
                            input.GetGamepadButton(SDL_GAMEPAD_BUTTON_START) == KEY_DOWN ||
                            Engine::GetInstance().cinematics->HasSkipBeenRequested();
        }

        if (skipRequested || !Engine::GetInstance().cinematics->IsPlaying())
        {
            if (skipRequested) {
                Engine::GetInstance().cinematics->StopVideo();
            }
            creditsVideoActive_ = false;
            
            // Queue immediate scene change to MAIN MENU
            hasPendingSceneChange = true;
            pendingScene = SceneID::MAIN_MENU;
            
            // Start a FADE_IN: it immediately sets the screen to solid black (covering the gameplay)
            // and over 1200ms it will reveal the Main Menu
            Engine::GetInstance().render->StartFade(FadeDirection::FADE_IN, 1200.0f);
        }
        else 
        {
            // Draw "Press SPACE to Skip" overlay while playing
            int winW = 0, winH = 0;
            Engine::GetInstance().window->GetWindowSize(winW, winH);
            SDL_Color skipColor = { 255, 255, 255, 120 };
            Engine::GetInstance().render->DrawMenuTextCentered("Press SPACE to Skip", { winW - 300, winH - 60, 280, 30 }, skipColor, 0.4f);
        }
        return;
    }

    // Fading to black before boss death video — wait for fade, then launch video
    if (bossDeathFading_)
    {
        if (!Engine::GetInstance().render->IsFadingOut())
        {
            bossDeathFading_ = false;
            Engine::GetInstance().cinematics->PlayVideo(bossDeathVideoPath_.c_str());
            Engine::GetInstance().render->StartFade(FadeDirection::FADE_IN, 1.0f);
            bossDeathVideoActive_ = true;
        }
        return;
    }

    // If a boss death video is playing, wait for it to finish before transitioning
    if (bossDeathVideoActive_)
    {
        if (!Engine::GetInstance().cinematics->IsPlaying())
        {
            bossDeathVideoActive_ = false;
            if (bossDeathNeedsMapLoad_)
            {
                subMapTarget_      = bossDeathMapTarget_;
                subMapSpawnId_     = bossDeathSpawnId_;
                pendingSubMapLoad_ = true;
                waitingForFade_    = true;
                fadeTargetScene_   = SceneID::GAMEPLAY;
                Engine::GetInstance().render->StartFade(FadeDirection::FADE_OUT, 1200.0f);
            }
            else
            {
                Engine::GetInstance().audio->PlayMusic("assets/audio/music/backgroundmusic.wav", 2.0f);
            }
        }
        return;
    }

    auto boss = activeBoss_.lock();

    // Lazy search: if no boss cached yet, look in the entity list
    if (!boss) {
        for (auto& e : Engine::GetInstance().entityManager->entities) {
            if (e->type == EntityType::BOSS_1 || e->type == EntityType::BOSS_2) {
                boss = std::dynamic_pointer_cast<Boss>(e);
                if (boss) { activeBoss_ = boss; break; }
            }
        }
    }

    if (!boss) {
        // Boss has been removed from the entity list (dead + cleaned up)
        if (isBossFightActive_) {
            isBossFightActive_ = false;
            Engine::GetInstance().audio->PlayMusic("assets/audio/music/backgroundmusic.wav", 2.0f);
            LOG("Boss fight ended — restoring gameplay music");
        }
        return;
    }

    if (boss->IsEngaged() && !isBossFightActive_)
    {
        isBossFightActive_   = true;
        bossHealthDisplay_   = 1.0f;
        Engine::GetInstance().audio->PlayMusic("assets/audio/music/Echoes_of_Slumber_Boss_Fight _Music.wav", 2.0f);
        auto& tex = *Engine::GetInstance().textures;
        texBossBarEmpty_     = tex.Load("assets/textures/UI/UI_Boss_HealthBar_Empty.png");
        texBossBarFull_      = tex.Load("assets/textures/UI/UI_Boss_HealthBar_Full.png");
        texBossBarIndicator_ = tex.Load("assets/textures/UI/UI_Boss_HealthBar_Indicator.png");
    }

    if (boss->IsDead() && isBossFightActive_)
    {
        isBossFightActive_ = false;
        auto& tex = *Engine::GetInstance().textures;
        tex.UnLoad(texBossBarEmpty_);     texBossBarEmpty_     = nullptr;
        tex.UnLoad(texBossBarFull_);      texBossBarFull_      = nullptr;
        tex.UnLoad(texBossBarIndicator_); texBossBarIndicator_ = nullptr;

        // Pick video and destination based on which boss died
        bool isB1 = std::dynamic_pointer_cast<Boss1>(boss) != nullptr;
        const char* videoPath  = isB1 ? "assets/video/Anchor_2.mp4"   : "assets/video/Anchor_3.mp4";
        bossDeathNeedsMapLoad_ = true;
        bossDeathMapTarget_    = isB1 ? "Map2.tmx"                     : "MapLvl3ZonaAlta.tmx";
        bossDeathSpawnId_      = isB1 ? "Main"                         : "J";

        activeBoss_.reset();
        bossDeathVideoPath_ = videoPath;
        bossDeathFading_    = true;
        Engine::GetInstance().render->StartFade(FadeDirection::FADE_OUT, 600.0f);
    }
}

void Scene::DrawBossHUD(int winW, int winH)
{
    auto boss = activeBoss_.lock();
    if (!boss || !isBossFightActive_ || boss->IsDead()) return;
    if (!texBossBarEmpty_ || !texBossBarFull_ || !texBossBarIndicator_) return;

    auto& render = *Engine::GetInstance().render;

    const int BAR_W  = 800;
    const int BAR_H  = 50;
    const int BAR_X  = (winW - BAR_W) / 2;
    // Drowning Plush's room has a lower ceiling on screen — nudge its HUD down so the title clears it
    const int BAR_Y  = std::dynamic_pointer_cast<Boss1>(boss) ? 72 : 45;
    const int IND_SZ = 50;

    float realPct = boss->GetHealthPercent();
    realPct = (realPct < 0.0f) ? 0.0f : (realPct > 1.0f) ? 1.0f : realPct;
    bossHealthDisplay_ += (realPct - bossHealthDisplay_) * 0.08f;

    const int NAME_H  = 22;
    const int BAR_TOP = BAR_Y + NAME_H + 22;

    SDL_Rect nameArea = { BAR_X, BAR_Y, BAR_W, NAME_H };
    render.DrawMenuTextCentered(boss->GetBossName(), nameArea, { 230, 220, 200, 255 });

    render.DrawTextureAlpha(texBossBarEmpty_, BAR_X, BAR_TOP, BAR_W, BAR_H);

    int clipW = (int)(BAR_W * bossHealthDisplay_);
    if (clipW > 0)
    {
        SDL_FRect src = { 0.0f, 0.0f, (float)clipW, (float)BAR_H };
        SDL_FRect dst = { (float)BAR_X, (float)BAR_TOP, (float)clipW, (float)BAR_H };
        SDL_RenderTexture(render.renderer, texBossBarFull_, &src, &dst);
    }

    int indX = BAR_X + clipW - IND_SZ / 2;
    indX = std::max(BAR_X + 4, std::min(indX, BAR_X + BAR_W - IND_SZ - 9));
    int indY = BAR_TOP + BAR_H / 2 - IND_SZ / 2;
    render.DrawTextureAlpha(texBossBarIndicator_, indX, indY, IND_SZ, IND_SZ);
}

void Scene::UnloadGameplay()
{
	if (puzzleManager2) {
		puzzleManager2.reset();
	}
	if (puzzleManager_) {
		delete puzzleManager_;
		puzzleManager_ = nullptr;
	}
	isPuzzleMap_ = false;

	if (puzzleManager3_) {
		delete puzzleManager3_;
		puzzleManager3_ = nullptr;
	}
	isPuzzleMap3Lever_ = false;
	isPuzzleMap3Buttons_ = false;

	Engine::GetInstance().uiManager->CleanUp();
	player.reset();
	Engine::GetInstance().entityManager->CleanUp();
	Engine::GetInstance().map->CleanUp();
	Engine::GetInstance().physics->FlushPendingDeletes();
	isPaused_ = false;
	showPauseOptions_ = false;
	showMapViewer_ = false;
	showInventory_ = false;

	for (int i = 0; i < MAX_HEALTH_SLOTS; i++) {
		if (texHealth_[i]) { Engine::GetInstance().textures->UnLoad(texHealth_[i]); texHealth_[i] = nullptr; }
	}
	if (texGameOver_) { SDL_DestroyTexture(texGameOver_); texGameOver_ = nullptr; }
	if (texDamageVignette_) { Engine::GetInstance().textures->UnLoad(texDamageVignette_); texDamageVignette_ = nullptr; }

	if (texGameOverScreenBase_) { Engine::GetInstance().textures->UnLoad(texGameOverScreenBase_); texGameOverScreenBase_ = nullptr; }
	if (texGameOverText_) { Engine::GetInstance().textures->UnLoad(texGameOverText_); texGameOverText_ = nullptr; }
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

	if (texSlingshotCollectible_) { Engine::GetInstance().textures->UnLoad(texSlingshotCollectible_); texSlingshotCollectible_ = nullptr; }
	slingshotCollected_ = false;

	if (texStuffedAnimalCollectible_) { Engine::GetInstance().textures->UnLoad(texStuffedAnimalCollectible_); texStuffedAnimalCollectible_ = nullptr; }
	stuffedAnimalCollected_ = false;

	if (texInventoryBg_) { Engine::GetInstance().textures->UnLoad(texInventoryBg_); texInventoryBg_ = nullptr; }
	if (texAbilitiesLocked_) { Engine::GetInstance().textures->UnLoad(texAbilitiesLocked_); texAbilitiesLocked_ = nullptr; }
	if (texAbilitiesBlanket_) { Engine::GetInstance().textures->UnLoad(texAbilitiesBlanket_); texAbilitiesBlanket_ = nullptr; }
	if (texAbilitiesBlanketSling_) { Engine::GetInstance().textures->UnLoad(texAbilitiesBlanketSling_); texAbilitiesBlanketSling_ = nullptr; }
	if (texAbilitiesAll_) { Engine::GetInstance().textures->UnLoad(texAbilitiesAll_); texAbilitiesAll_ = nullptr; }

	if (texAbilityBlanketIcon_) { Engine::GetInstance().textures->UnLoad(texAbilityBlanketIcon_); texAbilityBlanketIcon_ = nullptr; }
	if (texAbilitySlingshotIcon_) { Engine::GetInstance().textures->UnLoad(texAbilitySlingshotIcon_); texAbilitySlingshotIcon_ = nullptr; }
	if (texAbilityStuffedAnimalIcon_) { Engine::GetInstance().textures->UnLoad(texAbilityStuffedAnimalIcon_); texAbilityStuffedAnimalIcon_ = nullptr; }
	if (texAbilityAnchorIcon_) { Engine::GetInstance().textures->UnLoad(texAbilityAnchorIcon_); texAbilityAnchorIcon_ = nullptr; }

	// Unload Memories UI textures
	if (texMemoria1Base_) { Engine::GetInstance().textures->UnLoad(texMemoria1Base_); texMemoria1Base_ = nullptr; }
	if (texMemoria1N1_) { Engine::GetInstance().textures->UnLoad(texMemoria1N1_); texMemoria1N1_ = nullptr; }
	if (texMemoria1N2_) { Engine::GetInstance().textures->UnLoad(texMemoria1N2_); texMemoria1N2_ = nullptr; }

	if (texMemoria2Base_) { Engine::GetInstance().textures->UnLoad(texMemoria2Base_); texMemoria2Base_ = nullptr; }
	if (texMemoria2N1_) { Engine::GetInstance().textures->UnLoad(texMemoria2N1_); texMemoria2N1_ = nullptr; }
	if (texMemoria2N2_) { Engine::GetInstance().textures->UnLoad(texMemoria2N2_); texMemoria2N2_ = nullptr; }

	if (texMemoria3Base_) { Engine::GetInstance().textures->UnLoad(texMemoria3Base_); texMemoria3Base_ = nullptr; }
	if (texMemoria3N1_) { Engine::GetInstance().textures->UnLoad(texMemoria3N1_); texMemoria3N1_ = nullptr; }
	if (texMemoria3N2_) { Engine::GetInstance().textures->UnLoad(texMemoria3N2_); texMemoria3N2_ = nullptr; }
	if (texMemoria3N3_) { Engine::GetInstance().textures->UnLoad(texMemoria3N3_); texMemoria3N3_ = nullptr; }

	// Unload Fullscreen memories
	if (texMemoriaFrameFullScreen_) { Engine::GetInstance().textures->UnLoad(texMemoriaFrameFullScreen_); texMemoriaFrameFullScreen_ = nullptr; }
	if (texMemoria1Full1_) { Engine::GetInstance().textures->UnLoad(texMemoria1Full1_); texMemoria1Full1_ = nullptr; }
	if (texMemoria1Full2_) { Engine::GetInstance().textures->UnLoad(texMemoria1Full2_); texMemoria1Full2_ = nullptr; }

	if (texMemoria2Full1_) { Engine::GetInstance().textures->UnLoad(texMemoria2Full1_); texMemoria2Full1_ = nullptr; }
	if (texMemoria2Full2_) { Engine::GetInstance().textures->UnLoad(texMemoria2Full2_); texMemoria2Full2_ = nullptr; }

	if (texMemoria3Full1_) { Engine::GetInstance().textures->UnLoad(texMemoria3Full1_); texMemoria3Full1_ = nullptr; }
	if (texMemoria3Full2_) { Engine::GetInstance().textures->UnLoad(texMemoria3Full2_); texMemoria3Full2_ = nullptr; }
	if (texMemoria3Full3_) { Engine::GetInstance().textures->UnLoad(texMemoria3Full3_); texMemoria3Full3_ = nullptr; }

	// Unload Minimap Ornate Frame
	if (texMinimapFrame_) { Engine::GetInstance().textures->UnLoad(texMinimapFrame_); texMinimapFrame_ = nullptr; }
}

// ============================================================================
//  Map switching logic (F1 / F2 / F3 / F4)
// ============================================================================

void Scene::LoadMap(int index)
{
	if (index < 0 || (size_t)index >= levels_.size()) return;
	
	// Prevent double trigger if already loading
	if (waitingForFade_ && fadeTargetScene_ == SceneID::LOADING && targetLevelIndex_ == index) return;

	LOG("=== Initiating Transition to Map %d: %s ===", index + 1, levels_[index].name.c_str());

	targetLevelIndex_ = index;
	waitingForFade_ = true;
	fadeTargetScene_ = SceneID::LOADING;
	
	// Start fade out before showing loading screen
	Engine::GetInstance().render->StartFade(FadeDirection::FADE_OUT, 600.0f);
}

void Scene::LoadMap1() { LoadMap(0); }
void Scene::LoadMap2() { LoadMap(1); }
void Scene::LoadMap3() { LoadMap(2); }
void Scene::LoadMap4() { LoadMap(3); }

bool Scene::RequestCheckpointActivation(const std::string& checkpointId, const Vector2D& spawnPosition)
{
	if (checkpointId.empty() || IsCheckpointTransitionActive() || isGameOver_ || waitingForFade_)
		return false;

	Engine::GetInstance().audio->PlayFx(checkpointFxId, 0, true);

	pendingCheckpointId_ = checkpointId;
	pendingCheckpointSpawn_ = spawnPosition;
	checkpointTransitionMode_ = CheckpointTransitionMode::ACTIVATE;
	checkpointTransitionPhase_ = CheckpointTransitionPhase::FADE_OUT;
	checkpointBlackHoldTimer_ = 0.0f;
	checkpointNotifyAfterFade_ = false;
	Engine::GetInstance().render->StartFade(FadeDirection::FADE_OUT, 350.0f);
	return true;
}

bool Scene::RequestCheckpointRespawn()
{
	if (IsCheckpointTransitionActive() || waitingForFade_)
		return false;

	if (!Engine::GetInstance().saveSystem->HasCheckpointSave())
		return false;

	checkpointTransitionMode_ = CheckpointTransitionMode::RESPAWN;
	checkpointTransitionPhase_ = CheckpointTransitionPhase::FADE_OUT;
	checkpointBlackHoldTimer_ = 0.0f;
	checkpointNotifyAfterFade_ = false;
	screenDamageTimer_ = 0.0f;
	SetGameOverVisible(false);
	Engine::GetInstance().render->StartFade(FadeDirection::FADE_OUT, 450.0f);
	return true;
}

bool Scene::IsCheckpointTransitionActive() const
{
	return checkpointTransitionPhase_ != CheckpointTransitionPhase::NONE;
}

void Scene::SetActiveCheckpointId(const std::string& checkpointId)
{
	activeCheckpointId_ = checkpointId;
}

void Scene::SyncCheckpointEntities()
{
	for (const auto& entity : Engine::GetInstance().entityManager->entities)
	{
		if (entity->type != EntityType::CHECKPOINT) continue;
		auto checkpoint = std::dynamic_pointer_cast<Checkpoint>(entity);
		if (!checkpoint) continue;
		checkpoint->SetActivated(!activeCheckpointId_.empty() &&
			checkpoint->GetCheckpointId() == activeCheckpointId_);
	}
}

void Scene::ResolveCheckpointTransition()
{
	if (checkpointTransitionMode_ == CheckpointTransitionMode::ACTIVATE)
	{
		SetActiveCheckpointId(pendingCheckpointId_);
		SyncCheckpointEntities();

		if (player)
		{
			const int maxHp = std::max(1, player->maxHealth);
			player->health = maxHp;
			player->Revive();
			player->isWakingUp = false;
			player->wakeUpAnimStarted = true;
			ResetHealthUI(player->health);
		}

		Vector2D checkpointPlayerPos = pendingCheckpointSpawn_;
		if (player)
		{
			checkpointPlayerPos = Vector2D(
				pendingCheckpointSpawn_.getX() - (float)player->texW * 0.5f,
				pendingCheckpointSpawn_.getY() - (float)player->texH * 0.5f);
		}

		Engine::GetInstance().saveSystem->QuickSaveAt(
			checkpointPlayerPos.getX(),
			checkpointPlayerPos.getY(),
			pendingCheckpointId_);
		checkpointNotifyAfterFade_ = true;
		LOG("Checkpoint activated: %s", pendingCheckpointId_.c_str());
	}
	else if (checkpointTransitionMode_ == CheckpointTransitionMode::RESPAWN)
	{
		if (Engine::GetInstance().saveSystem->QuickLoadImmediate())
		{
			isGameOver_ = false;
			gameOverFadeTimer_ = 0.0f;
			inGameIntroActive_ = false;
			isAutoEntering_ = false;
			inGameIntroTimer_ = 0.0f;
			autoEntryProgress_ = 0.0f;

			if (player)
			{
				player->StartWakeUp(2.0f);
				ResetHealthUI(player->health);
				Engine::GetInstance().render->SetCameraPosition(player->position.getX(), player->position.getY());
			}

			Engine::GetInstance().render->SetCameraSway(true);
			Engine::GetInstance().render->SetCameraClamping(true);
			Engine::GetInstance().render->SetCameraMovement(true);
			Engine::GetInstance().render->cameraZoom = 1.0f;
			Engine::GetInstance().render->blurIntensity = 0.0f;
			Engine::GetInstance().audio->PlayMusic("assets/audio/music/Echoes_of_Slumber_In_Game.wav", 1.0f);
			Engine::GetInstance().audio->SetMusicVolume(musicVolume_);
			Engine::GetInstance().audio->SetSFXVolume(sfxVolume_);
			LOG("Checkpoint respawn completed");
		}
	}
}

bool Scene::UpdateCheckpointTransition(float dt)
{
	if (!IsCheckpointTransitionActive()) return false;

	if (checkpointTransitionPhase_ == CheckpointTransitionPhase::FADE_OUT)
	{
		if (Engine::GetInstance().render->IsFadeComplete())
		{
			ResolveCheckpointTransition();
			checkpointBlackHoldTimer_ = 220.0f;
			checkpointTransitionPhase_ = CheckpointTransitionPhase::HOLD_BLACK;
		}
		return true;
	}

	if (checkpointTransitionPhase_ == CheckpointTransitionPhase::HOLD_BLACK)
	{
		checkpointBlackHoldTimer_ -= dt;
		if (checkpointBlackHoldTimer_ <= 0.0f)
		{
			Engine::GetInstance().render->StartFade(FadeDirection::FADE_IN, 650.0f);
			checkpointTransitionPhase_ = CheckpointTransitionPhase::FADE_IN;
		}
		return true;
	}

	if (checkpointTransitionPhase_ == CheckpointTransitionPhase::FADE_IN)
	{
		if (Engine::GetInstance().render->IsFadeComplete())
		{
			checkpointTransitionMode_ = CheckpointTransitionMode::NONE;
			checkpointTransitionPhase_ = CheckpointTransitionPhase::NONE;
			pendingCheckpointId_.clear();

			if (checkpointNotifyAfterFade_)
			{
				checkpointSaveTimer_ = 3000.0f;
				checkpointNotifyAfterFade_ = false;
			}
		}
		return true;
	}

	return true;
}

void Scene::PostUpdateGameplay()
{
	// Quick save/load shortcuts (only when not paused)
	if (!isPaused_ && !isGameOver_ && !IsCheckpointTransitionActive()) {
		if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_F5) == KEY_DOWN)
			Engine::GetInstance().saveSystem->QuickLoad();
		if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_F6) == KEY_DOWN)
			Engine::GetInstance().saveSystem->QuickSave();

		// F7: Unlock all abilities
		if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_F7) == KEY_DOWN)
		{
			if (player) {
				capaCollected_ = true;
				player->SetHasBlanket(true);
				capaNotifTimer_ = CAPA_NOTIF_DURATION;

				slingshotCollected_ = true;
				player->SetHasSlingshot(true);
				slingshotNotifTimer_ = SLINGSHOT_NOTIF_DURATION;

				stuffedAnimalCollected_ = true;
				player->SetHasStuffedAnimal(true);
				stuffedAnimalNotifTimer_ = STUFFED_ANIMAL_NOTIF_DURATION;

				LOG("DEBUG: All abilities unlocked (Cape, Slingshot, Stuffed Animal)!");
				Engine::GetInstance().audio->PlayFx(player->pickCoinFxId);
			}
		}

		// F8: Give 10 keys
		if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_F8) == KEY_DOWN)
		{
			if (player) {
				player->keys += 10;
				LOG("DEBUG: Gave 10 keys to player (Total: %d)", player->keys);
				Engine::GetInstance().audio->PlayFx(player->pickCoinFxId);
			}
		}

		// F1 / F2 / F3 / F4: switch maps
		if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_F1) == KEY_DOWN)
			LoadMap1();
		if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_F2) == KEY_DOWN)
			LoadMap2();
		if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_F3) == KEY_DOWN)
			LoadMap3();
		if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_F4) == KEY_DOWN)
			LoadMap4();
	}

	{
		int winW = 0, winH = 0;
		Engine::GetInstance().window->GetWindowSize(winW, winH);
		DrawBottomFog(winW, winH);
	}


	if (isPuzzleMap_ && puzzleManager_) {
		float camX = (float)Engine::GetInstance().render->camera.x;
		float camY = (float)Engine::GetInstance().render->camera.y;
		puzzleManager_->Render(Engine::GetInstance().render->renderer, camX, camY);
	}

	if ((isLvl3Map_ || isLvl3Puzzle_) && puzzleManager3_) {
		float camX = (float)Engine::GetInstance().render->camera.x;
		float camY = (float)Engine::GetInstance().render->camera.y;
		int winW, winH;
		SDL_GetRenderOutputSize(Engine::GetInstance().render->renderer, &winW, &winH);
		puzzleManager3_->RenderLever(Engine::GetInstance().render->renderer, camX, camY);
		puzzleManager3_->RenderButtons(Engine::GetInstance().render->renderer, camX, camY);
		puzzleManager3_->RenderFlash(Engine::GetInstance().render->renderer, winW, winH);
	}

	// --- Draw Health HUD ---
	if (player && !player->isWakingUp && !isPaused_ && !showInventory_ && !showMapViewer_
		&& currentMapFile_ != "MapLvl2ZonaBoss.tmx" && currentMapFile_ != "MapLvl3ZonaBoss.tmx") {
		SDL_Rect r;
		const SDL_Rect* frame = nullptr;
		SDL_Texture* texToDraw = nullptr;

		// Generic health HUD drawing using diff-based array indexing
		// diff = maxHealth - currentHealth → index into texHealth_[] / animHealth_[]
		{
			int maxHP = player ? player->maxHealth : healthSlotCount_;
			int diff = maxHP - currentHealthUI_;
			if (diff == 0 && texHealth_[0]) {
				// Full health: show first frame of slot 0
				texToDraw = texHealth_[0];
				r = animHealth_[0].GetCurrentFrame();
				frame = &r;
			}
			else if (diff > 0 && diff <= healthSlotCount_) {
				int idx = diff - 1;
				if (texHealth_[idx]) {
					texToDraw = texHealth_[idx];
					r = animHealth_[idx].GetCurrentFrame();
					frame = &r;
				}
			}
		}
         if (texToDraw && frame) {
             Engine::GetInstance().render->DrawTexture(texToDraw, 40, 40, frame, 0.0f, -1.0f, 0, INT_MAX, INT_MAX, SDL_FLIP_NONE, 0.25f);
         }

         Player::EquippedItem eq = player->GetEquippedItem();

         // --- Unified Diamond-Shaped Ability & Item HUD next to Health ---
         int hudStartX = 185;
         int hudY = 75;
         int hudIconSize = 54;
         int hudGap = 12;

         // 1. Blanket (Manta) HUD Icon
         if (texAbilityBlanketIcon_) {
             Uint8 alpha = player->HasBlanket() ? 255 : 60;
             // Draw a glowing golden outline around the icon slot if equipped
             if (eq == Player::EquippedItem::BLANKET && player->HasBlanket()) {
                 SDL_Rect borderRect = { hudStartX - 3, hudY - 3, hudIconSize + 6, hudIconSize + 6 };
                 Engine::GetInstance().render->DrawRectangle(borderRect, 255, 195, 0, 255, false, false);
             }
             Engine::GetInstance().render->DrawTextureAlpha(texAbilityBlanketIcon_, hudStartX, hudY, hudIconSize, hudIconSize, alpha);
         }

         // 2. Slingshot (Tirachinas) HUD Icon
         if (texAbilitySlingshotIcon_) {
             int posX = hudStartX + hudIconSize + hudGap;
             Uint8 alpha = player->HasSlingshot() ? 255 : 60;
             // Draw a glowing golden outline around the icon slot if equipped
             if (eq == Player::EquippedItem::SLINGSHOT && player->HasSlingshot()) {
                 SDL_Rect borderRect = { posX - 3, hudY - 3, hudIconSize + 6, hudIconSize + 6 };
                 Engine::GetInstance().render->DrawRectangle(borderRect, 255, 195, 0, 255, false, false);
             }
             Engine::GetInstance().render->DrawTextureAlpha(texAbilitySlingshotIcon_, posX, hudY, hudIconSize, hudIconSize, alpha);
         }

         // 3. Stuffed Animal (Oso/Peluche) HUD Icon
         if (texAbilityStuffedAnimalIcon_) {
             int posX = hudStartX + (hudIconSize + hudGap) * 2;
             Uint8 alpha = player->HasStuffedAnimal() ? 255 : 60;
             // Draw a glowing golden outline around the icon slot if equipped
             if (eq == Player::EquippedItem::STUFFED_ANIMAL && player->HasStuffedAnimal()) {
                 SDL_Rect borderRect = { posX - 3, hudY - 3, hudIconSize + 6, hudIconSize + 6 };
                 Engine::GetInstance().render->DrawRectangle(borderRect, 255, 195, 0, 255, false, false);
             }
             Engine::GetInstance().render->DrawTextureAlpha(texAbilityStuffedAnimalIcon_, posX, hudY, hudIconSize, hudIconSize, alpha);
         }

		// --- Draw Dynamic Minimap ---
		if (!texMinimapFrame_)
		{
			texMinimapFrame_ = Engine::GetInstance().textures->Load("assets/textures/UI/UI_Minimap.png");
		}

		if (texMinimapFrame_)
		{
			auto& render = *Engine::GetInstance().render;
			int winW = 0, winH = 0;
			Engine::GetInstance().window->GetWindowSize(winW, winH);
			
			// Position: Bottom-Right corner of the screen
			int miniW = 220;
			int miniH = 220;
			int miniX = winW - miniW - 20;
			int miniY = winH - miniH - 20;

			// Inner boundaries for terrain viewport (accounting for the ornate border thickness)
			int borderThickness = 12;
			int innerX = miniX + borderThickness;
			int innerY = miniY + borderThickness;
			int innerW = miniW - borderThickness * 2;
			int innerH = miniH - borderThickness * 2;
			int centerX = innerX + innerW / 2;
			int centerY = innerY + innerH / 2;

			// Draw a dark solid container background behind the scrolling map terrain
			SDL_Rect miniBg = { innerX, innerY, innerW, innerH };
			render.DrawRectangle(miniBg, 12, 16, 26, 210, true, false);

			// Center the map viewport around the player's pixel position
			float pWorldX = player->position.getX();
			float pWorldY = player->position.getY();

			// 1 minimap pixel = 4 world pixels (perfect for 64px / 32px tiles to show context)
			int tileWidth = Engine::GetInstance().map->GetTileWidth();
			int tileHeight = Engine::GetInstance().map->GetTileHeight();
			if (tileWidth <= 0) tileWidth = 64;
			if (tileHeight <= 0) tileHeight = 64;

			float worldToMiniScale = 4.0f / (float)tileWidth;

			// Scan local tile neighborhood around the player's tile position
			int pTileX = (int)(pWorldX / (float)tileWidth);
			int pTileY = (int)(pWorldY / (float)tileHeight);
			int radius = 25; // Scan range of 25 tiles out to fill the larger viewport

			for (int dy = -radius; dy <= radius; dy++)
			{
				for (int dx = -radius; dx <= radius; dx++)
				{
					int tx = pTileX + dx;
					int ty = pTileY + dy;

					// Draw all layers that have "Draw" enabled (so we see the actual level visuals!)
					for (const auto& mapLayer : Engine::GetInstance().map->mapData.layers)
					{
						if (mapLayer->properties.GetProperty("Draw") != NULL && mapLayer->properties.GetProperty("Draw")->value == true)
						{
							if (tx >= 0 && tx < mapLayer->width && ty >= 0 && ty < mapLayer->height)
							{
								int gid = mapLayer->Get(tx, ty);
								if (gid != 0)
								{
									TileSet* tileSet = Engine::GetInstance().map->GetTilesetFromTileId(gid);
									if (tileSet != nullptr && tileSet->texture != nullptr)
									{
										SDL_Rect tileRect = tileSet->GetRect(gid);

										// Calculate sub-pixel world position offset from player
										float wTileX = (float)tx * (float)tileWidth;
										float wTileY = (float)ty * (float)tileHeight;

										float offsetWorldX = wTileX - pWorldX;
										float offsetWorldY = wTileY - pWorldY;

										// Convert offset to minimap coordinates
										float offsetMiniX = offsetWorldX * worldToMiniScale;
										float offsetMiniY = offsetWorldY * worldToMiniScale;

										int drawTileX = (int)(centerX + offsetMiniX);
										int drawTileY = (int)(centerY + offsetMiniY);

										// Viewport Clipping - only render if inside the inner frame boundary
										// Standard tiles are 4px wide in our minimap scale
										if (drawTileX >= innerX && drawTileX + 4 <= innerX + innerW &&
											drawTileY >= innerY && drawTileY + 4 <= innerY + innerH)
										{
											// Draw the real tile texture scaled down, speed=0.0f to lock to HUD space!
											float drawScale = 4.0f / (float)tileWidth;
											render.DrawTexture(tileSet->texture, drawTileX, drawTileY, &tileRect, 0.0f, -1.0f, 0, INT_MAX, INT_MAX, SDL_FLIP_NONE, drawScale);
										}
									}
								}
							}
						}
					}
				}
			}

			// --- Draw Collectibles on Radar if not yet picked up ---
			// Cape / Blanket Collectible (Blue Dot)
			if (!capaCollected_)
			{
				float offsetWorldX = capaX_ - pWorldX;
				float offsetWorldY = (capaY_ - 50.0f) - pWorldY; // account for float offset center
				float offsetMiniX = offsetWorldX * worldToMiniScale;
				float offsetMiniY = offsetWorldY * worldToMiniScale;

				int drawX = (int)(centerX + offsetMiniX);
				int drawY = (int)(centerY + offsetMiniY);

				if (drawX >= innerX + 3 && drawX <= innerX + innerW - 3 &&
					drawY >= innerY + 3 && drawY <= innerY + innerH - 3)
				{
					SDL_Rect dot = { drawX - 3, drawY - 3, 6, 6 };
					// High-contrast neon blue color for the Cape!
					render.DrawRectangle(dot, 0, 190, 255, 255, true, false);
					SDL_Rect dotOutline = { drawX - 4, drawY - 4, 8, 8 };
					render.DrawRectangle(dotOutline, 255, 255, 255, 200, false, false);
				}
			}

			// Slingshot / Tirachinas Collectible (Orange Dot)
			if (!slingshotCollected_)
			{
				float offsetWorldX = slingshotX_ - pWorldX;
				float offsetWorldY = (slingshotY_ - 50.0f) - pWorldY; // account for float offset center
				float offsetMiniX = offsetWorldX * worldToMiniScale;
				float offsetMiniY = offsetWorldY * worldToMiniScale;

				int drawX = (int)(centerX + offsetMiniX);
				int drawY = (int)(centerY + offsetMiniY);

				if (drawX >= innerX + 3 && drawX <= innerX + innerW - 3 &&
					drawY >= innerY + 3 && drawY <= innerY + innerH - 3)
				{
					SDL_Rect dot = { drawX - 3, drawY - 3, 6, 6 };
					// High-contrast neon orange color for the Slingshot!
					render.DrawRectangle(dot, 255, 120, 0, 255, true, false);
					SDL_Rect dotOutline = { drawX - 4, drawY - 4, 8, 8 };
					render.DrawRectangle(dotOutline, 255, 255, 255, 200, false, false);
				}
			}

			// --- Draw Player Indicator (Blinking glowing yellow dot in the exact center) ---
			float gameTime = (float)SDL_GetTicks();
			Uint8 pAlpha = (Uint8)(170 + 85.0f * sinf(gameTime * 0.007f));
			SDL_Rect playerDot = { centerX - 3, centerY - 3, 6, 6 };
			render.DrawRectangle(playerDot, 255, 235, 30, pAlpha, true, false);
			SDL_Rect playerBorder = { centerX - 4, centerY - 4, 8, 8 };
			render.DrawRectangle(playerBorder, 255, 255, 255, 255, false, false);

			// --- Draw Ornate Frame Overlay right on top ---
			int frameW = 0, frameH = 0;
			Engine::GetInstance().textures->GetSize(texMinimapFrame_, frameW, frameH);
			if (frameW > 0)
			{
				SDL_Rect frameSec = { 0, 0, frameW, frameH };
				float scaleFrame = 220.0f / (float)frameW;
				render.DrawTexture(texMinimapFrame_, miniX, miniY, &frameSec, 0.0f, -1.0f, 0, INT_MAX, INT_MAX, SDL_FLIP_NONE, scaleFrame);
			}
		}
	}

	// --- Boss health bar ---
	{
		int winW = 0, winH = 0;
		Engine::GetInstance().window->GetWindowSize(winW, winH);
		DrawBossHUD(winW, winH);
	}

	// --- Game Over Screen ---
	if (isGameOver_) {
		int winW = 0, winH = 0;
		Engine::GetInstance().window->GetWindowSize(winW, winH);

		// Draw the dark angular panel background with built-in blurred vignette and "GAME OVER" text scaled to fill the window (with soft alpha fade-in)
		if (texGameOverScreenBase_) {
			float progress = gameOverFadeTimer_ / 1.2f; // Fade in over 1.2 seconds
			if (progress > 1.0f) progress = 1.0f;
			Uint8 alphaVal = (Uint8)(progress * 255.0f);

			SDL_SetTextureScaleMode(texGameOverScreenBase_, SDL_SCALEMODE_LINEAR);
			Engine::GetInstance().render->DrawTextureAlphaF(texGameOverScreenBase_, 0.0f, 0.0f, (float)winW, (float)winH, alphaVal);
		}
	}

	// --- Checkpoint Save Notification ---
	if (!isPaused_ && checkpointSaveTimer_ > 0.0f) {
		checkpointSaveTimer_ -= Engine::GetInstance().GetDt();
		if (texCheckpointSaved_) {
			float tw, th;
			SDL_GetTextureSize(texCheckpointSaved_, &tw, &th);
			int winW = 0, winH = 0;
			Engine::GetInstance().window->GetWindowSize(winW, winH);

			float drawX = 40.0f;
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
		render3.DrawMenuTextCentered("You can't do this; you need an object", { ncpX, ncpY, ncpW, ncpH }, ncpColor, 0.35f);
	}

	// --- No bear notification ---
	if (noBearNotifTimer_ > 0.0f) {
		noBearNotifTimer_ -= Engine::GetInstance().GetDt();

		int winWBear = 0, winHBear = 0;
		Engine::GetInstance().window->GetWindowSize(winWBear, winHBear);
		auto& renderBear = *Engine::GetInstance().render;

		Uint8 alphaBear = 255;
		if (noBearNotifTimer_ < 800.0f)
			alphaBear = (Uint8)(255.0f * (noBearNotifTimer_ / 800.0f));

		// Panel dimensions
		const int nbW = 380;
		const int nbH = 36;
		const int nbX = (winWBear - nbW) / 2;
		const int nbY = 40;

		// Black filled panel
		SDL_Rect nbPanel = { nbX, nbY, nbW, nbH };
		renderBear.DrawRectangle(nbPanel, 0, 0, 0, alphaBear, true, false);

		// White border (outline) — 2px
		SDL_Rect nbBorder = { nbX - 2, nbY - 2, nbW + 4, nbH + 4 };
		renderBear.DrawRectangle(nbBorder, 255, 255, 255, alphaBear, false, false);

		// Text centered inside the panel (scaled down)
		SDL_Color nbColor = { 255, 255, 255, alphaBear };
		const char* msg = bearNotifHasStuffed_ ? "Press 3 to equip the Teddy Bear!" : "You need to find the Teddy Bear first!";
		renderBear.DrawMenuTextCentered(
			msg,
			{ nbX, nbY, nbW, nbH },
			nbColor,
			0.35f
		);
	}

	// --- Slingshot pickup notification ---
	if (slingshotNotifTimer_ > 0.0f) {
		slingshotNotifTimer_ -= Engine::GetInstance().GetDt();

		int winW4 = 0, winH4 = 0;
		Engine::GetInstance().window->GetWindowSize(winW4, winH4);
		auto& render4 = *Engine::GetInstance().render;

		Uint8 alpha4 = 255;
		if (slingshotNotifTimer_ < 800.0f)
			alpha4 = (Uint8)(255.0f * (slingshotNotifTimer_ / 800.0f));

		// Panel dimensions
		const int spW = 340;
		const int spH = 36;
		const int spX = (winW4 - spW) / 2;
		const int spY = 40;

		// Black filled panel
		SDL_Rect spPanel = { spX, spY, spW, spH };
		render4.DrawRectangle(spPanel, 0, 0, 0, alpha4, true, false);

		// White border (outline)
		SDL_Rect spBorder = { spX - 2, spY - 2, spW + 4, spH + 4 };
		render4.DrawRectangle(spBorder, 255, 255, 255, alpha4, false, false);

		// Text
		SDL_Color spColor = { 255, 255, 255, alpha4 };
		render4.DrawMenuTextCentered(
			"Slingshot found! Left click to aim and fire",
			{ spX, spY, spW, spH },
			spColor,
			0.35f
		);
	}

	// --- Stuffed Animal pickup notification ---
	if (stuffedAnimalNotifTimer_ > 0.0f) {
		stuffedAnimalNotifTimer_ -= Engine::GetInstance().GetDt();

		int winW = 0, winH = 0;
		Engine::GetInstance().window->GetWindowSize(winW, winH);
		auto& render = *Engine::GetInstance().render;

		Uint8 alpha = 255;
		if (stuffedAnimalNotifTimer_ < 800.0f)
			alpha = (Uint8)(255.0f * (stuffedAnimalNotifTimer_ / 800.0f));

		// Panel dimensions
		const int spW = 340;
		const int spH = 36;
		const int spX = (winW - spW) / 2;
		const int spY = 80;

		// Black filled panel
		SDL_Rect spPanel = { spX, spY, spW, spH };
		render.DrawRectangle(spPanel, 0, 0, 0, alpha, true, false);

		// White border (outline)
		SDL_Rect spBorder = { spX - 2, spY - 2, spW + 4, spH + 4 };
		render.DrawRectangle(spBorder, 255, 255, 255, alpha, false, false);

		// Text
		SDL_Color spColor = { 255, 255, 255, alpha };
		render.DrawMenuTextCentered(
			"Teddy Bear found! Press O to Summon",
			{ spX, spY, spW, spH },
			spColor,
			0.35f
		);
	}

	if (screenDamageTimer_ > 0.0f && texDamageVignette_) {
		int winW = 0, winH = 0;
		Engine::GetInstance().window->GetWindowSize(winW, winH);
		Uint8 alpha = (Uint8)(255.0f * (screenDamageTimer_ / 1000.0f));
		Engine::GetInstance().render->DrawTextureAlphaF(texDamageVignette_, 0.0f, 0.0f, (float)winW, (float)winH, alpha);
	}

	if (showMapViewer_) {
		int winW = 0, winH = 0;
		Engine::GetInstance().window->GetWindowSize(winW, winH);
		DrawMapViewer(winW, winH);
	}
	else if (isPaused_ && !showInventory_) {
		DrawPauseMenu();
	}

	// Make sure the game is hidden while videos are playing
	if (endGameVideoActive_ || creditsVideoActive_ || bossDeathVideoActive_) {
		int winW = 0, winH = 0;
		Engine::GetInstance().window->GetWindowSize(winW, winH);
		SDL_Rect fullscreen = { 0, 0, winW, winH };
		Engine::GetInstance().render->DrawRectangle(fullscreen, 0, 0, 0, 255, true, false);
	}
} 
		// ── Inventory ─────────────────────────────────────────────────────────────────
void Scene::DrawInventory(int winW, int winH)
{
	auto& render = *Engine::GetInstance().render;

	// 1. Draw the split-screen background template texture
	if (texInventoryBg_)
	{
		render.DrawTextureAlpha(texInventoryBg_, 0, 0, winW, winH, 255);
	}
	else
	{
		// Fallback dim background if texture not loaded
		SDL_Rect dimBg = { 0, 0, winW, winH };
		render.DrawRectangle(dimBg, 5, 8, 15, 230, true, false);
	}

	// 2. Compute left section (Abilities) layout & draw the active Diamond Graphic
	int diamSize = (winW < 1280) ? 440 : 520;
	int diamX = (winW / 2 - diamSize) / 2;
	int diamY = (winH - diamSize) / 2;

	bool blanket = player && player->HasBlanket();
	bool slingshot = player && player->HasSlingshot();
	bool stuffed = player && player->HasStuffedAnimal();

	SDL_Texture* diamTex = texAbilitiesLocked_;
	if (blanket && slingshot && stuffed)
	{
		diamTex = texAbilitiesAll_;
	}
	else if (blanket && slingshot)
	{
		diamTex = texAbilitiesBlanketSling_;
	}
	else if (blanket)
	{
		diamTex = texAbilitiesBlanket_;
	}
	else
	{
		diamTex = texAbilitiesLocked_;
	}

	if (diamTex)
	{
		render.DrawTextureAlpha(diamTex, diamX, diamY, diamSize, diamSize, 255);
	}

	// 2b. Compute exact slot centers on the wheel texture
	// In the wheel, the slots form an inner diamond with a 0.15 radius (0.35f to 0.65f)
	int topX = diamX + diamSize / 2;
	int topY = diamY + (int)(diamSize * 0.35f);

	int leftX = diamX + (int)(diamSize * 0.35f);
	int leftY = diamY + diamSize / 2;

	int rightX_diam = diamX + (int)(diamSize * 0.65f);
	int rightY_diam = diamY + diamSize / 2;

	int bottomX = diamX + diamSize / 2;
	int bottomY = diamY + (int)(diamSize * 0.65f);

	// Dynamic icon size (100px when diamSize=520, which covers the lock diamond completely!)
	int iconSize = (int)(diamSize * 0.192f);

	// Mouse detection based on the true slot centers!
	auto& input = Engine::GetInstance().input;
	Vector2D mousePos = input->GetMousePosition();

	// Hover radius is half of iconSize plus some padding
	int hoverRadius = iconSize / 2;

	bool hoverTop = (mousePos.getX() >= topX - hoverRadius && mousePos.getX() <= topX + hoverRadius &&
					 mousePos.getY() >= topY - hoverRadius && mousePos.getY() <= topY + hoverRadius);

	bool hoverLeft = (mousePos.getX() >= leftX - hoverRadius && mousePos.getX() <= leftX + hoverRadius &&
					  mousePos.getY() >= leftY - hoverRadius && mousePos.getY() <= leftY + hoverRadius);

	bool hoverRight = (mousePos.getX() >= rightX_diam - hoverRadius && mousePos.getX() <= rightX_diam + hoverRadius &&
					   mousePos.getY() >= rightY_diam - hoverRadius && mousePos.getY() <= rightY_diam + hoverRadius);

	bool hoverBottom = (mousePos.getX() >= bottomX - hoverRadius && mousePos.getX() <= bottomX + hoverRadius &&
						mousePos.getY() >= bottomY - hoverRadius && mousePos.getY() <= bottomY + hoverRadius);

	// Gamepad navigation for inventory diamond
	if (input->IsGamepadConnected())
	{
		static Uint64 lastTick = SDL_GetTicks();
		Uint64 currentTick = SDL_GetTicks();
		float frameTime = (float)(currentTick - lastTick);
		lastTick = currentTick;

		static float invRepeatTimer = 0.0f;
		if (invRepeatTimer > 0.0f) invRepeatTimer -= frameTime;

		bool pressLeft = input->GetGamepadButton(SDL_GAMEPAD_BUTTON_DPAD_LEFT) == KEY_DOWN ||
		                 (input->GetLeftStickX() < -0.5f && invRepeatTimer <= 0.0f);
		bool pressRight = input->GetGamepadButton(SDL_GAMEPAD_BUTTON_DPAD_RIGHT) == KEY_DOWN ||
		                  (input->GetLeftStickX() > 0.5f && invRepeatTimer <= 0.0f);
		bool pressUp = input->GetGamepadButton(SDL_GAMEPAD_BUTTON_DPAD_UP) == KEY_DOWN ||
		               (input->GetLeftStickY() < -0.5f && invRepeatTimer <= 0.0f);

		if (pressLeft && blanket)
		{
			inventorySel_ = 0;
			invRepeatTimer = 250.0f;
			Engine::GetInstance().audio->PlayFx(menuClickFxId, 0, true);
		}
		else if (pressUp && slingshot)
		{
			inventorySel_ = 1;
			invRepeatTimer = 250.0f;
			Engine::GetInstance().audio->PlayFx(menuClickFxId, 0, true);
		}
		else if (pressRight && stuffed)
		{
			inventorySel_ = 2;
			invRepeatTimer = 250.0f;
			Engine::GetInstance().audio->PlayFx(menuClickFxId, 0, true);
		}

		// Equip on A/SOUTH press
		if (input->GetGamepadButton(SDL_GAMEPAD_BUTTON_SOUTH) == KEY_DOWN)
		{
			if (inventorySel_ == 0 && blanket)
			{
				player->SetEquippedItem(Player::EquippedItem::BLANKET);
				Engine::GetInstance().audio->PlayFx(menuClickFxId, 0, true);
			}
			else if (inventorySel_ == 1 && slingshot)
			{
				player->SetEquippedItem(Player::EquippedItem::SLINGSHOT);
				Engine::GetInstance().audio->PlayFx(menuClickFxId, 0, true);
			}
			else if (inventorySel_ == 2 && stuffed)
			{
				player->SetEquippedItem(Player::EquippedItem::STUFFED_ANIMAL);
				Engine::GetInstance().audio->PlayFx(menuClickFxId, 0, true);
			}
		}
	}

	// Handle item equipping clicks!
	if (input->GetMouseButtonDown(SDL_BUTTON_LEFT) == KEY_DOWN)
	{
		if (hoverLeft && blanket)
		{
			player->SetEquippedItem(Player::EquippedItem::BLANKET);
			Engine::GetInstance().audio->PlayFx(menuClickFxId, 0, true);
		}
		else if (hoverTop && slingshot)
		{
			player->SetEquippedItem(Player::EquippedItem::SLINGSHOT);
			Engine::GetInstance().audio->PlayFx(menuClickFxId, 0, true);
		}
		else if (hoverRight && stuffed)
		{
			player->SetEquippedItem(Player::EquippedItem::STUFFED_ANIMAL);
			Engine::GetInstance().audio->PlayFx(menuClickFxId, 0, true);
		}
	}

	// Draw hover overlays inside the diamond (feedback for active slots)
	if ((hoverLeft || (input->IsGamepadConnected() && inventorySel_ == 0)) && blanket)
	{
		SDL_Rect hov = { leftX - iconSize/2 - 2, leftY - iconSize/2 - 2, iconSize + 4, iconSize + 4 };
		render.DrawRectangle(hov, 255, 255, 255, 120, false, false);
	}
	if ((hoverTop || (input->IsGamepadConnected() && inventorySel_ == 1)) && slingshot)
	{
		SDL_Rect hov = { topX - iconSize/2 - 2, topY - iconSize/2 - 2, iconSize + 4, iconSize + 4 };
		render.DrawRectangle(hov, 255, 255, 255, 120, false, false);
	}
	if ((hoverRight || (input->IsGamepadConnected() && inventorySel_ == 2)) && stuffed)
	{
		SDL_Rect hov = { rightX_diam - iconSize/2 - 2, rightY_diam - iconSize/2 - 2, iconSize + 4, iconSize + 4 };
		render.DrawRectangle(hov, 255, 255, 255, 120, false, false);
	}

	// Draw active equipping golden frame overlays inside the diamond
	Player::EquippedItem eq = player ? player->GetEquippedItem() : Player::EquippedItem::NONE;
	if (eq != Player::EquippedItem::NONE)
	{
		int eqX = 0, eqY = 0;
		if (eq == Player::EquippedItem::BLANKET && blanket) { eqX = leftX; eqY = leftY; }
		else if (eq == Player::EquippedItem::SLINGSHOT && slingshot) { eqX = topX; eqY = topY; }
		else if (eq == Player::EquippedItem::STUFFED_ANIMAL && stuffed) { eqX = rightX_diam; eqY = rightY_diam; }

		if (eqX != 0)
		{
			SDL_Rect eqBorder = { eqX - iconSize/2 - 3, eqY - iconSize/2 - 3, iconSize + 6, iconSize + 6 };
			render.DrawRectangle(eqBorder, 255, 195, 0, 255, false, false);
			SDL_Rect eqBorder2 = { eqX - iconSize/2 - 5, eqY - iconSize/2 - 5, iconSize + 10, iconSize + 10 };
			render.DrawRectangle(eqBorder2, 255, 215, 80, 160, false, false);
		}
	}



	// =========================================================================
	//  ANCHORED MEMORIES (Right column)
	// =========================================================================
	float cX = 3.0f * (float)winW / 4.0f;
	float cY = (float)winH / 2.0f + 10.0f;

	float scale = (winW < 1280) ? 0.58f : 0.72f;

	// Fragment sizes
	float w1 = 580.0f * scale, h1 = 360.0f * scale; // Fragment 1 landscape
	float w2 = 360.0f * scale, h2 = 580.0f * scale; // Fragment 2 portrait
	float w3 = 580.0f * scale, h3 = 360.0f * scale; // Fragment 3 landscape

	// Layout matching reference: tight puzzle cluster with overlapping pieces
	// Fragment 1 (top-left): landscape, upper-left of cluster
	float x1 = cX - w1 * 0.55f;
	float y1 = cY - h1 * 0.95f;

	// Fragment 2 (right): portrait, overlaps fragment 1 on the right side
	float x2 = cX + w2 * 0.15f;
	float y2 = cY - h2 * 0.75f;

	// Fragment 3 (bottom): landscape, below and overlapping both
	float x3 = cX - w3 * 0.45f;
	float y3 = cY + h3 * 0.05f;

	SDL_FRect rect1 = { x1, y1, w1, h1 };
	SDL_FRect rect2 = { x2, y2, w2, h2 };
	SDL_FRect rect3 = { x3, y3, w3, h3 };

	// Mouse hover checks
	Vector2D mouseP = input->GetMousePosition();
	float mx = mouseP.getX();
	float my = mouseP.getY();

	// Hover logic (ignore if showMemoryViewer_ is active)
	bool hover1 = !showMemoryViewer_ && (mx >= rect1.x && mx <= rect1.x + rect1.w && my >= rect1.y && my <= rect1.y + rect1.h);
	bool hover2 = !showMemoryViewer_ && (mx >= rect2.x && mx <= rect2.x + rect2.w && my >= rect2.y && my <= rect2.y + rect2.h);
	bool hover3 = !showMemoryViewer_ && (mx >= rect3.x && mx <= rect3.x + rect3.w && my >= rect3.y && my <= rect3.y + rect3.h);

	// Update timers
	float invDt = Engine::GetInstance().GetDt();
	const float invFadeSpeed = 0.005f;

	if (hover1) memoryHoverTimers_[0] = std::min(1.0f, memoryHoverTimers_[0] + invFadeSpeed * invDt);
	else        memoryHoverTimers_[0] = std::max(0.0f, memoryHoverTimers_[0] - invFadeSpeed * invDt);

	if (hover2) memoryHoverTimers_[1] = std::min(1.0f, memoryHoverTimers_[1] + invFadeSpeed * invDt);
	else        memoryHoverTimers_[1] = std::max(0.0f, memoryHoverTimers_[1] - invFadeSpeed * invDt);

	if (hover3)
	{
		memoryHoverTimers_[2] += invFadeSpeed * invDt;
		if (memoryHoverTimers_[2] > 3.0f) memoryHoverTimers_[2] -= 3.0f;
	}
	else
	{
		// Smoothly return back to page 0 (normal phase)
		if (memoryHoverTimers_[2] > 0.0f)
		{
			if (memoryHoverTimers_[2] > 1.5f) {
				memoryHoverTimers_[2] += invFadeSpeed * invDt;
				if (memoryHoverTimers_[2] >= 3.0f) memoryHoverTimers_[2] = 0.0f;
			} else {
				memoryHoverTimers_[2] = std::max(0.0f, memoryHoverTimers_[2] - invFadeSpeed * invDt);
			}
		}
	}

	// Trigger full screen click handlers
	if (input->GetMouseButtonDown(SDL_BUTTON_LEFT) == KEY_DOWN && !showMemoryViewer_)
	{
		if (hover1)
		{
			showMemoryViewer_ = true;
			activeMemoryIndex_ = 0;
			activeMemoryPage_ = 0;
			Engine::GetInstance().audio->PlayFx(menuClickFxId, 0, true);
		}
		else if (hover2)
		{
			showMemoryViewer_ = true;
			activeMemoryIndex_ = 1;
			activeMemoryPage_ = 0;
			Engine::GetInstance().audio->PlayFx(menuClickFxId, 0, true);
		}
		else if (hover3)
		{
			showMemoryViewer_ = true;
			activeMemoryIndex_ = 2;
			activeMemoryPage_ = 0;
			Engine::GetInstance().audio->PlayFx(menuClickFxId, 0, true);
		}
	}

	// --- 1. RENDER FRAGMENTS ---
	// Fragment 1 (Top-Left)
	if (texMemoria1Base_ && texMemoria1N1_ && texMemoria1N2_)
	{
		render.DrawTextureAlphaF(texMemoria1Base_, rect1.x, rect1.y, rect1.w, rect1.h, 255);
		Uint8 alphaN1 = (Uint8)((1.0f - memoryHoverTimers_[0]) * 255.0f);
		Uint8 alphaN2 = (Uint8)(memoryHoverTimers_[0] * 255.0f);

		if (alphaN1 > 0) render.DrawTextureAlphaF(texMemoria1N1_, rect1.x, rect1.y, rect1.w, rect1.h, alphaN1);
		if (alphaN2 > 0) render.DrawTextureAlphaF(texMemoria1N2_, rect1.x, rect1.y, rect1.w, rect1.h, alphaN2);
	}

	// Fragment 2 (Top-Right)
	if (texMemoria2Base_ && texMemoria2N1_ && texMemoria2N2_)
	{
		render.DrawTextureAlphaF(texMemoria2Base_, rect2.x, rect2.y, rect2.w, rect2.h, 255);
		Uint8 alphaN1 = (Uint8)((1.0f - memoryHoverTimers_[1]) * 255.0f);
		Uint8 alphaN2 = (Uint8)(memoryHoverTimers_[1] * 255.0f);

		if (alphaN1 > 0) render.DrawTextureAlphaF(texMemoria2N1_, rect2.x, rect2.y, rect2.w, rect2.h, alphaN1);
		if (alphaN2 > 0) render.DrawTextureAlphaF(texMemoria2N2_, rect2.x, rect2.y, rect2.w, rect2.h, alphaN2);
	}

	// Fragment 3 (Bottom)
	if (texMemoria3Base_ && texMemoria3N1_ && texMemoria3N2_ && texMemoria3N3_)
	{
		render.DrawTextureAlphaF(texMemoria3Base_, rect3.x, rect3.y, rect3.w, rect3.h, 255);

		float t = memoryHoverTimers_[2];
		float a1 = 255.0f, a2 = 0.0f, a3 = 0.0f;
		if (t > 0.0f)
		{
			if (t <= 1.0f) {
				a1 = (1.0f - t) * 255.0f;
				a2 = t * 255.0f;
				a3 = 0.0f;
			}
			else if (t <= 2.0f) {
				float progress = t - 1.0f;
				a1 = 0.0f;
				a2 = (1.0f - progress) * 255.0f;
				a3 = progress * 255.0f;
			}
			else if (t <= 3.0f) {
				float progress = t - 2.0f;
				a1 = progress * 255.0f;
				a2 = 0.0f;
				a3 = (1.0f - progress) * 255.0f;
			}
		}

		if (a1 > 0) render.DrawTextureAlphaF(texMemoria3N1_, rect3.x, rect3.y, rect3.w, rect3.h, (Uint8)a1);
		if (a2 > 0) render.DrawTextureAlphaF(texMemoria3N2_, rect3.x, rect3.y, rect3.w, rect3.h, (Uint8)a2);
		if (a3 > 0) render.DrawTextureAlphaF(texMemoria3N3_, rect3.x, rect3.y, rect3.w, rect3.h, (Uint8)a3);
	}

	// Outline/glow hover overlays for memory fragments
	if (hover1) render.DrawRectangle({ (int)rect1.x, (int)rect1.y, (int)rect1.w, (int)rect1.h }, 255, 255, 255, 60, false, false);
	if (hover2) render.DrawRectangle({ (int)rect2.x, (int)rect2.y, (int)rect2.w, (int)rect2.h }, 255, 255, 255, 60, false, false);
	if (hover3) render.DrawRectangle({ (int)rect3.x, (int)rect3.y, (int)rect3.w, (int)rect3.h }, 255, 255, 255, 60, false, false);


	// --- 2. FULLSCREEN VIEWER MODE OVERLAY ---
	if (showMemoryViewer_ && activeMemoryIndex_ != -1)
	{
		// Render dim backdrop
		SDL_Rect overlay = { 0, 0, winW, winH };
		render.DrawRectangle(overlay, 0, 0, 0, 220, true, false);

		// Calculate frame metrics
		float scaleX = (float)winW / 1920.0f;
		float scaleY = (float)winH / 1080.0f;
		float fScale = std::min(scaleX, scaleY) * 0.95f;
		float frameW = 1920.0f * fScale;
		float frameH = 1080.0f * fScale;
		float frameX = (winW - frameW) / 2.0f;
		float frameY = (winH - frameH) / 2.0f;
		float centerX = frameX + frameW / 2.0f;
		float centerY = frameY + frameH / 2.0f;

		// Select texture and original dimensions
		SDL_Texture* activeTex = nullptr;
		int imgW = 1920;
		int imgH = 1080;
		int pageCount = 0;

		if (activeMemoryIndex_ == 0) // Memory 1
		{
			pageCount = 2;
			if (activeMemoryPage_ == 0) { activeTex = texMemoria1Full1_; imgW = 1128; imgH = 716; }
			else                       { activeTex = texMemoria1Full2_; imgW = 1005; imgH = 544; }
		}
		else if (activeMemoryIndex_ == 1) // Memory 2
		{
			pageCount = 2;
			if (activeMemoryPage_ == 0) activeTex = texMemoria2Full1_;
			else                       activeTex = texMemoria2Full2_;
			imgW = 1920; imgH = 1080;
		}
		else if (activeMemoryIndex_ == 2) // Memory 3
		{
			pageCount = 3;
			if (activeMemoryPage_ == 0)      activeTex = texMemoria3Full1_;
			else if (activeMemoryPage_ == 1) activeTex = texMemoria3Full2_;
			else                            activeTex = texMemoria3Full3_;
			imgW = 1920; imgH = 1080;
		}

		// Draw Memory Artwork centered inside the frame coordinates
		if (activeTex)
		{
			float drawW = (float)imgW * fScale;
			float drawH = (float)imgH * fScale;
			float drawX = centerX - drawW / 2.0f;
			float drawY = centerY - drawH / 2.0f;
			render.DrawTextureAlphaF(activeTex, drawX, drawY, drawW, drawH, 255);
		}

		// Draw Ornate Frame overlay on top of artwork
		if (texMemoriaFrameFullScreen_)
		{
			render.DrawTextureAlphaF(texMemoriaFrameFullScreen_, frameX, frameY, frameW, frameH, 255);
		}

		// Draw pagination dots at the bottom of the frame
		if (pageCount > 1)
		{
			int dotSpacing = 24;
			float startDotX = centerX - ((pageCount - 1) * dotSpacing) / 2.0f;
			float dotY = frameY + frameH - 60.0f * fScale;

			for (int i = 0; i < pageCount; i++)
			{
				SDL_Rect dotRect = { (int)(startDotX + i * dotSpacing - 5), (int)(dotY - 5), 10, 10 };
				if (i == activeMemoryPage_)
				{
					// Active dot: Glowing gold
					render.DrawRectangle({dotRect.x-1, dotRect.y-1, dotRect.w+2, dotRect.h+2}, 255, 215, 0, 180, true, false);
					render.DrawRectangle(dotRect, 255, 255, 255, 255, true, false);
				}
				else
				{
					// Inactive dot: Semi-translucent grey
					render.DrawRectangle(dotRect, 120, 130, 140, 150, true, false);
				}
			}
		}

		// Visual arrow overlay hints on left/right edges for page switching
		SDL_Color arrowColor = { 180, 210, 240, 180 };
		if (pageCount > 1)
		{
			render.DrawText("<", (int)(frameX + 35.0f * fScale), (int)(centerY - 20.0f), 24, 40, arrowColor);
			render.DrawText(">", (int)(frameX + frameW - 45.0f * fScale), (int)(centerY - 20.0f), 24, 40, arrowColor);
		}

		// Small helper instructions text in footer
		SDL_Rect hintArea = { (int)frameX, (int)(frameY + frameH - 35.0f * fScale), (int)frameW, 20 };
		render.DrawMenuTextCentered("Click bordes o Flechas para cambiar de pagina  |  ESC / Click fuera para cerrar", hintArea, { 180, 200, 220, 180 }, 0.28f);
	}

	// 5. Instructions Footer Overlay
	SDL_Rect footer = { 0, winH - 45, winW, 30 };
	if (Engine::GetInstance().input->IsGamepadConnected())
	{
		render.DrawMenuTextCentered("Clic en slots para Equipar  |  ARRIBA o 'I' para cerrar", footer, { 180, 210, 240, 200 }, 0.35f);
	}
	else
	{
		render.DrawMenuTextCentered("Clic en slots para Equipar (Teclas 1, 2, 3 en juego)  |  'I' para cerrar", footer, { 180, 210, 240, 200 }, 0.35f);
	}
}

// ============================================================================
//  Map viewer
// ============================================================================

void Scene::DrawMapViewer(int winW, int winH)
{
	// Check for Map Reveal Cheat/Purchase Input
	auto& mapInput = *Engine::GetInstance().input;
	if (mapInput.GetKey(SDL_SCANCODE_M) == KEY_DOWN ||
		mapInput.GetGamepadButton(SDL_GAMEPAD_BUTTON_WEST) == KEY_DOWN)
	{
		mapRevealed_[currentMapFile_] = true;
		LOG("Map purchased/revealed for %s", currentMapFile_.c_str());
	}

	auto& render = *Engine::GetInstance().render;
	auto& map = *Engine::GetInstance().map;
	int   scale = Engine::GetInstance().window->GetScale();

	SDL_Rect fullBg = { 0, 0, winW, winH };
	render.DrawRectangle(fullBg, 5, 8, 15, 240, true, false);

	// Title bar — tall enough so text isn't clipped
	const int titleBarH = 60;
	SDL_Rect titleBar = { 0, 0, winW, titleBarH };
	render.DrawRectangle(titleBar, 10, 16, 30, 255, true, false);
	SDL_Rect titleAccent = { 0, titleBarH - 2, winW, 2 };
	render.DrawRectangle(titleAccent, 80, 140, 200, 255, true, false);
	render.DrawMenuTextCentered("MAP", { 0, 10, winW, 40 }, { 180, 210, 240, 255 });

	// Bottom hint bar — taller to avoid text clipping
	const int hintBarH = 44;
	SDL_Rect hintBar = { 0, winH - hintBarH, winW, hintBarH };
	render.DrawRectangle(hintBar, 10, 16, 30, 220, true, false);
	if (Engine::GetInstance().input->IsGamepadConnected())
		render.DrawText("L-Stick: pan  |  R2: zoom in  |  L2: zoom out  |  West: buy map  |  Touchpad: back",
			winW / 2 - 340, winH - hintBarH + 12, 0, 0, { 120, 160, 200, 200 });
	else
		render.DrawText("Left click + drag: pan  |  +/-: zoom  |  M: buy map  |  ESC: back",
			winW / 2 - 290, winH - hintBarH + 12, 0, 0, { 120, 160, 200, 200 });

	// Map viewport area — inset between the bars with a small margin
	const int viewX = 10;
	const int viewY = titleBarH + 4;
	const int viewW = winW - 20;
	const int viewH = winH - viewY - hintBarH - 4;

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

	// -- Clamp map offset to map boundaries -------------------------------
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
		if (!drawProp || !drawProp->value) continue;

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

	// Draw Fog of War overlay
	if (!mapRevealed_[currentMapFile_])
	{
		Vector2D mapSize = map.GetMapSizeInPixels();
		int cols = (int)ceilf(mapSize.getX() / 320.0f);
		int rows = (int)ceilf(mapSize.getY() / 180.0f);

		for (int cy = 0; cy < rows; cy++)
		{
			for (int cx = 0; cx < cols; cx++)
			{
				if (visitedCells_[currentMapFile_].find({ cx, cy }) == visitedCells_[currentMapFile_].end())
				{
					float worldX = cx * 320.0f;
					float worldY = cy * 180.0f;
					float worldW = 320.0f;
					float worldH = 180.0f;

					float screenX = (float)viewX + mapViewOffsetX_ + worldX * mapViewZoom_;
					float screenY = (float)viewY + mapViewOffsetY_ + worldY * mapViewZoom_;
					float screenW = worldW * mapViewZoom_;
					float screenH = worldH * mapViewZoom_;

					// Clip/cull
					if (screenX + screenW < (float)viewX || screenX > (float)viewX + (float)viewW) continue;
					if (screenY + screenH < (float)viewY || screenY > (float)viewY + (float)viewH) continue;

					SDL_Rect cellRect = { (int)screenX, (int)screenY, (int)screenW, (int)screenH };
					render.DrawRectangle(cellRect, 5, 8, 15, 230, true, false);
				}
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

		bool spriteNativeRight = (src.y / 128 == 3 || src.y / 128 == 1 || src.y / 128 == 0);
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

// -- Pause menu helpers --------------------------------------------------------

void Scene::LoadPauseMenuButtons()
{
	texPauseBackground_ = Engine::GetInstance().textures->Load("assets/textures/Menu/UI_Pause_Menu_base+background.png");
	texPauseButtonWhite_ = Engine::GetInstance().textures->Load("assets/textures/Menu/UI_Pause_Menu_button_white.png");
	texPauseButtonBlack_ = Engine::GetInstance().textures->Load("assets/textures/Menu/UI_Pause_Menu_button_black.png");
	texButtonFragmented_ = Engine::GetInstance().textures->Load("assets/textures/Menu/UI_Button_white_fragmented.png");
	texSettingsBase_ = Engine::GetInstance().textures->Load("assets/textures/UI/UI_Settings_Main_Menu_FIXED.png");
	texSettingsPause_ = Engine::GetInstance().textures->Load("assets/textures/UI/UI_Settings_Main_Menu_FIXED.png");

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

	// Create Back button for the Options Panel
	SDL_Rect backOptPos = { winW / 2 - btnW / 2, winH / 2 + 185, btnW, btnH }; // Lowered by 25 pixels
	auto btnOptBack = Engine::GetInstance().uiManager->CreateUIElement(UIElementType::BUTTON, BTN_PAUSE_OPT_BACK, "back", backOptPos, this);
	if (btnOptBack) {
		btnOptBack->isVisible = false;
		btnOptBack->state = UIElementState::DISABLED;
		if (texPauseButtonWhite_) btnOptBack->SetTexture(texPauseButtonWhite_);
		if (texButtonFragmented_) btnOptBack->SetHoverTexture(texButtonFragmented_);
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
	if (!visible) {
		gameOverFadeTimer_ = 0.0f;
	}
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

	for (int i = 0; i < MAX_HEALTH_SLOTS; i++) {
		animHealth_[i].Reset();
	}

	int maxHP = player ? player->maxHealth : healthSlotCount_;
	int lostHP = maxHP - health;

	for (int i = 0; i < lostHP; i++) {
		if (i >= 0 && i < healthSlotCount_) {
			animHealth_[i].SetFinished();
		}
	}
}

void Scene::ShowNoCapeNotification()
{
	noCapeNotifTimer_ = 3000.0f;
}

void Scene::ShowNoBearNotification(bool hasStuffedAnimal)
{
	noBearNotifTimer_ = 3000.0f;
	bearNotifHasStuffed_ = hasStuffedAnimal;
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

	// Render texture using the original 840.0f width, but make it significantly taller (780.0f height)
	const float panelScale = (float)winW * 0.50f / 840.0f;
	const int panelImgW = (int)(840.0f * panelScale);
	const int panelImgH = (int)(780.0f * panelScale);
	const int panelImgX = (winW - panelImgW) / 2;
	const int panelImgY = (winH - panelImgH) / 2;

	// Shift the background texture up slightly so texts sit better in the white space
	const int panelRenderY = panelImgY - (int)(100.0f * panelScale);

	if (texSettingsPause_) {
		SDL_FRect pRect = { (float)panelImgX, (float)panelRenderY, (float)panelImgW, (float)panelImgH };
		SDL_RenderTexture(render.renderer, texSettingsPause_, nullptr, &pRect);
	}

	// Track area for sliders and controls — positioned to align with panel icons
	const int trackX = panelImgX + (int)(panelImgW * 0.44f);
	const int trackW = (int)(panelImgW * 0.42f);

	// 3 rows evenly distributed
	const int row0Y = panelImgY + (int)(panelImgH * 0.33f); // MUSIC
	const int row1Y = panelImgY + (int)(panelImgH * 0.49f); // SOUNDS
	const int row2Y = panelImgY + (int)(panelImgH * 0.62f); // DISPLAY

	// Handle input and close menu if Back clicked/button pressed
	if (HandleVolumeSliderInput(trackX, trackW, row0Y, row1Y, row2Y)) {
		showPauseOptions_ = false;
		SetPauseOptionsPanelVisible(false);
		return;
	}

	SDL_Color labelColor = { 40, 55, 70, 255 };
	SDL_Color valColor   = { 30, 45, 60, 255 };

	// --- Row 0: MUSIC ---
	render.DrawMenuTextCentered("MUSIC", { trackX, row0Y, trackW / 2, 20 }, labelColor);
	char vol[8];
	snprintf(vol, sizeof(vol), "%d", static_cast<int>(musicVolume_ * 100.0f));
	render.DrawMenuTextCentered(vol, { trackX + trackW / 2, row0Y, trackW / 2, 20 }, valColor);

	SDL_Rect mBarBg = { trackX, row0Y + 25, trackW, 6 };
	render.DrawRectangle(mBarBg, 10, 15, 25, 255, true, false);
	int mFill = static_cast<int>(static_cast<float>(trackW) * musicVolume_);
	SDL_Rect mBarFill = { trackX, row0Y + 25, mFill, 6 };
	render.DrawRectangle(mBarFill, 100, 180, 255, 255, true, false);
	SDL_Rect mKnob = { trackX + mFill - 4, row0Y + 22, 8, 12 };
	render.DrawRectangle(mKnob, 200, 220, 255, 255, true, false);

	// --- Row 1: SOUNDS ---
	render.DrawMenuTextCentered("SOUNDS", { trackX, row1Y, trackW / 2, 20 }, labelColor);
	snprintf(vol, sizeof(vol), "%d", static_cast<int>(sfxVolume_ * 100.0f));
	render.DrawMenuTextCentered(vol, { trackX + trackW / 2, row1Y, trackW / 2, 20 }, valColor);

	SDL_Rect sBarBg = { trackX, row1Y + 25, trackW, 6 };
	render.DrawRectangle(sBarBg, 10, 15, 25, 255, true, false);
	int sFill = static_cast<int>(static_cast<float>(trackW) * sfxVolume_);
	SDL_Rect sBarFill = { trackX, row1Y + 25, sFill, 6 };
	render.DrawRectangle(sBarFill, 100, 180, 255, 255, true, false);
	SDL_Rect sKnob = { trackX + sFill - 4, row1Y + 22, 8, 12 };
	render.DrawRectangle(sKnob, 200, 220, 255, 255, true, false);

	// --- Row 2: DISPLAY ---
	render.DrawMenuTextCentered("DISPLAY", { trackX, row2Y, trackW, 20 }, labelColor);

	const char* modeNames[] = { "WINDOWED", "FULLSCREEN", "BORDERLESS" };
	int arrowW = 30;
	SDL_Rect leftArrowArea  = { trackX + 10, row2Y + 24, arrowW, 26 };
	SDL_Rect modeArea       = { trackX + 10 + arrowW, row2Y + 24, trackW - 20 - arrowW * 2, 26 };
	SDL_Rect rightArrowArea = { trackX + trackW - 10 - arrowW, row2Y + 24, arrowW, 26 };

	SDL_Color arrowColor = { 100, 180, 255, 255 };
	render.DrawMenuTextCentered("<", leftArrowArea, arrowColor);
	render.DrawMenuTextCentered(modeNames[windowModeIndex_], modeArea, valColor);
	render.DrawMenuTextCentered(">", rightArrowArea, arrowColor);

	// Handle display mode clicks
	auto& input = *Engine::GetInstance().input;
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

	// --- Gamepad selection indicator ---
	if (Engine::GetInstance().input->IsGamepadConnected()) {
		int selY = row0Y;
		if (optionsSliderSel_ == 1) selY = row1Y;
		else if (optionsSliderSel_ == 2) selY = row2Y;

		SDL_Rect selHighlight;
		if (optionsSliderSel_ == 3) {
			int winW = 0, winH = 0;
			Engine::GetInstance().window->GetWindowSize(winW, winH);
			int btnW = 315;
			selHighlight = { winW / 2 - btnW / 2 - 4, winH / 2 + 160 - 2, btnW + 8, 130 + 4 }; // Approximate
		}
		else {
			selHighlight = { trackX - 10, selY - 4, trackW + 20, 46 };
		}
		render.DrawRectangle(selHighlight, 60, 100, 180, 50, true, false);

		render.DrawMenuTextCentered(">", { selHighlight.x, selHighlight.y + selHighlight.h / 2 - 10, 16, 20 }, { 100, 200, 255, 255 });
	}
}


void Scene::HandlePauseMenuUIEvents(UIElement* uiElement)
{
	if (waitingForFade_) return;

	Engine::GetInstance().audio->PlayFx(menuClickFxId, 0, true);

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

bool Scene::HandleVolumeSliderInput(int trackX, int trackW, int row0Y, int row1Y, int row2Y)
{
	auto& input = *Engine::GetInstance().input;
	float dt = Engine::GetInstance().GetDt();

	// -- ESC key to go back --
	if (input.GetKey(SDL_SCANCODE_ESCAPE) == KEY_DOWN) {
		Engine::GetInstance().audio->PlayFx(menuClickFxId);
		return true;
	}

	// -- Mouse slider dragging / clicks ---------------------------------
	if (input.GetMouseButtonDown(SDL_BUTTON_LEFT) == KEY_DOWN ||
		input.GetMouseButtonDown(SDL_BUTTON_LEFT) == KEY_REPEAT)
	{
		Vector2D mousePos = input.GetMousePosition();
		int mouseX = (int)mousePos.getX();
		int mouseY = (int)mousePos.getY();

		// Music slider hit box
		SDL_Rect mSliderHit = { trackX - 10, row0Y + 15, trackW + 20, 28 };
		if (mouseX >= mSliderHit.x && mouseX <= mSliderHit.x + mSliderHit.w &&
			mouseY >= mSliderHit.y && mouseY <= mSliderHit.y + mSliderHit.h) {

			musicVolume_ = (float)(mouseX - trackX) / (float)trackW;
			if (musicVolume_ < 0.0f) musicVolume_ = 0.0f;
			if (musicVolume_ > 1.0f) musicVolume_ = 1.0f;
			Engine::GetInstance().audio->SetMusicVolume(musicVolume_);
		}

		// SFX slider hit box
		SDL_Rect sSliderHit = { trackX - 10, row1Y + 15, trackW + 20, 28 };
		if (mouseX >= sSliderHit.x && mouseX <= sSliderHit.x + sSliderHit.w &&
			mouseY >= sSliderHit.y && mouseY <= sSliderHit.y + sSliderHit.h) {

			sfxVolume_ = (float)(mouseX - trackX) / (float)trackW;
			if (sfxVolume_ < 0.0f) sfxVolume_ = 0.0f;
			if (sfxVolume_ > 1.0f) sfxVolume_ = 1.0f;
			Engine::GetInstance().audio->SetSFXVolume(sfxVolume_);
		}
	}

	// -- Gamepad D-pad / stick slider navigation --------------------------
	if (input.GetGamepadButton(SDL_GAMEPAD_BUTTON_EAST) == KEY_DOWN) {
		return true; // Back
	}

	if (input.GetGamepadButton(SDL_GAMEPAD_BUTTON_DPAD_UP) == KEY_DOWN ||
		(input.GetLeftStickY() < -0.5f && sliderRepeatTimer_ <= 0.0f))
	{
		optionsSliderSel_ = std::max(0, optionsSliderSel_ - 1);
		sliderRepeatTimer_ = 250.0f;
	}
	if (input.GetGamepadButton(SDL_GAMEPAD_BUTTON_DPAD_DOWN) == KEY_DOWN ||
		(input.GetLeftStickY() > 0.5f && sliderRepeatTimer_ <= 0.0f))
	{
		optionsSliderSel_ = std::min(3, optionsSliderSel_ + 1);
		sliderRepeatTimer_ = 250.0f;
	}

	// Row 3 = BACK: confirm with South button
	if (optionsSliderSel_ == 3 && input.GetGamepadButton(SDL_GAMEPAD_BUTTON_SOUTH) == KEY_DOWN) {
		return true;
	}

	float volStep = 0.05f;
	bool stepLeft = input.GetGamepadButton(SDL_GAMEPAD_BUTTON_DPAD_LEFT) == KEY_DOWN;
	bool stepRight = input.GetGamepadButton(SDL_GAMEPAD_BUTTON_DPAD_RIGHT) == KEY_DOWN;

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
	else if (optionsSliderSel_ == 2) {
		// Display mode switching via gamepad
		if (stepLeft || stepRight) {
			windowModeIndex_ = (windowModeIndex_ + (stepRight ? 1 : 2)) % 3;
			WindowMode modes[] = { WindowMode::WINDOWED, WindowMode::FULLSCREEN, WindowMode::BORDERLESS };
			Engine::GetInstance().window->SetWindowMode(modes[windowModeIndex_]);
			Engine::GetInstance().audio->PlayFx(menuClickFxId);
		}
	}

	if (sliderRepeatTimer_ > 0.0f)
		sliderRepeatTimer_ -= dt;

	return false;
}

void Scene::DrawBottomFog(int winW, int winH)
{
	if (currentMapFile_ != "MapTemplate.tmx") return;

	auto& render = *Engine::GetInstance().render;

	// Mesures on tiled for reference
	SDL_Rect fogBottom = {
		6000,
		8636,
		4200,
		99999
	};
	render.DrawRectangle(fogBottom, 0, 0, 0, 255, true, true);

	SDL_Rect fogDoor = {
		1080,   // X 
		8004,   // Y
		912,    // W
		1000     // H
	};
	render.DrawRectangle(fogDoor, 0, 0, 0, 255, true, true);
}

void Scene::LoadSubMap(const std::string& tmxFile, const std::string& spawnId)
{
	if (waitingForFade_) return;

	previousMapFile_ = currentMapFile_;
	previousSpawnId_ = "return_" + subMapSpawn_; 

	subMapTarget_ = tmxFile;
	subMapSpawn_ = spawnId;
	pendingSubMapLoad_ = true;

	waitingForFade_ = true;
	fadeTargetScene_ = SceneID::GAMEPLAY;
	Engine::GetInstance().render->StartFade(FadeDirection::FADE_OUT, 400.0f);
}

// Spawns a static, invisible collider over a defeated boss's entrance portal so the
// player can no longer walk back into the arena — same recipe as the "Wall" map entities.
void Scene::SealBossPortal(float x, float y, float w, float h)
{
	int cx = (int)(x + w * 0.5f);
	int cy = (int)(y + h * 0.5f);
	PhysBody* wall = Engine::GetInstance().physics->CreateRectangle(cx, cy, (int)w, (int)h, bodyType::STATIC, 0.0f);
	if (wall) wall->ctype = ColliderType::UNKNOWN;
}

// Programmatic map switch (e.g. returning the player to the hub after a boss fight) —
// reuses the same submap fade/load pipeline as portal traversal, just triggered by gameplay logic.
void Scene::RequestSubMapTeleport(const std::string& tmxFile, const std::string& spawnId)
{
	if (waitingForFade_) return;

	subMapTarget_ = tmxFile;
	subMapSpawnId_ = spawnId;
	pendingSubMapLoad_ = true;

	waitingForFade_ = true;
	fadeTargetScene_ = SceneID::GAMEPLAY;
	Engine::GetInstance().render->StartFade(FadeDirection::FADE_OUT, 400.0f);
}

Vector2D Scene::GetSpawnPosition(const std::string& spawnId)
{
	float x = 0.0f, y = 0.0f;
	Engine::GetInstance().map->GetSpawnById(spawnId, x, y);
	return Vector2D(x, y);
}

void Scene::CheckPortalCollisions(float dt)
{
	if (!player || waitingForFade_ || isPaused_ || bossDeathVideoActive_ || bossDeathFading_ || endGameVideoActive_) return;

	if (isPuzzleMap_ && puzzleManager_ && !puzzleManager_->IsPortalOpen()) {
		SDL_FRect exitRect = puzzleManager_->GetExitPortalRect();
		float px2 = player->position.getX() + 32.0f;
		float py2 = player->position.getY() + 48.0f;
		if (px2 >= exitRect.x && px2 <= exitRect.x + exitRect.w &&
			py2 >= exitRect.y && py2 <= exitRect.y + exitRect.h) {
			return;  
		}
	}

	if (isLvl3Map_ && puzzleManager3_ && !puzzleManager3_->IsLeverActivated()) {
		SDL_FRect blocked3 = puzzleManager3_->GetBlockedPortalRect();
		float pcx3 = player->position.getX() + 32.0f;
		float pcy3 = player->position.getY() + 48.0f;
		if (pcx3 >= blocked3.x && pcx3 <= blocked3.x + blocked3.w &&
			pcy3 >= blocked3.y && pcy3 <= blocked3.y + blocked3.h) {
			return;
		}
	}

	// Bloquear portales A y B hasta que se completen los botones
	if (isPuzzleMap3Lever_ && puzzleManager3_ && !puzzleManager3_->AreBothButtonsDone()) {
		SDL_FRect pA = puzzleManager3_->GetPortalARect();
		SDL_FRect pB = puzzleManager3_->GetPortalBRect();
		float pcx3 = player->position.getX() + 32.0f;
		float pcy3 = player->position.getY() + 48.0f;
		if ((pA.w > 0 && pcx3 >= pA.x && pcx3 <= pA.x + pA.w && pcy3 >= pA.y && pcy3 <= pA.y + pA.h) ||
			(pB.w > 0 && pcx3 >= pB.x && pcx3 <= pB.x + pB.w && pcy3 >= pB.y && pcy3 <= pB.y + pB.h)) {
			return;
		}
	}

	if (portalCooldown_ > 0.0f) {
		portalCooldown_ -= dt;
		return;
	}


	SDL_FRect blockedExit = { 0,0,0,0 };
	bool hasBlockedExit = false;
	if (isPuzzleMap_ && puzzleManager_ && !puzzleManager_->IsPortalOpen()) {
		blockedExit = puzzleManager_->GetExitPortalRect();
		hasBlockedExit = true;
	}

	if (isPuzzleMap3Lever_ && puzzleManager3_ && !puzzleManager3_->IsLeverActivated()) {
		blockedExit = puzzleManager3_->GetBlockedPortalRect();
		hasBlockedExit = true;
	}
	auto portals = Engine::GetInstance().map->GetPortals();
	if (portals.empty()) return;

	float px = player->position.getX();
	float py = player->position.getY();
	float pcx = px + 32.0f;
	float pcy = py + 48.0f;

	for (auto& portal : portals)
	{
		if (pcx >= portal.x && pcx <= portal.x + portal.w &&
			pcy >= portal.y && pcy <= portal.y + portal.h)
		{

			if (hasBlockedExit) {
				float pcx2 = player->position.getX() + 32.0f;
				float pcy2 = player->position.getY() + 48.0f;
				if (pcx2 >= blockedExit.x && pcx2 <= blockedExit.x + blockedExit.w &&
					pcy2 >= blockedExit.y && pcy2 <= blockedExit.y + blockedExit.h) {
					continue; 
				}
			}

			bool isLevelPortal = portal.targetFile.size() < 4 ||
				portal.targetFile.substr(portal.targetFile.size() - 4) != ".tmx";

			if (isLevelPortal)
			{
				// Portal de nivel
				std::string targetWithExt = portal.targetFile + ".tmx";
				for (int i = 0; i < (int)levels_.size(); i++)
				{
					if (levels_[i].file == targetWithExt)
					{
						pendingLevelSpawnId_ = portal.spawnId;
						hasPendingLevelSpawn_ = true;
						LoadMap(i);
						return;
					}
				}
				LOG("PORTAL: Level target '%s' not found in levels_", portal.targetFile.c_str());
			}
			else
			{
				subMapTarget_ = portal.targetFile;
				subMapSpawnId_ = portal.spawnId;
				pendingSubMapLoad_ = true;

				waitingForFade_ = true;
				fadeTargetScene_ = SceneID::GAMEPLAY;
				Engine::GetInstance().render->StartFade(FadeDirection::FADE_OUT, 400.0f);
			}
			return;
		}
	}
}

void Scene::ExecuteSubMapLoad()
{

    int savedHealth = player ? player->health : 3;

    player.reset();

    Engine::GetInstance().entityManager->CleanUp();
    Engine::GetInstance().physics->FlushPendingDeletes();
    Engine::GetInstance().map->CleanUp();
    Engine::GetInstance().physics->FlushPendingDeletes();

	if (puzzleManager2) {
		puzzleManager2->Reset();
	}

    currentMapFile_ = subMapTarget_;
    if (currentMapFile_.find("ZonaBoss") != std::string::npos)
    {
        Engine::GetInstance().audio->PlayMusic(nullptr);
    }
    else
    {
        Engine::GetInstance().audio->PlayMusic("assets/audio/music/Echoes_of_Slumber_In_Game.wav", 2.0f);
    }
    Engine::GetInstance().map->Load("assets/maps/", currentMapFile_);

	float portalSpawnX = 0.0f, portalSpawnY = 0.0f;
	bool portalSpawnFound = Engine::GetInstance().map->GetSpawnById(subMapSpawnId_, portalSpawnX, portalSpawnY);
	
	
	Engine::GetInstance().map->LoadEntities(player, portalSpawnFound, portalSpawnX, portalSpawnY);

    if (player == nullptr)
    {
        player = std::dynamic_pointer_cast<Player>(
        Engine::GetInstance().entityManager->CreateEntity(EntityType::PLAYER));
        player->position = Vector2D(200.0f, 400.0f); 
        player->Start();
    }

    if (player) {
        player->maxHealth = healthSlotCount_;
        if (savedHealth > healthSlotCount_) {
            savedHealth = healthSlotCount_;
        }
    }
    player->health = savedHealth;

    float spawnX = 0.0f, spawnY = 0.0f;
	if (Engine::GetInstance().map->GetSpawnById(subMapSpawnId_, spawnX, spawnY))
	{
		player->SetPosition(Vector2D(spawnX - player->texW / 2.0f, spawnY - player->texH / 2.0f));
		player->position = Vector2D(spawnX, spawnY);
	}
    else
    {
        LOG("WARNING PORTAL: Spawn '%s' not found in '%s'!",
            subMapSpawnId_.c_str(), subMapTarget_.c_str());
    }

	currentHealthUI_ = player->health;
	activeHealthAnim_ = 0;
    isGameOver_ = false;

	inGameIntroActive_ = false;
	isAutoEntering_ = false;
	inGameIntroTimer_ = 0.0f;
	autoEntryProgress_ = 0.0f;

	if (player) {
		player->isWakingUp = false;
		player->wakeUpAnimStarted = true;
	}

	Engine::GetInstance().render->SetCameraPosition(
		player->position.getX(), player->position.getY());

	Engine::GetInstance().render->SetCameraSway(true);
	Engine::GetInstance().render->SetCameraClamping(true);
	Engine::GetInstance().render->SetCameraMovement(true);
	Engine::GetInstance().render->cameraZoom = 1.0f;
	Engine::GetInstance().render->blurIntensity = 0.0f;

	portalCooldown_ = 1500.0f;

	isPuzzleMap_ = (currentMapFile_ == "MapLvl1ZonaPuzzle.tmx");
	if (isPuzzleMap_) {
		if (!puzzleManager_) puzzleManager_ = new PuzzleManager();
		puzzleManager_->Init(Engine::GetInstance().render->renderer);

		std::vector<PuzzleFigure> figuresOnly;
		SDL_FRect exitRect = { 0, 0, 48, 48 };

		for (auto& obj : Engine::GetInstance().map->GetPuzzleObjects()) {
			PuzzleFigure f;
			f.name = obj.name;
			f.worldRect = obj.rect;
			figuresOnly.push_back(f);
		}

		for (auto& portal : Engine::GetInstance().map->GetPortals()) {
			if (portal.spawnId == "spawn_return_puzzle_exit") {
				exitRect = { portal.x, portal.y, portal.w, portal.h };
				LOG("PUZZLE: Exit portal encontrado en (%.0f, %.0f) size %.0fx%.0f",
					portal.x, portal.y, portal.w, portal.h);
				break;
			}
		}

		puzzleManager_->LoadFiguresFromObjects(figuresOnly, exitRect);
		puzzleTimeoutPending_ = false;

		LOG("PUZZLE: Loaded %d figures. Exit portal at (%.0f, %.0f)", (int)figuresOnly.size(), exitRect.x, exitRect.y);
	}
	else {
		if (puzzleManager_) {
			delete puzzleManager_;
			puzzleManager_ = nullptr;
		}
	}

	// --- PUZZLE3 INIT ---
	isLvl3Map_ = (currentMapFile_ == "Map3.tmx");
	isLvl3Puzzle_ = (currentMapFile_ == "MapLvl3ZonaPuzzle3.tmx");
	isLvl3Puzzle1_ = (currentMapFile_ == "MapLvl3ZonaPuzzle1.tmx");
	isLvl3Puzzle2_ = (currentMapFile_ == "MapLvl3ZonaPuzzle2.tmx");
	LOG("PUZZLE3 DEBUG: currentMapFile_='%s', isLvl3Puzzle_=%d", currentMapFile_.c_str(), (int)isLvl3Puzzle_);

	if (isLvl3Map_ || isLvl3Puzzle_ || isLvl3Puzzle1_ || isLvl3Puzzle2_) {
		if (!puzzleManager3_) puzzleManager3_ = new PuzzleManager3();
		puzzleManager3_->Init(Engine::GetInstance().render->renderer);

		if (isLvl3Map_) {
			// Cargar palanca y portal bloqueado desde PuzzleObjects del mapa
			LeverData3 lever;
			SDL_FRect blockedPortal = { 0, 0, 48, 96 };

			for (auto& obj : Engine::GetInstance().map->GetPuzzleObjects()) {
				if (obj.name == "Lever") {
					lever.worldRect = obj.rect;
				}
			}

			// Portal bloqueado: leer desde layer Portals con spawnId = "spawn_puzzle3_entrance"
			for (auto& portal : Engine::GetInstance().map->GetPortals()) {
				if (portal.spawnId == "PuzzleMain" && portal.target == "MapLvl3ZonaPuzzle3.tmx") {
					blockedPortal = { portal.x, portal.y, portal.w, portal.h };
					LOG("PUZZLE3: Portal bloqueado encontrado en (%.0f, %.0f)", portal.x, portal.y);
					break;
				}
			}

			puzzleManager3_->LoadLever(lever, blockedPortal);

			SDL_FRect portalA = { 0,0,0,0 }, portalB = { 0,0,0,0 };
			for (auto& portal : Engine::GetInstance().map->GetPortals()) {
				if (portal.spawnId == "A") portalA = { portal.x, portal.y, portal.w, portal.h };
				if (portal.spawnId == "B") portalB = { portal.x, portal.y, portal.w, portal.h };
			}
			puzzleManager3_->LoadExtraPortals(portalA, portalB);
			LOG("PUZZLE3: Palanca cargada en (%.0f, %.0f)", lever.worldRect.x, lever.worldRect.y);
		}

		if (isLvl3Puzzle_) {
			// Cargar botones desde PuzzleObjects
			std::vector<ButtonData3> buttons;
			for (auto& obj : Engine::GetInstance().map->GetPuzzleObjects3()) {
				std::string fullName = obj.name;
				int clicks = 1;
				size_t sep = fullName.find('|');
				if (sep != std::string::npos) {
					clicks = std::stoi(fullName.substr(sep + 1));
					fullName = fullName.substr(0, sep);
				}
				if (fullName == "Button" || fullName == "button_2") {
					ButtonData3 btn;
					btn.worldRect = obj.rect;
					btn.requiredClicks = clicks;
					buttons.push_back(btn);
					LOG("PUZZLE3: Boton '%s' cargado, requiredClicks=%d", fullName.c_str(), clicks);
				}
			}
			puzzleManager3_->LoadButtons(buttons);

		}

		if (isLvl3Puzzle1_) {
			std::vector<ButtonData3P1> buttons;
			for (auto& obj : Engine::GetInstance().map->GetPuzzleObjects3()) {
				std::string fullName = obj.name;
				size_t sep = fullName.find('|');
				if (sep != std::string::npos) fullName = fullName.substr(0, sep);
				if (fullName == "Button") {
					ButtonData3P1 btn;
					btn.worldRect = obj.rect;
					btn.platformTarget = obj.platformTarget;  // leer desde Tiled
					btn.flipH = obj.flipH;
					buttons.push_back(btn);
					LOG("PUZZLE1: Boton cargado, target='%s'", btn.platformTarget.c_str());
				}
			}
			puzzleManager3_->LoadPuzzle1Buttons(buttons);
		}
	}
	else {
		if (puzzleManager3_) {
			delete puzzleManager3_;
			puzzleManager3_ = nullptr;
		}
		isLvl3Map_ = false;
		isLvl3Puzzle_ = false;
		isLvl3Puzzle1_ = false;
		isLvl3Puzzle2_ = false;
	}

	isPuzzleMap3Lever_ = isLvl3Map_;
	isPuzzleMap3Buttons_ = isLvl3Puzzle_;
	LOG("PORTAL: Sub-map load complete. Final health=%d", player->health);
}

void Scene::TriggerEndGameCinematic()
{
    // Solo permitir que se active el final del juego en el nivel 4 (índice 3).
    if (currentLevelIndex_ != 3) return;

    if (endGameTriggered_) return;  // already triggered once this session, ignore
    endGameTriggered_ = true;
    endGameFading_    = true;
    // Stop the game music immediately and fade the screen to black.
    // The actual video is launched in UpdateBossFight() once the screen is fully black.
    Engine::GetInstance().audio->SetMusicVolume(0.0f);
    Engine::GetInstance().render->StartCameraShake(1200.0f, 15.0f);
    Engine::GetInstance().render->StartFade(FadeDirection::FADE_OUT, 1200.0f);
}