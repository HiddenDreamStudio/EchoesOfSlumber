#include "Audio.h"
#include "Log.h"

Audio::Audio() {
    name = "audio";
}

Audio::~Audio() {
    // Make sure everything is freed in CleanUp
}

bool Audio::LoadWavFile(const char* path, SoundData& out) {
    // SDL_LoadWAV fills spec + allocates buf; free with SDL_free() later.
    if (!SDL_LoadWAV(path, &out.spec, &out.buf, &out.len)) {
        SDL_Log("SDL_LoadWAV failed for %s: %s", path, SDL_GetError());
        return false;
    }
    return true;
}

void Audio::FreeSound(SoundData& s) {
    if (s.buf) {
        SDL_free(s.buf);
        s.buf = nullptr;
        s.len = 0;
        s.spec = SDL_AudioSpec{};
    }
}

bool Audio::EnsureDeviceOpen() {
    if (device_ != 0) return true;

    // Ask for a reasonable default device format (float32, stereo, 48k).
    SDL_AudioSpec want{};
    want.format = SDL_AUDIO_F32;
    want.channels = 2;
    want.freq = 48000;

    device_ = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &want);
    if (device_ == 0) {
        LOG("Audio: SDL_OpenAudioDevice failed: %s", SDL_GetError());
        return false;
    }

    // Query actual device format (may differ from 'want')
    if (!SDL_GetAudioDeviceFormat(device_, &device_spec_, nullptr)) {
        LOG("Audio: SDL_GetAudioDeviceFormat failed: %s", SDL_GetError());
        SDL_CloseAudioDevice(device_);
        device_ = 0;
        return false;
    }

    // Start audio
    SDL_ResumeAudioDevice(device_);

    return true;
}

bool Audio::EnsureStreams() {
    if (!EnsureDeviceOpen()) return false;

    if (!music_stream_) {
        music_stream_ = SDL_CreateAudioStream(nullptr, &device_spec_);
        if (!music_stream_) {
            LOG("Audio: SDL_CreateAudioStream (music) failed: %s", SDL_GetError());
            return false;
        }
        if (!SDL_BindAudioStream(device_, music_stream_)) {
            LOG("Audio: SDL_BindAudioStream (music) failed: %s", SDL_GetError());
            SDL_DestroyAudioStream(music_stream_);
            music_stream_ = nullptr;
            return false;
        }
    }

	// Set music volume
    SDL_SetAudioStreamGain(music_stream_, music_volume_);

    if (!sfx_stream_) {
        sfx_stream_ = SDL_CreateAudioStream(nullptr, &device_spec_);
        if (!sfx_stream_) {
            LOG("Audio: SDL_CreateAudioStream (sfx) failed: %s", SDL_GetError());
            return false;
        }
        if (!SDL_BindAudioStream(device_, sfx_stream_)) {
            LOG("Audio: SDL_BindAudioStream (sfx) failed: %s", SDL_GetError());
            SDL_DestroyAudioStream(sfx_stream_);
            sfx_stream_ = nullptr;
            return false;
        }
    }

	// Set SFX volume
    SDL_SetAudioStreamGain(sfx_stream_, sfx_volume_);

    return true;
}


bool Audio::Awake() {
    LOG("Audio: initializing SDL3 audio");
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != true /* SDL3 returns bool */) {
        LOG("SDL_INIT_AUDIO failed: %s", SDL_GetError());
        active = false;
        return true; // don't hard-fail the app
    }

    if (!EnsureDeviceOpen()) {
        active = false;
        return true;
    }

    return true;
}

bool Audio::Update(float dt) {
    if (active && music_stream_ && music_data_.buf != nullptr) {
        if (SDL_GetAudioStreamAvailable(music_stream_) < (int)music_data_.len) {
            SDL_PutAudioStreamData(music_stream_, music_data_.buf, music_data_.len);
        }
    }
    return true;
}

