// ----------------------------------------------------
// Timer with milisecons precision
// ----------------------------------------------------

#include "Timer.h"
#include <SDL3\SDL_timer.h>

// L2: TODO 1: Fill Start(), Read(), ReadSec() methods
// they are simple, one line each!

Timer::Timer()
{
	Start();
}

void Timer::Start()
{
	startTime = SDL_GetTicks();
}

int Timer::ReadSec() const
{
	return (SDL_GetTicks() - startTime) / 1000;
}

float Timer::ReadMSec() const
{
	return (float)(SDL_GetTicks() - startTime);
}
