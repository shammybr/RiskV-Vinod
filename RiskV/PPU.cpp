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
		if (nes && nes->currentRom && nes->mapper) {
			uint32_t mappedAddress = 0;
			

			if (nes->mapper->ppuMapRead(address, mappedAddress)) {
				return nes->currentRom->vCHRMemory[mappedAddress];
			}
			else {
				return nes->currentRom->read(address);
			}
		}

		return 0x00;
	}

	else if (address >= PPUSTART && address <= PPUADDRESSVRAMEND) {
		address = address & 0x0FFF;


		if (nes) {
			if (nes->currentRom) {
				uint16_t mirroredAddress = mirrorAddress(address, nes->mapper->mirroringMode);

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

	
	bgShifterAttrLow = (bgShifterAttrLow & 0xFF00) | ((bgNextTileAttr & 0b01) ? 0xFF : 0x00);
	bgShifterAttrHigh = (bgShifterAttrHigh & 0xFF00) | ((bgNextTileAttr & 0b10) ? 0xFF : 0x00);
}

void PPU::updateShifters() {
    if (mask & 0b00011000) {
        bgShifterPatternLow <<= 1;
        bgShifterPatternHigh <<= 1;
        
      
		bgShifterAttrLow <<= 1;
		bgShifterAttrHigh <<= 1;
    }
}
void PPU::ppuWrite(uint16_t address, uint8_t data) {

	address = address & PPUENDMIRROED;


	if (address <= PPUADDRESSROMEND) {
		if (nes && nes->currentRom && nes->mapper) {
			uint32_t mappedAddress = 0;
			// Notice we are calling ppuMapWrite here!
			if (nes->mapper->ppuMapWrite(address, mappedAddress)) {
				nes->currentRom->vCHRMemory[mappedAddress] = data;

				// Temporary log:
				static int chrWrites = 0;
				if (chrWrites < 5) {
					std::cout << "CHR-RAM Write! Address: 0x" << std::hex << mappedAddress
						<< " Data: 0x" << (int)data << std::endl;
					chrWrites++;
				}
			}
		}

	}

	else if (address >= PPUSTART && address <= PPUADDRESSVRAMEND) {
		address = address & 0x0FFF;



		if (nes) {
			if (nes->currentRom) {
				uint16_t mirroredAddress = mirrorAddress(address, nes->mapper->mirroringMode);

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
uint8_t PPU::logCpuRead(uint16_t registerN) {
	uint8_t data = 0x00;

	switch (registerN) {
	case 2:
		data = status;


		break;

	case 4: //OAMDATA
		data = oam[oamAddress];

		break;
	case 7:

		data = ppuDataBuffer;
		uint8_t ppuDataBuffer2 = ppuRead(vramAddress);

		if (vramAddress >= 0x3F00) {
			data = ppuDataBuffer2;
		}

		break;
	}



	return data;
}

uint8_t PPU::cpuRead(uint16_t registerN){
	uint8_t data = ppuDataLatch;

	switch (registerN) {
		case 2:
			//top 3 bits                        //bottom 5
			data = (status & 0xE0) | (ppuDataLatch & 0x1F);

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

	ppuDataLatch = data;


	return data;
}

void PPU::cpuWrite(uint16_t registerN, uint8_t data){
	ppuDataLatch = data;

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
		for (int i = 0; i < spriteCount; i++) {

			uint8_t sY = scanlineSprites[i].y;
			uint8_t sTile = scanlineSprites[i].tile;
			uint8_t sAttr = scanlineSprites[i].attr;
			uint8_t sX = scanlineSprites[i].x;
			uint8_t originalIndex = scanlineSprites[i].originalIndex;

			int diffX = currentX - sX;
			int diffY = currentY - (sY + 1);

			bool is8x16 = (ctrl & 0b00100000) > 0;
			int spriteHeight = is8x16 ? 16 : 8;

			if (diffX >= 0 && diffX < 8) {

				if (sAttr & 0b10000000) diffY = (spriteHeight - 1) - diffY;
				if (sAttr & 0b01000000) diffX = 7 - diffX;

				uint16_t spriteTable = 0;
				uint8_t tileIndex = sTile;

				if (is8x16) {
					spriteTable = (sTile & 0x01) ? 0x1000 : 0x0000;
					tileIndex = sTile & 0xFE;
					if (diffY >= 8) {
						tileIndex++;
						diffY -= 8;
					}
				}
				else {
					spriteTable = (ctrl & 0b00001000) ? 0x1000 : 0x0000;
				}

				uint16_t spriteAddress = spriteTable + (tileIndex * 16) + diffY;

				uint8_t spriteLow = ppuRead(spriteAddress);
				uint8_t spriteHigh = ppuRead(spriteAddress + 8);

				uint8_t bitMux = 0x80 >> diffX;
				uint8_t p0 = (spriteLow & bitMux) > 0;
				uint8_t p1 = (spriteHigh & bitMux) > 0;
				uint8_t pixel = (p1 << 1) | p0;

				if (pixel != 0) {
					fgPixel = pixel;
					fgPalette = (sAttr & 0b00000011) + 4;
					fgPriority = (sAttr & 0b00100000) == 0;

				
					if (originalIndex == 0 && bgPixel != 0) {
						if (currentX != 255 && (currentX >= 8 || (mask & 0b00000110) == 0b00000110)) {
							status |= 0b01000000;
						}
					}
					break;
				}
			}
		}

	}

	if (currentX < 8) {
		if (!(mask & 0b00000100)) fgPixel = 0; 
		if (!(mask & 0b00000010)) bgPixel = 0; 
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

		if (pixelMode && nes->frameMode) {
			if (!lineMode) {
				if (currentX + 1 < 256) videoBuffer[currentY * 256 + (currentX + 1)] = 0x30;
				if (currentX + 2 < 256) videoBuffer[currentY * 256 + (currentX + 2)] = 0x30;
				if (currentX + 3 < 256) videoBuffer[currentY * 256 + (currentX + 3)] = 0x30;
			}
			else {
				for (int i = currentX + 1; i < 256; i++) {
					videoBuffer[currentY * 256 + i] = 0x30;
				}
			}
		}
	}


}

void PPU::evaluateSprites(){
	spriteCount = 0;

	bool is8x16 = (ctrl & 0b00100000) > 0;
	int spriteHeight = is8x16 ? 16 : 8;

	for (int i = 0; i < 64; i++) {
		uint8_t sY = oam[i * 4];

		
		int diffY = scanline - (sY + 1);

		if (diffY >= 0 && diffY < spriteHeight) {
			if (spriteCount < 8) {
				
				scanlineSprites[spriteCount].y = sY;
				scanlineSprites[spriteCount].tile = oam[i * 4 + 1];
				scanlineSprites[spriteCount].attr = oam[i * 4 + 2];
				scanlineSprites[spriteCount].x = oam[i * 4 + 3];
				scanlineSprites[spriteCount].originalIndex = i; 
				spriteCount++;
			}
			else {

				status |= 0b00100000;
				break; 
			}
		}
	}
}

uint16_t PPU::mirrorAddress(uint16_t address, EMirrorMode mirrorMode){


	switch (mirrorMode) {
		case MHORIZONTAL:
			return ((address >> 1) & 0x0400) | (address & 0x03FF);
		case MVERTICAL:
			return (address & 0x07FF);
		case MONESCREENLO:
			return (address & 0x03FF);
		case MONESCREENHI:
			return (address & 0x03FF) + 0x0400;
		default:
			return address & 0x07FF;
	}

	return address & 0x07FF;
}

void PPU::incrementScrollX() {
	
	if (mask & 0b00011000) {
		if ((vramAddress & 0x001F) == 31) { 
			vramAddress &= ~0x001F;         
			vramAddress ^= 0x0400;         
		}
		else {
			vramAddress++;                  
		}
	}
}

void PPU::incrementScrollY() {
	if (mask & 0b00011000) {
		if ((vramAddress & 0x7000) != 0x7000) { 
			vramAddress += 0x1000;              
		}
		else {
			vramAddress &= ~0x7000;             
			int y = (vramAddress & 0x03E0) >> 5; 

			if (y == 29) {
				y = 0;                          
				vramAddress ^= 0x0800;          
			}
			else if (y == 31) {
				y = 0;                         
			}
			else {
				y++;                            
			}
			vramAddress = (vramAddress & ~0x03E0) | (y << 5); 
		}
	}
}

void PPU::transferAddressX() {
	
	if (mask & 0b00011000) {
		vramAddress = (vramAddress & 0xFBE0) | (tempVramAddress & 0x041F);
	}
}

void PPU::transferAddressY() {
	
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
					incrementScrollX(); 
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



		if (scanline >= 0 && scanline < 240) {
			if (cycle == 0) {
				evaluateSprites();
			}

			if (cycle >= 1 && cycle <= 256) {
				drawPixel();
				if (nes->frameMode && pixelMode) {
					if (!lineMode) {
						if (cycle % pixelsToWait == 0)
							nes->canStep = false;

					

					}
					else {
						if (cycle == 1) {
							nes->canStep = false;
							lineMode = false;
						}
					}
				}
			}
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

	

	if (scanline == -1 && cycle == 339 && oddFrame && (mask & 0b00011000)) {
		cycle++;
	}


	cycle++;

	if (cycle >= 341) {
		cycle = 0;
		scanline++;

		if (scanline >= 261) {
			scanline = -1;
			frameEnd = true;
			oddFrame = !oddFrame;
		}
	}



	return frameEnd ? 1 : 0;
}