#include "Cinematics.h"
#include "Engine.h"
#include "Render.h"
#include "Window.h"
#include "Input.h"
#include "Log.h"

// FFmpeg headers (C linkage)
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

Cinematics::Cinematics() : Module()
{
	name = "cinematics";
}

Cinematics::~Cinematics()
{
}

bool Cinematics::Awake()
{
	LOG("Cinematics module awake");
	return true;
}

bool Cinematics::Start()
{
	return true;
}

bool Cinematics::Update(float dt)
{
	if (!playing) return true;

	// Check for skip (Space or Escape or Enter)
	if (Engine::GetInstance().input->GetKey(SDL_SCANCODE_SPACE) == KEY_DOWN ||
		Engine::GetInstance().input->GetKey(SDL_SCANCODE_RETURN) == KEY_DOWN)
	{
		skipRequested = true;
	}

	if (skipRequested) {
		StopVideo();
		return true;
	}

	// Accumulate time
	elapsedMs += dt;
	double elapsedSec = elapsedMs / 1000.0;

	// Decode frames until we catch up to the current time
	while (elapsedSec >= videoFramePts) {
		if (!DecodeNextFrame()) {
			// End of video
			StopVideo();
			return true;
		}
	}

	return true;
}

bool Cinematics::PostUpdate()
{
	if (!playing) return true;

	RenderFrame();

	return true;
}

bool Cinematics::CleanUp()
{
	CloseVideo();
	return true;
}

// ===================================================================
// Public API
// ===================================================================

bool Cinematics::PlayVideo(const char* path)
{
	if (playing) {
		CloseVideo();
	}

	if (!OpenVideo(path)) {
		LOG("Cinematics: failed to open video: %s", path);
		return false;
	}

	playing = true;
	skipRequested = false;
	elapsedMs = 0.0f;
	videoClock = 0.0;
	videoFramePts = 0.0;

	LOG("Cinematics: playing %s (%dx%d)", path, videoWidth, videoHeight);

	// Decode the first frame immediately so we have something to show
	DecodeNextFrame();

	return true;
}

void Cinematics::StopVideo()
{
	if (!playing) return;
	LOG("Cinematics: stopped");
	playing = false;
	CloseVideo();
}

bool Cinematics::IsPlaying() const
{
	return playing;
}

// ===================================================================
// Internal: Open / Close
// ===================================================================