bool Audio::CleanUp() {
    // If audio is inactive or already quit elsewhere, don't touch SDL objects.
    if (!active || !SDL_WasInit(SDL_INIT_AUDIO)) {
        music_stream_ = nullptr;
        sfx_stream_ = nullptr;
        device_ = 0;
        sfx_.clear();
        FreeSound(music_data_);
        return true;
    }

    LOG("Audio: cleaning up");

    // Optional: stop pulling data while we tear down.
    if (device_ != 0) SDL_PauseAudioDevice(device_);

    // Destroy streams (auto-unbinds if bound).
    if (music_stream_) {
        SDL_DestroyAudioStream(music_stream_);
        music_stream_ = nullptr;
    }
    FreeSound(music_data_);

    if (sfx_stream_) {
        SDL_DestroyAudioStream(sfx_stream_);
        sfx_stream_ = nullptr;
    }
    for (auto& s : sfx_) FreeSound(s);
    sfx_.clear();

    // Close device after streams are gone.
    if (device_ != 0) {
        SDL_CloseAudioDevice(device_);
        device_ = 0;
    }

    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    active = false;
    return true;
}

bool Audio::PlayMusic(const char* path, float fadeTime) {
    if (!active) return false;
    if (!EnsureStreams()) return false;

    // Stop any existing music: clear stream + free buffer
    if (music_stream_) {
        SDL_ClearAudioStream(music_stream_);
    }
    FreeSound(music_data_);

    if (path == nullptr) {
        return true;
    }

    // Load WAV into memory
    if (!LoadWavFile(path, music_data_)) {
        LOG("Audio: cannot load music %s: %s", path, SDL_GetError());
        return false;
    }

    // Set input format of the stream to match this file
    if (!SDL_SetAudioStreamFormat(music_stream_, &music_data_.spec, &device_spec_)) {
        LOG("Audio: SDL_SetAudioStreamFormat(music) failed: %s", SDL_GetError());
        return false;
    }

    // Queue once (simple play). For looping, requeue when drained (TODO).
    if (!SDL_PutAudioStreamData(music_stream_, music_data_.buf, music_data_.len)) {
        LOG("Audio: SDL_PutAudioStreamData(music) failed: %s", SDL_GetError());
        return false;
    }

    LOG("Audio: playing music %s", path);
    return true;
}

int Audio::LoadFx(const char* path) {
    if (!active) return 0;
    if (!EnsureStreams()) return 0;

    SoundData s{};
    if (!LoadWavFile(path, s)) {
        LOG("Audio: cannot load fx %s: %s", path, SDL_GetError());
        return 0;
    }

    sfx_.push_back(s);
    return static_cast<int>(sfx_.size()); // 1-based outward index
}

bool Audio::PlayFx(int id, int repeat) {
    if (!active) return false;
    if (id <= 0 || id > static_cast<int>(sfx_.size())) return false;
    if (!EnsureStreams()) return false;

    const SoundData& s = sfx_[static_cast<size_t>(id - 1)];

    // Make sure the SFX stream input format matches this sound
    if (!SDL_SetAudioStreamFormat(sfx_stream_, &s.spec, &device_spec_)) {
        LOG("Audio: SDL_SetAudioStreamFormat(sfx) failed: %s", SDL_GetError());
        return false;
    }

    SDL_ClearAudioStream(sfx_stream_);

    // Queue sound 'repeat+1' times
    for (int i = 0; i <= repeat; ++i) {
        if (!SDL_PutAudioStreamData(sfx_stream_, s.buf, s.len)) {
            LOG("Audio: SDL_PutAudioStreamData(sfx) failed: %s", SDL_GetError());
            return false;
        }
    }

    return true;
}

void Audio::SetMusicVolume(float volume)
{
    // clamp
    if (volume < 0.0f) volume = 0.0f;
    else if (volume > 1.0f) volume = 1.0f;

    music_volume_ = volume;

    if (music_stream_) {
        SDL_SetAudioStreamGain(music_stream_, music_volume_);
    }
}

void Audio::SetSFXVolume(float volume)
{
    // clamp
    if (volume < 0.0f) volume = 0.0f;
    else if (volume > 1.0f) volume = 1.0f;

    sfx_volume_ = volume;

    if (sfx_stream_) {
        SDL_SetAudioStreamGain(sfx_stream_, sfx_volume_);
    }
}
