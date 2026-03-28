#pragma once
#include "PPU.h"
#include "NES.h"
#include <cstdint>
#include "NESROM.h"
#include "NESLogger.h"


PPU::PPU(NES* _nes) {
	nes = _nes;
	cycle = 0;
	scanline = 0;
}

uint8_t PPU::ppuRead(uint16_t address) {
	address = address & PPUENDMIRROED;


	if (address <= PPUADDRESSROMEND) {
		if (nes) {
			if (nes->currentRom) {
				return nes->currentRom->read(address);
			}
		}
	}

	else if (address >= PPUSTART && address <= PPUADDRESSVRAMEND) {
		address = address & 0x0FFF;


		if (nes) {
			if (nes->currentRom) {
				uint16_t mirroredAddress = mirrorAddress(address, nes->currentRom->mirrorMode);

				return vram[mirroredAddress];
			}
		}
		


		return 0x00;

	}

	else if (address >= PPUADDRESSPALLETESTART && address <= PPUENDMIRROED) {

		address = address & 0x001F;

		//aceita que doi menos
		if (address == 0x0010) address = 0x0000;
		if (address == 0x0014) address = 0x0004;
		if (address == 0x0018) address = 0x0008;
		if (address == 0x001C) address = 0x000C;


		return palette[address];

	}

	return 0;
}

uint8_t PPU::fetchAttributeByte() {
	uint16_t address = 0x23C0 | (vramAddress & 0x0C00) | ((vramAddress >> 4) & 0x38) | ((vramAddress >> 2) & 0x07);
	uint8_t attributeByte = ppuRead(address);


	if (vramAddress & 0b0100'0000) {
		attributeByte >>= 4;
	}


	if (vramAddress & 0b0000'0010) {
		attributeByte >>= 2;
	}

	return attributeByte & 0b0011;
}
uint8_t PPU::fetchPatternTableLow() {

	uint16_t tableSide = (ctrl & 0x10) ? 0x1000 : 0x0000;

	uint16_t address = tableSide + (bgNextTileID << 4) + ((vramAddress >> 12) & 0x07);


	return ppuRead(address);
}


uint8_t PPU::fetchPatternTableHigh() {
	uint16_t table_side = (ctrl & 0x10) ? 0x1000 : 0x0000;
	uint16_t address = table_side + (bgNextTileID << 4) + ((vramAddress >> 12) & 0x07);

	return ppuRead(address + 8);

}

void PPU::loadBackgroundShifters() {
	
	bgShifterPatternLow = (bgShifterPatternLow & 0xFF00) | bgNextTileLSB;
	bgShifterPatternHigh = (bgShifterPatternHigh & 0xFF00) | bgNextTileMSB;

	//big nvidia inflation move
	bgShifterAttrLow = (bgShifterAttrLow & 0xFF00) | ((bgNextTileAttr & 0b01) ? 0xFF : 0x00);
	bgShifterAttrHigh = (bgShifterAttrHigh & 0xFF00) | ((bgNextTileAttr & 0b10) ? 0xFF : 0x00);
}

void PPU::updateShifters() {
	// 0x2001
	if (mask & 0b1000) {
		bgShifterPatternLow <<= 1;
		bgShifterPatternHigh <<= 1;
		bgShifterAttrLow <<= 1;
		bgShifterAttrHigh <<= 1;
	}
}

void PPU::ppuWrite(uint16_t address, uint8_t data) {

	address = address & PPUENDMIRROED;


	if (address <= PPUADDRESSROMEND) {
		if (nes) {
			if (nes->currentRom) {
				nes->currentRom->write(address, data);
			}
		}

	}

	else if (address >= PPUSTART && address <= PPUADDRESSVRAMEND) {
		address = address & 0x0FFF;



		if (nes) {
			if (nes->currentRom) {
				uint16_t mirroredAddress = mirrorAddress(address, nes->currentRom->mirrorMode);

				vram[mirroredAddress] = data;
			}
		}

	}

	else if (address >= PPUADDRESSPALLETESTART && address <= PPUENDMIRROED) {

		address = address & 0x001F;

		//aceita que doi menos
		if (address == 0x0010) address = 0x0000;
		if (address == 0x0014) address = 0x0004;
		if (address == 0x0018) address = 0x0008;
		if (address == 0x001C) address = 0x000C;


		palette[address] = data;
	}


}

uint8_t PPU::cpuRead(uint16_t registerN){
	uint8_t data = 0x00;

	switch (registerN) {
		case 2:
			data = status;

			//bit 7 vai pra 0
			status &= ~0b1000'0000;
			addressLatch = false;

			nmiSignal = false;
			break;

		case 4: //OAMDATA
			data = oam[oamAddress];

			break;
		case 7: 
			
			data = ppuDataBuffer;
			ppuDataBuffer = ppuRead(vramAddress);

			if (vramAddress >= 0x3F00) {
				data = ppuDataBuffer;
			}

			//bit 4 =  +32
			if (ctrl & 0b0100) {
				vramAddress += 32;
			}
			else {
				vramAddress += 1;
			}

			break;
	}



	return data;
}

