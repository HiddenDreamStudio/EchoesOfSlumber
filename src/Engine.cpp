#include <iostream>
#include <sstream>
#include <iomanip>

#include "Engine.h"
#include "Window.h"
#include "Input.h"
#include "Render.h"
#include "Textures.h"
#include "Audio.h"
#include "Scene.h"
#include "EntityManager.h"
#include "Map.h"
#include "Physics.h"
#include "Log.h"
#include "UIManager.h"
#include "Cinematics.h"
#include "SaveSystem.h"
#include "tracy/Tracy.hpp"

// Constructor
Engine::Engine() {

	LOG("Constructor Engine::Engine");

	Timer timer = Timer();
    startupTime = Timer();
    frameTime = PerfTimer();
    lastSecFrameTime = PerfTimer();
    frames = 0;

    // Modules
    window = std::make_shared<Window>();
    input = std::make_shared<Input>();
    render = std::make_shared<Render>();
    textures = std::make_shared<Textures>();
    audio = std::make_shared<Audio>();
    physics = std::make_shared<Physics>();
    scene = std::make_shared<Scene>();
    map = std::make_shared<Map>();
    entityManager = std::make_shared<EntityManager>();
	uiManager = std::make_shared<UIManager>(); 
	cinematics = std::make_shared<Cinematics>();
	saveSystem = std::make_shared<SaveSystem>();

    // Ordered for awake / Start / Update
    // Reverse order of CleanUp
    AddModule(std::static_pointer_cast<Module>(window));
    AddModule(std::static_pointer_cast<Module>(input));
    AddModule(std::static_pointer_cast<Module>(textures));
    AddModule(std::static_pointer_cast<Module>(audio));
    AddModule(std::static_pointer_cast<Module>(physics));
    AddModule(std::static_pointer_cast<Module>(map));
    AddModule(std::static_pointer_cast<Module>(entityManager));
    AddModule(std::static_pointer_cast<Module>(scene));
	AddModule(std::static_pointer_cast<Module>(uiManager)); 
	AddModule(std::static_pointer_cast<Module>(cinematics));
	AddModule(std::static_pointer_cast<Module>(saveSystem));

    // Render last 
    AddModule(std::static_pointer_cast<Module>(render));

	LOG("Timer Engine Constructor: %f", timer.ReadMSec());
}

// Static method to get the instance of the Engine class, following the singleton pattern
Engine& Engine::GetInstance() {
    static Engine instance; // Guaranteed to be destroyed and instantiated on first use
    return instance;
}

void Engine::AddModule(std::shared_ptr<Module> module){
    module->Init();
    moduleList.push_back(module);
}

// Called before render is available
bool Engine::Awake() {

    Timer timer = Timer();

    LOG("Engine::Awake");

    LoadConfig();
    gameTitle = configFile.child("config").child("engine").child("title").child_value();
    targetFrameRate = configFile.child("config").child("engine").child("targetFrameRate").attribute("value").as_int();

    bool result = true;
    for (const auto& module : moduleList) {
		module->LoadParameters(configFile.child("config").child(module.get()->name.c_str()));
        result =  module->Awake();

        if (!result) {
			break;
		}
    }

	LOG("Timer Engine Awake(): %f", timer.ReadMSec());

    return result;
}

// Called before the first frame
bool Engine::Start() {

    Timer timer = Timer();

    LOG("Engine::Start");

    bool result = true;
    for (const auto& module : moduleList) {
        result = module->Start();
        if (!result) {
            break;
        }
    }

	LOG("Timer Engine Start(): %f", timer.ReadMSec());
	
    return result;
}

// Called each loop iteration
bool Engine::Update() {

    ZoneScoped;

    bool ret = true;
    PrepareUpdate();

    if (input->GetWindowEvent(WE_QUIT) == true)
        ret = false;

    if (ret == true)
        ret = PreUpdate();

    if (ret == true)
        ret = DoUpdate();

    if (ret == true)
        ret = PostUpdate();

    FinishUpdate();
    return ret;
}

// Called before quitting
bool Engine::CleanUp() {

    Timer timer = Timer();

    LOG("Engine::CleanUp");

    bool result = true;
    for (const auto& module : moduleList) {
        result = module->CleanUp();
        if (!result) {
            break;
        }
    }

	LOG("Timer Engine CleanUp(): %f", timer.ReadMSec());

    return result;
}

// ---------------------------------------------
void Engine::PrepareUpdate()
{
    frameTime.Start();
}

// ---------------------------------------------
void Engine::FinishUpdate()
{
    ZoneScoped;
    // Cap the framerate
    double currentDt = frameTime.ReadMs();
	float maxFrameDuration = 1000.0f / targetFrameRate;
    if (targetFrameRate > 0 && currentDt < maxFrameDuration) {
        Uint32 delay = (Uint32)(maxFrameDuration - currentDt);
        PerfTimer delayTimer = PerfTimer();
        SDL_Delay(delay);
    }

    frameCount++;

    // Amount of time since game start
    secondsSinceStartup = startupTime.ReadSec();

    // dt in milliseconds
    dt = (float)frameTime.ReadMs();
	if (dt > 33.0f) dt = 33.0f; // Prevent huge dt spikes (like map loading) from instantly blowing through animations and physics routines

    // FPS tracking
    lastSecFrameCount++;

    // Average FPS for the whole game life
    if (lastSecFrameTime.ReadMs() > 1000) {
        lastSecFrameTime.Start();
        averageFps = (averageFps + lastSecFrameCount) / 2;
        framesPerSecond = lastSecFrameCount;
        lastSecFrameCount = 0;
    }

    // Shows the time measurements in the window title
    std::stringstream ss;
    ss << scene->GetTilePosDebug()
        << gameTitle << ": Av.FPS: " << std::fixed << std::setprecision(2) << averageFps
        << " Last sec frames: " << framesPerSecond
        << " Last dt: " << std::fixed << std::setprecision(3) << dt
        << " Time since startup: " << secondsSinceStartup
        << " Frame Count: " << frameCount;

    std::string titleStr = ss.str();

    window.get()->SetTitle(titleStr.c_str());
}


// Call modules before each loop iteration
bool Engine::PreUpdate()
{
    ZoneScoped;

    //Iterates the module list and calls PreUpdate on each module
    bool result = true;
    for (const auto& module : moduleList) {
        result = module->PreUpdate();
        if (!result) {
            break;
        }
    }

    return result;
}

// Call modules on each loop iteration
bool Engine::DoUpdate()
{
    ZoneScoped;
    //Iterates the module list and calls Update on each module
    bool result = true;
    for (const auto& module : moduleList) {
        result = module->Update(dt);
        if (!result) {
            break;
        }
    }

    return result;
}

// Call modules after each loop iteration
bool Engine::PostUpdate()
{
    ZoneScoped;

    //Iterates the module list and calls PostUpdate on each module
    bool result = true;
    for (const auto& module : moduleList) {
        result = module->PostUpdate();
        if (!result) {
            break;
        }
    }

    return result;
}

// Load config from XML file
bool Engine::LoadConfig()
{
    bool ret = true;

    pugi::xml_parse_result result = configFile.load_file("config.xml");
    if (result)
    {
        LOG("config.xml parsed without errors");
    }
    else
    {
        LOG("Error loading config.xml: %s", result.description());
    }

    return ret;
}


