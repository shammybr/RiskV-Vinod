#pragma once
#include "NES.h"
#include <iostream>

NES::NES() {
	ppu = new PPU(this);
	apu = new APU();
	on = true;



}

void NES::loadRom(NESROM* rom){
	currentRom = rom;


	switch (currentRom->mapperID) {
	case 0:
		mapper = new Mapper000(currentRom->header.prgChunks);

		break;



	default:
		std::cout << "ERRO no Mapper ID: " << (int)currentRom->mapperID << std::endl;
		break;
	}

	reset();
}

void NES::unloadRom() {
	currentRom = nullptr;
}

void NES::reset() {
	
	regA = 0x000;
	regX = 0x000;
	regY = 0x000;
	regSP = 0xFD;
	regP = 0x24;
	regPC = 0x000;



	//le little indiaaaaaaaaaaan lelelelele
	uint8_t lByte = read(0xFFFC);
	uint8_t hByte = read(0xFFFD);

	regPC = lByte + (hByte << 8);



}

uint8_t NES::read(uint16_t address) {

	uint8_t data = 0b0000'0000;

	if (address <= INTERNALMIRROED) {
		//mirror
		data = memory[address & INTERNALEND];

	}
	else if (address >= PPUSTART && address <= PPUENDMIRROED) {
		data = ppu->cpuRead(address & 0x0007);
	}
	else if (address == 0x4015) {
		data = apu->read(address);
	}
	else if (address == 0x4016) { // Player 1
		
		uint8_t data = (controllerState[0] & 0x80) > 0;

		
		controllerState[0] <<= 1;
		

		return data;
	}
	else if (address == 0x4017) { // Player 2
		uint8_t data = (controllerState[1] & 0x80) > 0;
		
		controllerState[1] <<= 1;
		
		return data;
	}


	else if (address >= ROMSTART && address <= ROMEND) {
		if (currentRom) {
			if (mapper) {
				uint32_t mappedAddress = 0;

				if (mapper->cpuMapRead(address, mappedAddress)) {
					data = currentRom->vPRGMemory[mappedAddress];
				}
			}
		}
	}

	if (address == 0x4014) {
		static int dmaCount = 0;
		printf("DMA Count: %d\n", ++dmaCount);
	}

	return data;
}

void NES::write(uint16_t address, uint8_t data) {

	if (address <= INTERNALMIRROED) {
		//mirror
		memory[address & INTERNALEND] = data;
	}
	else if (address >= PPUSTART && address <= PPUENDMIRROED) {
		ppu->cpuWrite(address & 0x0007, data);

	}
	else if (address == 0x4014) { // OAM DMA
		uint16_t dmaBase = data << 8;
		for (int i = 0; i < 256; i++) {
			ppu->oam[ppu->oamAddress] = read(dmaBase + i);
			ppu->oamAddress++;
		}

		currentCycles += 512;
		extraCycles += 512;
	}
	else if (address == 0x4016) { //controlurrr
		if (data & 0x0001) {
			controllerState[0] = controller[0];
			controllerState[1] = controller[1];
		}
	}
	else if (address >= 0x4000 && address <= 0x4017){

		apu->write(address, data);

	}

	else {
		uint32_t mappedAddress = 0;
		mapper->cpuMapWrite(address, mappedAddress, data);
	}

	if (address == 0x4014) {
		static int dmaCount = 0;
	}

}



void NES::updateFlags(uint8_t value) {

	if (value == 0) {
		regP = regP | 0b0000'0010; // ON 
	}
	else {
		regP = regP & ~0b0000'0010; // OFF
	}

	if (value & 0b1000'0000) {
		regP = regP | 0b1000'0000; // ON
	}
	else {
		regP = regP & ~0b1000'0000; // OFF
	}
}

bool NES::hasPageCrossed(uint16_t addr1, uint16_t addr2){
	return (addr1 & 0xFF00) != (addr2 & 0xFF00);
}

uint16_t NES::addressImmediate(uint16_t address) {
	regPC++;

	return address;

}