void PPU::cpuWrite(uint16_t registerN, uint8_t data){
	switch (registerN) {
		case 0: 
			ctrl = data;

			tempVramAddress = (tempVramAddress & ~0x0C00) | ((data & 0x03) << 10);

			if ((ctrl & 0b1000'0000) && (status & 0b1000'0000)) {
				nmiSignal = true;
			}
			break;

		case 1:
			mask = data;
			break;

		case 3: //OAMADDR
			oamAddress = data;

			break;
		case 4: //OAMDATA
			oam[oamAddress] = data;
			oamAddress++;

			break;
		case 5:
			if (addressLatch == false) {
				fineX = data & 0b0111;
				
				tempVramAddress = (tempVramAddress & ~0x001F) | ((data >> 3) & 0x001F);
				addressLatch = true;
			}
			else {
				
				tempVramAddress = (tempVramAddress & ~0x7000) | ((data & 0b0111) << 12);
				
				tempVramAddress = (tempVramAddress & ~0x03E0) | ((data & 0b1111'1000) << 2);
				addressLatch = false;
			}
			break;

		case 6: // 0x2006: PPUADDR

			if (addressLatch == false) {

				//high byte (6 bits)
				tempVramAddress = (uint16_t)((data & 0b0011'1111) << 8) | (tempVramAddress & 0b1111'1111);
				addressLatch = true;
			}
			else {
									//limpa low bits
				tempVramAddress = (tempVramAddress & 0xFF00) | data;

				vramAddress = tempVramAddress;
				addressLatch = false;
			}

			break;

		case 7: // PPUDATA
			ppuWrite(vramAddress, data);

			//bit 4 = +32
			if (ctrl & 0b0100) {
				vramAddress += 32;
			}
			else {
				vramAddress += 1;
			}
			break;
	}
}



void PPU::drawPixel(){
	uint8_t bgPixel = 0x00;
	uint8_t bgPalette = 0x00;

	int currentX = cycle - 1;
	int currentY = scanline;

	//background rendering
	if (mask & 0b1000) {

		uint16_t bitMux = 0x8000 >> fineX;


		uint8_t p0_pixel = (bgShifterPatternLow & bitMux) > 0;
		uint8_t p1_pixel = (bgShifterPatternHigh & bitMux) > 0;
		bgPixel = (p1_pixel << 1) | p0_pixel;


		uint8_t pal0 = (bgShifterAttrLow & bitMux) > 0;
		uint8_t pal1 = (bgShifterAttrHigh & bitMux) > 0;
		bgPalette = (pal1 << 1) | pal0;
	}

	uint8_t fgPixel = 0x00;
	uint8_t fgPalette = 0x00;
	uint8_t fgPriority = 0x00;

	if (mask & 0b00010000) {
		//64 sprites
		for (int i = 0; i < 64; i++) {


			uint8_t sY = oam[i * 4];
			uint8_t sTile = oam[i * 4 + 1];
			uint8_t sAttr = oam[i * 4 + 2];
			uint8_t sX = oam[i * 4 + 3];

			int diffX = currentX - sX;
			int diffY = currentY - (sY + 1); // sprites are delayed by 1 scanline

			if (diffX >= 0 && diffX < 8 && diffY >= 0 && diffY < 8) {

				if (sAttr & 0b10000000) diffY = 7 - diffY; // vertical flip
				if (sAttr & 0b01000000) diffX = 7 - diffX; // horizontal flip

				uint16_t spriteTable = (ctrl & 0b00001000) ? 0x1000 : 0x0000;
				uint16_t spriteAddress = spriteTable + (sTile * 16) + diffY;

				uint8_t spriteLow = ppuRead(spriteAddress);
				uint8_t spriteHigh = ppuRead(spriteAddress + 8);

				uint8_t bitMux = 0x80 >> diffX;
				uint8_t p0 = (spriteLow & bitMux) > 0;
				uint8_t p1 = (spriteHigh & bitMux) > 0;
				uint8_t pixel = (p1 << 1) | p0;

				if (pixel != 0) {
					fgPixel = pixel;
					fgPalette = (sAttr & 0b00000011) + 4; // sprites use palettes 4, 5, 6, and 7
					fgPriority = (sAttr & 0b00100000) == 0; // 0 = front 

					// sprite 0 hit collision
					if (i == 0 && bgPixel != 0) {
						
						if (currentX != 255 && (currentX >= 8 || (mask & 0b00000110) == 0b00000110)) {
							status |= 0b01000000; 
						}
					}


					break;
				}
			}
		}

	}


	uint8_t finalPixel = 0x00;
	uint8_t finalPalette = 0x00;

	if (bgPixel == 0 && fgPixel == 0) {
		finalPixel = 0x00;
		finalPalette = 0x00;
	}
	else if (bgPixel == 0 && fgPixel > 0) {
		finalPixel = fgPixel;
		finalPalette = fgPalette;
	}
	else if (bgPixel > 0 && fgPixel == 0) {
		finalPixel = bgPixel;
		finalPalette = bgPalette;
	}
	else {
		
		if (fgPriority) {
			finalPixel = fgPixel;
			finalPalette = fgPalette;
		}
		else {
			finalPixel = bgPixel;
			finalPalette = bgPalette;
		}
	}


	uint8_t finalPixelColor = 0x00;

	if (finalPixel == 0) {
		finalPixelColor = ppuRead(0x3F00); // universal background color
	}
	else {
									//base of palette ram               * 4
		uint16_t palette_address =		 0x3F00 +				(finalPalette << 2) + finalPixel;
		finalPixelColor = ppuRead(palette_address);
	}

	if (currentX >= 0 && currentX < 256 && currentY >= 0 && currentY < 240 && videoBuffer != nullptr) {
		uint8_t ntsc_pixel = (finalPixelColor & 0x3F) | (mask & 0xE0);
		videoBuffer[currentY * 256 + currentX] = ntsc_pixel;
	}


}

uint16_t PPU::mirrorAddress(uint16_t address, EMirrorMode mirrorMode){
	//ignora bit 11
	if (mirrorMode == MVERTICAL) {
		return (address & 0x07FF);
	}
	else {
				//shift 1 right - keep bit 10, -> original
		return ((address >> 1) & 0b0100'0000'0000) | (address & 0b0011'1111'1111);
	}

	return address & 0x07FF;
}

void PPU::incrementScrollX() {
	// Only scroll if background or sprites are enabled
	if (mask & 0b00011000) {
		if ((vramAddress & 0x001F) == 31) { // If coarse X == 31 (end of nametable)
			vramAddress &= ~0x001F;         // Reset coarse X to 0
			vramAddress ^= 0x0400;          // Toggle the horizontal nametable bit
		}
		else {
			vramAddress++;                  // Otherwise, just move one tile right
		}
	}
}

void PPU::incrementScrollY() {
	if (mask & 0b00011000) {
		if ((vramAddress & 0x7000) != 0x7000) { // If fine Y < 7
			vramAddress += 0x1000;              // Increment fine Y
		}
		else {
			vramAddress &= ~0x7000;             // Reset fine Y to 0
			int y = (vramAddress & 0x03E0) >> 5; // Extract coarse Y

			if (y == 29) {
				y = 0;                          // Reset coarse Y
				vramAddress ^= 0x0800;          // Toggle the vertical nametable bit
			}
			else if (y == 31) {
				y = 0;                          // Coarse Y can be set out of bounds by games, just reset it
			}
			else {
				y++;                            // Otherwise, move one tile down
			}
			vramAddress = (vramAddress & ~0x03E0) | (y << 5); // Put coarse Y back
		}
	}
}

void PPU::transferAddressX() {
	// Reloads the X position from tempVramAddress at the end of a scanline
	if (mask & 0b00011000) {
		vramAddress = (vramAddress & 0xFBE0) | (tempVramAddress & 0x041F);
	}
}

void PPU::transferAddressY() {
	// Reloads the Y position from tempVramAddress at the start of a frame
	if (mask & 0b00011000) {
		vramAddress = (vramAddress & 0x841F) | (tempVramAddress & 0x7BE0);
	}
}

uint8_t PPU::step(NESLogger* logger) {
	bool frameEnd = false;

	if (scanline >= -1 && scanline < 240) {
		
		if ((cycle >= 1 && cycle <= 256) || (cycle >= 321 && cycle <= 336)) {
			if (mask & 0b1000) { 
				updateShifters();

				switch ((cycle - 1) % 8) {
				case 0:
					loadBackgroundShifters();
					bgNextTileID = ppuRead(0x2000 | (vramAddress & 0x0FFF));
					break;
				case 2:
					bgNextTileAttr = fetchAttributeByte();
					break;
				case 4:
					bgNextTileLSB = fetchPatternTableLow();
					break;
				case 6:
					bgNextTileMSB = fetchPatternTableHigh();

					
					break;
				case 7:
					incrementScrollX(); // <--- ADD THIS: Move the camera right!
					break;
				}
			}
		}

		// At the end of the visible pixels on a scanline, drop down a row
		if (cycle == 256) {
			incrementScrollY();
		}

		// At the start of the horizontal blank, snap the X camera back to the left
		if (cycle == 257) {
			loadBackgroundShifters();
			transferAddressX();
		}



		if (scanline >= 0 && scanline < 240 && cycle >= 1 && cycle <= 256) {
			drawPixel();
		}
	}

	// VBlank 
	if (scanline == 241 && cycle == 1) {
		status |= 0b10000000;
		if (ctrl & 0b10000000) {
			nmiSignal = true;
		}
	}


	if (scanline == -1) {
		if (cycle == 1) {
			status &= ~0b11100000;  //Vbank, sprite 0 , overflow
			nmiSignal = false;
		}

		if (cycle >= 280 && cycle <= 304) {
			transferAddressY();
		}

	}

	

	cycle++;
	if (cycle >= 341) {
		cycle = 0;
		scanline++;

		if (scanline >= 261) {
			scanline = -1;
			frameEnd = true;
		}
	}



	return frameEnd ? 1 : 0;
}