bool Cinematics::OpenVideo(const char* path)
{
	// Open container
	if (avformat_open_input(&fmtCtx, path, nullptr, nullptr) < 0) {
		LOG("Cinematics: avformat_open_input failed for %s", path);
		return false;
	}

	if (avformat_find_stream_info(fmtCtx, nullptr) < 0) {
		LOG("Cinematics: avformat_find_stream_info failed");
		CloseVideo();
		return false;
	}

	// Find video stream
	videoStreamIdx = -1;
	audioStreamIdx = -1;
	for (unsigned i = 0; i < fmtCtx->nb_streams; i++) {
		if (fmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && videoStreamIdx < 0) {
			videoStreamIdx = (int)i;
		}
		if (fmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && audioStreamIdx < 0) {
			audioStreamIdx = (int)i;
		}
	}

	if (videoStreamIdx < 0) {
		LOG("Cinematics: no video stream found in %s", path);
		CloseVideo();
		return false;
	}

	// Open video codec
	const AVCodec* vCodec = avcodec_find_decoder(fmtCtx->streams[videoStreamIdx]->codecpar->codec_id);
	if (!vCodec) {
		LOG("Cinematics: video decoder not found");
		CloseVideo();
		return false;
	}

	videoCodecCtx = avcodec_alloc_context3(vCodec);
	avcodec_parameters_to_context(videoCodecCtx, fmtCtx->streams[videoStreamIdx]->codecpar);
	if (avcodec_open2(videoCodecCtx, vCodec, nullptr) < 0) {
		LOG("Cinematics: avcodec_open2 failed for video");
		CloseVideo();
		return false;
	}

	videoWidth = videoCodecCtx->width;
	videoHeight = videoCodecCtx->height;
	AVRational tb = fmtCtx->streams[videoStreamIdx]->time_base;
	timeBase = av_q2d(tb);

	// Open audio codec (optional — don't fail if it doesn't work)
	if (audioStreamIdx >= 0) {
		const AVCodec* aCodec = avcodec_find_decoder(fmtCtx->streams[audioStreamIdx]->codecpar->codec_id);
		if (aCodec) {
			audioCodecCtx = avcodec_alloc_context3(aCodec);
			avcodec_parameters_to_context(audioCodecCtx, fmtCtx->streams[audioStreamIdx]->codecpar);
			if (avcodec_open2(audioCodecCtx, aCodec, nullptr) < 0) {
				avcodec_free_context(&audioCodecCtx);
				audioCodecCtx = nullptr;
				audioStreamIdx = -1;
			}
		}
	}

	// Set up audio resampler + SDL audio stream
	if (audioCodecCtx) {
		AVChannelLayout outLayout = AV_CHANNEL_LAYOUT_STEREO;
		int ret = swr_alloc_set_opts2(&swrCtx,
			&outLayout, AV_SAMPLE_FMT_FLT, 48000,
			&audioCodecCtx->ch_layout, audioCodecCtx->sample_fmt, audioCodecCtx->sample_rate,
			0, nullptr);
		if (ret < 0 || swr_init(swrCtx) < 0) {
			swr_free(&swrCtx);
			swrCtx = nullptr;
		}

		// Create SDL audio stream for cinematic audio
		SDL_AudioSpec srcSpec{};
		srcSpec.format = SDL_AUDIO_F32;
		srcSpec.channels = 2;
		srcSpec.freq = 48000;

		audioDevice = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &srcSpec);
		if (audioDevice != 0) {
			audioStream = SDL_CreateAudioStream(&srcSpec, &srcSpec);
			if (audioStream) {
				SDL_BindAudioStream(audioDevice, audioStream);
				SDL_ResumeAudioDevice(audioDevice);
			}
		}
	}

	// Allocate frames and packet
	frame = av_frame_alloc();
	rgbaFrame = av_frame_alloc();
	packet = av_packet_alloc();

	// Set up RGBA frame buffer
	rgbaFrame->format = AV_PIX_FMT_RGBA;
	rgbaFrame->width = videoWidth;
	rgbaFrame->height = videoHeight;
	av_image_alloc(rgbaFrame->data, rgbaFrame->linesize, videoWidth, videoHeight, AV_PIX_FMT_RGBA, 32);

	// Set up pixel format converter
	swsCtx = sws_getContext(
		videoWidth, videoHeight, videoCodecCtx->pix_fmt,
		videoWidth, videoHeight, AV_PIX_FMT_RGBA,
		SWS_BILINEAR, nullptr, nullptr, nullptr);

	// Create SDL texture for rendering
	SDL_Renderer* renderer = Engine::GetInstance().render->renderer;
	videoTexture = SDL_CreateTexture(renderer,
		SDL_PIXELFORMAT_RGBA32,
		SDL_TEXTUREACCESS_STREAMING,
		videoWidth, videoHeight);

	if (!videoTexture) {
		LOG("Cinematics: SDL_CreateTexture failed: %s", SDL_GetError());
		CloseVideo();
		return false;
	}

	return true;
}

void Cinematics::CloseVideo()
{
	if (videoTexture) {
		SDL_DestroyTexture(videoTexture);
		videoTexture = nullptr;
	}

	if (swsCtx) {
		sws_freeContext(swsCtx);
		swsCtx = nullptr;
	}

	if (swrCtx) {
		swr_free(&swrCtx);
		swrCtx = nullptr;
	}

	if (audioStream) {
		SDL_DestroyAudioStream(audioStream);
		audioStream = nullptr;
	}
	if (audioDevice != 0) {
		SDL_CloseAudioDevice(audioDevice);
		audioDevice = 0;
	}

	if (rgbaFrame) {
		if (rgbaFrame->data[0]) {
			av_freep(&rgbaFrame->data[0]);
		}
		av_frame_free(&rgbaFrame);
		rgbaFrame = nullptr;
	}

	if (frame) {
		av_frame_free(&frame);
		frame = nullptr;
	}

	if (packet) {
		av_packet_free(&packet);
		packet = nullptr;
	}

	if (videoCodecCtx) {
		avcodec_free_context(&videoCodecCtx);
		videoCodecCtx = nullptr;
	}

	if (audioCodecCtx) {
		avcodec_free_context(&audioCodecCtx);
		audioCodecCtx = nullptr;
	}

	if (fmtCtx) {
		avformat_close_input(&fmtCtx);
		fmtCtx = nullptr;
	}

	videoStreamIdx = -1;
	audioStreamIdx = -1;
	videoWidth = 0;
	videoHeight = 0;
}

