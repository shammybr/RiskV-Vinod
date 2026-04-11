#pragma once
#include "NES.h"
#include <iostream>

NES::NES() {
	ppu = new PPU(this);
	apu = new APU(this);
	on = true;



}

void NES::loadRom(NESROM* rom){
	currentRom = rom;


	switch (currentRom->mapperID) {
	case 0:
		mapper = new Mapper000(currentRom->header.prgChunks, currentRom->mirrorMode);

		break;

	case 1:
		mapper = new Mapper001(currentRom->header.prgChunks, currentRom->header.chrChunks);

		break;

	default:
		std::cout << "ERRO no Mapper ID: " << (int)currentRom->mapperID << std::endl;
		break;
	}


	for (int i = 0; i < 8192; i++) {
		saveRam[i] = 0x00;
	}

	std::string saveName = "ROM/" + currentRom->romName + ".sav";

	std::ifstream saveFile(saveName, std::ios::binary);

	if (saveFile.is_open()) {
		saveFile.read(reinterpret_cast<char*>(saveRam), 8192);
		saveFile.close();
	}
	else {
		std::cout << "Error: falha ao ler save file!" << std::endl;
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

	//regPC = 0xC000;

	opQueue[0] = OP_FETCH_OPCODE;
	queueSize = 1;
	queueIndex = 0;
	
}

uint8_t NES::logRead(uint16_t address) {

	uint8_t data = 0b0000'0000;

	if (address <= INTERNALMIRROED) {
		//mirror
		data = memory[address & INTERNALEND];

	}
	else if (address >= PPUSTART && address <= PPUENDMIRROED) {
		data = ppu->logCpuRead(address & 0x0007);
	}
	else if (address == 0x4015) {
		data = apu->read(address);
	}
	else if (address == 0x4016) { // Player 1

		uint8_t data = (controllerState[0] & 0x80) > 0;

		return data;
	}
	else if (address == 0x4017) { // Player 2
		uint8_t data = (controllerState[1] & 0x80) > 0;

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

	if (address >= 0x6000 && address <= 0x7FFF) {
		if (mapper) {
			uint32_t mappedAddress = 0;
			if (mapper->cpuMapRead(address, mappedAddress)) {
				return saveRam[mappedAddress];
			}
		}
	}

	if (address == 0x4014) {
		static int dmaCount = 0;
		printf("DMA Count: %d\n", ++dmaCount);
		apu->debugTest7 = true;
	}

	return data;
}


uint8_t NES::read(uint16_t address) {

	if (address == 0x4013) {
		printf("[Cycle %llu] CPU Read $4013 |  Bus holds: %02X \n",
			currentCycles, cpuDataBus);
		traceCPU = true;
		LOGGO = true;
	}
	uint8_t data = cpuDataBus;

	if (address <= INTERNALMIRROED) {
		//mirror
		data = memory[address & INTERNALEND];

	}
	else if (address >= PPUSTART && address <= PPUENDMIRROED) {
		data = ppu->cpuRead(address & 0x0007);
	}
	else if (address == 0x4015) {
		data = (apu->read(address) & 0xDF) | (data & 0x20);


		bool isIRQActive = apu->frameIRQ || apu->dpcm->irqPending;

		printf("[Cycle %llu] CPU Read $4015 | Return: %02X | Bus holds: %02X | Bit 5: %d\n",
			currentCycles, data, cpuDataBus, (cpuDataBus & 0x20) >> 5);
	//	irqLatched = isIRQActive && !(regP & 0x04);

	}
	else if (address == 0x4016) { // Player 1
		
		uint8_t controllerBit = (controllerState[0] & 0x80) ? 1 : 0;
		data = (data & 0xE0) | controllerBit;

		controllerState[0] <<= 1;
	}
	else if (address == 0x4017) { // Player 2
		uint8_t controllerBit = (controllerState[0] & 0x80) ? 1 : 0;
		data = (data & 0xE0) | controllerBit;

		controllerState[1] <<= 1;
	}


	else if (address >= ROMSTART && address <= ROMEND) {
		if (currentRom) {
			if (mapper) {
				uint32_t mappedAddress = 0;

				if (mapper->cpuMapRead(address, mappedAddress)) {
					data = currentRom->vPRGMemory[mappedAddress];
				}
				if (address >= 0xFFFA) {
					//		printf("VECTOR ACCESS: [%04X] -> Mapped: [%08X] -> Data: [%02X]\n",
						//address, mappedAddress, data);
				}
			}
			else {
				if (address >= 0xFFFA) printf("VECTOR ACCESS FAILED TO MAP: [%04X]\n", address);
			}
		}
	}

	if (address >= 0x6000 && address <= 0x7FFF) {
		if (mapper) {
			uint32_t mappedAddress = 0;
			if (mapper->cpuMapRead(address, mappedAddress)) {
				data = saveRam[mappedAddress];
			}
		}
	}

	if (address == 0x4014) {
		static int dmaCount = 0;
		printf("DMA Count: %d\n", ++dmaCount);
	}

	if (address != 0x4015) {
		cpuDataBus = data;
	}


	if (address == 0x4013) {
		printf("[Cycle %llu] CPU Read $4013 |  data holds: %02X \n",
			data);
	}
	return data;
}

void NES::write(uint16_t address, uint8_t data) {
	cpuDataBus = data;

	if (address <= INTERNALMIRROED) {
		//mirror
		memory[address & INTERNALEND] = data;
	}
	else if (address >= PPUSTART && address <= PPUENDMIRROED) {
		ppu->cpuWrite(address & 0x0007, data);

	}
	else if (address == 0x4014) { // OAM DMA
		dmaPage = data;
		dmaAddress = 0;
		dmaWaiting = true;
		oamDmaState = 0; // 0 = Halt, 1 = Align, 2 = Read, 3 = Write

		// 1 cycle  + 512 for the transfer
		extraCycles += 513;
		if (currentCycles % 2 == 1) { // Write occurred on an odd cycle
			extraCycles += 1;
		}


	

	//	uint16_t dmaBase = data << 8;
	//	for (int i = 0; i < 256; i++) {
	//		ppu->oam[ppu->oamAddress] = read(dmaBase + i);
	//		ppu->oamAddress++;
	//	}

	//	currentCycles += 512;
//		extraCycles += 512;
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

	if (address >= 0x6000 && address <= 0x7FFF) {
		uint32_t mappedAddress = 0;
		if (mapper->cpuMapWrite(address, mappedAddress, data)) {
			saveRam[mappedAddress] = data;

			wannaSave = true;
			saveTime = 60;


		}
	}

	if (address == 0x4014) {
		static int dmaCount = 0;
	}

}

void NES::saveGame() {
	if (saveTime <= 0) {
		if (currentRom) {

			std::string saveName = "ROM/" + currentRom->romName + ".sav";

			std::ofstream saveFile(saveName, std::ios::binary);

			if (saveFile.is_open()) {
				saveFile.write(reinterpret_cast<char*>(saveRam), 8192);
				saveFile.close();
				std::cout << "Jogo salvo em " << saveName << std::endl;
			}
			else {
				std::cout << "Error: falha ao criar save file! " << saveName << std::endl;
			}

			wannaSave = false;
			
		}
	}
	else {
		saveTime--;
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

	if (isPageCrossed) {
		uint16_t dummyAddress = (base & 0xFF00) | (current & 0x00FF);
		read(dummyAddress); 
	}

}

uint16_t NES::addressAbsoluteY(uint16_t address) {

	uint16_t base = addressAbsolute(address);
	uint16_t current = base + regY;
	isPageCrossed = hasPageCrossed(base, current);

	if (isPageCrossed) {
		uint16_t dummyAddress = (base & 0xFF00) | (current & 0x00FF);
		read(dummyAddress);
	}

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

int NES::step(NESLogger* logger, bool isCISC) {


	if (isCISC) {
		return CISCStep(logger);
	}
	else {
		return RISCStep(logger);
	}
	

}


int NES::CISCStep(NESLogger* logger) {
	uint8_t opcode = read(regPC++);

	//log
	InstructionInfo info = logger->opTable[opcode];
	int baseCycles = info.cycles;
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


	case 0xEA:; break; //ayy lmao

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
	else if (apu->frameIRQ && !(regP & 0b0000'0100)) {

		irq();
		extraCycles += 7;
	}

	return baseCycles + extraCycles;
}

int NES::RISCStep(NESLogger* logger) {


	MicroOp currentOp = opQueue[queueIndex];
	MicroOp nextOp = opQueue[queueIndex + 1];
	isWritingMemory = (nextOp == OP_WRITE_MEM || nextOp == OP_PUSH_DATA);

	if (regPC == 0x4013) {
	//	LOGGO = true;
	}
	if (false) {
	
		printf("Cycle: %llu [%s] | PC: %04X | I: %02X | P: %02X | Latch: %d | Error: %02X | Q: %d/%d | Op: %d\n",
			currentCycles,
			(currentCycles % 2 == 0 ? "EVEN" : "ODD "), 
			regPC,
			iReg,
			regP,
			irqLatched,
			memory[16], 
			queueIndex,
			queueSize,
			(int)currentOp);
		
	}

	


	bool shouldPoll = false;
	bool isBranch = ((iReg & 0x1F) == 0x10);

	queueIndex++;

	if (isBranch) {
		if (currentOp == OP_BRANCH_CHECK) {
			
			bool condition = false;
			switch (iReg) {
			case 0x90: condition = !(regP & 0x01); break;
			case 0xB0: condition = (regP & 0x01); break;
			case 0xF0: condition = (regP & 0x02); break;
			case 0xD0: condition = !(regP & 0x02); break;
			case 0x30: condition = (regP & 0x80); break;
			case 0x10: condition = !(regP & 0x80); break;
			case 0x50: condition = !(regP & 0x40); break;
			case 0x70: condition = (regP & 0x40); break;
			}
		
			shouldPoll = !condition;

			shouldPoll = false;
		}
		else if (currentOp == OP_BRANCH_UPDATE_PC) {
			
			shouldPoll = false;
		}
		else if (currentOp == OP_DUMMY_READ) {

			shouldPoll = true;
		}
	}
	else {
	
		shouldPoll = (queueIndex == queueSize - 1);
	}


	if (currentOp == OP_FETCH_OPCODE && !isBranch) {
		shouldPoll = false; 
	}

	if (shouldPoll) {

		bool isInterruptDisabled = (regP & 0x04) > 0;

		bool isIRQActive = (apu->frameIRQ && !apu->irqInhibit) || apu->dpcm->irqPending;

		if (false) {
			printf("[Cycle %llu] POLLING: Opcode %02X | isIRQActive: %d | I_Flag: %d\n",
				currentCycles, iReg, isIRQActive, isInterruptDisabled);
		}

		if (ppu->nmiSignal)
			nmiLatched = true;



		if (isIRQActive && !(regP & 0x04)){

			irqLatched = true;
		}
		



	}



	switch (currentOp) {
		case OP_FETCH_OPCODE: {
			if (false && instructionPC != 0) {
				uint64_t cyclesTaken = currentCycles - instructionStartCycle;
				printf("[PC: %04X] Opcode: %02X | Cycles Taken: %llu\n",
					instructionPC, currentOpcodeLog, cyclesTaken);
			}


			if (nmiLatched) {
				nmiLatched = false;
				ppu->nmiSignal = false; 
				connectedWire = nullptr;
				iReg = 0xFF;

				addressLatch = regPC & 0xFF;
				addressHighLatch = (regPC >> 8) & 0xFF;

				opQueue[0] = OP_DUMMY_READ;
				opQueue[1] = OP_DUMMY_READ;
				opQueue[2] = OP_PUSH_DATA;
				opQueue[3] = OP_PUSH_DATA;
				opQueue[4] = OP_PUSH_DATA;
				opQueue[5] = OP_FETCH_NMI_LOW;
				opQueue[6] = OP_FETCH_NMI_HIGH;
				opQueue[7] = OP_FETCH_OPCODE;

				queueSize = 8;
				queueIndex = 0;
				break;
			}
			else if (irqLatched) {
				if (traceCPU) {
					printf("[Cycle %llu] FETCH HIJACKED! Jumping to IRQ Sequence.\n", currentCycles);
				}

				irqLatched = false;
				iReg = 0xFE;
				connectedWire = nullptr;

				addressLatch = regPC & 0xFF;
				addressHighLatch = (regPC >> 8) & 0xFF;

				opQueue[0] = OP_DUMMY_READ;
				opQueue[1] = OP_DUMMY_READ;
				opQueue[2] = OP_PUSH_DATA;
				opQueue[3] = OP_PUSH_DATA;
				opQueue[4] = OP_PUSH_DATA;
				opQueue[5] = OP_FETCH_IRQ_LOW;
				opQueue[6] = OP_FETCH_IRQ_HIGH;
				opQueue[7] = OP_FETCH_OPCODE;

				queueSize = 8;
				queueIndex = 0;
				break;
			}

			uint8_t opcode = read(regPC);
			addressBus = regPC;
			regPC++;

			iDecode(opcode);

			if (traceCPU) {
				instructionStartCycle = currentCycles;
				instructionPC = regPC;
				currentOpcodeLog = opcode;
			}
			if (traceCPU) {
				if (opcode == 0x58 || opcode == 0xEA) {
					printf("\n[Cycle %llu] FETCHED OPCODE: %02X\n", currentCycles, opcode);
				}
			}

			break;

		}

			break;
		case OP_FETCH_LOW_BYTE:
			addressLatch = read(regPC);
			addressBus = regPC;
			regPC++;

			break;
		case OP_FETCH_HIGH_BYTE:
			addressHighLatch = read(regPC);
			addressBus = regPC;
			regPC++;

			//jmp
			if (iReg == 0x4C || iReg == 0x20) {
				regPC = (addressHighLatch << 8) | addressLatch;
			}

			break;
		case OP_FETCH_IMMEDIATE:
			dataLatch = read(regPC);
			addressBus = regPC;
			regPC++;

			if (mathOP != OP_NONE) {
				executeALU(mathOP);
				mathOP = OP_NONE;
			}
			else if (connectedWire != nullptr) {
				*connectedWire = dataLatch;
				updateFlags(*connectedWire);
			}

			break;

		case OP_READ_MEM: {
			uint16_t addr = (addressHighLatch << 8) | addressLatch;
			addressBus = addr;
			dataLatch = read(addr);

			if (mathOP != OP_NONE) {
			
				bool isRMW = (mathOP == OP_ALU_ASL || mathOP == OP_ALU_LSR ||
					mathOP == OP_ALU_ROL || mathOP == OP_ALU_ROR ||
					mathOP == OP_ALU_INC || mathOP == OP_ALU_DEC);

				if (!isRMW) {
					executeALU(mathOP);
					mathOP = OP_NONE;
				}
			}
			else if (connectedWire != nullptr) {
			
				*connectedWire = dataLatch;
				updateFlags(*connectedWire);
			}
			break;
		}

		case OP_WRITE_MEM: {
			uint16_t addr = (addressHighLatch << 8) | addressLatch;
			addressBus = addr;

			uint8_t outData = (connectedWire != nullptr) ? *connectedWire : dataLatch;
			write(addr, outData);

			if (mathOP != OP_NONE) {
				executeALU(mathOP); 
				mathOP = OP_NONE;   
			}
			break;
		}

		case OP_DUMMY_READ: {
			uint16_t addr = (addressHighLatch << 8) | addressLatch;
			addressBus = addr;
			read(addr);

			break;
		}

		

		case OP_ADD_X_LOW: {
			uint16_t sum = addressLatch + regX;
			uint8_t newLow = sum & 0xFF;

			bool pageCrossed = (sum > 0xFF) && !isZeroPage;

		
			bool isWriteOp = false;
			for (int i = queueIndex; i < queueSize; i++) {
				if (opQueue[i] == OP_WRITE_MEM || opQueue[i] == OP_INTERNAL_INC_DEC) {
					isWriteOp = true;
					break;
				}
			}
			bool isReadOp = !isWriteOp;


			if (isReadOp && !pageCrossed && !isZeroPage) {
				addressLatch = newLow;

				queueIndex++;

				
				uint16_t addr = (addressHighLatch << 8) | addressLatch;
				addressBus = addr;
				dataLatch = read(addr);

				
				if (mathOP != OP_NONE) {
					executeALU(mathOP);
					mathOP = OP_NONE;
				}
				else if (connectedWire != nullptr) {
					*connectedWire = dataLatch;
					updateFlags(*connectedWire);
				}


			//	extraCycles = -1; 
				break;
			}


			uint16_t dummyAddr = isZeroPage ? addressLatch : ((addressHighLatch << 8) | newLow);
			read(dummyAddr); 

			addressLatch = newLow;


			if (pageCrossed) {
				addressHighLatch = (addressHighLatch + 1) & 0xFF;
			}
			break;
		}

		case OP_ADD_Y_LOW: {
			uint16_t sum = addressLatch + regY;
			uint8_t newLow = sum & 0xFF;

			bool pageCrossed = (sum > 0xFF) && !isZeroPage;


			bool isWriteOp = (opQueue[queueIndex] == OP_WRITE_MEM) || (opQueue[queueIndex] == OP_INTERNAL_INC_DEC);
			bool isReadOp = !isWriteOp;


			if (isReadOp && !pageCrossed && !isZeroPage) {
				addressLatch = newLow;

				queueIndex++;

				uint16_t addr = (addressHighLatch << 8) | addressLatch;
				addressBus = addr;
				dataLatch = read(addr);


				if (mathOP != OP_NONE) {
					executeALU(mathOP);
					mathOP = OP_NONE;
				}
				else if (connectedWire != nullptr) {
					*connectedWire = dataLatch;
					updateFlags(*connectedWire);
				}

			//	extraCycles = -1;
				break;
			}


			uint16_t dummyAddr = isZeroPage ? addressLatch : ((addressHighLatch << 8) | newLow);
			addressBus = dummyAddr;
			read(dummyAddr);

			addressLatch = newLow;


			if (pageCrossed) {
				addressHighLatch = (addressHighLatch + 1) & 0xFF;
			}
			break;
		}

		case OP_POINTER_READ_LOW: {

			uint16_t fullPtrAddr = (addressHighLatch << 8) | addressLatch;
			addressBus = fullPtrAddr;
			dataLatch = read(fullPtrAddr);

		//	dataLatch = read(addressLatch & 0xFF);
			break;
		}

		case OP_POINTER_READ_HIGH: {

			uint16_t fullPtrAddr = (addressHighLatch << 8) | addressLatch;

			uint16_t nextPtrAddr;

			if (iReg == 0x6C) {
				
				nextPtrAddr = (fullPtrAddr & 0xFF00) | ((fullPtrAddr + 1) & 0xFF);
			}
			else {
				
				nextPtrAddr = (fullPtrAddr + 1) & 0xFF;
			}

			addressBus = nextPtrAddr;
			addressHighLatch = read(nextPtrAddr);
			addressLatch = dataLatch;

			//jmp
			if (iReg == 0x6C) {
				regPC = (addressHighLatch << 8) | addressLatch;
			}
		
			break;
		}

		case OP_PUSH_DATA: {
			uint8_t data = 0;

			if (connectedWire != nullptr) {
				data = *connectedWire;

				if (iReg == 0x08) {
					data |= 0x30;
				}
			}
			else if (iReg == 0x20) { 
				data = (queueIndex == 3) ? (regPC >> 8) : (regPC & 0xFF);
			}
			else if (iReg == 0x00) {
				if (queueIndex == 2) data = (regPC >> 8);
				else if (queueIndex == 3) data = (regPC & 0xFF);
				else if (queueIndex == 4) data = regP | 0x30; 
			}
			else if (iReg == 0xFF || iReg == 0xFE) {
				if (queueIndex == 3) data = (regPC >> 8);
				else if (queueIndex == 4) data = (regPC & 0xFF);
				else if (queueIndex == 5) {
					
					data = (regP & ~0x10) | 0x20;

				
					regP |= 0x04;
				}
			}

			addressBus = 0x0100 | regSP;
			write(0x0100 | regSP, data);
			regSP--;
			break;
		}

		case OP_FETCH_NMI_LOW: {
			addressLatch = read(0xFFFA);
			addressBus = 0xFFFA;
			break;
		}

		case OP_FETCH_NMI_HIGH: {
			addressHighLatch = read(0xFFFB);
			addressBus = 0xFFFB;
		
			regPC = (addressHighLatch << 8) | addressLatch;

			regP |= 0x04;
			break;
		}

		case OP_FETCH_IRQ_LOW: {
			addressLatch = read(0xFFFE);
			addressBus = 0xFFFE;
			printf("--- VECTOR FETCH LOW: %02X ---\n", addressLatch);
			break;
		}

		case OP_FETCH_IRQ_HIGH: {
			addressHighLatch = read(0xFFFF);
			addressBus = 0xFFFF;
			printf("--- VECTOR FETCH LOW: %02X ---\n", addressLatch);
			regPC = (addressHighLatch << 8) | addressLatch;

			regP |= 0x04;
			break;
		}


		case OP_PULL_DATA: {
			regSP++;
			addressBus = 0x0100 | regSP;
			dataLatch = read(0x0100 | regSP);
			

			if (connectedWire != nullptr) {
				*connectedWire = dataLatch;
				if (connectedWire == &regA) updateFlags(*connectedWire);
			}
			else if (iReg == 0x60) {
			
				if (queueIndex == 3) addressLatch = dataLatch;
				if (queueIndex == 4) addressHighLatch = dataLatch;
			}
			else if (iReg == 0x40) {
				if (queueIndex == 3) {
					regP = (dataLatch & 0xEF) | 0x20;
				}
				if (queueIndex == 4) addressLatch = dataLatch;
				if (queueIndex == 5) {
					addressHighLatch = dataLatch;
					regPC = (addressHighLatch << 8) | addressLatch; // jump
				}
			}
			break;
		}

		case OP_DUMMY_STACK_READ: {
			addressBus = 0x0100 | regSP;
			read(0x0100 | regSP);
			break;
		}

		case OP_BRANCH_CHECK: {
			addressBus = regPC;
			dataLatch = read(regPC);
			regPC++;
		

			bool condition = false;
			switch (iReg) {
			case 0x90: condition = ((regP & 0x01) == 0); break; // BCC
			case 0xB0: condition = ((regP & 0x01) != 0); break; // BCS
			case 0xF0: condition = ((regP & 0x02) != 0); break; // BEQ
			case 0xD0: condition = ((regP & 0x02) == 0); break; // BNE
			case 0x30: condition = ((regP & 0x80) != 0); break; // BMI
			case 0x10: condition = ((regP & 0x80) == 0); break; // BPL
			case 0x50: condition = ((regP & 0x40) == 0); break; // BVC
			case 0x70: condition = ((regP & 0x40) != 0); break; // BVS
			}

		
			if (condition) {
				for (int i = queueSize; i > queueIndex; i--) {
					opQueue[i] = opQueue[i - 1];
				}
				opQueue[queueIndex] = OP_BRANCH_UPDATE_PC;
	
				queueSize++;
			}
			break;
		}

		case OP_BRANCH_UPDATE_PC: {
			uint16_t oldPC = regPC;
			regPC += (int8_t)dataLatch;  // Apply the branch offset

			// Check for page crossing
			if ((oldPC & 0xFF00) != (regPC & 0xFF00)) {
				for (int i = queueSize; i > queueIndex; i--) {
					opQueue[i] = opQueue[i - 1];
				}
				opQueue[queueIndex] = OP_DUMMY_READ;
				queueSize++;
			}
			break;
		}

		case OP_JUMP_CALC: {
			regPC = (addressHighLatch << 8) | addressLatch;

		
			if (iReg == 0x60) regPC++;
			break;
		}

		case OP_TRANSFER_REG: {
			addressBus = regPC;
			read(regPC); 

			switch (iReg) {
			case 0xAA: regX = regA; updateFlags(regX); break; // TAX
			case 0xA8: regY = regA; updateFlags(regY); break; // TAY
			case 0x8A: regA = regX; updateFlags(regA); break; // TXA
			case 0x98: regA = regY; updateFlags(regA); break; // TYA
			case 0xBA: regX = regSP; updateFlags(regX); break; // TSX
			case 0x9A: regSP = regX; break;                    // TXS (No flags updated)
			}
			break;
		}

		case OP_INTERNAL_INC_DEC: {
			addressBus = regPC;
			read(regPC);

		
			if (mathOP != OP_NONE) {
				executeALU(mathOP);
				mathOP = OP_NONE;
			}
			break;
		}

		case OP_SET_FLAG: {
			addressBus = regPC;
			read(regPC);

			switch (iReg) {
			case 0x38: regP |= 0x01; break; // SEC
			case 0xF8: regP |= 0x08; break; // SED
			case 0x78: regP |= 0x04; break; // SEI
			}
			break;
		}

		case OP_CLEAR_FLAG: {
			addressBus = regPC;
			read(regPC); 

			if (traceCPU) {
				if (iReg == 0x58) { // Only log CLI
					printf("[Cycle %llu] EXECUTED CLI! I_Flag is now 0.\n", currentCycles);
				}
			}


			switch (iReg) {
			case 0x18: regP &= ~0x01; break; // CLC
			case 0xD8: regP &= ~0x08; break; // CLD
			case 0x58: regP &= ~0x04; break; // CLI
			case 0xB8: regP &= ~0x40; break; // CLV
			}



			break;
		}

		default: std::cout << "currentOp ERROR MEME NUMBER: " << currentOp << std::endl;	break;

	}



	int returnedCycles = 1 + extraCycles;

	//currentCycles += returnedCycles;


	return returnedCycles;


}

void NES::executeALU(MicroOp mathOP){

	switch (mathOP) {
		case OP_ALU_ADC: {
			uint8_t carry = regP & 0x01; 
			uint16_t sum = regA + dataLatch + carry;

			if (~(regA ^ dataLatch) & (regA ^ sum) & 0x80) regP |= 0x40; else regP &= ~0x40; // V Flag
			if (sum > 0xFF) regP |= 0x01; else regP &= ~0x01; // C Flag

			regA = sum & 0xFF;
			updateFlags(regA);
			break;
		}

		case OP_ALU_SBC: {
			uint8_t carry = regP & 0x01;
			uint16_t invertedData = dataLatch ^ 0xFF;
			uint16_t sum = regA + invertedData + carry;

			if (~(regA ^ invertedData) & (regA ^ sum) & 0x80) regP |= 0x40; else regP &= ~0x40; // V Flag
			if (sum > 0xFF) regP |= 0x01; else regP &= ~0x01; // C Flag

			regA = sum & 0xFF;
			updateFlags(regA);
			break;
		}

		case OP_ALU_AND: {
			regA = regA & dataLatch;
			updateFlags(regA);
			break;
		}

		case OP_ALU_ORA: {
			regA = regA | dataLatch;
			updateFlags(regA);
			break;
		}

		case OP_ALU_EOR: {
			regA = regA ^ dataLatch;
			updateFlags(regA);
			break;
		}

		case OP_ALU_BIT: { 
			uint8_t result = regA & dataLatch;
			if (result == 0) regP |= 0x02; else regP &= ~0x02;     
			regP = (regP & 0x3F) | (dataLatch & 0xC0);             
			break;
		}

		case OP_ALU_CMP: {
			uint16_t diff = regA - dataLatch;
			if (regA >= dataLatch) regP |= 0x01; else regP &= ~0x01; 
			updateFlags(diff & 0xFF);
			break;
		}

		case OP_ALU_CPX: {
			uint16_t diff = regX - dataLatch;
			if (regX >= dataLatch) regP |= 0x01; else regP &= ~0x01;
			updateFlags(diff & 0xFF);
			break;
		}

		case OP_ALU_CPY: {
			uint16_t diff = regY - dataLatch;
			if (regY >= dataLatch) regP |= 0x01; else regP &= ~0x01;
			updateFlags(diff & 0xFF);
			break;
		}

		case OP_ALU_ASL: {
			uint8_t target = (connectedWire != nullptr) ? *connectedWire : dataLatch;
			if (target & 0x80) regP |= 0x01; else regP &= ~0x01; // C Flag
			target <<= 1;

			if (connectedWire != nullptr) *connectedWire = target; else dataLatch = target;
			updateFlags(target);
			break;
		}

		case OP_ALU_LSR: {
			uint8_t target = (connectedWire != nullptr) ? *connectedWire : dataLatch;
			if (target & 0x01) regP |= 0x01; else regP &= ~0x01; // C Flag
			target >>= 1;

			if (connectedWire != nullptr) *connectedWire = target; else dataLatch = target;
			updateFlags(target);
			break;
		}

		case OP_ALU_ROL: {
			uint8_t target = (connectedWire != nullptr) ? *connectedWire : dataLatch;
			uint8_t oldCarry = (regP & 0x01);
			if (target & 0x80) regP |= 0x01; else regP &= ~0x01; // C Flag
			target = (target << 1) | oldCarry;

			if (connectedWire != nullptr) *connectedWire = target; else dataLatch = target;
			updateFlags(target);
			break;
		}

		case OP_ALU_ROR: {
			uint8_t target = (connectedWire != nullptr) ? *connectedWire : dataLatch;
			uint8_t oldCarry = (regP & 0x01);
			if (target & 0x01) regP |= 0x01; else regP &= ~0x01; // C Flag
			target = (target >>= 1) | (oldCarry << 7);

			if (connectedWire != nullptr) *connectedWire = target; else dataLatch = target;
			updateFlags(target);
			break;
		}

		case OP_ALU_INC: {
			uint8_t target = (connectedWire != nullptr) ? *connectedWire : dataLatch;
			target++;
			if (connectedWire != nullptr) *connectedWire = target; else dataLatch = target;
			updateFlags(target);
			break;
		}

		case OP_ALU_DEC: {
			uint8_t target = (connectedWire != nullptr) ? *connectedWire : dataLatch;
			target--;
			if (connectedWire != nullptr) *connectedWire = target; else dataLatch = target;
			updateFlags(target);
			break;
		}

		case OP_ALU_SLO: {
			// 1. ASL part: Shift dataLatch left, Bit 7 goes to Carry
			uint8_t oldData = dataLatch;
			(oldData & 0x80) ? (regP |= 0x01) : (regP &= ~0x01); // Set Carry
			dataLatch = oldData << 1;

			// 2. ORA part: Accumulator |= shifted value
			regA |= dataLatch;
			updateFlags(regA); 
		
			break;
		}
	}

}

void NES::iDecode(uint8_t opcode) {
	queueIndex = 0;
	iReg = opcode;
	connectedWire = nullptr;
	mathOP = OP_NONE;

	addressHighLatch = 1;
	isZeroPage = false;

	switch (opcode) {

			//LDA
		case 0xA9: 
			connectedWire = &regA;
			opQueue[0] = OP_FETCH_IMMEDIATE;
			queueSize = 1;

			//I_LDA(addressImmediate(regPC)); 
			
			break;

		case 0xA5: 
			connectedWire = &regA;
			addressHighLatch = 0x0000;

			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_READ_MEM;

			queueSize = 2;

			isZeroPage = true;
		//	I_LDA(addressZeroPage(regPC)); 
			break;


		case 0xB5: 
			connectedWire = &regA;
			addressHighLatch = 0x0000;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_ADD_X_LOW;
			opQueue[2] = OP_READ_MEM;

			queueSize = 3;

			isZeroPage = true;
			//I_LDA(addressZeroPageX(regPC)); 
			break;
		case 0xAD: 
			connectedWire = &regA;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_FETCH_HIGH_BYTE;
			opQueue[2] = OP_READ_MEM;

			queueSize = 3;

			//I_LDA(addressAbsolute(regPC)); 
			break;


		case 0xBD: 
			connectedWire = &regA;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_FETCH_HIGH_BYTE;
			opQueue[2] = OP_ADD_X_LOW;
			opQueue[3] = OP_READ_MEM;

			queueSize = 4;

			//I_LDA(addressAbsoluteX(regPC)); 
			break;

		case 0xB9: 
			connectedWire = &regA;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_FETCH_HIGH_BYTE;
			opQueue[2] = OP_ADD_Y_LOW;
			opQueue[3] = OP_READ_MEM;

			queueSize = 4;

			//I_LDA(addressAbsoluteY(regPC)); 
			break;
		case 0xA1: 
			connectedWire = &regA;
			addressHighLatch = 0x0000;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_ADD_X_LOW;
			opQueue[2] = OP_POINTER_READ_LOW;
			opQueue[3] = OP_POINTER_READ_HIGH;
			opQueue[4] = OP_READ_MEM;
			


			queueSize = 5;

			isZeroPage = true;
			//I_LDA(addressIndirectX(regPC)); 
			break;
		case 0xB1: 
			connectedWire = &regA;
			addressHighLatch = 0x0000;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_POINTER_READ_LOW;
			opQueue[2] = OP_POINTER_READ_HIGH;
			opQueue[3] = OP_ADD_Y_LOW;
			opQueue[4] = OP_READ_MEM;



			queueSize = 5;


			//I_LDA(addressIndirectY(regPC)); 
			break;

			// load X Register
		case 0xA2: 
			connectedWire = &regX;
			opQueue[0] = OP_FETCH_IMMEDIATE;
			queueSize = 1;

			//I_LDX(addressImmediate(regPC)); 
			break;
		case 0xA6: 
			connectedWire = &regX;
			addressHighLatch = 0x0000;

			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_READ_MEM;

			queueSize = 2; 

			isZeroPage = true;
			//I_LDX(addressZeroPage(regPC)); 
			
			break;
		case 0xB6: 
			connectedWire = &regX;
			addressHighLatch = 0x0000;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_ADD_Y_LOW;
			opQueue[2] = OP_READ_MEM;

			queueSize = 3;

			isZeroPage = true;
			//I_LDX(addressZeroPageY(regPC)); 
			
			break;
		case 0xAE: 
			connectedWire = &regX;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_FETCH_HIGH_BYTE;
			opQueue[2] = OP_READ_MEM;

			queueSize = 3;


			//I_LDX(addressAbsolute(regPC));
			
			break;
		case 0xBE:
			connectedWire = &regX;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_FETCH_HIGH_BYTE;
			opQueue[2] = OP_ADD_Y_LOW;
			opQueue[3] = OP_READ_MEM;
			
			queueSize = 4;

			//I_LDX(addressAbsoluteY(regPC)); 
			
			break;

			// Y 
		case 0xA0: 
			connectedWire = &regY;
			opQueue[0] = OP_FETCH_IMMEDIATE;
			queueSize = 1;

			
			//I_LDY(addressImmediate(regPC)); 
			
			break;
		case 0xA4: 
			connectedWire = &regY;
			addressHighLatch = 0x0000;

			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_READ_MEM;

			queueSize = 2;

			isZeroPage = true;
			//I_LDY(addressZeroPage(regPC));
			
			break;
		case 0xB4: 
			connectedWire = &regY;
			addressHighLatch = 0x0000;

			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_ADD_X_LOW;
			opQueue[2] = OP_READ_MEM;

			queueSize = 3;

			isZeroPage = true;
			//I_LDY(addressZeroPageX(regPC)); 
			
			break;
		case 0xAC: 
			connectedWire = &regY;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_FETCH_HIGH_BYTE;
			opQueue[2] = OP_READ_MEM;

			queueSize = 3;


			//I_LDY(addressAbsolute(regPC)); 
			
			break;
		case 0xBC: 
			
			connectedWire = &regY;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_FETCH_HIGH_BYTE;
			opQueue[2] = OP_ADD_X_LOW;
			opQueue[3] = OP_READ_MEM;

			queueSize = 4;

			//I_LDY(addressAbsoluteX(regPC)); 
			
			break;

			// store X 
		case 0x86: 
			connectedWire = &regX;
			addressHighLatch = 0x0000;

			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_WRITE_MEM;

			queueSize = 2;

			isZeroPage = true;
			//I_STX(addressZeroPage(regPC)); 
			
			break;

		case 0x96: 
			connectedWire = &regX;
			addressHighLatch = 0x0000;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_ADD_Y_LOW;
			opQueue[2] = OP_WRITE_MEM;

			queueSize = 3;

			isZeroPage = true;
			//I_STX(addressZeroPageY(regPC)); 
			break;

		case 0x8E: 
			connectedWire = &regX;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_FETCH_HIGH_BYTE;
			opQueue[2] = OP_WRITE_MEM;

			queueSize = 3;


			//I_STX(addressAbsolute(regPC)); 
			break;

			// Y 
		case 0x84: 
			
			connectedWire = &regY;
			addressHighLatch = 0x0000;

			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_WRITE_MEM;

			queueSize = 2;

			isZeroPage = true;
			//I_STY(addressZeroPage(regPC)); 
			break;

		case 0x94: 
			connectedWire = &regY;
			addressHighLatch = 0x0000;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_ADD_X_LOW;
			opQueue[2] = OP_WRITE_MEM;

			queueSize = 3;

			isZeroPage = true;
			//I_STY(addressZeroPageX(regPC)); 
			break;

		case 0x8C: 
			connectedWire = &regY;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_FETCH_HIGH_BYTE;
			opQueue[2] = OP_WRITE_MEM;

			queueSize = 3;

			//I_STY(addressAbsolute(regPC)); 
			break;

			// accumulator
		case 0x85: 
			connectedWire = &regA;
			addressHighLatch = 0x0000;

			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_WRITE_MEM;
			queueSize = 2;

			isZeroPage = true;
			//I_STA(addressZeroPage(regPC)); 
			break;

		case 0x95: 
			
			connectedWire = &regA;
			addressHighLatch = 0x0000;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_ADD_X_LOW;
			opQueue[2] = OP_WRITE_MEM;

			queueSize = 3;

			isZeroPage = true;
			//I_STA(addressZeroPageX(regPC)); 
			break;

		case 0x8D: 
			connectedWire = &regA;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_FETCH_HIGH_BYTE;
			opQueue[2] = OP_WRITE_MEM;

			queueSize = 3;



			//I_STA(addressAbsolute(regPC)); 
			break;

		case 0x9D: 
			connectedWire = &regA;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_FETCH_HIGH_BYTE;
			opQueue[2] = OP_ADD_X_LOW;
			opQueue[3] = OP_WRITE_MEM;

			queueSize = 4;



			//I_STA(addressAbsoluteX(regPC)); 
			break;

		case 0x99: 
			
			connectedWire = &regA;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_FETCH_HIGH_BYTE;
			opQueue[2] = OP_ADD_Y_LOW;
			opQueue[3] = OP_WRITE_MEM;

			queueSize = 4;


			//I_STA(addressAbsoluteY(regPC)); 
			break;

		case 0x81: 
			connectedWire = &regA;
			addressHighLatch = 0x0000;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_ADD_X_LOW;
			opQueue[2] = OP_POINTER_READ_LOW;
			opQueue[3] = OP_POINTER_READ_HIGH;
			opQueue[4] = OP_WRITE_MEM;



			queueSize = 5;

			isZeroPage = true;

			//I_STA(addressIndirectX(regPC)); 
			break;

		case 0x91: 
			connectedWire = &regA;
			addressHighLatch = 0x0000;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_POINTER_READ_LOW;
			opQueue[2] = OP_POINTER_READ_HIGH;
			opQueue[3] = OP_ADD_Y_LOW;
			opQueue[4] = OP_WRITE_MEM;



			queueSize = 5;

			//I_STA(addressIndirectY(regPC));
			break;

			//ATTACK DAMAGE CARRY
		case 0x69: 
			mathOP = OP_ALU_ADC;
			opQueue[0] = OP_FETCH_IMMEDIATE;
			queueSize = 1;


			//I_ADC(addressImmediate(regPC));
			break;

		case 0x65:
			mathOP = OP_ALU_ADC;
			addressHighLatch = 0x0000;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_READ_MEM;
			queueSize = 2;

			isZeroPage = true;
			//I_ADC(addressZeroPage(regPC))
			break;

		case 0x75:
			mathOP = OP_ALU_ADC;
			addressHighLatch = 0x0000;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_ADD_X_LOW;
			opQueue[2] = OP_READ_MEM;
			queueSize = 3;

			isZeroPage = true;
			//I_ADC(addressZeroPageX(regPC))
			break;

		case 0x6D:
			mathOP = OP_ALU_ADC;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_FETCH_HIGH_BYTE;
			opQueue[2] = OP_READ_MEM;
			queueSize = 3;

			//I_ADC(addressAbsolute(regPC))
			break;

		case 0x7D:
			mathOP = OP_ALU_ADC;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_FETCH_HIGH_BYTE;
			opQueue[2] = OP_ADD_X_LOW;
			opQueue[3] = OP_READ_MEM;
			queueSize = 4;

			//I_ADC(addressAbsolute(regPC))
			break;

		case 0x79:
			mathOP = OP_ALU_ADC;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_FETCH_HIGH_BYTE;
			opQueue[2] = OP_ADD_Y_LOW;
			opQueue[3] = OP_READ_MEM;
			queueSize = 4;

			//I_ADC(addressAbsoluteX(regPC))
			break;

		case 0x61:
			mathOP = OP_ALU_ADC;
			addressHighLatch = 0x0000;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_ADD_X_LOW;
			opQueue[2] = OP_POINTER_READ_LOW;
			opQueue[3] = OP_POINTER_READ_HIGH;
			opQueue[4] = OP_READ_MEM;
			queueSize = 5;

			//I_ADC(addressAbsoluteY(regPC))
			break;

		case 0x71:
			mathOP = OP_ALU_ADC;
			addressHighLatch = 0x0000;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_POINTER_READ_LOW;
			opQueue[2] = OP_POINTER_READ_HIGH;
			opQueue[3] = OP_ADD_Y_LOW;
			opQueue[4] = OP_READ_MEM;
			queueSize = 5;

			// I_ADC(addressIndirectY(regPC))
			break;

			// subtract with carry
		case 0xE9:
			mathOP = OP_ALU_SBC;
			opQueue[0] = OP_FETCH_IMMEDIATE;
			queueSize = 1;

			// I_SBC(addressImmediate(regPC)); 
			break;
		case 0xE5:
			mathOP = OP_ALU_SBC;
			addressHighLatch = 0x0000;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_READ_MEM;
			queueSize = 2;
			
			isZeroPage = true;
			// I_SBC(addressZeroPage(regPC)); 
			break;
		case 0xF5:
			mathOP = OP_ALU_SBC;
			addressHighLatch = 0x0000;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_ADD_X_LOW;
			opQueue[2] = OP_READ_MEM;
			queueSize = 3;

			isZeroPage = true;
			// I_SBC(addressZeroPageX(regPC)); 
			break;
		case 0xED:
			mathOP = OP_ALU_SBC;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_FETCH_HIGH_BYTE;
			opQueue[2] = OP_READ_MEM;
			queueSize = 3;

			// I_SBC(addressAbsolute(regPC)); 
			break;
		case 0xFD:
			mathOP = OP_ALU_SBC;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_FETCH_HIGH_BYTE;
			opQueue[2] = OP_ADD_X_LOW;
			opQueue[3] = OP_READ_MEM;
			queueSize = 4;

			// I_SBC(addressAbsoluteX(regPC)); 
			break;
		case 0xF9:
			mathOP = OP_ALU_SBC;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_FETCH_HIGH_BYTE;
			opQueue[2] = OP_ADD_Y_LOW;
			opQueue[3] = OP_READ_MEM;
			queueSize = 4;

			// I_SBC(addressAbsoluteY(regPC)); 
			break;
		case 0xE1:
			mathOP = OP_ALU_SBC;
			addressHighLatch = 0x0000;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_ADD_X_LOW;
			opQueue[2] = OP_POINTER_READ_LOW;
			opQueue[3] = OP_POINTER_READ_HIGH;
			opQueue[4] = OP_READ_MEM;
			queueSize = 5;

			isZeroPage = true;
			// I_SBC(addressIndirectX(regPC)); 
			break;
		case 0xF1:
			mathOP = OP_ALU_SBC;
			addressHighLatch = 0x0000;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_POINTER_READ_LOW;
			opQueue[2] = OP_POINTER_READ_HIGH;
			opQueue[3] = OP_ADD_Y_LOW;
			opQueue[4] = OP_READ_MEM;
			queueSize = 5;

			// I_SBC(addressIndirectY(regPC)); 
			break;

			// logic AND
		case 0x29:
			mathOP = OP_ALU_AND;
			opQueue[0] = OP_FETCH_IMMEDIATE;
			queueSize = 1;

			// I_AND(addressImmediate(regPC)); 
			break;
		case 0x25:
			mathOP = OP_ALU_AND;
			addressHighLatch = 0x0000;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_READ_MEM;
			queueSize = 2;

			isZeroPage = true;
			// I_AND(addressZeroPage(regPC)); 
			break;
		case 0x35:
			mathOP = OP_ALU_AND;
			addressHighLatch = 0x0000;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_ADD_X_LOW;
			opQueue[2] = OP_READ_MEM;
			queueSize = 3;

			isZeroPage = true;
			// I_AND(addressZeroPageX(regPC)); 
			break;
		case 0x2D:
			mathOP = OP_ALU_AND;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_FETCH_HIGH_BYTE;
			opQueue[2] = OP_READ_MEM;
			queueSize = 3;

			// I_AND(addressAbsolute(regPC)); 
			break;
		case 0x3D:
			mathOP = OP_ALU_AND;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_FETCH_HIGH_BYTE;
			opQueue[2] = OP_ADD_X_LOW;
			opQueue[3] = OP_READ_MEM;
			queueSize = 4;

			// I_AND(addressAbsoluteX(regPC)); 
			break;
		case 0x39:
			mathOP = OP_ALU_AND;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_FETCH_HIGH_BYTE;
			opQueue[2] = OP_ADD_Y_LOW;
			opQueue[3] = OP_READ_MEM;
			queueSize = 4;

			// I_AND(addressAbsoluteY(regPC)); 
			break;
		case 0x21:
			mathOP = OP_ALU_AND;
			addressHighLatch = 0x0000;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_ADD_X_LOW;
			opQueue[2] = OP_POINTER_READ_LOW;
			opQueue[3] = OP_POINTER_READ_HIGH;
			opQueue[4] = OP_READ_MEM;
			queueSize = 5;

			isZeroPage = true;
			// I_AND(addressIndirectX(regPC)); 
			break;
		case 0x31:
			mathOP = OP_ALU_AND;
			addressHighLatch = 0x0000;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_POINTER_READ_LOW;
			opQueue[2] = OP_POINTER_READ_HIGH;
			opQueue[3] = OP_ADD_Y_LOW;
			opQueue[4] = OP_READ_MEM;
			queueSize = 5;

			// I_AND(addressIndirectY(regPC)); 
			break;

			// logic ORA
		case 0x09:
			mathOP = OP_ALU_ORA;
			opQueue[0] = OP_FETCH_IMMEDIATE;
			queueSize = 1;

			// I_ORA(addressImmediate(regPC)); 
			break;
		case 0x05:
			mathOP = OP_ALU_ORA;
			addressHighLatch = 0x0000;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_READ_MEM;
			queueSize = 2;

			isZeroPage = true;
			// I_ORA(addressZeroPage(regPC)); 
			break;
		case 0x15:
			mathOP = OP_ALU_ORA;
			addressHighLatch = 0x0000;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_ADD_X_LOW;
			opQueue[2] = OP_READ_MEM;
			queueSize = 3;

			isZeroPage = true;
			// I_ORA(addressZeroPageX(regPC)); 
			break;
		case 0x0D:
			mathOP = OP_ALU_ORA;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_FETCH_HIGH_BYTE;
			opQueue[2] = OP_READ_MEM;
			queueSize = 3;

			// I_ORA(addressAbsolute(regPC)); 
			break;
		case 0x1D:
			mathOP = OP_ALU_ORA;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_FETCH_HIGH_BYTE;
			opQueue[2] = OP_ADD_X_LOW;
			opQueue[3] = OP_READ_MEM;
			queueSize = 4;

			// I_ORA(addressAbsoluteX(regPC)); 
			break;
		case 0x19:
			mathOP = OP_ALU_ORA;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_FETCH_HIGH_BYTE;
			opQueue[2] = OP_ADD_Y_LOW;
			opQueue[3] = OP_READ_MEM;
			queueSize = 4;

			// I_ORA(addressAbsoluteY(regPC)); 
			break;
		case 0x01:
			mathOP = OP_ALU_ORA;
			addressHighLatch = 0x0000;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_ADD_X_LOW;
			opQueue[2] = OP_POINTER_READ_LOW;
			opQueue[3] = OP_POINTER_READ_HIGH;
			opQueue[4] = OP_READ_MEM;
			queueSize = 5;

			isZeroPage = true;
			// I_ORA(addressIndirectX(regPC)); 
			break;
		case 0x11:
			mathOP = OP_ALU_ORA;
			addressHighLatch = 0x0000;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_POINTER_READ_LOW;
			opQueue[2] = OP_POINTER_READ_HIGH;
			opQueue[3] = OP_ADD_Y_LOW;
			opQueue[4] = OP_READ_MEM;
			queueSize = 5;

			// I_ORA(addressIndirectY(regPC)); 
			break;

			// logic EOR
		case 0x49:
			mathOP = OP_ALU_EOR;
			opQueue[0] = OP_FETCH_IMMEDIATE;
			queueSize = 1;

			// I_EOR(addressImmediate(regPC)); 
			break;
		case 0x45:
			mathOP = OP_ALU_EOR;
			addressHighLatch = 0x0000;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_READ_MEM;
			queueSize = 2;

			isZeroPage = true;
			// I_EOR(addressZeroPage(regPC)); 
			break;
		case 0x55:
			mathOP = OP_ALU_EOR;
			addressHighLatch = 0x0000;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_ADD_X_LOW;
			opQueue[2] = OP_READ_MEM;
			queueSize = 3;


			isZeroPage = true;
			// I_EOR(addressZeroPageX(regPC)); 
			break;
		case 0x4D:
			mathOP = OP_ALU_EOR;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_FETCH_HIGH_BYTE;
			opQueue[2] = OP_READ_MEM;
			queueSize = 3;

			// I_EOR(addressAbsolute(regPC)); 
			break;
		case 0x5D:
			mathOP = OP_ALU_EOR;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_FETCH_HIGH_BYTE;
			opQueue[2] = OP_ADD_X_LOW;
			opQueue[3] = OP_READ_MEM;
			queueSize = 4;

			// I_EOR(addressAbsoluteX(regPC)); 
			break;
		case 0x59:
			mathOP = OP_ALU_EOR;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_FETCH_HIGH_BYTE;
			opQueue[2] = OP_ADD_Y_LOW;
			opQueue[3] = OP_READ_MEM;
			queueSize = 4;

			// I_EOR(addressAbsoluteY(regPC)); 
			break;
		case 0x41:
			mathOP = OP_ALU_EOR;
			addressHighLatch = 0x0000;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_ADD_X_LOW;
			opQueue[2] = OP_POINTER_READ_LOW;
			opQueue[3] = OP_POINTER_READ_HIGH;
			opQueue[4] = OP_READ_MEM;
			queueSize = 5;

			isZeroPage = true;
			// I_EOR(addressIndirectX(regPC)); 
			break;
		case 0x51:
			mathOP = OP_ALU_EOR;
			addressHighLatch = 0x0000;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_POINTER_READ_LOW;
			opQueue[2] = OP_POINTER_READ_HIGH;
			opQueue[3] = OP_ADD_Y_LOW;
			opQueue[4] = OP_READ_MEM;
			queueSize = 5;

			// I_EOR(addressIndirectY(regPC)); 
			break;


			// branches

		case 0x90: // BCC
		case 0xB0: // BCS
		case 0xF0: // BEQ
		case 0xD0: // BNE
		case 0x30: // BMI
		case 0x10: // BPL
		case 0x50: // BVC
		case 0x70: // BVS
			opQueue[0] = OP_BRANCH_CHECK;
			queueSize = 1;
			break;

			// LSR Accumulator
		case 0x4A:
			connectedWire = &regA;
			mathOP = OP_ALU_LSR;
			opQueue[0] = OP_INTERNAL_INC_DEC; 
			queueSize = 1;
			// regA = I_LSR(regA); 
			break;

			// LSR Zero Page
		case 0x46:
			mathOP = OP_ALU_LSR;
			addressHighLatch = 0x0000;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_READ_MEM;
			opQueue[2] = OP_WRITE_MEM; 
			opQueue[3] = OP_WRITE_MEM;
			queueSize = 4;

			isZeroPage = true;
			/* uint16_t addr = addressZeroPage(regPC);
			   write(addr, I_LSR(read(addr))); */
			break;

		case 0x56:
			mathOP = OP_ALU_LSR;
			addressHighLatch = 0x0000;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_ADD_X_LOW;
			opQueue[2] = OP_READ_MEM;
			opQueue[3] = OP_WRITE_MEM;
			opQueue[4] = OP_WRITE_MEM;
			queueSize = 5;

			isZeroPage = true;
			/* uint16_t addr = addressZeroPageX(regPC);
			   write(addr, I_LSR(read(addr))); */
			break;

		case 0x4E:
			mathOP = OP_ALU_LSR;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_FETCH_HIGH_BYTE;
			opQueue[2] = OP_READ_MEM;
			opQueue[3] = OP_WRITE_MEM;
			opQueue[4] = OP_WRITE_MEM;
			queueSize = 5;
			/* uint16_t addr = addressAbsolute(regPC);
			   write(addr, I_LSR(read(addr))); */
			break;

		case 0x5E:
			mathOP = OP_ALU_LSR;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_FETCH_HIGH_BYTE;
			opQueue[2] = OP_ADD_X_LOW;
			opQueue[3] = OP_READ_MEM;
			opQueue[4] = OP_WRITE_MEM;
			opQueue[5] = OP_WRITE_MEM;
			queueSize = 6;
			/* uint16_t addr = addressAbsoluteX(regPC);
			   write(addr, I_LSR(read(addr))); */
			break;

			// ROL
		case 0x2A:
			connectedWire = &regA;
			mathOP = OP_ALU_ROL;
			opQueue[0] = OP_INTERNAL_INC_DEC;
			queueSize = 1;
			// regA = I_ROL(regA); 
			break;

		case 0x26:
			mathOP = OP_ALU_ROL;
			addressHighLatch = 0x0000;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_READ_MEM;
			opQueue[2] = OP_WRITE_MEM;
			opQueue[3] = OP_WRITE_MEM;
			queueSize = 4;

			isZeroPage = true;
			/* uint16_t addr = addressZeroPage(regPC);
			   write(addr, I_ROL(read(addr))); */
			break;

		case 0x36:
			mathOP = OP_ALU_ROL;
			addressHighLatch = 0x0000;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_ADD_X_LOW;
			opQueue[2] = OP_READ_MEM;
			opQueue[3] = OP_WRITE_MEM;
			opQueue[4] = OP_WRITE_MEM;
			queueSize = 5;

			isZeroPage = true;
			/* uint16_t addr = addressZeroPageX(regPC);
			   write(addr, I_ROL(read(addr))); */
			break;

		case 0x2E:
			mathOP = OP_ALU_ROL;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_FETCH_HIGH_BYTE;
			opQueue[2] = OP_READ_MEM;
			opQueue[3] = OP_WRITE_MEM;
			opQueue[4] = OP_WRITE_MEM;
			queueSize = 5;
			/* uint16_t addr = addressAbsolute(regPC);
			   write(addr, I_ROL(read(addr))); */
			break;

		case 0x3E:
			mathOP = OP_ALU_ROL;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_FETCH_HIGH_BYTE;
			opQueue[2] = OP_ADD_X_LOW;
			opQueue[3] = OP_READ_MEM;
			opQueue[4] = OP_WRITE_MEM;
			opQueue[5] = OP_WRITE_MEM;
			queueSize = 6;
			/* uint16_t addr = addressAbsoluteX(regPC);
			   write(addr, I_ROL(read(addr))); */
			break;

			// ROR
		case 0x6A:
			connectedWire = &regA;
			mathOP = OP_ALU_ROR;
			opQueue[0] = OP_INTERNAL_INC_DEC;
			queueSize = 1;
			// regA = I_ROR(regA); 
			break;

		case 0x66:
			mathOP = OP_ALU_ROR;
			addressHighLatch = 0x0000;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_READ_MEM;
			opQueue[2] = OP_WRITE_MEM;
			opQueue[3] = OP_WRITE_MEM;
			queueSize = 4;

			isZeroPage = true;
			/* uint16_t addr = addressZeroPage(regPC);
			   write(addr, I_ROR(read(addr))); */
			break;

		case 0x76:
			mathOP = OP_ALU_ROR;
			addressHighLatch = 0x0000;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_ADD_X_LOW;
			opQueue[2] = OP_READ_MEM;
			opQueue[3] = OP_WRITE_MEM;
			opQueue[4] = OP_WRITE_MEM;
			queueSize = 5;

			isZeroPage = true;
			/* uint16_t addr = addressZeroPageX(regPC);
			   write(addr, I_ROR(read(addr))); */
			break;

		case 0x6E:
			mathOP = OP_ALU_ROR;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_FETCH_HIGH_BYTE;
			opQueue[2] = OP_READ_MEM;
			opQueue[3] = OP_WRITE_MEM;
			opQueue[4] = OP_WRITE_MEM;
			queueSize = 5;
			/* uint16_t addr = addressAbsolute(regPC);
			   write(addr, I_ROR(read(addr))); */
			break;

		case 0x7E:
			mathOP = OP_ALU_ROR;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_FETCH_HIGH_BYTE;
			opQueue[2] = OP_ADD_X_LOW;
			opQueue[3] = OP_READ_MEM;
			opQueue[4] = OP_WRITE_MEM;
			opQueue[5] = OP_WRITE_MEM;
			queueSize = 6;
			/* uint16_t addr = addressAbsoluteX(regPC);
			   write(addr, I_ROR(read(addr))); */
			break;

			// ASL Accumulator
		case 0x0A:
			connectedWire = &regA;
			mathOP = OP_ALU_ASL;
			opQueue[0] = OP_INTERNAL_INC_DEC;
			queueSize = 1;
			// regA = I_ASL(regA); 
			break;

			// ASL Zero Page
		case 0x06:
			mathOP = OP_ALU_ASL;
			addressHighLatch = 0x0000;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_READ_MEM;
			opQueue[2] = OP_WRITE_MEM;
			opQueue[3] = OP_WRITE_MEM;
			queueSize = 4;

			isZeroPage = true;
			/* uint16_t addr = addressZeroPage(regPC);
			   uint8_t data = read(addr);
			   write(addr, I_ASL(data)); */
			break;

		case 0x16:
			mathOP = OP_ALU_ASL;
			addressHighLatch = 0x0000;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_ADD_X_LOW;
			opQueue[2] = OP_READ_MEM;
			opQueue[3] = OP_WRITE_MEM;
			opQueue[4] = OP_WRITE_MEM;
			queueSize = 5;
			/* uint16_t addr = addressZeroPageX(regPC);
			   uint8_t data = read(addr);
			   write(addr, I_ASL(data)); */
			break;

		case 0x0E:
			mathOP = OP_ALU_ASL;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_FETCH_HIGH_BYTE;
			opQueue[2] = OP_READ_MEM;
			opQueue[3] = OP_WRITE_MEM;
			opQueue[4] = OP_WRITE_MEM;
			queueSize = 5;
			/* uint16_t addr = addressAbsolute(regPC);
			   uint8_t data = read(addr);
			   write(addr, I_ASL(data)); */
			break;

		case 0x1E:
			mathOP = OP_ALU_ASL;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_FETCH_HIGH_BYTE;
			opQueue[2] = OP_ADD_X_LOW;
			opQueue[3] = OP_READ_MEM;
			opQueue[4] = OP_WRITE_MEM;
			opQueue[5] = OP_WRITE_MEM;
			queueSize = 6;
			/* uint16_t addr = addressAbsoluteX(regPC);
			   uint8_t data = read(addr);
			   write(addr, I_ASL(data)); */
			break;


			// +1 -1 regs
		case 0xE8:
			connectedWire = &regX;
			mathOP = OP_ALU_INC;
			opQueue[0] = OP_INTERNAL_INC_DEC;
			queueSize = 1;
			// I_INX(); 
			break;
		case 0xCA:
			connectedWire = &regX;
			mathOP = OP_ALU_DEC;
			opQueue[0] = OP_INTERNAL_INC_DEC;
			queueSize = 1;
			// I_DEX(); 
			break;
		case 0xC8:
			connectedWire = &regY;
			mathOP = OP_ALU_INC;
			opQueue[0] = OP_INTERNAL_INC_DEC;
			queueSize = 1;
			// I_INY(); 
			break;
		case 0x88:
			connectedWire = &regY;
			mathOP = OP_ALU_DEC;
			opQueue[0] = OP_INTERNAL_INC_DEC;
			queueSize = 1;
			// I_DEY(); 
			break;

			// +1 -1 memory
		case 0xE6:
			mathOP = OP_ALU_INC;
			addressHighLatch = 0x0000;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_READ_MEM;
			opQueue[2] = OP_WRITE_MEM;
			opQueue[3] = OP_WRITE_MEM;
			queueSize = 4;

			isZeroPage = true;
			// I_INC(addressZeroPage(regPC)); 
			break;
		case 0xF6:
			mathOP = OP_ALU_INC;
			addressHighLatch = 0x0000;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_ADD_X_LOW;
			opQueue[2] = OP_READ_MEM;
			opQueue[3] = OP_WRITE_MEM;
			opQueue[4] = OP_WRITE_MEM;
			queueSize = 5;

			isZeroPage = true;
			// I_INC(addressZeroPageX(regPC)); 
			break;
		case 0xEE:
			mathOP = OP_ALU_INC;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_FETCH_HIGH_BYTE;
			opQueue[2] = OP_READ_MEM;
			opQueue[3] = OP_WRITE_MEM;
			opQueue[4] = OP_WRITE_MEM;
			queueSize = 5;
			// I_INC(addressAbsolute(regPC)); 
			break;
		case 0xFE:
			mathOP = OP_ALU_INC;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_FETCH_HIGH_BYTE;
			opQueue[2] = OP_ADD_X_LOW;
			opQueue[3] = OP_READ_MEM;
			opQueue[4] = OP_WRITE_MEM;
			opQueue[5] = OP_WRITE_MEM;
			queueSize = 6;
			// I_INC(addressAbsoluteX(regPC)); 
			break;
		case 0xC6:
			mathOP = OP_ALU_DEC;
			addressHighLatch = 0x0000;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_READ_MEM;
			opQueue[2] = OP_WRITE_MEM;
			opQueue[3] = OP_WRITE_MEM;
			queueSize = 4;

			isZeroPage = true;
			// I_DEC(addressZeroPage(regPC)); 
			break;
		case 0xD6:
			mathOP = OP_ALU_DEC;
			addressHighLatch = 0x0000;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_ADD_X_LOW;
			opQueue[2] = OP_READ_MEM;
			opQueue[3] = OP_WRITE_MEM;
			opQueue[4] = OP_WRITE_MEM;
			queueSize = 5;

			isZeroPage = true;
			// I_DEC(addressZeroPageX(regPC)); 
			break;
		case 0xCE:
			mathOP = OP_ALU_DEC;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_FETCH_HIGH_BYTE;
			opQueue[2] = OP_READ_MEM;
			opQueue[3] = OP_WRITE_MEM;
			opQueue[4] = OP_WRITE_MEM;
			queueSize = 5;
			// I_DEC(addressAbsolute(regPC)); 
			break;
		case 0xDE:
			mathOP = OP_ALU_DEC;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_FETCH_HIGH_BYTE;
			opQueue[2] = OP_ADD_X_LOW;
			opQueue[3] = OP_READ_MEM;
			opQueue[4] = OP_WRITE_MEM;
			opQueue[5] = OP_WRITE_MEM;
			queueSize = 6;
			// I_DEC(addressAbsoluteX(regPC)); 
			break;

			// STACK
		case 0x48:
			connectedWire = &regA;
			opQueue[0] = OP_DUMMY_READ;
			opQueue[1] = OP_PUSH_DATA;
			queueSize = 2;
			// I_PHA(); 
			break;
		case 0x68:
			connectedWire = &regA;
			opQueue[0] = OP_DUMMY_READ;
			opQueue[1] = OP_DUMMY_STACK_READ;
			opQueue[2] = OP_PULL_DATA;
			queueSize = 3;
			// I_PLA(); 
			break;
		case 0x08:
			connectedWire = &regP;
			opQueue[0] = OP_DUMMY_READ;
			opQueue[1] = OP_PUSH_DATA;
			queueSize = 2;
			// I_PHP(); 
			break;
		case 0x28:
			connectedWire = &regP;
			opQueue[0] = OP_DUMMY_READ;
			opQueue[1] = OP_DUMMY_STACK_READ;
			opQueue[2] = OP_PULL_DATA;
			queueSize = 3;
			// I_PLP(); 
			break;

			// Compare Accumulator
		case 0xC9:
			mathOP = OP_ALU_CMP;
			opQueue[0] = OP_FETCH_IMMEDIATE;
			queueSize = 1;
			// I_CMP(addressImmediate(regPC)); 
			break;
		case 0xC5:
			mathOP = OP_ALU_CMP;
			addressHighLatch = 0x0000;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_READ_MEM;
			queueSize = 2;

			isZeroPage = true;
			// I_CMP(addressZeroPage(regPC)); 
			break;
		case 0xD5:
			mathOP = OP_ALU_CMP;
			addressHighLatch = 0x0000;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_ADD_X_LOW;
			opQueue[2] = OP_READ_MEM;
			queueSize = 3;

			isZeroPage = true;
			// I_CMP(addressZeroPageX(regPC)); 
			break;
		case 0xCD:
			mathOP = OP_ALU_CMP;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_FETCH_HIGH_BYTE;
			opQueue[2] = OP_READ_MEM;
			queueSize = 3;
			// I_CMP(addressAbsolute(regPC)); 
			break;
		case 0xDD:
			mathOP = OP_ALU_CMP;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_FETCH_HIGH_BYTE;
			opQueue[2] = OP_ADD_X_LOW;
			opQueue[3] = OP_READ_MEM;
			queueSize = 4;
			// I_CMP(addressAbsoluteX(regPC)); 
			break;
		case 0xD9:
			mathOP = OP_ALU_CMP;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_FETCH_HIGH_BYTE;
			opQueue[2] = OP_ADD_Y_LOW;
			opQueue[3] = OP_READ_MEM;
			queueSize = 4;
			// I_CMP(addressAbsoluteY(regPC)); 
			break;
		case 0xC1:
			mathOP = OP_ALU_CMP;
			addressHighLatch = 0x0000;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_ADD_X_LOW;
			opQueue[2] = OP_POINTER_READ_LOW;
			opQueue[3] = OP_POINTER_READ_HIGH;
			opQueue[4] = OP_READ_MEM;
			queueSize = 5;

			isZeroPage = true;
			// I_CMP(addressIndirectX(regPC)); 
			break;
		case 0xD1:
			mathOP = OP_ALU_CMP;
			addressHighLatch = 0x0000;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_POINTER_READ_LOW;
			opQueue[2] = OP_POINTER_READ_HIGH;
			opQueue[3] = OP_ADD_Y_LOW;
			opQueue[4] = OP_READ_MEM;
			queueSize = 5;
			// I_CMP(addressIndirectY(regPC)); 
			break;

			// Compare X 
		case 0xE0:
			mathOP = OP_ALU_CPX;
			opQueue[0] = OP_FETCH_IMMEDIATE;
			queueSize = 1;
			// I_CPX(addressImmediate(regPC)); 
			break;
		case 0xE4:
			mathOP = OP_ALU_CPX;
			addressHighLatch = 0x0000;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_READ_MEM;
			queueSize = 2;

			isZeroPage = true;
			// I_CPX(addressZeroPage(regPC)); 
			break;
		case 0xEC:
			mathOP = OP_ALU_CPX;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_FETCH_HIGH_BYTE;
			opQueue[2] = OP_READ_MEM;
			queueSize = 3;
			// I_CPX(addressAbsolute(regPC)); 
			break;

			// Compare Y 
		case 0xC0:
			mathOP = OP_ALU_CPY;
			opQueue[0] = OP_FETCH_IMMEDIATE;
			queueSize = 1;
			// I_CPY(addressImmediate(regPC)); 
			break;
		case 0xC4:
			mathOP = OP_ALU_CPY;
			addressHighLatch = 0x0000;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_READ_MEM;
			queueSize = 2;

			isZeroPage = true;
			// I_CPY(addressZeroPage(regPC)); 
			break;
		case 0xCC:
			mathOP = OP_ALU_CPY;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_FETCH_HIGH_BYTE;
			opQueue[2] = OP_READ_MEM;
			queueSize = 3;
			// I_CPY(addressAbsolute(regPC)); 
			break;

			// SUBROUTINES 
		case 0x20:
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_DUMMY_STACK_READ;
			opQueue[2] = OP_PUSH_DATA; // Push PC High
			opQueue[3] = OP_PUSH_DATA; // Push PC Low
			opQueue[4] = OP_FETCH_HIGH_BYTE;
			queueSize = 5;
			// I_JSR(addressAbsolute(regPC)); 
			break;
		case 0x60:
			opQueue[0] = OP_DUMMY_READ;
			opQueue[1] = OP_DUMMY_STACK_READ;
			opQueue[2] = OP_PULL_DATA; // Pull PC Low
			opQueue[3] = OP_PULL_DATA; // Pull PC High
			opQueue[4] = OP_JUMP_CALC; // Increment PC
			queueSize = 5;
			// I_RTS(); 
			break;

			// FLAG
		case 0x38:
		case 0xF8:
		case 0x78:
			opQueue[0] = OP_SET_FLAG;
			queueSize = 1;
			// I_SEC(); I_SED(); I_SEI(); 
			break;

		case 0x18:
		case 0xD8:
		case 0x58:
		case 0xB8:
			opQueue[0] = OP_CLEAR_FLAG;
			queueSize = 1;
			// I_CLC(); I_CLD(); I_CLI(); I_CLV(); 
			break;

			// copies
		case 0xAA:
		case 0xA8:
		case 0x8A:
		case 0x98:
		case 0xBA:
		case 0x9A:
			opQueue[0] = OP_TRANSFER_REG;
			queueSize = 1;
			// I_TAX(); I_TAY(); I_TXA(); I_TYA(); I_TSX(); I_TXS(); 
			break;



			// bit
		case 0x24:
			mathOP = OP_ALU_BIT; 
			addressHighLatch = 0x0000;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_READ_MEM;
			queueSize = 2;

			isZeroPage = true;
			// I_BIT(addressZeroPage(regPC)); 
			break;
		case 0x2C:
			mathOP = OP_ALU_BIT; 
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_FETCH_HIGH_BYTE;
			opQueue[2] = OP_READ_MEM;
			queueSize = 3;
			// I_BIT(addressAbsolute(regPC)); 
			break;

			// JUMPS
		case 0x4C:
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_FETCH_HIGH_BYTE;
			queueSize = 2;
			// I_JMP(addressAbsolute(regPC)); 
			break;
		case 0x6C:
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_FETCH_HIGH_BYTE;
			opQueue[2] = OP_POINTER_READ_LOW;
			opQueue[3] = OP_POINTER_READ_HIGH;
			queueSize = 4;
			// I_JMP(addressAbsoluteIndirect(regPC)); 
			break;

		case 0x00:
			// BRK (7 cycles)
			regPC++;

			opQueue[0] = OP_DUMMY_READ;
			opQueue[1] = OP_PUSH_DATA; 
			opQueue[2] = OP_PUSH_DATA; 
			opQueue[3] = OP_PUSH_DATA; 
			opQueue[4] = OP_FETCH_IRQ_LOW;  
			opQueue[5] = OP_FETCH_IRQ_HIGH; 

			queueSize = 6; 
			// I_BREAK(); 
			break;

		case 0x40:
			// RTI (6 cycles)
			opQueue[0] = OP_DUMMY_READ;
			opQueue[1] = OP_DUMMY_STACK_READ;
			opQueue[2] = OP_PULL_DATA; 
			opQueue[3] = OP_PULL_DATA; 
			opQueue[4] = OP_PULL_DATA; 
			queueSize = 5;

			// I_RTI(); 
			break;


			//meme codes



		/// NOOP


		case 0xEA: case 0x1A: case 0x3A: case 0x5A: case 0x7A: case 0xDA: case 0xFA:
			opQueue[0] = OP_DUMMY_READ;
			queueSize = 1;
			break;

			
		case 0x80: case 0x82: case 0x89: case 0xC2: case 0xE2:
			opQueue[0] = OP_FETCH_IMMEDIATE;
			queueSize = 1;
			break;


		case 0x04: case 0x44: case 0x64:
			addressHighLatch = 0x0000;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_READ_MEM;
			queueSize = 2;
			isZeroPage = true;
			break;


		case 0x14: case 0x34: case 0x54: case 0x74: case 0xD4: case 0xF4:
			addressHighLatch = 0x0000;
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_ADD_X_LOW;
			opQueue[2] = OP_READ_MEM;
			queueSize = 3;
			isZeroPage = true;
			break;


		case 0x0C:
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_FETCH_HIGH_BYTE;
			opQueue[2] = OP_READ_MEM;
			queueSize = 3;
			break;


		case 0x1C: case 0x3C: case 0x5C: case 0x7C: case 0xDC: case 0xFC:
			opQueue[0] = OP_FETCH_LOW_BYTE;
			opQueue[1] = OP_FETCH_HIGH_BYTE;
			opQueue[2] = OP_ADD_X_LOW;
			opQueue[3] = OP_READ_MEM;
			queueSize = 4;
			break;

		case 0x1F: // SLO Absolute, X
			mathOP = OP_ALU_SLO;
			opQueue[0] = OP_FETCH_LOW_BYTE;  
			opQueue[1] = OP_FETCH_HIGH_BYTE; 
			opQueue[2] = OP_ADD_X_LOW;        
			opQueue[3] = OP_READ_MEM;         
			opQueue[4] = OP_WRITE_MEM;        
			opQueue[5] = OP_WRITE_MEM;       
			queueSize = 6;
			break;

		default: 

			std::cout << "OPCODE ERROR MEME NUMBER: " << opcode << std::endl;
			opQueue[0] = OP_DUMMY_READ;
			queueSize = 1;
			break;

	}

	opQueue[queueSize] = OP_FETCH_OPCODE;
	queueSize++;

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


void NES::irq() {

	push((regPC >> 8) & 0xFF);
	push(regPC & 0xFF);

	
	push((regP & ~0b0001'0000) | 0b0010'0000);

	// Interrupt Disable flag
	regP |= 0b0000'0100;


	uint8_t lByte = read(0xFFFE);
	uint8_t hByte = read(0xFFFF);

	regPC = lByte + (hByte << 8);
}



//mappers
Mapper000::Mapper000(uint8_t prgBanks, EMirrorMode _mirroringMode){

	prgChunks = prgBanks;
	mirroringMode = _mirroringMode;
	
}

Mapper001::Mapper001(uint8_t prgChunks, uint8_t chrChunks) {
	prgBanks = prgChunks;
	chrBanks = chrChunks;

	prgBankOffset[0] = 0;
	prgBankOffset[1] = (prgBanks - 1) * 0x4000;
	controlRegister = 0x0C;
}

void Mapper001::updateOffsets() {
	
	uint8_t prgMode = (controlRegister >> 2) & 0x03;

	
	uint8_t targetBank = prgBank & 0x0F;


	uint8_t fixedBankFirst = 0;
	uint8_t fixedBankLast = prgBanks - 1;


	if (prgBanks == 32) {
		
		uint8_t suromBit = chrBank0 & 0x10;

		targetBank |= suromBit;
		fixedBankFirst |= suromBit;
		fixedBankLast = 0x0F | suromBit;
	}


	switch (prgMode) {
	case 0:
	case 1:
		targetBank &= 0xFE;
		prgBankOffset[0] = targetBank * 0x4000;
		prgBankOffset[1] = (targetBank + 1) * 0x4000;
		break;
	case 2:
		prgBankOffset[0] = fixedBankFirst * 0x4000;
		prgBankOffset[1] = targetBank * 0x4000;
		break;
	case 3:
		prgBankOffset[0] = targetBank * 0x4000;
		prgBankOffset[1] = fixedBankLast * 0x4000;
		break;
	}

	uint8_t chrMode = (controlRegister >> 4) & 0x01;

	if (chrMode == 0) {
		
		uint8_t bank = chrBank0 & 0xFE;
		chrBankOffset[0] = bank * 0x1000;
		chrBankOffset[1] = (bank + 1) * 0x1000;
	}
	else {

		chrBankOffset[0] = chrBank0 * 0x1000;
		chrBankOffset[1] = chrBank1 * 0x1000;
	}

	targetBank %= prgBanks;

	if (chrBanks > 0) {
		uint8_t safeBank0 = chrBank0 % (chrBanks * 2); 
		uint8_t safeBank1 = chrBank1 % (chrBanks * 2);

		if (chrMode == 0) {
			uint8_t bank = safeBank0 & 0xFE;
			chrBankOffset[0] = bank * 0x1000;
			chrBankOffset[1] = (bank + 1) * 0x1000;
		}
		else {
			chrBankOffset[0] = safeBank0 * 0x1000;
			chrBankOffset[1] = safeBank1 * 0x1000;
		}
	}
	
}

bool Mapper001::cpuMapWrite(uint16_t address, uint32_t& mappedAddress, uint8_t data) {
	if (address >= 0x8000) {

		
		if (data & 0x80) {
			shiftRegister = 0x10;
			controlRegister |= 0x0C; 
			updateOffsets();
			return true;
		}

		
		uint8_t bit = data & 0x01;

		
		bool isFull = (shiftRegister & 0x01) > 0;

		shiftRegister >>= 1;
		shiftRegister |= (bit << 4); 

		
		if (isFull) {

			
			uint8_t targetRegister = (address >> 13) & 0x03;



			switch (targetRegister) {
			case 0: // $8000 - $9FFF (Control)
				controlRegister = shiftRegister;


			
				switch (controlRegister & 0x03) {
				case 0: mirroringMode = MONESCREENLO; break;
				case 1: mirroringMode = MONESCREENHI; break;
				case 2: mirroringMode = MVERTICAL; break;
				case 3: mirroringMode = MHORIZONTAL; break;
				}
			
				break;
			case 1: // $A000 - $BFFF (CHR Bank 0)
				chrBank0 = shiftRegister;
				break;
			case 2: // $C000 - $DFFF (CHR Bank 1)
				chrBank1 = shiftRegister;
				break;
			case 3: // $E000 - $FFFF (PRG Bank)
				prgBank = shiftRegister;
				break;
			}

			
			updateOffsets();

			
			shiftRegister = 0x10;
		}

		return true;
	}

	
	if (address >= 0x6000 && address <= 0x7FFF) {
		mappedAddress = address & 0x1FFF;
		return true;
	}

	return false;
}

bool Mapper001::cpuMapRead(uint16_t address, uint32_t& mappedAddress) {

	
	if (address >= 0x8000 && address <= 0xBFFF) {
		mappedAddress = prgBankOffset[0] + (address & 0x3FFF);
		return true;
	}

	
	if (address >= 0xC000 && address <= 0xFFFF) {
		mappedAddress = prgBankOffset[1] + (address & 0x3FFF);
		return true;
	}


	if (address >= 0x6000 && address <= 0x7FFF) {
		mappedAddress = address & 0x1FFF;
		return true;
	}



	return false;
}

bool Mapper001::ppuMapRead(uint16_t address, uint32_t& mappedAddress) {
	if (address < 0x2000) {
		
		if (chrBanks == 0) {
			mappedAddress = address;
			return true;
		}

		
		if (address <= 0x0FFF) {
			mappedAddress = chrBankOffset[0] + (address & 0x0FFF);
		}
		else {
			mappedAddress = chrBankOffset[1] + (address & 0x0FFF);
		}
		return true;
	}
	return false;
}

bool Mapper001::ppuMapWrite(uint16_t address, uint32_t& mappedAddress) {
	return ppuMapRead(address, mappedAddress);
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

bool Mapper000::ppuMapWrite(uint16_t address, uint32_t& mappedAddress) {
	if (address >= 0x0000 && address <= 0x1FFF) {
		if (prgChunks == 0) {
		
			mappedAddress = address;
			return true;
		}
	}
	return false; 
}

bool Mapper000::ppuMapRead(uint16_t address, uint32_t& mappedAddress)
{
	return false;
}
