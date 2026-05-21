#pragma once

#include "Module.h"
#include <memory>
#include <atomic>

#if __has_include("discordpp.h")
#define EOS_DISCORD_SDK_AVAILABLE 1
#include "discordpp.h"
#else
#define EOS_DISCORD_SDK_AVAILABLE 0
#endif

class DiscordManager : public Module
{
public:
    DiscordManager();
    virtual ~DiscordManager();

    bool Awake() override;
    bool Start() override;
    bool Update(float dt) override;
    bool CleanUp() override;

    void UpdatePresence(const char* state, const char* details);

private:
    void Authenticate();

#if EOS_DISCORD_SDK_AVAILABLE
    std::shared_ptr<discordpp::Client> client;
#endif
    const uint64_t APPLICATION_ID = 1476916209667805430;
    std::atomic<bool> isReady = false;
    std::atomic<int64_t> startTime = 0;
};
