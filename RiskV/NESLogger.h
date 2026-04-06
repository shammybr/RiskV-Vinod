#pragma once
#include <string>
#include <cstdint>
#include <fstream>
#include <vector>
#include "lib/json/json.hpp" 

using json = nlohmann::json;

class NES;


struct EmuStep {
	uint16_t pc;
	uint8_t opcode;
	uint8_t regA;
	uint8_t regX;
	uint8_t regY;
	uint8_t regP;
	uint8_t regSP;
	uint64_t totalCpuCycles;
	uint64_t totalPpuCycles;
	uint64_t totalApuCycles;
	int ppuScanline;
	int ppuCycle;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(EmuStep, pc, opcode, regA, regX, regY, regP, regSP, totalCpuCycles, totalPpuCycles, totalApuCycles, ppuScanline, ppuCycle);


enum AddrMode {
    IMP, IMM, ZP, ZPX, ZPY, REL, ABS, ABSX, ABSY, IND, INDX, INDY
};

struct InstructionInfo {
    const char* name;
    AddrMode mode;
    uint8_t bytes;
    uint8_t cycles;
    uint8_t extraCycles;
};

class NESLogger {
public:
    InstructionInfo opTable[256];

    NESLogger();

	void openJson();


    std::string getLogStep(NES* nes);
	void flush();
	int jsonlogStep(NES* nes, uint64_t totalCpuCycles, uint64_t totalPpuCycles, uint64_t totalApuCycles, bool last);

	std::vector<EmuStep> jsonLogBuffer;
	std::ofstream jsonFile;
};