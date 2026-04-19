#pragma once

#include "Module.h"
#include <SDL3/SDL.h>
#include <string>

// Forward declarations for FFmpeg (avoid pulling in heavy headers everywhere)
struct AVFormatContext;
struct AVCodecContext;
struct AVFrame;
struct AVPacket;
struct SwsContext;
struct SwrContext;

class Cinematics : public Module
{
public:

	Cinematics();
	virtual ~Cinematics();

	bool Awake() override;
	bool Start() override;
	bool Update(float dt) override;
	bool PostUpdate() override;
	bool CleanUp() override;

	// Play a video file as a full-screen cinematic
	// Returns false if the file can't be opened
	bool PlayVideo(const char* path);

	// Stop the current cinematic immediately
	void StopVideo();

	// Returns true while a cinematic is playing
	bool IsPlaying() const;

private:

	// Internal helpers
	bool OpenVideo(const char* path);
	void CloseVideo();
	bool DecodeNextFrame(bool skipRender = false);
	void RenderFrame();

	// FFmpeg state
	AVFormatContext* fmtCtx = nullptr;
	AVCodecContext*  videoCodecCtx = nullptr;
	AVFrame*         frame = nullptr;
	AVFrame*         rgbaFrame = nullptr;
	AVPacket*        packet = nullptr;
	SwsContext*      swsCtx = nullptr;

	// Audio (optional)
	AVCodecContext*  audioCodecCtx = nullptr;
	SwrContext*      swrCtx = nullptr;
	SDL_AudioStream* audioStream = nullptr;

	int videoStreamIdx = -1;
	int audioStreamIdx = -1;

	// SDL rendering
	SDL_Texture*    videoTexture = nullptr;
	int videoWidth = 0;
	int videoHeight = 0;

	// Playback timing
	bool playing = false;
	double videoClock = 0.0;       // current presentation time (seconds)
	double videoFramePts = 0.0;    // PTS of the last decoded frame
	double timeBase = 0.0;         // stream time_base as double
	float  elapsedMs = 0.0f;       // accumulated time since play started

	// Skip control
	bool skipRequested = false;

	// Audio device (borrowed from Audio module)
	SDL_AudioDeviceID audioDevice = 0;
};
