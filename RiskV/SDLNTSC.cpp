#pragma once
#include "SDLNTSC.h"
#include "SDL3/SDL.h"
#include <iostream>
#include <vector>


SDLNTSC::SDLNTSC() : nesBuffer(NES_WIDTH* NES_HEIGHT, 0),
                    ntscOutput(NTSC_OUT_WIDTH* NTSC_OUT_HEIGHT, 0) 
{

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL Init Failed: " << SDL_GetError() << std::endl;
    }

    else {
        // Create Window and Hardware Renderer
        window = SDL_CreateWindow("NES NTSC Emulator", NTSC_OUT_WIDTH, NTSC_OUT_HEIGHT * 2, 0);
        renderer = SDL_CreateRenderer(window, NULL);

        // Create the Streaming Texture
        // NOTE: nes_ntsc outputs 16-bit RGB565 pixels by default unless configured otherwise.
        texture = SDL_CreateTexture(renderer,
            SDL_PIXELFORMAT_RGB565,
            SDL_TEXTUREACCESS_STREAMING,
            NTSC_OUT_WIDTH,
            NTSC_OUT_HEIGHT);


        ntsc = (nes_ntsc_t*)malloc(sizeof(nes_ntsc_t));
        setup = nes_ntsc_composite; // Or nes_ntsc_svideo, nes_ntsc_rgb
        nes_ntsc_init(ntsc, &setup);




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
    // Toggle the NTSC artifact phase every frame for authentic shimmering
    burst_phase ^= 1;

    // 4. Run the NTSC Filter
    // This takes your 8-bit NES array, applies the CRT math, and fills the 16-bit NTSC array
    nes_ntsc_blit(ntsc,
        nesBuffer.data(), NES_WIDTH, // Input buffer and pitch (width)
        burst_phase,                 // Alternating artifact phase
        NES_WIDTH, NES_HEIGHT,       // Input dimensions
        ntscOutput.data(),           // Output buffer
        NTSC_OUT_WIDTH * sizeof(uint16_t)); // Output pitch in bytes

    // 5. Push to the GPU and render
    SDL_UpdateTexture(texture, NULL, ntscOutput.data(), NTSC_OUT_WIDTH * sizeof(uint16_t));

    SDL_RenderClear(renderer);
    SDL_RenderTexture(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

bool SDLNTSC::poll() {
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            return false;
        }
    }

    return true;
}
