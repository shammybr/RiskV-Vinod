#pragma once
#include <string>
#include <cstdint>

class NES;

enum AddrMode {
    IMP, IMM, ZP, ZPX, ZPY, REL, ABS, ABSX, ABSY, IND, INDX, INDY
};

struct InstructionInfo {
    const char* name;
    AddrMode mode;
    uint8_t bytes;
    uint8_t cycles;
};

class NESLogger {
public:
    InstructionInfo opTable[256];

    NESLogger();


    std::string getLogStep(NES* nes);
};