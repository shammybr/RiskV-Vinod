#pragma once
#include "lib/nes_ntsc/nes_ntsc.h"
#include "SDL3/SDL_events.h"
#include <vector>  
#include <cstdint>

struct NoWire {
	SDL_FRect wire;
	NoWire* prox = NULL;

	NoWire(SDL_FRect line) {
		wire = line;

	}
};


struct SWire {
	int iWires = 0;

	NoWire* firstLine = NULL;

	void insertLine(SDL_FRect line) {

		iWires++;

		if (firstLine == NULL) {
			firstLine = new NoWire(line);
			
		}
		else {
			NoWire* prox = firstLine;

			while (prox->prox != NULL) {
			
				prox = prox->prox;
			}	

			prox->prox = new NoWire(line);
		}


	}

};


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
	struct SDL_Texture* CPUtexture;
	struct SDL_AudioSpec spec;
	struct SDL_AudioStream* audioStream;
	SDL_Event event;
	nes_ntsc_t* ntsc;
	nes_ntsc_setup_t setup;
	SDL_FRect gameRect;
	SDL_FRect cpuRect;

	SDL_FRect cpuTopPins[20];
	SDL_FRect cpuTopWires[20];
	SDL_FRect cpuBotPins[20];

	SWire* cpuBotWires[20];


	SDL_FRect lineLeft;
	SDL_FRect lineRight;

	SDL_FRect lineTop;
	SDL_FRect lineBottom;


	//RAM
	SDL_FRect RAMlineLeft;
	SDL_FRect RAMlineRight;

	SDL_FRect RAMlineTop;
	SDL_FRect RAMlineBottom;


	SDL_FRect RAMTopPins[12];
	SDL_FRect RAMBotPins[12];

	SWire* RAMBotWires[12];


	float lineThicc = 4.0f;
	float wireThicc = 2.0f;

	int burst_phase;

	SDLNTSC(int windowW, int windowH);

	void calculateLines();

	void quit();
	void draw();
	void DrawGame();
	void DrawCPU();
	bool poll(uint8_t* controller, std::vector<float>& audioBuffer);

	void playAudio(std::vector<float>& audioBuffer);

};