uint16_t NES::addressZeroPage(uint16_t address) {
	uint16_t data = read(address);
	regPC++;

	return data;

}

uint16_t NES::addressZeroPageX(uint16_t address) {
						//base + regX      mirror em 0x00FF
	uint16_t data = ((read(address) + regX) & 0x00FF);
	regPC++;

			
	return data;

}

uint16_t NES::addressZeroPageY(uint16_t address) {
	uint16_t data = ((read(address) + regY) & 0x00FF);
	regPC++;

	return data;

}

uint16_t NES::addressAbsolute(uint16_t address) {
	uint8_t lByte = read(address);
	regPC++;
	uint8_t hByte = read(regPC);
	regPC++;



	return lByte + (hByte << 8);

}

uint16_t NES::addressAbsoluteX(uint16_t address) {

	uint16_t base = addressAbsolute(address);
	uint16_t current = base + regX;
	isPageCrossed = hasPageCrossed(base, current);
	return current;


}

uint16_t NES::addressAbsoluteY(uint16_t address) {

	uint16_t base = addressAbsolute(address);
	uint16_t current = base + regY;
	isPageCrossed = hasPageCrossed(base, current);
	return current;

}

uint16_t NES::addressAbsoluteIndirect(uint16_t address) {

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

uint16_t NES::addressIndirectX(uint16_t address) {
	uint8_t indirect = read(address);
	regPC++;

	indirect = (indirect + regX) & 0x00FF;

	uint8_t lByte = read(indirect);
	uint8_t hByte = read((indirect + 1) & 0x00FF);

	return lByte + (hByte << 8);

}

uint16_t NES::addressIndirectY(uint16_t address) {
	uint8_t indirect = read(address);
	regPC++;

	uint8_t lByte = read(indirect);
	uint8_t hByte = read((indirect + 1) & 0x00FF);

	return ((lByte + (hByte << 8)) + regY);

}

uint16_t NES::addressRelative(uint16_t address) {

	return read(regPC++);


}

void NES::push(uint8_t data) {

	        //stack
	write(0x0100 + regSP, data);
	regSP--;

}

uint8_t NES::pull() {
	regSP++;
						//stack
	uint8_t data = read(0x0100 + regSP);
	

	return data;
}

void NES::compare(uint8_t regData, uint8_t data) {
	uint8_t result = regData - data;

	//carry
	if (regData >= data) {
		regP = regP | 0b0000'0001; // ON
	}
	else {
		regP = regP & ~0b0000'0001; // OFF
	}

	//zero
	if (regData == data) {
		regP = regP | 0b0000'0010; // ON
	}
	else {
		regP = regP & ~0b0000'0010; // OFF
	}

	//negative
	if (result & 0b1000'0000) {
		regP = regP | 0b1000'0000; // ON
	}
	else {
		regP = regP & ~0b1000'0000; // OFF
	}
}


void NES::branch(uint8_t offset) {
	int8_t signedOffset = static_cast<int8_t>(offset);
	regPC++;

	uint16_t newPC = regPC + signedOffset;

	if ((newPC & 0xFF00) != (regPC & 0xFF00)) {

		isPageCrossed = true;
	}

	regPC = newPC;


}

//shift 1 bit left
uint8_t NES::I_ASL(uint8_t data) {

	if (data & 0b1000'0000) {
		regP = regP | 0b0000'0001; // the father is on
	}
	else {
		regP = regP & ~0b0000'0001; // carry off
	}


	uint8_t result = data << 1;
	updateFlags(result);
	return result;
}


//shift 1 bit right
uint8_t NES::I_LSR(uint8_t data) {

	if (data & 0b0000'0001) {
		regP = regP | 0b0000'0001; // the father is on
	}
	else {
		regP = regP & ~0b0000'0001; // carry off
	}


	uint8_t result = data >> 1;
	updateFlags(result);
	return result;
}

//rotate left
uint8_t NES::I_ROL(uint8_t data) {
	uint8_t carry = (regP & 0b0000'0001);

	if (data & 0b1000'0000) {
		regP = regP | 0b0000'0001;

	}
	else {
		regP = regP & ~0b0000'0001;
	}

	uint8_t result = (data << 1) | carry;
	updateFlags(result);
	return result;

}

//rotate right
uint8_t NES::I_ROR(uint8_t data) {
	uint8_t carry = (regP & 0b0000'0001);

	if (data & 0b0000'0001) {
		regP = regP | 0b0000'0001;

	}
	else {
		regP = regP & ~0b0000'0001;
	}

	uint8_t result = (data >> 1) | (carry << 7);
	updateFlags(result);
	return result;

}




void NES::I_LDA(uint16_t address) {

	regA = read(address);

	updateFlags(regA);
}

void NES::I_LDX(uint16_t address) {

	regX = read(address);

	updateFlags(regX);
}

void NES::I_LDY(uint16_t address) {

	regY = read(address);

	updateFlags(regY);
}

void NES::I_STA(uint16_t address) {

	write(address, regA);


}

void NES::I_STX(uint16_t address) {

	write(address, regX);


}

void NES::I_STY(uint16_t address) {

	write(address, regY);


}


void NES::I_ADC(uint16_t address) {

	uint8_t data = read(address);
										//carry flag
	uint16_t sum = regA + data + (regP & 0b0000'0001);

	//carry flag
	if (sum > 0xFF) {
		regP = regP | 0b0000'0001; //ON
	}
	else {
		regP = regP & ~0b0000'0001; // OFF
	}

	//overflow
	if ((~(regA ^ data) & (regA ^ sum)) & 0b1000'0000) {
		regP = regP | 0b0100'0000; //ON
	}
	else {
		regP = regP & ~0b0100'0000; //OFF
	}

	regA = sum & 0b1111'1111;

	updateFlags(regA);
}

//SISTEMA BRASILEIRO DE CARRY
void NES::I_SBC(uint16_t address) {

	uint8_t data = read(address);
	uint16_t inverted_data = data ^ 0b1111'1111;

	uint16_t sum = regA + inverted_data + (regP & 0b0000'0001);

	//carry flag
	if (sum > 0xFF) {
		regP = regP | 0b0000'0001; //ON
	}
	else {
		regP = regP & ~0b0000'0001; // OFF
	}

	//overflow
	if ((~(regA ^ inverted_data) & (regA ^ sum)) & 0b1000'0000) {
		regP = regP | 0b0100'0000; //ON
	}
	else {
		regP = regP & ~0b0100'0000; //OFF
	}

	regA = sum & 0b1111'1111;

	updateFlags(regA);

}
//BRANCH IF EQUOLLLSL 0
void NES::I_BEQ(uint16_t address) {
	uint8_t offset = read(address);

					//zero flag
	if (regP & 0b0000'0010) {
		branch(offset);
		extraCycles++;
	}
	else {

		regPC++;
	}
}

//BRANCH IF NOTTO EQUOLLLSL 0
void NES::I_BNE(uint16_t address) {
	uint8_t offset = read(address);
	         

			
	if (!(regP & 0b0000'0010)) {
		branch(offset);
		extraCycles++;
	}
	else {

		regPC++;
	}
}

//BRANCH IF CARRY CLEAR
void NES::I_BCC(uint16_t address) {

	uint8_t offset = read(address);

	if (!(regP & 0b0000'0001)) {
		branch(offset);
		extraCycles++;
	}
	else {

		regPC++;
	}
}

//BRANCH IF CARRY SET
void NES::I_BCS(uint16_t address) {

	uint8_t offset = read(address);


	if (regP & 0b0000'0001) {
		branch(offset);
		extraCycles++;
	}
	else {

		regPC++;
	}
}

//BRANCH IF OVERFLOW CLEAR
void NES::I_BVC(uint16_t address) {
	uint8_t offset = read(address);



	if (!(regP & 0b0100'0000)) {
		branch(offset);
		extraCycles++;
	}
	else {

		regPC++;
	}
}

//BRANCH IF OVERFLOW SET
void NES::I_BVS(uint16_t address) {
	uint8_t offset = read(address);



	if (regP & 0b0100'0000) {
		branch(offset);
		extraCycles++;
	}
	else {

		regPC++;
	}
}

//BRANCH IF PLUS
void NES::I_BPL(uint16_t address) {
	uint8_t offset = read(address);



	if (!(regP & 0b1000'0000)) {
		branch(offset);
		extraCycles++;
	}
	else {

		regPC++;
	}
}

//BRANCH IF MINUS TECH TIPS
void NES::I_BMI(uint16_t address) {
	uint8_t offset = read(address);



	if (regP & 0b1000'0000) {
		branch(offset);
		extraCycles++;
	}
	else {

		regPC++;
	}
}

//+1
void NES::I_INX() {
	regX++;

	updateFlags(regX);

}

//-1
void NES::I_DEX() {
	regX--;

	updateFlags(regX);

}

//+1

void NES::I_INY() {
	regY++;

	updateFlags(regY);

}

//-1
void NES::I_DEY() {
	regY--;

	updateFlags(regY);

}

//+1 memory
void NES::I_INC(uint16_t address) {
	uint8_t data = read(address);
	data++;
	write(address, data);
	updateFlags(data);
}

//-1 memory
void NES::I_DEC(uint16_t address) {
	uint8_t data = read(address);
	data--;
	write(address, data);
	updateFlags(data);
}

//PUSH ACCUMULATOR
void NES::I_PHA() {
	push(regA);
}

//PULL ACCUMULATOR
void NES::I_PLA() {
	regA = pull();
	updateFlags(regA);
}

//PUSH PROCESSOR STATUS
void NES::I_PHP() {
	push(regP | 0b0011'0000);

}

//PULL PROCESSOR STATUS
void NES::I_PLP() {
	regP = pull();

	regP = regP | 0b0010'0000;
	regP = regP & ~0b0001'0000;
}

//compare
void NES::I_CMP(uint16_t address) {
	compare(regA, read(address));
}

void NES::I_CPX(uint16_t address) {
	compare(regX, read(address));
}

void NES::I_CPY(uint16_t address) {
	compare(regY, read(address));
}

//push return address
void NES::I_JSR(uint16_t address) {
	uint16_t returnAddress = regPC - 1;

	//high byte
	push((returnAddress >> 8) & 0xFF);
	//low byte
	push(returnAddress & 0xFF);

	regPC = address;
}

//PULL return address
void NES::I_RTS() {
	uint8_t lByte = pull();
	uint8_t hByte = pull();

	regPC = (lByte + (hByte << 8)) + 1;
}


//Carry Flag 
void NES::I_SEC() {
	regP = regP | 0b0000'0001; 
}

// Decimal Flag
void NES::I_SED() {
	regP = regP | 0b0000'1000; 
}

// Interrupt Disable
void NES::I_SEI() {
	regP = regP | 0b0000'0100; 
}


// Carry Flag 
void NES::I_CLC() {
	regP = regP & ~0b0000'0001; 
}

// Decimal
void NES::I_CLD() {
	regP = regP & ~0b0000'1000; 
}

// Interrupt Disable
void NES::I_CLI() {
	regP = regP & ~0b0000'0100; 
}

// Overflow 
void NES::I_CLV() {
	regP = regP & ~0b0100'0000; 
}

void NES::I_TAX() {
	regX = regA;
	updateFlags(regX);
}

void NES::I_TAY() {
	regY = regA;
	updateFlags(regY);
}


void NES::I_TXA() {
	regA = regX;
	updateFlags(regA);
}

void NES::I_TYA() {
	regA = regY;
	updateFlags(regA);
}

void NES::I_TSX() {
	regX = regSP;
	updateFlags(regX);
}

void NES::I_TXS() {
	regSP = regX;
}

void NES::I_BIT(uint16_t address) {
	uint8_t data = read(address);
	uint8_t result = regA & data;

	// Zero Flag
	if (result == 0) {
		regP = regP | 0b0000'0010; // ON
	}
	else {
		regP = regP & ~0b0000'0010; // OFF
	}

	// Negative 
	if (data & 0b1000'0000) {
		regP = regP | 0b1000'0000; 
	}
	else {
		regP = regP & ~0b1000'0000; 
	}

	// Overflow 
	if (data & 0b0100'0000) {
		regP = regP | 0b0100'0000; 
	}
	else {
		regP = regP & ~0b0100'0000; 
	}
}

void NES::I_BREAK() {
	regPC = regPC + 1;

	push((regPC >> 8) & 0xFF);
	push(regPC & 0xFF);

	push(regP | 0b0011'0000);

	regP = regP | 0b0000'0100;

	uint8_t lByte = read(0xFFFE);
	uint8_t hByte = read(0xFFFF);

	regPC = lByte + (hByte << 8);

}

void NES::I_RTI() {

	regP = pull();

	regP = regP & ~0b0001'0000;
	regP = regP | 0b0010'0000;

	uint8_t lByte = pull();
	uint8_t hByte = pull();


	regPC = lByte + (hByte << 8);




}
void NES::I_AND(uint16_t address) {
	regA = regA & read(address);
	updateFlags(regA);
}

void NES::I_ORA(uint16_t address) {
	regA = regA | read(address);
	updateFlags(regA);
}

void NES::I_EOR(uint16_t address) {
	regA = regA ^ read(address);
	updateFlags(regA);
}

void NES::I_JMP(uint16_t address) {
	regPC = address;
}

uint8_t NES::step(NESLogger* logger) {


	uint8_t opcode = read(regPC++);

	//log
	InstructionInfo info = logger->opTable[opcode];
	uint8_t baseCycles = info.cycles;
	extraCycles = 0;

	isPageCrossed = false;


	switch (opcode) {

		//LDA
		case 0xA9: I_LDA(addressImmediate(regPC)); break;
		case 0xA5: I_LDA(addressZeroPage(regPC)); break;
		case 0xB5: I_LDA(addressZeroPageX(regPC)); break;
		case 0xAD: I_LDA(addressAbsolute(regPC)); break;
		case 0xBD: I_LDA(addressAbsoluteX(regPC)); break;
		case 0xB9: I_LDA(addressAbsoluteY(regPC)); break;
		case 0xA1: I_LDA(addressIndirectX(regPC)); break;
		case 0xB1: I_LDA(addressIndirectY(regPC)); break;

		// load X Register
		case 0xA2: I_LDX(addressImmediate(regPC)); break;
		case 0xA6: I_LDX(addressZeroPage(regPC)); break;
		case 0xB6: I_LDX(addressZeroPageY(regPC)); break;
		case 0xAE: I_LDX(addressAbsolute(regPC)); break;
		case 0xBE: I_LDX(addressAbsoluteY(regPC)); break;

		// Y 
		case 0xA0: I_LDY(addressImmediate(regPC)); break;
		case 0xA4: I_LDY(addressZeroPage(regPC)); break;
		case 0xB4: I_LDY(addressZeroPageX(regPC)); break;
		case 0xAC: I_LDY(addressAbsolute(regPC)); break;
		case 0xBC: I_LDY(addressAbsoluteX(regPC)); break;

		// store X 
		case 0x86: I_STX(addressZeroPage(regPC)); break;
		case 0x96: I_STX(addressZeroPageY(regPC)); break;
		case 0x8E: I_STX(addressAbsolute(regPC)); break;

		// Y 
		case 0x84: I_STY(addressZeroPage(regPC)); break;
		case 0x94: I_STY(addressZeroPageX(regPC)); break;
		case 0x8C: I_STY(addressAbsolute(regPC)); break;

		// accumulator
		case 0x85: I_STA(addressZeroPage(regPC)); break;
		case 0x95: I_STA(addressZeroPageX(regPC)); break;
		case 0x8D: I_STA(addressAbsolute(regPC)); break;
		case 0x9D: I_STA(addressAbsoluteX(regPC)); break;
		case 0x99: I_STA(addressAbsoluteY(regPC)); break;
		case 0x81: I_STA(addressIndirectX(regPC)); break;
		case 0x91: I_STA(addressIndirectY(regPC)); break;

		//ATTACK DAMAGE CARRY
		case 0x69: I_ADC(addressImmediate(regPC)); break;
		case 0x65: I_ADC(addressZeroPage(regPC)); break;
		case 0x75: I_ADC(addressZeroPageX(regPC)); break;
		case 0x6D: I_ADC(addressAbsolute(regPC)); break;
		case 0x7D: I_ADC(addressAbsoluteX(regPC)); break;
		case 0x79: I_ADC(addressAbsoluteY(regPC)); break;
		case 0x61: I_ADC(addressIndirectX(regPC)); break;
		case 0x71: I_ADC(addressIndirectY(regPC)); break;

		// subtract with carry
		case 0xE9: I_SBC(addressImmediate(regPC)); break;
		case 0xE5: I_SBC(addressZeroPage(regPC)); break;
		case 0xF5: I_SBC(addressZeroPageX(regPC)); break;
		case 0xED: I_SBC(addressAbsolute(regPC)); break;
		case 0xFD: I_SBC(addressAbsoluteX(regPC)); break;
		case 0xF9: I_SBC(addressAbsoluteY(regPC)); break;
		case 0xE1: I_SBC(addressIndirectX(regPC)); break;
		case 0xF1: I_SBC(addressIndirectY(regPC)); break;

		//logic
		case 0x29: I_AND(addressImmediate(regPC)); break;
		case 0x25: I_AND(addressZeroPage(regPC)); break;
		case 0x35: I_AND(addressZeroPageX(regPC)); break;
		case 0x2D: I_AND(addressAbsolute(regPC)); break;
		case 0x3D: I_AND(addressAbsoluteX(regPC)); break;
		case 0x39: I_AND(addressAbsoluteY(regPC)); break;
		case 0x21: I_AND(addressIndirectX(regPC)); break;
		case 0x31: I_AND(addressIndirectY(regPC)); break;

		case 0x09: I_ORA(addressImmediate(regPC)); break;
		case 0x05: I_ORA(addressZeroPage(regPC));  break;
		case 0x15: I_ORA(addressZeroPageX(regPC)); break;
		case 0x0D: I_ORA(addressAbsolute(regPC)); break;
		case 0x1D: I_ORA(addressAbsoluteX(regPC)); break;
		case 0x19: I_ORA(addressAbsoluteY(regPC)); break;
		case 0x01: I_ORA(addressIndirectX(regPC)); break;
		case 0x11: I_ORA(addressIndirectY(regPC)); break;

		case 0x49: I_EOR(addressImmediate(regPC)); break;
		case 0x45: I_EOR(addressZeroPage(regPC));  break;
		case 0x55: I_EOR(addressZeroPageX(regPC)); break;
		case 0x4D: I_EOR(addressAbsolute(regPC)); break;
		case 0x5D: I_EOR(addressAbsoluteX(regPC)); break;
		case 0x59: I_EOR(addressAbsoluteY(regPC)); break;
		case 0x41: I_EOR(addressIndirectX(regPC)); break;
		case 0x51: I_EOR(addressIndirectY(regPC)); break;



		// branches
		case 0x90: I_BCC(regPC); break;
		case 0xB0: I_BCS(regPC); break;
		case 0xF0: I_BEQ(regPC); break;
		case 0xD0: I_BNE(regPC); break;
		case 0x30: I_BMI(regPC); break;
		case 0x10: I_BPL(regPC); break;
		case 0x50: I_BVC(regPC); break;
		case 0x70: I_BVS(regPC); break;


		// LSR Accumulator
		case 0x4A: regA = I_LSR(regA); break;

		// LSR Zero Page
		case 0x46: {
			uint16_t addr = addressZeroPage(regPC);
			write(addr, I_LSR(read(addr)));
			break;
		}

		case 0x56: {
			uint16_t addr = addressZeroPageX(regPC);
			write(addr, I_LSR(read(addr)));
			break;
		}

		case 0x4E: {
			uint16_t addr = addressAbsolute(regPC);
			write(addr, I_LSR(read(addr)));
			break;
		}

		case 0x5E: {
			uint16_t addr = addressAbsoluteX(regPC);
			write(addr, I_LSR(read(addr)));
			break;
		}

		// ROL
		case 0x2A: regA = I_ROL(regA); break;

		case 0x26: {
			uint16_t addr = addressZeroPage(regPC);
			write(addr, I_ROL(read(addr)));
			break;
		}

		case 0x36: {
			uint16_t addr = addressZeroPageX(regPC);
			write(addr, I_ROL(read(addr)));
			break;
		}

		case 0x2E: {
			uint16_t addr = addressAbsolute(regPC);
			write(addr, I_ROL(read(addr)));
			break;
		}

		case 0x3E: {
			uint16_t addr = addressAbsoluteX(regPC);
			write(addr, I_ROL(read(addr)));
			break;
		}

		// ROR
		case 0x6A: regA = I_ROR(regA); break;

		case 0x66: {
			uint16_t addr = addressZeroPage(regPC);
			write(addr, I_ROR(read(addr)));
			break;
		}

		case 0x76: {
			uint16_t addr = addressZeroPageX(regPC);
			write(addr, I_ROR(read(addr)));
			break;
		}

		case 0x6E: {
			uint16_t addr = addressAbsolute(regPC);
			write(addr, I_ROR(read(addr)));
			break;
		}

		case 0x7E: {
			uint16_t addr = addressAbsoluteX(regPC);
			write(addr, I_ROR(read(addr)));
			break;
		}

		// ASL Accumulator
		case 0x0A: regA = I_ASL(regA); break;

		// ASL Zero Page
		case 0x06: {
			uint16_t addr = addressZeroPage(regPC);
			uint8_t data = read(addr);
			write(addr, I_ASL(data));
			break;
		}

		case 0x16: {
			uint16_t addr = addressZeroPageX(regPC);
			uint8_t data = read(addr);
			write(addr, I_ASL(data));
			break;
		}

		case 0x0E: {
			uint16_t addr = addressAbsolute(regPC);
			uint8_t data = read(addr);
			write(addr, I_ASL(data));
			break;
		}

		case 0x1E: {
			uint16_t addr = addressAbsoluteX(regPC);
			uint8_t data = read(addr);
			write(addr, I_ASL(data));
			break;
		}


		 // +1 -1 regs
		case 0xE8: I_INX(); break;
		case 0xCA: I_DEX(); break;
		case 0xC8: I_INY(); break;
		case 0x88: I_DEY(); break;

		// +1 -1 memory
		case 0xE6: I_INC(addressZeroPage(regPC)); break;
		case 0xF6: I_INC(addressZeroPageX(regPC)); break;
		case 0xEE: I_INC(addressAbsolute(regPC)); break;
		case 0xFE: I_INC(addressAbsoluteX(regPC)); break;
		case 0xC6: I_DEC(addressZeroPage(regPC)); break;
		case 0xD6: I_DEC(addressZeroPageX(regPC)); break;
		case 0xCE: I_DEC(addressAbsolute(regPC)); break;
		case 0xDE: I_DEC(addressAbsoluteX(regPC)); break;

		// STACK
		case 0x48: I_PHA(); break;
		case 0x68: I_PLA(); break;
		case 0x08: I_PHP(); break;
		case 0x28: I_PLP(); break;

		// Compare Accumulator
		case 0xC9: I_CMP(addressImmediate(regPC)); break;
		case 0xC5: I_CMP(addressZeroPage(regPC)); break;
		case 0xD5: I_CMP(addressZeroPageX(regPC)); break;
		case 0xCD: I_CMP(addressAbsolute(regPC)); break;
		case 0xDD: I_CMP(addressAbsoluteX(regPC)); break;
		case 0xD9: I_CMP(addressAbsoluteY(regPC)); break;
		case 0xC1: I_CMP(addressIndirectX(regPC)); break;
		case 0xD1: I_CMP(addressIndirectY(regPC)); break;

		// Compare X 
		case 0xE0: I_CPX(addressImmediate(regPC)); break;
		case 0xE4: I_CPX(addressZeroPage(regPC)); break;
		case 0xEC: I_CPX(addressAbsolute(regPC)); break;

		// Compare Y 
		case 0xC0: I_CPY(addressImmediate(regPC)); break;
		case 0xC4: I_CPY(addressZeroPage(regPC)); break;
		case 0xCC: I_CPY(addressAbsolute(regPC)); break;

		// SUBROUTINES 
		case 0x20: I_JSR(addressAbsolute(regPC)); break;
		case 0x60: I_RTS(); break;

		// FLAG
		case 0x38: I_SEC(); break;
		case 0xF8: I_SED(); break;
		case 0x78: I_SEI(); break;

		case 0x18: I_CLC(); break;
		case 0xD8: I_CLD(); break;
		case 0x58: I_CLI(); break;
		case 0xB8: I_CLV(); break;

		//copies
		case 0xAA: I_TAX(); break;
		case 0xA8: I_TAY(); break;
		case 0x8A: I_TXA(); break;
		case 0x98: I_TYA(); break;
		case 0xBA: I_TSX(); break;
		case 0x9A: I_TXS(); break;


		case 0xEA: ; break; //ayy lmao
		
		//bit
		case 0x24: I_BIT(addressZeroPage(regPC)); break;
		case 0x2C: I_BIT(addressAbsolute(regPC)); break;

		// JUMPS
		case 0x4C: I_JMP(addressAbsolute(regPC)); break;        
		case 0x6C: I_JMP(addressAbsoluteIndirect(regPC)); break;

		case 0x00: I_BREAK(); break;
		case 0x40: I_RTI(); break;



		//meme codes


		case 0x0C: 

			addressAbsolute(regPC);



			// 3. Cycles: This instruction takes 4 cycles. 
			// If you have a cycle counter, add +1 if addressAbsolute sets isPageCrossed.
			break;

		case 0x04: 

			addressZeroPage(regPC);



			// 3. Cycles: This instruction takes 3 cycles.
			break;

		case 0x14: 

			addressZeroPageX(regPC);

			
			// 3. Cycles: This instruction takes 4 cycles.
			break;


		

		default: std::cout << "OPCODE ERROR MEME NUMBER: " << opcode << std::endl;	break;
		
	}

	if (isPageCrossed) {
		extraCycles += info.extraCycles;
	}

	currentCycles += baseCycles + extraCycles;

	if (ppu->nmiSignal) {
		nmi();
		extraCycles += 7;
	}

	return baseCycles + extraCycles;

}



void NES::nmi() {
	
	push((regPC >> 8) & 0xFF);
	push(regPC & 0xFF);


	push((regP & ~0b0001'0000) | 0b0010'0000);


	regP |= 0b0000'0100;


	uint8_t lByte = read(0xFFFA);
	uint8_t hByte = read(0xFFFB);


	regPC = lByte + (hByte << 8);


	ppu->nmiSignal = false;
}





//mappers
Mapper000::Mapper000(uint8_t prgBanks){

	prgChunks = prgBanks;

	
}

bool Mapper000::cpuMapRead(uint16_t address, uint32_t& mappedAddress)
{

	if (address >= 0x8000 && address <= 0xFFFF) {

		//32KB game (2 chunks)
		if (prgChunks > 1) {
			mappedAddress = address & 0x7FFF;
		}
		//16KB game
		else {
			mappedAddress = address & 0x3FFF;
		}
		return true;
	}
	return false;
}

bool Mapper000::cpuMapWrite(uint16_t address, uint32_t& mappedAddress, uint8_t data){
	if (address >= ROMSTART && address <= ROMEND) {

		//TODO

		return false;
	}
	return false;


}