// ===================================================================
// Internal: Decode and render
// ===================================================================

bool Cinematics::DecodeNextFrame()
{
	while (av_read_frame(fmtCtx, packet) >= 0)
	{
		if (packet->stream_index == videoStreamIdx)
		{
			int ret = avcodec_send_packet(videoCodecCtx, packet);
			av_packet_unref(packet);
			if (ret < 0) continue;

			ret = avcodec_receive_frame(videoCodecCtx, frame);
			if (ret == AVERROR(EAGAIN)) continue;
			if (ret < 0) return false; // decode error or EOF

			// Update PTS
			if (frame->pts != AV_NOPTS_VALUE) {
				videoFramePts = frame->pts * timeBase;
			}

			// Convert to RGBA
			sws_scale(swsCtx,
				frame->data, frame->linesize, 0, videoHeight,
				rgbaFrame->data, rgbaFrame->linesize);

			// Update SDL texture
			SDL_UpdateTexture(videoTexture, nullptr, rgbaFrame->data[0], rgbaFrame->linesize[0]);

			return true;
		}
		else if (packet->stream_index == audioStreamIdx && audioCodecCtx && swrCtx && audioStream)
		{
			// Decode audio and push to SDL stream
			int ret = avcodec_send_packet(audioCodecCtx, packet);
			av_packet_unref(packet);
			if (ret < 0) continue;

			AVFrame* aFrame = av_frame_alloc();
			while (avcodec_receive_frame(audioCodecCtx, aFrame) >= 0) {
				// Resample to float32 stereo 48kHz
				int outSamples = swr_get_out_samples(swrCtx, aFrame->nb_samples);
				int bufSize = outSamples * 2 * sizeof(float); // 2 channels, float32
				uint8_t* outBuf = (uint8_t*)av_malloc(bufSize);
				if (outBuf) {
					int converted = swr_convert(swrCtx, &outBuf, outSamples,
						(const uint8_t**)aFrame->data, aFrame->nb_samples);
					if (converted > 0) {
						SDL_PutAudioStreamData(audioStream, outBuf, converted * 2 * sizeof(float));
					}
					av_free(outBuf);
				}
			}
			av_frame_free(&aFrame);
		}
		else {
			av_packet_unref(packet);
		}
	}

	// If we got here, av_read_frame returned < 0 → end of file
	return false;
}

void Cinematics::RenderFrame()
{
	if (!videoTexture) return;

	SDL_Renderer* renderer = Engine::GetInstance().render->renderer;

	// Calculate letterboxed destination rect to maintain aspect ratio
	int winW = 0, winH = 0;
	Engine::GetInstance().window->GetWindowSize(winW, winH);

	float videoAspect = (float)videoWidth / (float)videoHeight;
	float windowAspect = (float)winW / (float)winH;

	SDL_FRect dst;
	if (videoAspect > windowAspect) {
		// Pillarbox (video wider — fit to width)
		dst.w = (float)winW;
		dst.h = (float)winW / videoAspect;
		dst.x = 0;
		dst.y = ((float)winH - dst.h) / 2.0f;
	} else {
		// Letterbox (video taller — fit to height)
		dst.h = (float)winH;
		dst.w = (float)winH * videoAspect;
		dst.x = ((float)winW - dst.w) / 2.0f;
		dst.y = 0;
	}

	// Clear screen to black and draw the video frame
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);
	SDL_RenderTexture(renderer, videoTexture, nullptr, &dst);
}
