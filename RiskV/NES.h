#pragma once
#include <cstdint>
#include "NESROM.h"
#include "NESLogger.h"
#include "PPU.h"
#include "APU.h"

#define INTERNALSTART 0x0000
#define INTERNALEND 0x07FF
#define INTERNALMIRROED 0x1FFF
#define PPUADDRESSROMEND 0x1FFF
#define PPUSTART 0x2000
#define PPUEND 0x2007
#define PPUENDMIRROED 0x3FFF
#define PPUADDRESSVRAMEND 0x3EFF
#define PPUADDRESSPALLETESTART 0x3F00
#define APUSTART 0x4000
#define APUEND 0x401F
#define ROMSTART 0x8000
#define ROMEND 0xFFFF

using std::uint16_t;
using std::uint8_t;

class Mapper000 {
public:
	uint8_t prgChunks;

	Mapper000(uint8_t prgBanks);


	bool cpuMapRead(uint16_t address, uint32_t& mappedAddress);
	bool cpuMapWrite(uint16_t address, uint32_t& mappedAddress, uint8_t data);
};


class NES {
public:
	uint8_t memory[2048] = {};
	uint8_t regA = 0x000;
	uint8_t regX = 0x000;
	uint8_t regY = 0x000;
	uint8_t regSP = 0x000;
	uint8_t regP = 0x000;
	uint16_t regPC = 0x000;

	//  A, B, Select, Start, Up, Down, Left, Right
	// 0x80 = A, 0x01 = Right

	uint8_t controller[2] = { 0, 0 };      //live
	uint8_t controllerState[2] = { 0, 0 }; 


	uint64_t currentCycles = 7;
	int cyclesRemaining = 0;

public:
	bool on = false;
	bool isPageCrossed = false;
	uint8_t extraCycles = 0;

	NESROM* currentRom = nullptr;
	Mapper000* mapper = nullptr;
	PPU* ppu = nullptr;
	APU* apu = nullptr;

public:
	NES();

	void loadRom(NESROM* rom);

	void unloadRom();

	uint8_t read(uint16_t address);
	void write(uint16_t address, uint8_t data);

	void updateFlags(uint8_t value);
	
	bool hasPageCrossed(uint16_t addr1, uint16_t addr2);

	uint16_t addressImmediate(uint16_t address);

	uint16_t addressZeroPage(uint16_t address);

	uint16_t addressZeroPageX(uint16_t address);

	uint16_t addressZeroPageY(uint16_t address);

	uint16_t addressAbsolute(uint16_t address);

	uint16_t addressAbsoluteX(uint16_t address);

	uint16_t addressAbsoluteY(uint16_t address);

	uint16_t addressAbsoluteIndirect(uint16_t address);

	uint16_t addressIndirectX(uint16_t address);

	uint16_t addressIndirectY(uint16_t address);

	uint16_t addressRelative(uint16_t address);

	void push(uint8_t data);

	uint8_t pull();

	void compare(uint8_t regData, uint8_t data);

	void branch(uint8_t offset);

	uint8_t I_ASL(uint8_t data);

	uint8_t I_LSR(uint8_t data);

	uint8_t I_ROL(uint8_t data);

	uint8_t I_ROR(uint8_t data);



	void I_LDA(uint16_t address);

	void I_LDX(uint16_t address);

	void I_LDY(uint16_t address);

	void I_STA(uint16_t address);

	void I_STX(uint16_t address);

	void I_STY(uint16_t address);

	void I_ADC(uint16_t address);

	void I_SBC(uint16_t address);

	void I_BEQ(uint16_t address);

	void I_BNE(uint16_t address);

	void I_BCC(uint16_t address);

	void I_BCS(uint16_t address);

	void I_BVC(uint16_t address);

	void I_BVS(uint16_t address);

	void I_BPL(uint16_t address);

	void I_BMI(uint16_t address);

	void I_INX();

	void I_DEX();

	void I_INY();

	void I_DEY();

	void I_INC(uint16_t address);

	void I_DEC(uint16_t address);

	void I_PHA();

	void I_PLA();

	void I_PHP();

	void I_PLP();

	void I_CMP(uint16_t address);

	void I_CPX(uint16_t address);

	void I_CPY(uint16_t address);

	void I_JSR(uint16_t address);

	void I_RTS();

	void I_SEC();

	void I_SED();

	void I_SEI();

	void I_CLC();

	void I_CLD();

	void I_CLI();

	void I_CLV();

	void I_TAX();

	void I_TAY();

	void I_TXA();

	void I_TYA();

	void I_TSX();

	void I_TXS();

	void I_BIT(uint16_t address);

	void I_BREAK();

	void I_RTI();

	void I_AND(uint16_t address);

	void I_ORA(uint16_t address);

	void I_EOR(uint16_t address);

	void I_JMP(uint16_t address);

	void reset();

	uint8_t step(NESLogger* logger);

	void nmi();



};

