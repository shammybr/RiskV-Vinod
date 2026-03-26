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

//shift 1 bit left
uint8_t NES::asl(uint8_t data) {

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
uint8_t NES::lsr(uint8_t data) {

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
uint8_t NES::rol(uint8_t data) {
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
uint8_t NES::ror(uint8_t data) {
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


void NES::branch(uint8_t offset) {
	int8_t signedOffset = static_cast<int8_t>(offset);

	regPC = regPC + signedOffset;


}


void NES::I_LDA(uint16_t address) {

	regA = read(address);

	updateFlags(regA);
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
void NES::IBEQ(uint16_t address) {
	uint8_t offset = read(address);

					//zero flag
	if (offset & 0b0000'0010) {
		branch(offset);
	}

}

//BRANCH IF NOTTO EQUOLLLSL 0
void NES::IBNE(uint16_t address) {
	uint8_t offset = read(address);
	         

			
	if (!(offset & ~0b0000'0010)) {
		branch(offset);
	}

}

//BRANCH IF CARRY CLEAR
void NES::IBCC(uint16_t address) {
	uint8_t offset = read(address);



	if (!(offset & ~0b0000'0001)) {
		branch(offset);
	}

}

//BRANCH IF CARRY SET
void NES::IBCS(uint16_t address) {
	uint8_t offset = read(address);



	if ((offset & ~0b0000'0001)) {
		branch(offset);
	}

}

//BRANCH IF OVERFLOW CLEAR
void NES::IBVC(uint16_t address) {
	uint8_t offset = read(address);



	if (!(offset & ~0b0100'0000)) {
		branch(offset);
	}

}

//BRANCH IF OVERFLOW SET
void NES::IBVS(uint16_t address) {
	uint8_t offset = read(address);



	if ((offset & ~0b0100'0000)) {
		branch(offset);
	}

}

//BRANCH IF PLUS
void NES::IBPL(uint16_t address) {
	uint8_t offset = read(address);



	if (!(offset & ~0b1000'0000)) {
		branch(offset);
	}

}

//BRANCH IF MINUS TECH TIPS
void NES::IBMI(uint16_t address) {
	uint8_t offset = read(address);



	if ((offset & ~0b1000'0000)) {
		branch(offset);
	}

}



void NES::step() {

	uint8_t opcode = read(regPC++);


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

		//ATTACK DAMAGE CARRY
		case 0x69: I_ADC(addressImmediate(regPC)); break;
		case 0x65: I_ADC(addressZeroPage(regPC)); break;

		// subtract with carry
		case 0xE9: I_SBC(addressImmediate(regPC)); break;
		case 0xE5: I_SBC(addressZeroPage(regPC)); break;

		//logic
		case 0x29: I_AND(addressImmediate(regPC)); break;
		case 0x25: I_AND(addressZeroPage(regPC)); break;

		case 0x09: I_ORA(addressImmediate(regPC)); break;
		case 0x05: I_ORA(addressZeroPage(regPC)); break;

		case 0x49: I_EOR(addressImmediate(regPC)); break;
		case 0x45: I_EOR(addressZeroPage(regPC)); break;

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
		case 0x4A: regA = lsr(regA); break;

		// LSR Zero Page
		case 0x46: {
			uint16_t addr = addressZeroPage(regPC);
			write(addr, lsr(read(addr)));
			break;
		}

		// ROL
		case 0x2A: regA = rol(regA); break; 

		case 0x26: {
			uint16_t addr = addressZeroPage(regPC);
			write(addr, rol(read(addr)));
			break;
		}

		// ROR
		case 0x6A: regA = ror(regA); break; 

		case 0x66: {
			uint16_t addr = addressZeroPage(regPC);
			write(addr, ror(read(addr)));
			break;
		}

		// ASL Accumulator
		case 0x0A: regA = asl(regA); break;

		// ASL Zero Page
		case 0x06: {
			uint16_t addr = addressZeroPage(regPC);
			uint8_t data = read(addr);
			write(addr, asl(data));
			break;
		}

		default: std::cout << "OPCODE ERROR MEME NUMBER: " << opcode << std::endl;	break;
		
	}




}