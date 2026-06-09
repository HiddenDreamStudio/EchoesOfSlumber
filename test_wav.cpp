#include <SDL3/SDL.h>
#include <iostream>

int main() {
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        std::cerr << "SDL_InitSubSystem failed: " << SDL_GetError() << std::endl;
        return 1;
    }
    
    SDL_AudioSpec spec;
    Uint8* buf = nullptr;
    Uint32 len = 0;
    
    if (!SDL_LoadWAV("assets/audio/fx/Tirachines/slingshot_shoot.wav", &spec, &buf, &len)) {
        std::cerr << "SDL_LoadWAV failed: " << SDL_GetError() << std::endl;
    } else {
        std::cout << "Successfully loaded WAV! Length: " << len << " bytes. Channels: " << spec.channels << " Freq: " << spec.freq << std::endl;
        SDL_free(buf);
    }
    
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    return 0;
}
