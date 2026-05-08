#pragma once
#include <cstdint>
#include "NESROM.h"
#include "NESLogger.h"
#include "PPU.h"
#include "APU.h"
#include <queue>

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

enum MicroOp {
	OP_NONE,
	OP_FETCH_OPCODE,
	OP_FETCH_LOW_BYTE,
	OP_FETCH_HIGH_BYTE,
	OP_FETCH_IMMEDIATE,

	OP_FETCH_NMI_LOW,
	OP_FETCH_NMI_HIGH,
	OP_FETCH_IRQ_LOW,
	OP_FETCH_IRQ_HIGH,
	OP_READ_MEM,
	OP_WRITE_MEM,
	OP_DUMMY_READ,

	OP_ALU_ADC,
	OP_ALU_AND, 
	OP_ALU_ORA, 
	OP_ALU_EOR,

	OP_ALU_CMP,
	OP_ALU_CPX,
	OP_ALU_CPY,
	
	OP_ALU_SBC,
	OP_ALU_ASL,
	OP_ALU_LSR,
	OP_ALU_ROL,
	OP_ALU_ROR,

	OP_ALU_INC,
	OP_ALU_DEC,
	OP_ALU_SLO,
	OP_ALU_BIT,

	OP_ADD_X_LOW, 
	OP_ADD_Y_LOW,

	OP_POINTER_READ_LOW,
	OP_POINTER_READ_HIGH,

	OP_PUSH_DATA,
	OP_PULL_DATA,
	OP_DUMMY_STACK_READ,

	OP_BRANCH_CHECK,
	OP_BRANCH_UPDATE_PC,
	OP_JUMP_CALC,

	OP_TRANSFER_REG,

	OP_INTERNAL_INC_DEC,

	OP_SET_FLAG,
	OP_CLEAR_FLAG

};



class Mapper {
public:
	uint8_t prgChunks;

	Mapper() {};

	EMirrorMode mirroringMode;

	virtual bool cpuMapRead(uint16_t address, uint32_t& mappedAddress) { return 0; };
	virtual bool cpuMapWrite(uint16_t address, uint32_t& mappedAddress, uint8_t data) { return 0; };
	virtual bool ppuMapRead(uint16_t address, uint32_t& mappedAddress) { return 0; };
	virtual bool ppuMapWrite(uint16_t address, uint32_t& mappedAddress) { return 0; };

};


class Mapper001 : public Mapper {
private:
	uint8_t prgBanks = 0;
	uint8_t chrBanks = 0;

	uint8_t shiftRegister = 0x10;

	//  MMC1 Registers
	uint8_t controlRegister = 0x0C; 
	uint8_t chrBank0 = 0x00;
	uint8_t chrBank1 = 0x00;
	uint8_t prgBank = 0x00;


	uint32_t prgBankOffset[2];
	uint32_t chrBankOffset[2];



	void updateOffsets();

public:
	Mapper001(uint8_t prgChunks, uint8_t chrChunks);

	bool cpuMapRead(uint16_t address, uint32_t& mappedAddress) override;
	bool ppuMapRead(uint16_t address, uint32_t& mappedAddress) override;
	bool ppuMapWrite(uint16_t address, uint32_t& mappedAddress) override;
	bool cpuMapWrite(uint16_t address, uint32_t& mappedAddress, uint8_t data) override;

	// You will also need ppuMapRead and ppuMapWrite later for the graphics!
};

class Mapper000: public Mapper {
public:
	uint8_t prgChunks;

	Mapper000(uint8_t prgBanks, EMirrorMode _mirroringMode);


	bool cpuMapRead(uint16_t address, uint32_t& mappedAddress) override;
	bool cpuMapWrite(uint16_t address, uint32_t& mappedAddress, uint8_t data) override;
	bool ppuMapWrite(uint16_t address, uint32_t& mappedAddress) override;
	bool ppuMapRead(uint16_t address, uint32_t& mappedAddress) override;
};


class NES {
public:
	uint8_t memory[2048] = {};
	uint32_t saveRam[8192]{};

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

	uint8_t cpuDataBus = 0;

	uint64_t currentCycles = 7;
	int cyclesRemaining = 0;

	bool wannaSave = false;
	uint8_t saveTime = 0;

public:
	bool on = false;
	bool isPageCrossed = false;
	bool irqLatched = false;
	bool nmiLatched = false;
	bool isWritingMemory = false;

	int extraCycles = 0;

	NESROM* currentRom = nullptr;
	Mapper* mapper = nullptr;
	PPU* ppu = nullptr;
	APU* apu = nullptr;

	uint8_t dmaPage;
	uint8_t dmaAddress = 0;
	uint8_t dmaData = 0;
	uint8_t oamDmaState = 0;
	uint8_t dpcmHaltCycles = 0;
	bool dpcmActive = false;
	bool dmaWaiting = false;
	bool LOGGO = false;

	bool traceCPU = false;
	uint64_t instructionStartCycle = 0;
	uint16_t instructionPC = 0;
	uint16_t addressBus = 0;
	uint16_t oldAddressBus = 0;
	uint8_t currentOpcodeLog = 0;
public:
	NES();

	void loadRom(NESROM* rom);

	void unloadRom();

	uint8_t read(uint16_t address);
	void write(uint16_t address, uint8_t data);

	void saveGame();

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

	uint8_t logRead(uint16_t address);

	int step(NESLogger* logger, bool isCISC);

	int CISCStep(NESLogger* logger);


	void nmi();

	void irq();


//////// RISC //////////////
public:

	uint8_t  iReg; // opcode
	uint16_t addressLatch;    
	uint16_t addressHighLatch;
	uint8_t  dataLatch;           
	uint8_t* connectedWire;
	
	MicroOp mathOP;
	MicroOp opQueue[20];
	const char* opStrings[42] = {
		"NO OP",
		"FETCH OPCODE",
		"FETCH LOW_BYTE",
		"FETCH HIGH_BYTE",
		"FETCH IMMEDIATE",
		"FETCH NMI LOW",
		"FETCH NMI HIGH",
		"FETCH IRQ LOW",
		"FETCH IRQ HIGH",
		"READ MEM",
		"WRITE MEM",
		"OP DUMMY READ",

		"ALU ADC",
		"ALU AND",
		"ALU ORA",
		"ALU EOR",

		"ALU CMP",
		"ALU CPX",
		"ALU CPY",

		"ALU SBC",
		"ALU ASL",
		"ALU LSR",
		"ALU ROL",
		"ALU ROR",

		"ALU INC",
		"ALU DEC",
		"ALU SLO",
		"ALU BIT",

		"ADD X LOW",
		"ADD Y LOW",

		"POINTER READ LOW",
		"POINTER READ HIGH",

		"PUSH DATA",
		"PULL DATA",
		"DUMMY STACK READ",

		"BRANCH CHECK",
		"BRANCH UPDATE PC",
		"JUMP CALC",

		"TRANSFER REG",

		"INTERNAL INC_DEC",

		"SET FLAG",
		"CLEAR FLAG"
	};
	static constexpr char HEX_CHARS[] = "0123456789ABCDEF";

	char history[20][50] = { 0 };

	int historyN = 0;
	bool frameMode = false;
	bool logMicroOps = false;
	bool canStep = true;

	int queueSize = 0;
	int queueIndex = 0;
	bool isZeroPage = false;
	bool isBranchInstruction = false;
	bool isStrobeActive = false;


	int RISCStep(NESLogger* logger);
	void executeALU(MicroOp mathOP);
	void iDecode(uint8_t opcode);

};

