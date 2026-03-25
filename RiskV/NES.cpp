#include "NES.h"

NES::NES() {
	Reset();


}

NES::reset() {
	regA = 0x000;
	regX = 0x000;
	regY = 0x000;
	regSP = 0xFD;
	regP = 0x24;
	regPC = 0x000;


	//le little indiaaaaaaaaaaan lelelelele
	uint8_t lByte = read(0xFFFC);
	uint8_t hByte = read(0xFFFD);

	regPC = lowByte + (highByte << 8);
}

uint8_t NES::read(uint16_t address) {

	if (address <= 0x1FFF) {
		//mirror
		return memory[address & 0x07FF];

	}
	else if (address <= 0x3FFF) {
		return memory[address & 0x2007];
	}
	else {
		return memory[address];
	}

}

void NES::write(uint16_t address, uint8_t data) {

	if (address <= 0x1FFF) {
		//mirror
		memory[address & 0x07FF] = data;
	}
	else if (address <= 0x3FFF) {
		memory[address & 0x2007] = data;
	}
	else {
		memory[address] = data;
	}

}

void NES::I_LDA(uint16_t address) {

	regA = read(address);

	updateFlags(regA);
}

void NES::updateFlags(uint8_t value) {

	if (value == 0) {
		regP = regP | 0b0000'0010; // ON 
	}
	else {
		regP = regP & ~0b0000'0010; // OFF
	}

	if (value & 0b0100'0000) {
		regP = regP | 0b0100'0000; // ON
	}
	else {
		regP = regP | 0b0100'0000; // OFF
	}
}

uint16_t NES::addressImmediate(uint16_t address) {
	regPC++;

	return address;

}



uint16_t NES::addressZeroPage(uint16_t address) {
	regPC++;

	return read(address);

}

uint16_t NES::addressZeroPageX(uint16_t address) {
	regPC++;

			//base + regX mirror em 0x00FF
	return ( (read(address) + regX ) & 0x00FF ) ;

}

uint16_t NES::addressZeroPageY(uint16_t address) {
	regPC++;

	return ((read(address) + regY) & 0x00FF);

}

uint16_t NES::addressAbsolute(uint16_t address) {
	uint8_t lByte = read(address);
	regPC++;
	uint8_t hByte = read(regPC++);
	regPC++;



	return lByte + (hByte << 8);

}

uint16_t NES::addressAbsoluteX(uint16_t address) {


	return addressAbsolute(address) + regX;

}

uint16_t NES::addressAbsoluteY(uint16_t address) {


	return addressAbsolute(address) + regY;

}

uint16_t NES::address_Absolute_Indirect(uint16_t address) {

	uint16_t absolute = addressAbsolute(address);

	uint8_t lByte = read(absolute);
	uint8_t hByte;

	//le JMP bug
	if ((absolute & 0x00FF) == 0x00FF) {
		hByte = read(absolute & 0xFF00);
	}
	else {
		hByte = read(++absolute);
	}

	return lByte + (hByte << 8);
}

void NES::step() {

	uint8_t opcode = read(regPC++);


	switch (opcode) {

		case 0xA9: {
				
			I_LDA(address_Immediate(regPC));

			break;
		}

		case 0xA5: {

			I_LDA(address_ZeroPage(regPC));

			break;
		}


		case 0xEA: {

			break;
		}

		default: {
			std::cout << "OPCODE ERROR MEME NUMBER: " << opcode << std::endl;
			break;
		}
	}




}