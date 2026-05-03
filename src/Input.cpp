#include "Engine.h"
#include "Input.h"
#include "Window.h"
#include "Log.h"
#include <cmath>

#define MAX_KEYS 300

Input::Input() : Module()
{
	name = "input";

	keyboard = new KeyState[MAX_KEYS];
	memset(keyboard, KEY_IDLE, sizeof(KeyState) * MAX_KEYS);
	memset(mouseButtons, KEY_IDLE, sizeof(KeyState) * NUM_MOUSE_BUTTONS);
	memset(padButtons, KEY_IDLE, sizeof(KeyState) * NUM_GAMEPAD_BUTTONS);
	memset(windowEvents, 0, sizeof(windowEvents));
	mouseMotionX = mouseMotionY = mouseX = mouseY = 0;
}

// Destructor
Input::~Input()
{
	delete[] keyboard;
}

// Called before render is available
bool Input::Awake()
{
	LOG("Init SDL input event system");
	bool ret = true;

	if (SDL_InitSubSystem(SDL_INIT_EVENTS) != true)
	{
		LOG("SDL_EVENTS could not initialize! SDL_Error: %s\n", SDL_GetError());
		ret = false;
	}

	if (SDL_InitSubSystem(SDL_INIT_GAMEPAD) != true)
	{
		LOG("SDL_GAMEPAD could not initialize! SDL_Error: %s\n", SDL_GetError());
		// Not fatal — game works without gamepad
	}
	else
	{
		LOG("SDL Gamepad subsystem initialized");
		OpenFirstGamepad();
	}

	return ret;
}

void Input::OpenFirstGamepad()
{
	if (gamepad != nullptr) return;

	int count = 0;
	SDL_JoystickID* joysticks = SDL_GetGamepads(&count);
	if (joysticks && count > 0)
	{
		gamepad = SDL_OpenGamepad(joysticks[0]);
		if (gamepad)
		{
			const char* name = SDL_GetGamepadName(gamepad);
			LOG("Gamepad connected: %s", name ? name : "Unknown");
		}
		else
		{
			LOG("Failed to open gamepad: %s", SDL_GetError());
		}
	}
	SDL_free(joysticks);
}

// Called before the first frame
bool Input::Start()
{
	SDL_StopTextInput(Engine::GetInstance().window->window);
	return true;
}

