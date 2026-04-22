#include <iostream>
#include "Engine.h"
#include "Log.h"

int main(int argc, char* argv[]) {

	LOG("Engine starting ...");

	//Initializes the engine state
	Engine::EngineState state = Engine::EngineState::CREATE;
	int result = EXIT_FAILURE;

	while (state != Engine::EngineState::EXIT)
	{
		switch (state)
		{
			// Allocate the engine
		case Engine::EngineState::CREATE:
			LOG("CREATION PHASE ===============================");
			state = Engine::EngineState::AWAKE;

			break;

			// Awake all modules
		case Engine::EngineState::AWAKE:
			LOG("AWAKE PHASE ===============================");
			if (Engine::GetInstance().Awake() == true)
				state = Engine::EngineState::START;
			else
			{
				LOG("ERROR: Awake failed");
				state = Engine::EngineState::FAIL;
			}

			break;

			// Call all modules before first frame
		case Engine::EngineState::START:
			LOG("START PHASE ===============================");
			if (Engine::GetInstance().Start() == true )
			{
				state = Engine::EngineState::LOOP;
				LOG("UPDATE PHASE ===============================");
			}
			else
			{
				state = Engine::EngineState::FAIL;
				LOG("ERROR: Start failed");
			}
			break;

			// Loop all modules until we are asked to leave
		case Engine::EngineState::LOOP:
			if (Engine::GetInstance().Update() == false)
				state = Engine::EngineState::CLEAN;
			break;

			// Cleanup allocated memory
		case Engine::EngineState::CLEAN:
			LOG("CLEANUP PHASE ===============================");
			if (Engine::GetInstance().CleanUp() == true)
			{
				result = EXIT_SUCCESS;
				state = Engine::EngineState::EXIT;
			}
			else
				state = Engine::EngineState::FAIL;

			break;

			// Exit with errors and shame
		case Engine::EngineState::FAIL:
			LOG("Exiting with errors");
			result = EXIT_FAILURE;
			state = Engine::EngineState::EXIT;
			break;
		}
	}

	LOG("Closing Engine ===============================");

	return result;
}
