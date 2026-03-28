#pragma once
#include "lib/nes_ntsc/nes_ntsc.h"
#include "SDL3/SDL_events.h"
#include <vector>  
#include <cstdint>

class SDLNTSC {


public:
	static const int NES_WIDTH = 256;
	static const int NES_HEIGHT = 240;


	static const int NTSC_OUT_WIDTH = NES_NTSC_OUT_WIDTH(NES_WIDTH);
	static const int NTSC_OUT_HEIGHT = NES_HEIGHT;

	std::vector<uint8_t> nesBuffer;
	std::vector<uint16_t> ntscOutput;


	struct SDL_Window* window;
	struct SDL_Renderer* renderer;
	struct SDL_Texture* texture;
	SDL_Event event;
	nes_ntsc_t* ntsc;
	nes_ntsc_setup_t setup;

	int burst_phase;

	SDLNTSC();

	void quit();
	void draw();
	bool poll(uint8_t *controller);

};