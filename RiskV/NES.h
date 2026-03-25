#pragma once
#include <vector>

#define INTERNALSTART 0x0000
#define INTERNALEND 0x07FF
#define PPUSTART 0x2000
#define PPUEND 0x2007
#define APUSTART 0x4000
#define APUEND 0x401F
#define ROMSTART 0x8000
#define ROMEND 0xFFFF


class NES {
	uint8_t memory[0x10000] = {};
	uint8_t regA = 0x000;
	uint8_t regX = 0x000;
	uint8_t regY = 0x000;
	uint8_t regSP = 0x000;
	uint8_t regP = 0x000;
	uint16_t regPC = 0x000;


public:
	NES();

	uint8_t read(uint16_t address);
	void write(uint16_t address, uint8_t data);

	void reset();

	void step();
}

