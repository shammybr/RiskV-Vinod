#pragma once
#include <cstdint>

class NES;
class NESLogger;
enum EMirrorMode;

class PPU {
public:
	//registers
	uint8_t status = 0x00; // 0x2002
	uint8_t ctrl = 0x00;   // 0x2000
	uint8_t mask = 0x00;   // 0x2001

	uint16_t scanline = 0; //  0 - 261
	uint16_t cycle = 0;    //  0 - 340


	bool nmiSignal = false; 


	NES* nes = nullptr;
	uint8_t palette[32]{};
	uint8_t vram[2048]{};
	uint16_t vramAddress = 0x0000;

	// false =  high byte, true = low byte (w)
	bool addressLatch = false;

	// for 0x2007
	uint8_t ppuDataBuffer = 0x00;


	PPU(NES* _nes);

	uint8_t ppuRead(uint16_t address);
	void ppuWrite(uint16_t address, uint8_t data);

	uint8_t cpuRead(uint16_t registerN);
	void cpuWrite(uint16_t registerN, uint8_t data);
	uint8_t step(NESLogger* logger);

	uint16_t mirrorAddress(uint16_t address, EMirrorMode mirrorMode);
};