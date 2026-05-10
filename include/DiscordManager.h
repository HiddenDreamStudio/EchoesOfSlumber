#pragma once

#include "Module.h"
#include "discordpp.h"
#include <memory>
#include <atomic>

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

    std::shared_ptr<discordpp::Client> client;
    const uint64_t APPLICATION_ID = 1476916209667805430;
    bool isReady = false;
};
