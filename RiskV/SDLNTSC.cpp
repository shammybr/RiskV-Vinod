#pragma once
#include "SDLNTSC.h"
#include "SDL3/SDL.h"
#include <iostream>
#include <vector>


SDLNTSC::SDLNTSC() : nesBuffer(NES_WIDTH* NES_HEIGHT, 0),
                    ntscOutput(NTSC_OUT_WIDTH* NTSC_OUT_HEIGHT, 0) 
{

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL Init Failed: " << SDL_GetError() << std::endl;
    }

    else {
       
        window = SDL_CreateWindow("NES NTSC Emulator", NTSC_OUT_WIDTH, NTSC_OUT_HEIGHT * 2, 0);
        renderer = SDL_CreateRenderer(window, NULL);
        //vsync
        SDL_SetRenderVSync(renderer, 1);


        texture = SDL_CreateTexture(renderer,
            SDL_PIXELFORMAT_RGB565,
            SDL_TEXTUREACCESS_STREAMING,
            NTSC_OUT_WIDTH,
            NTSC_OUT_HEIGHT);


        ntsc = (nes_ntsc_t*)malloc(sizeof(nes_ntsc_t));
        setup = nes_ntsc_composite; 
        nes_ntsc_init(ntsc, &setup);


       
        spec.freq = 44100;
        spec.format = SDL_AUDIO_F32;
        spec.channels = 1;

       
        audioStream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
        SDL_ResumeAudioStreamDevice(audioStream);



        burst_phase = 0;

    }



}

void SDLNTSC::quit(){
    free(ntsc);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void SDLNTSC::draw(){
   
    burst_phase ^= 1;


    nes_ntsc_blit(ntsc,
        nesBuffer.data(), NES_WIDTH, 
        burst_phase,                 
        NES_WIDTH, NES_HEIGHT,       
        ntscOutput.data(),           
        NTSC_OUT_WIDTH * sizeof(uint16_t)); 

   
    SDL_UpdateTexture(texture, NULL, ntscOutput.data(), NTSC_OUT_WIDTH * sizeof(uint16_t));

    SDL_RenderClear(renderer);
    SDL_RenderTexture(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

bool SDLNTSC::poll(uint8_t *controller, std::vector<float>& audioBuffer) {
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP) {
            bool pressed = (event.type == SDL_EVENT_KEY_DOWN);
            uint8_t bitmask = 0;

   
            switch (event.key.key) {
            case SDLK_J:            bitmask = 0b10000000; break; // A
            case SDLK_K:            bitmask = 0b01000000; break; // B
            case SDLK_RSHIFT:  bitmask = 0b00100000; break; // Select
            case SDLK_RETURN:       bitmask = 0b00010000; break; // Start
            case SDLK_W:           bitmask = 0b00001000; break; // Up
            case SDLK_S:         bitmask = 0b00000100; break; // Down
            case SDLK_A:         bitmask = 0b00000010; break; // Left
            case SDLK_D:        bitmask = 0b00000001; break; // Right
            }

            if (pressed) {
                controller[0] |= bitmask;  
            }
            else {
                controller[0] &= ~bitmask; 
            }
        }




        else if (event.type == SDL_EVENT_QUIT) {
            return false;
        }
    }

    return true;
}

void SDLNTSC::playAudio(std::vector<float>& audioBuffer) {

    if (audioBuffer.size() > 0) {

        int max_queued_bytes = 4410 * sizeof(float);

        int currently_queued = SDL_GetAudioStreamQueued(audioStream);

        if (currently_queued < max_queued_bytes) {


            SDL_PutAudioStreamData(audioStream,
                audioBuffer.data(),
                audioBuffer.size() * sizeof(float));

        }

        audioBuffer.clear();
    }
}