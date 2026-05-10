#define DISCORDPP_IMPLEMENTATION
#include "DiscordManager.h"
#include "Log.h"
#include <iostream>

DiscordManager::DiscordManager() : Module()
{
    name = "discord";
}

DiscordManager::~DiscordManager()
{
}

bool DiscordManager::Awake()
{
    LOG("Initializing Discord Social SDK (Direct RPC Mode)...");
    client = std::make_shared<discordpp::Client>();

    // Set Application ID for Direct RPC (No authentication/token needed)
    client->SetApplicationId(APPLICATION_ID);

    // Set up logging
    client->AddLogCallback([](std::string message, discordpp::LoggingSeverity severity) {
        LOG("[Discord %s] %s", discordpp::EnumToString(severity), message.c_str());
    }, discordpp::LoggingSeverity::Info);

    // In Direct RPC mode, we might not get status changes like 'Ready', 
    // so we'll attempt to set presence directly once the client is initialized.
    isReady = true; 

    return true;
}

bool DiscordManager::Start()
{
    LOG("Setting initial Discord Presence...");
    UpdatePresence("Exploring the Subconscious", "Alpha Phase");
    return true;
}

bool DiscordManager::Update(float dt)
{
    discordpp::RunCallbacks();
    return true;
}

bool DiscordManager::CleanUp()
{
    LOG("Cleaning up Discord SDK");
    return true;
}

void DiscordManager::Authenticate()
{
    // Removed OAuth2 flow as per user request to avoid account connection popups.
    // Pure RPC doesn't require Bearer tokens for local presence.
}

void DiscordManager::UpdatePresence(const char* state, const char* details)
{
    if (!isReady) return;

    discordpp::Activity activity;
    activity.SetType(discordpp::ActivityTypes::Playing);
    activity.SetState(state);
    activity.SetDetails(details);

    client->UpdateRichPresence(activity, [](discordpp::ClientResult result) {
        if(result.Successful()) {
            LOG("Discord Rich Presence updated successfully!");
        } else {
            LOG("Discord Rich Presence update failed");
        }
    });
}
