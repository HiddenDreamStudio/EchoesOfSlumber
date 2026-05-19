#pragma once

#include "Module.h"
#include <SDL3/SDL.h>
#include <string>

enum class WindowMode {
	WINDOWED,
	FULLSCREEN,
	BORDERLESS
};

class Window : public Module
{
public:

	Window();

	// Destructor
	virtual ~Window();

	// Called before render is available
	bool Awake();

	// Called before quitting
	bool CleanUp();

	// Change title
	void SetTitle(const char* title);

	// Retrive window size
	void GetWindowSize(int& width, int& height) const;

	// Retrieve window scale
	int GetScale() const;

	void SetFullscreen(bool fullscreen);
	void SetBorderless(bool borderless);
	void SetWindowMode(WindowMode mode);
	WindowMode GetWindowMode() const { return currentMode; }

public:
	// The window we'll be rendering to
	SDL_Window* window;

	std::string title;
	int width = 1280;
	int height = 720;
	int scale = 1;

private:
	WindowMode currentMode = WindowMode::WINDOWED;
};
