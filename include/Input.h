#pragma once

#include "Module.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_rect.h>
#include "Vector2D.h"

#define NUM_MOUSE_BUTTONS 5
#define NUM_GAMEPAD_BUTTONS SDL_GAMEPAD_BUTTON_COUNT
#define GAMEPAD_STICK_DEADZONE 0.15f

enum EventWindow
{
	WE_QUIT = 0,
	WE_HIDE = 1,
	WE_SHOW = 2,
	WE_COUNT
};

enum KeyState
{
	KEY_IDLE = 0,
	KEY_DOWN,
	KEY_REPEAT,
	KEY_UP
};

class Input : public Module
{

public:

	Input();

	// Destructor
	virtual ~Input();

	// Called before render is available
	bool Awake();

	// Called before the first frame
	bool Start();

	// Called each loop iteration
	bool PreUpdate();

	// Called before quitting
	bool CleanUp();

	// Check key states (includes mouse and joy buttons)
	KeyState GetKey(int id) const
	{
		return keyboard[id];
	}

	KeyState GetMouseButtonDown(int id) const
	{
		return mouseButtons[id - 1];
	}

	// ── Gamepad ──────────────────────────────────────────────────────────────
	KeyState GetGamepadButton(int btn) const
	{
		if (btn < 0 || btn >= NUM_GAMEPAD_BUTTONS) return KEY_IDLE;
		return padButtons[btn];
	}

	float GetLeftStickX()  const { return leftStickX; }
	float GetLeftStickY()  const { return leftStickY; }
	float GetRightStickX() const { return rightStickX; }
	float GetRightStickY() const { return rightStickY; }
	float GetLeftTrigger()  const { return leftTrigger; }
	float GetRightTrigger() const { return rightTrigger; }
	KeyState GetTouchpadPressed() const { return touchpadState; }
	bool  IsGamepadConnected() const { return gamepad != nullptr; }

	// Check if a certain window event happened
	bool GetWindowEvent(EventWindow ev);

	// Get mouse / axis position
	Vector2D GetMousePosition();
	Vector2D GetMouseMotion();

private:
	bool windowEvents[WE_COUNT];
	KeyState* keyboard;
	KeyState mouseButtons[NUM_MOUSE_BUTTONS];
	int	mouseMotionX;
	int mouseMotionY;
	int mouseX;
	int mouseY;

	// ── Gamepad state ────────────────────────────────────────────────────────
	SDL_Gamepad* gamepad = nullptr;
	KeyState padButtons[NUM_GAMEPAD_BUTTONS];
	float leftStickX  = 0.0f;
	float leftStickY  = 0.0f;
	float rightStickX = 0.0f;
	float rightStickY = 0.0f;
	float leftTrigger  = 0.0f;
	float rightTrigger = 0.0f;

	// Touchpad state (event-driven for PS5 compatibility)
	KeyState touchpadState = KEY_IDLE;
	bool     touchpadRaw   = false;

	void OpenFirstGamepad();
};