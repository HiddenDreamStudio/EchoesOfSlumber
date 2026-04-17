#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <SDL3/SDL_rect.h>

struct AnimFrame {
    SDL_Rect rect{};
    int durationMs = 100;
};

class Animation {
public:
    Animation();
    void AddFrame(const SDL_Rect& r, int durationMs);
    void SetLoop(bool v);
    void Reset();
    bool HasFinishedOnce() const;
    void Update(float dt);
    const SDL_Rect& GetCurrentFrame() const;
    int GetFrameCount() const;

private:
    std::vector<AnimFrame> frames_;
    int currentIndex_ = 0;
    int timeInFrameMs_ = 0;
    bool loop_ = true;
    bool finishedOnce_ = false;

    static SDL_Rect kEmpty_;
};

class AnimationSet {
public:
    AnimationSet();

    // load from TSX with aliases {baseTileId -> name}
    bool LoadFromTSX(const char* tsxPath,
        const std::unordered_map<int, std::string>& aliases);

    // manage animations
    void SetCurrent(const std::string& name);
    void ResetCurrent(); // Reset current animation to frame 0
    void Update(float dtSeconds);
    const SDL_Rect& GetCurrentFrame() const;
    const std::string& GetCurrentName() const;

    bool Has(const std::string& name) const;

    void SetLoop(const std::string& name, bool loop);
    bool HasFinishedOnce(const std::string& name) const;
    void ResetCurrent();

private:
    int tileW_ = 0, tileH_ = 0, columns_ = 0;
    std::unordered_map<std::string, Animation> clips_;
    std::string currentName_;

    static SDL_Rect TileIdToRect(int tileid, int columns, int tileW, int tileH);
};