// Called each loop iteration
bool Input::PreUpdate()
{
	static SDL_Event event;

	int numKeys = 0;
	const bool* keys = SDL_GetKeyboardState(&numKeys);

	for (int i = 0; i < MAX_KEYS; ++i)
	{
		if (keys[i] == 1)
		{
			if (keyboard[i] == KEY_IDLE)
				keyboard[i] = KEY_DOWN;
			else
				keyboard[i] = KEY_REPEAT;
		}
		else
		{
			if (keyboard[i] == KEY_REPEAT || keyboard[i] == KEY_DOWN)
				keyboard[i] = KEY_UP;
			else
				keyboard[i] = KEY_IDLE;
		}
	}

	for (int i = 0; i < NUM_MOUSE_BUTTONS; ++i)
	{
		if (mouseButtons[i] == KEY_DOWN)
			mouseButtons[i] = KEY_REPEAT;

		if (mouseButtons[i] == KEY_UP)
			mouseButtons[i] = KEY_IDLE;
	}

	// ── Gamepad button states (polled, same logic as keyboard) ────────────
	if (gamepad)
	{
		for (int i = 0; i < NUM_GAMEPAD_BUTTONS; ++i)
		{
			bool pressed = SDL_GetGamepadButton(gamepad, (SDL_GamepadButton)i);
			if (pressed)
			{
				if (padButtons[i] == KEY_IDLE)
					padButtons[i] = KEY_DOWN;
				else
					padButtons[i] = KEY_REPEAT;
			}
			else
			{
				if (padButtons[i] == KEY_REPEAT || padButtons[i] == KEY_DOWN)
					padButtons[i] = KEY_UP;
				else
					padButtons[i] = KEY_IDLE;
			}
		}

		// ── Analog sticks (normalized to -1..+1 with deadzone) ───────────
		auto readAxis = [&](SDL_GamepadAxis axis) -> float {
			Sint16 raw = SDL_GetGamepadAxis(gamepad, axis);
			float normalized = (float)raw / 32767.0f;
			if (std::fabs(normalized) < GAMEPAD_STICK_DEADZONE)
				return 0.0f;
			return normalized;
		};

		leftStickX  = readAxis(SDL_GAMEPAD_AXIS_LEFTX);
		leftStickY  = readAxis(SDL_GAMEPAD_AXIS_LEFTY);
		rightStickX = readAxis(SDL_GAMEPAD_AXIS_RIGHTX);
		rightStickY = readAxis(SDL_GAMEPAD_AXIS_RIGHTY);

		// Triggers (0..32767, normalize to 0..1)
		auto readTrigger = [&](SDL_GamepadAxis axis) -> float {
			Sint16 raw = SDL_GetGamepadAxis(gamepad, axis);
			float normalized = (float)raw / 32767.0f;
			if (normalized < 0.05f) return 0.0f;
			return normalized;
		};
		leftTrigger  = readTrigger(SDL_GAMEPAD_AXIS_LEFT_TRIGGER);
		rightTrigger = readTrigger(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER);
	}
	else
	{
		// No gamepad — zero everything
		memset(padButtons, KEY_IDLE, sizeof(KeyState) * NUM_GAMEPAD_BUTTONS);
		leftStickX = leftStickY = rightStickX = rightStickY = 0.0f;
		leftTrigger = rightTrigger = 0.0f;
		touchpadRaw = false;
	}

	// ── Touchpad logic (poll) ──────────────────────────────────────────────
	if (touchpadRaw)
	{
		if (touchpadState == KEY_IDLE)
			touchpadState = KEY_DOWN;
		else
			touchpadState = KEY_REPEAT;
	}
	else
	{
		if (touchpadState == KEY_REPEAT || touchpadState == KEY_DOWN)
			touchpadState = KEY_UP;
		else
			touchpadState = KEY_IDLE;
	}

	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
		case SDL_EVENT_QUIT:
			windowEvents[WE_QUIT] = true;
			break;

		case SDL_EVENT_WINDOW_HIDDEN:
		case SDL_EVENT_WINDOW_MINIMIZED:
		case SDL_EVENT_WINDOW_FOCUS_LOST:
			windowEvents[WE_HIDE] = true;
			break;

		case SDL_EVENT_WINDOW_SHOWN:
		case SDL_EVENT_WINDOW_FOCUS_GAINED:
		case SDL_EVENT_WINDOW_MAXIMIZED:
		case SDL_EVENT_WINDOW_RESTORED:
			windowEvents[WE_SHOW] = true;
			break;

		case SDL_EVENT_MOUSE_BUTTON_DOWN:
			if (event.button.button >= 1 && event.button.button <= NUM_MOUSE_BUTTONS)
				mouseButtons[event.button.button - 1] = KEY_DOWN;
			break;

		case SDL_EVENT_MOUSE_BUTTON_UP:
			if (event.button.button >= 1 && event.button.button <= NUM_MOUSE_BUTTONS)
				mouseButtons[event.button.button - 1] = KEY_UP;
			break;

		case SDL_EVENT_MOUSE_MOTION:
		{
			int scale = Engine::GetInstance().window->GetScale();
			mouseMotionX = (int)(event.motion.xrel / scale);
			mouseMotionY = (int)(event.motion.yrel / scale);
			mouseX = (int)(event.motion.x / scale);
			mouseY = (int)(event.motion.y / scale);
		}
		break;

		// ── Gamepad hot-plug ─────────────────────────────────────────────
		case SDL_EVENT_GAMEPAD_ADDED:
			if (gamepad == nullptr)
			{
				gamepad = SDL_OpenGamepad(event.gdevice.which);
				if (gamepad)
				{
					const char* gpName = SDL_GetGamepadName(gamepad);
					LOG("Gamepad connected: %s", gpName ? gpName : "Unknown");
				}
			}
			break;

		case SDL_EVENT_GAMEPAD_REMOVED:
			if (gamepad != nullptr)
			{
				SDL_JoystickID removedId = event.gdevice.which;
				SDL_JoystickID currentId = SDL_GetJoystickID(SDL_GetGamepadJoystick(gamepad));
				if (removedId == currentId)
				{
					LOG("Gamepad disconnected");
					SDL_CloseGamepad(gamepad);
					gamepad = nullptr;
					memset(padButtons, KEY_IDLE, sizeof(KeyState) * NUM_GAMEPAD_BUTTONS);
					leftStickX = leftStickY = rightStickX = rightStickY = 0.0f;
					leftTrigger = rightTrigger = 0.0f;
					touchpadRaw = false;

					// Try to open another gamepad if available
					OpenFirstGamepad();
				}
			}
			break;

		// ── Touchpad events ──────────────────────────────────────────────
		case SDL_EVENT_GAMEPAD_TOUCHPAD_DOWN:
			touchpadRaw = true;
			break;
		case SDL_EVENT_GAMEPAD_TOUCHPAD_UP:
			touchpadRaw = false;
			break;
		}
	}

	return true;
}

// Called before quitting
bool Input::CleanUp()
{
	LOG("Quitting SDL event subsystem");

	if (gamepad != nullptr)
	{
		SDL_CloseGamepad(gamepad);
		gamepad = nullptr;
	}

	SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
	SDL_QuitSubSystem(SDL_INIT_EVENTS);
	return true;
}

bool Input::GetWindowEvent(EventWindow ev)
{
	return windowEvents[ev];
}

Vector2D Input::GetMousePosition()
{
	return Vector2D((float)mouseX,(float)mouseY);
}

Vector2D Input::GetMouseMotion()
{
	return Vector2D((float)mouseMotionX, (float)mouseMotionY);
}