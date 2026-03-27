#pragma once
#include "PPU.h"
#include "NES.h"
#include <cstdint>
#include "NESROM.h"
#include "NESLogger.h"


PPU::PPU(NES* _nes) {
	nes = _nes;
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
			break;

		case 1:
			mask = data;
			break;

		case 6: // 0x2006: PPUADDR

			if (addressLatch == false) {

				//high byte (6 bits)
				vramAddress = (uint16_t)((data & 0b0011'1111) << 8) | (vramAddress & 0b1111'1111);
				addressLatch = true;
			}
			else {
									//limpa low bits
				vramAddress = (vramAddress & 0xFF00) | data;
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

uint8_t PPU::step(NESLogger* logger) {
	cycle++;

	bool frameEnd = false;

	if (cycle >= 341) {
		cycle = 0;
		scanline++;

		if (scanline >= 262) {
			scanline = 0;
			frameEnd = true;
		}
	}

	if (scanline == 241 && cycle == 1) {
		status |= 0b1000'0000; // Set Bit 7


		if (ctrl & 0b1000'0000) {
			nmiSignal = true;
		}
	}

	// VBlank ends
	if (scanline == 261 && cycle == 1) {
		status &= ~0b1000'0000; 
		nmiSignal = false;

		
	}
	
	if (frameEnd) {
		return 1;
	}
	else {
		return 0;
	}
	
}