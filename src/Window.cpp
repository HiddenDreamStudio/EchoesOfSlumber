#include "Window.h"
#include "Log.h"
#include "Engine.h"

Window::Window() : Module()
{
	window = NULL;
	name = "window";
}

// Destructor
Window::~Window()
{
}

// Called before render is available
bool Window::Awake()
{
	LOG("Init SDL window & surface");
	bool ret = true;

	if (SDL_Init(SDL_INIT_VIDEO) != true)
	{
		LOG("SDL_VIDEO could not initialize! SDL_Error: %s\n", SDL_GetError());
		ret = false;
	}
	else
	{
		// Create window
		Uint32 flags = 0;
		bool fullscreen = configParameters.child("fullscreen").attribute("value").as_bool();
		bool borderless = configParameters.child("borderless").attribute("value").as_bool();
		bool resizable = configParameters.child("resizable").attribute("value").as_bool();
		bool fullscreen_window = configParameters.child("fullscreen_window").attribute("value").as_bool();

		//TODO Get the values from the config file
		width = configParameters.child("resolution").attribute("width").as_int();
		height = configParameters.child("resolution").attribute("height").as_int();
		scale = configParameters.child("resolution").attribute("scale").as_int();

		if (fullscreen == true)        flags |= SDL_WINDOW_FULLSCREEN;
		if (borderless == true)        flags |= SDL_WINDOW_BORDERLESS;
		if (resizable == true)         flags |= SDL_WINDOW_RESIZABLE;

		window = SDL_CreateWindow(Engine::GetInstance().gameTitle.c_str(), width, height, flags);

		if (window == NULL)
		{
			LOG("Window could not be created! SDL_Error: %s\n", SDL_GetError());
			ret = false;
		}
		else
		{
			if (fullscreen_window == true)
			{
				SDL_SetWindowFullscreenMode(window, nullptr); // use desktop resolution
				SDL_SetWindowFullscreen(window, true);
				currentMode = WindowMode::BORDERLESS;
			}
			else if (fullscreen == true) {
				currentMode = WindowMode::FULLSCREEN;
			}
			else {
				currentMode = WindowMode::WINDOWED;
			}
			SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
			SDL_ShowWindow(window);
		}
	}

	return ret;
}

// Called before quitting
bool Window::CleanUp()
{
	LOG("Destroying SDL window and quitting all SDL systems");

	// Destroy window
	if (window != NULL)
	{
		SDL_DestroyWindow(window);
	}

	// Quit SDL subsystems
	SDL_Quit();
	return true;
}

// Set new window title
void Window::SetTitle(const char* new_title)
{
	SDL_SetWindowTitle(window, new_title);
}

void Window::GetWindowSize(int& width, int& height) const
{
	width = this->width;
	height = this->height;
}

int Window::GetScale() const
{
	return scale;
}

void Window::SetFullscreen(bool fullscreen)
{
	if (fullscreen) {
		SDL_SetWindowFullscreenMode(window, nullptr);
		SDL_SetWindowFullscreen(window, true);
		currentMode = WindowMode::FULLSCREEN;
	}
	else {
		SDL_SetWindowFullscreen(window, false);
		currentMode = WindowMode::WINDOWED;
	}
}

void Window::SetBorderless(bool borderless)
{
	SDL_SetWindowBordered(window, !borderless);
	if (borderless) currentMode = WindowMode::BORDERLESS;
}

void Window::SetWindowMode(WindowMode mode)
{
	currentMode = mode;
	switch (mode)
	{
	case WindowMode::WINDOWED:
		SDL_SetWindowFullscreen(window, false);
		SDL_SetWindowBordered(window, true);
		SDL_SetWindowSize(window, width, height); // Restore config resolution
		SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
		break;
	case WindowMode::FULLSCREEN:
		// Fullscreen usually changes resolution to match display
		SDL_SetWindowFullscreenMode(window, nullptr);
		SDL_SetWindowFullscreen(window, true);
		break;
	case WindowMode::BORDERLESS:
	{
		// Borderless fullscreen windowed: cover the entire display without exclusive fullscreen
		SDL_SetWindowFullscreen(window, false);
		SDL_SetWindowBordered(window, false);
		SDL_DisplayID display = SDL_GetDisplayForWindow(window);
		SDL_Rect bounds;
		if (SDL_GetDisplayBounds(display, &bounds)) {
			SDL_SetWindowSize(window, bounds.w, bounds.h);
			SDL_SetWindowPosition(window, bounds.x, bounds.y);
		}
		break;
	}
	}
}
