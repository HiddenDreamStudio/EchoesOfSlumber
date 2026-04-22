#include "Animation.h"
#include <pugixml.hpp>
#include <cstdio>

// ---------- Animation ----------

SDL_Rect Animation::kEmpty_{ 0,0,0,0 };

Animation::Animation() {}

void Animation::AddFrame(const SDL_Rect& r, int durationMs) {
    frames_.push_back({ r, durationMs });
}

void Animation::SetLoop(bool v) { loop_ = v; }

void Animation::Reset() {
    currentIndex_ = 0;
    timeInFrameMs_ = 0;
    finishedOnce_ = false;
}

bool Animation::HasFinishedOnce() const { return finishedOnce_ && !loop_; }

void Animation::Update(float dt) {
    if (frames_.empty()) return;

    timeInFrameMs_ += static_cast<int>(dt);

    while (timeInFrameMs_ >= frames_[static_cast<size_t>(currentIndex_)].durationMs) {
        timeInFrameMs_ -= frames_[static_cast<size_t>(currentIndex_)].durationMs;

        if (currentIndex_ + 1 < static_cast<int>(frames_.size())) {
            ++currentIndex_;
        }
        else {
            if (loop_) {
                currentIndex_ = 0;
            }
            else {
                finishedOnce_ = true;
                currentIndex_ = static_cast<int>(frames_.size()) - 1;
                break;
            }
        }
    }
}

const SDL_Rect& Animation::GetCurrentFrame() const {
    if (frames_.empty()) return kEmpty_;
    return frames_[static_cast<size_t>(currentIndex_)].rect;
}

int Animation::GetFrameCount() const { return static_cast<int>(frames_.size()); }

int Animation::GetCurrentFrameIndex() const {
    return currentIndex_;
}

// ---------- AnimationSet ----------

AnimationSet::AnimationSet() {}

SDL_Rect AnimationSet::TileIdToRect(int tileid, int columns, int tileW, int tileH) {
    SDL_Rect r{};
    r.x = (tileid % columns) * tileW;
    r.y = (tileid / columns) * tileH;
    r.w = tileW;
    r.h = tileH;
    return r;
}

bool AnimationSet::LoadFromTSX(const char* tsxPath,
    const std::unordered_map<int, std::string>& aliases)
{
    pugi::xml_document doc;
    pugi::xml_parse_result ok = doc.load_file(tsxPath);
    if (!ok) {
        std::fprintf(stderr, "TSX load failed (%s): %s\n", tsxPath, ok.description());
        return false;
    }

    pugi::xml_node tileset = doc.child("tileset");
    if (!tileset) {
        std::fprintf(stderr, "TSX error: <tileset> missing (%s)\n", tsxPath);
        return false;
    }

    tileW_ = tileset.attribute("tilewidth").as_int();
    tileH_ = tileset.attribute("tileheight").as_int();
    columns_ = tileset.attribute("columns").as_int();

    for (pugi::xml_node tile = tileset.child("tile"); tile; tile = tile.next_sibling("tile")) {
        int baseId = tile.attribute("id").as_int(-1);
        pugi::xml_node animNode = tile.child("animation");
        if (!animNode) continue;

        std::string name;
        auto it = aliases.find(baseId);
        if (it != aliases.end()) {
            name = it->second;
        }
        else {
            name = "tile_" + std::to_string(baseId);
        }

        Animation clip;
        clip.SetLoop(true);

        for (pugi::xml_node f = animNode.child("frame"); f; f = f.next_sibling("frame")) {
            int frameId = f.attribute("tileid").as_int();
            int duration = f.attribute("duration").as_int(100);
            SDL_Rect r = TileIdToRect(frameId, columns_, tileW_, tileH_);
            clip.AddFrame(r, duration);
        }

        clip.Reset();
        clips_.emplace(name, std::move(clip));
    }

    // pick first as default current
    if (!clips_.empty()) currentName_ = clips_.begin()->first;

    return !clips_.empty();
}

void AnimationSet::SetCurrent(const std::string& name) {
    if (currentName_ == name) return; // no change
    if (!Has(name)) return;           // unknown name
    currentName_ = name;
    clips_[currentName_].Reset();
}

void AnimationSet::ResetCurrent() {
    if (Has(currentName_)) clips_[currentName_].Reset();
}

void AnimationSet::Update(float dtSeconds) {
    if (Has(currentName_)) clips_[currentName_].Update(dtSeconds);
}

const SDL_Rect& AnimationSet::GetCurrentFrame() const {
    static SDL_Rect dummy{ 0,0,0,0 };
    if (Has(currentName_)) return clips_.at(currentName_).GetCurrentFrame();
    return dummy;
}

const std::string& AnimationSet::GetCurrentName() const {
    return currentName_;
}

bool AnimationSet::Has(const std::string& name) const {
    return clips_.find(name) != clips_.end();
}

void AnimationSet::SetLoop(const std::string& name, bool loop) {
    if (Has(name)) {
        clips_[name].SetLoop(loop);
    }
}

bool AnimationSet::HasFinishedOnce(const std::string& name) const {
    if (Has(name)) {
        return clips_.at(name).HasFinishedOnce();
    }
    return false;
}

int AnimationSet::GetCurrentFrameIndex() const {
    if (Has(currentName_)) return clips_.at(currentName_).GetCurrentFrameIndex();
    return 0;
}

bool AnimationSet::LoadSequentialFromTSX(const char* tsxPath, const std::string& clipName, int frameDurationMs)
{
    pugi::xml_document doc;
    pugi::xml_parse_result ok = doc.load_file(tsxPath);
    if (!ok) {
        std::fprintf(stderr, "TSX sequential load failed (%s): %s\n", tsxPath, ok.description());
        return false;
    }

    pugi::xml_node tileset = doc.child("tileset");
    if (!tileset) {
        std::fprintf(stderr, "TSX error: <tileset> missing (%s)\n", tsxPath);
        return false;
    }

    tileW_ = tileset.attribute("tilewidth").as_int();
    tileH_ = tileset.attribute("tileheight").as_int();
    columns_ = tileset.attribute("columns").as_int();
    int tileCount = tileset.attribute("tilecount").as_int();

    Animation clip;
    clip.SetLoop(false);

    for (int i = 0; i < tileCount; ++i) {
        SDL_Rect r = TileIdToRect(i, columns_, tileW_, tileH_);
        clip.AddFrame(r, frameDurationMs);
    }

    clip.Reset();
    clips_.emplace(clipName, std::move(clip));

    if (currentName_.empty()) currentName_ = clipName;

    return true;
}

void Animation::UpdateBackwards(float dt) {
    if (frames_.empty()) return;

    timeInFrameMs_ += static_cast<int>(dt);

    while (timeInFrameMs_ >= frames_[static_cast<size_t>(currentIndex_)].durationMs) {
        timeInFrameMs_ -= frames_[static_cast<size_t>(currentIndex_)].durationMs;

        if (currentIndex_ > 0) {
            --currentIndex_;
        }
        else {
            currentIndex_ = 0;
            break; 
        }
    }
}

void AnimationSet::UpdateBackwards(float dtSeconds) {
    if (Has(currentName_)) clips_[currentName_].UpdateBackwards(dtSeconds);
}