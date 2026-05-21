#if __has_include("discordpp.h")
#define DISCORDPP_IMPLEMENTATION
#endif

#include "DiscordManager.h"
#include "Log.h"
#include <iostream>
#include <ctime>

#ifdef _WIN32
#include <windows.h>
#endif

DiscordManager::DiscordManager() : Module()
{
    name = "discord";
}

DiscordManager::~DiscordManager()
{
}

bool DiscordManager::Awake()
{
#if EOS_DISCORD_SDK_AVAILABLE
    LOG("Initializing Discord Social SDK (Direct RPC Mode)...");
    client = std::make_shared<discordpp::Client>();

    if (!client) {
        LOG("Discord: Failed to instantiate client!");
        return false;
    }

    // Set Application ID first
    client->SetApplicationId(APPLICATION_ID);

    // Get the current executable path for robust registration
    std::string exePath = "";
#ifdef _WIN32
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    exePath = buffer; // SDK handles quoting internally
#endif

    // Register the game with the local Discord client for auto-detection and Overlay
    if (client->RegisterLaunchCommand(APPLICATION_ID, exePath)) {
        LOG("Discord: Launch command registered successfully");
    } else {
        LOG("Discord: Failed to register launch command");
    }

    // Set up logging
    client->AddLogCallback([](std::string message, discordpp::LoggingSeverity severity) {
        LOG("[Discord %s] %s", discordpp::EnumToString(severity), message.c_str());
    }, discordpp::LoggingSeverity::Info);

    // Initialize global start time for the session timer
    startTime = (int64_t)time(nullptr);

    // In Direct RPC mode, we don't wait for callbacks; we activate immediately if client is valid
    isReady = true; 

    return true;
#else
    LOG("Discord Social SDK header not found. Rich Presence disabled.");
    isReady = false;
    startTime = (int64_t)time(nullptr);
    return true;
#endif
}

bool DiscordManager::Start()
{
#if EOS_DISCORD_SDK_AVAILABLE
    if (!isReady || !client) return true;

    LOG("Setting initial Discord Presence...");
    UpdatePresence("", "");
#endif
    return true;
}

bool DiscordManager::Update(float dt)
{
#if EOS_DISCORD_SDK_AVAILABLE
    if (client) {
        discordpp::RunCallbacks();
    }
#endif
    return true;
}

bool DiscordManager::CleanUp()
{
    LOG("Cleaning up Discord SDK");
    isReady = false;
#if EOS_DISCORD_SDK_AVAILABLE
    client.reset();
#endif
    return true;
}

void DiscordManager::Authenticate()
{
    // Removed OAuth2 flow as per user request to avoid account connection popups.
    // Pure RPC doesn't require Bearer tokens for local presence.
}

void DiscordManager::UpdatePresence(const char* state, const char* details)
{
#if EOS_DISCORD_SDK_AVAILABLE
    if (!isReady || !client) return;

    discordpp::Activity activity;
    activity.SetType(discordpp::ActivityTypes::Playing);
    
    if (state && state[0] != '\0')
        activity.SetState(state);
    
    if (details && details[0] != '\0')
        activity.SetDetails(details);

    // Add visual assets to improve game detection and professional look
    discordpp::ActivityAssets assets;
    assets.SetLargeImage("logo"); // Ensure 'logo' asset exists in Discord Portal
    assets.SetLargeText("Echoes of Slumber");
    activity.SetAssets(assets);

    // Persist the timer by setting the start timestamp
    discordpp::ActivityTimestamps timestamps;
    timestamps.SetStart(startTime);
    activity.SetTimestamps(timestamps);

    client->UpdateRichPresence(activity, [](discordpp::ClientResult result) {
        if(result.Successful()) {
            LOG("Discord Rich Presence updated successfully!");
        } else {
            LOG("Discord Rich Presence update failed");
        }
    });
#else
    (void)state;
    (void)details;
#endif
}
