#pragma once
#include <cstdint>

class NES;
class NESLogger;
enum EMirrorMode;
struct ScanlineSprite {
	uint8_t y;
	uint8_t tile;
	uint8_t attr;
	uint8_t x;
	uint8_t originalIndex; 
};


class PPU {
public:
	uint8_t oam[256];
	uint8_t oamAddress = 0;
	uint8_t ppuDataLatch = 0;


	//registers
	uint8_t status = 0x00; // 0x2002
	uint8_t ctrl = 0x00;   // 0x2000
	uint8_t mask = 0x00;   // 0x2001

	// latches
	uint8_t bgNextTileID = 0x00;
	uint8_t bgNextTileAttr = 0x00;
	uint8_t bgNextTileLSB = 0x00;
	uint8_t bgNextTileMSB = 0x00;

	// shift registers ---
	uint16_t bgShifterPatternLow = 0x0000;
	uint16_t bgShifterPatternHigh = 0x0000;
	uint16_t bgShifterAttrLow = 0x0000;
	uint16_t bgShifterAttrHigh = 0x0000;

	//  v (15 bits)
	uint16_t vramAddress = 0x0000;

	// 15 bits
	uint16_t tempVramAddress = 0x0000;

	// 3 bits
	uint8_t fineX = 0x00;

	// false =  high byte, true = low byte (w)
	bool addressLatch = false;


	bool oddFrame = false;
	bool pixelMode = false;
	bool lineMode = false;
	int pixelsToWait = 1;
	int16_t scanline = 0; //  0 - 261
	uint16_t cycle = 0;    //  0 - 340


	bool nmiSignal = false; 


	NES* nes = nullptr;
	uint8_t palette[32]{};
	uint8_t vram[2048]{};
	uint8_t* videoBuffer = nullptr;


	ScanlineSprite scanlineSprites[8];
	int spriteCount = 0;

	// for 0x2007
	uint8_t ppuDataBuffer = 0x00;


	PPU(NES* _nes);
	void incrementScrollY();
	void transferAddressX();
	void transferAddressY();
	uint8_t step(NESLogger* logger);
	uint8_t cpuRead(uint16_t registerN);
	uint8_t ppuRead(uint16_t address);
	uint8_t fetchAttributeByte();

	uint8_t fetchPatternTableLow();

	uint16_t mirrorAddress(uint16_t address, EMirrorMode mirrorMode);
	void incrementScrollX();
	uint8_t fetchPatternTableHigh();
	void loadBackgroundShifters();
	void updateShifters();
	void ppuWrite(uint16_t address, uint8_t data);
	uint8_t logCpuRead(uint16_t registerN);
	void cpuWrite(uint16_t registerN, uint8_t data);
	void drawPixel();


	void evaluateSprites();
};