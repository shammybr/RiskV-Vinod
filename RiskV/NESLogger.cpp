#include "NESLogger.h"
#include "NES.h"
#include <cstdio>

NESLogger::NESLogger() {
    // 1. Fill everything with a safe default
    for (int i = 0; i < 256; i++) {
        opTable[i] = { "???", IMP, 1 };
    }

    // 2. THE 151 OFFICIAL OPCODES

// --- ADC: Add with Carry ---
    opTable[0x69] = { "ADC", IMM,  2, 2, 0 };
    opTable[0x65] = { "ADC", ZP,   2, 3, 0 };
    opTable[0x75] = { "ADC", ZPX,  2, 4, 0 };
    opTable[0x6D] = { "ADC", ABS,  3, 4, 0 };
    opTable[0x7D] = { "ADC", ABSX, 3, 4, 1 }; // +1
    opTable[0x79] = { "ADC", ABSY, 3, 4, 1 }; // +1
    opTable[0x61] = { "ADC", INDX, 2, 6, 0 };
    opTable[0x71] = { "ADC", INDY, 2, 5, 1 }; // +1

    // --- AND: Logical AND ---
    opTable[0x29] = { "AND", IMM,  2, 2, 0 };
    opTable[0x25] = { "AND", ZP,   2, 3, 0 };
    opTable[0x35] = { "AND", ZPX,  2, 4, 0 };
    opTable[0x2D] = { "AND", ABS,  3, 4, 0 };
    opTable[0x3D] = { "AND", ABSX, 3, 4, 1 }; // +1
    opTable[0x39] = { "AND", ABSY, 3, 4, 1 }; // +1
    opTable[0x21] = { "AND", INDX, 2, 6, 0 };
    opTable[0x31] = { "AND", INDY, 2, 5, 1 }; // +1

    // --- ASL: Arithmetic Shift Left ---
    opTable[0x0A] = { "ASL", IMP,  1, 2, 0 };
    opTable[0x06] = { "ASL", ZP,   2, 5, 0 };
    opTable[0x16] = { "ASL", ZPX,  2, 6, 0 };
    opTable[0x0E] = { "ASL", ABS,  3, 6, 0 };
    opTable[0x1E] = { "ASL", ABSX, 3, 7, 0 }; // No bonus

    // --- BRANCH INSTRUCTIONS ---
    opTable[0x90] = { "BCC", REL,  2, 2, 1 };
    opTable[0xB0] = { "BCS", REL,  2, 2, 1 };
    opTable[0xF0] = { "BEQ", REL,  2, 2, 1 };
    opTable[0x30] = { "BMI", REL,  2, 2, 1 };
    opTable[0xD0] = { "BNE", REL,  2, 2, 1 };
    opTable[0x10] = { "BPL", REL,  2, 2, 1 };
    opTable[0x50] = { "BVC", REL,  2, 2, 1 };
    opTable[0x70] = { "BVS", REL,  2, 2, 1 };

    // --- BIT: Bit Test ---
    opTable[0x24] = { "BIT", ZP,   2, 3, 0 };
    opTable[0x2C] = { "BIT", ABS,  3, 4, 0 };

    // --- BRK: Force Interrupt ---
    opTable[0x00] = { "BRK", IMP,  1, 7, 0 };

    // --- CLEAR/SET FLAG INSTRUCTIONS ---
    opTable[0x18] = { "CLC", IMP,  1, 2, 0 };
    opTable[0xD8] = { "CLD", IMP,  1, 2, 0 };
    opTable[0x58] = { "CLI", IMP,  1, 2, 0 };
    opTable[0xB8] = { "CLV", IMP,  1, 2, 0 };
    opTable[0x38] = { "SEC", IMP,  1, 2, 0 };
    opTable[0xF8] = { "SED", IMP,  1, 2, 0 };
    opTable[0x78] = { "SEI", IMP,  1, 2, 0 };

    // --- CMP: Compare Accumulator ---
    opTable[0xC9] = { "CMP", IMM,  2, 2, 0 };
    opTable[0xC5] = { "CMP", ZP,   2, 3, 0 };
    opTable[0xD5] = { "CMP", ZPX,  2, 4, 0 };
    opTable[0xCD] = { "CMP", ABS,  3, 4, 0 };
    opTable[0xDD] = { "CMP", ABSX, 3, 4, 1 }; // +1
    opTable[0xD9] = { "CMP", ABSY, 3, 4, 1 }; // +1
    opTable[0xC1] = { "CMP", INDX, 2, 6, 0 };
    opTable[0xD1] = { "CMP", INDY, 2, 5, 1 }; // +1

    // --- CPX: Compare X Register ---
    opTable[0xE0] = { "CPX", IMM,  2, 2, 0 };
    opTable[0xE4] = { "CPX", ZP,   2, 3, 0 };
    opTable[0xEC] = { "CPX", ABS,  3, 4, 0 };

    // --- CPY: Compare Y Register ---
    opTable[0xC0] = { "CPY", IMM,  2, 2, 0 };
    opTable[0xC4] = { "CPY", ZP,   2, 3, 0 };
    opTable[0xCC] = { "CPY", ABS,  3, 4, 0 };

    // --- DEC: Decrement Memory ---
    opTable[0xC6] = { "DEC", ZP,   2, 5, 0 };
    opTable[0xD6] = { "DEC", ZPX,  2, 6, 0 };
    opTable[0xCE] = { "DEC", ABS,  3, 6, 0 };
    opTable[0xDE] = { "DEC", ABSX, 3, 7, 0 };

    // --- DEX/DEY: Decrement Registers ---
    opTable[0xCA] = { "DEX", IMP,  1, 2, 0 };
    opTable[0x88] = { "DEY", IMP,  1, 2, 0 };

    // --- EOR: Exclusive OR ---
    opTable[0x49] = { "EOR", IMM,  2, 2, 0 };
    opTable[0x45] = { "EOR", ZP,   2, 3, 0 };
    opTable[0x55] = { "EOR", ZPX,  2, 4, 0 };
    opTable[0x4D] = { "EOR", ABS,  3, 4, 0 };
    opTable[0x5D] = { "EOR", ABSX, 3, 4, 1 }; // +1
    opTable[0x59] = { "EOR", ABSY, 3, 4, 1 }; // +1
    opTable[0x41] = { "EOR", INDX, 2, 6, 0 };
    opTable[0x51] = { "EOR", INDY, 2, 5, 1 }; // +1

    // --- INC: Increment Memory ---
    opTable[0xE6] = { "INC", ZP,   2, 5, 0 };
    opTable[0xF6] = { "INC", ZPX,  2, 6, 0 };
    opTable[0xEE] = { "INC", ABS,  3, 6, 0 };
    opTable[0xFE] = { "INC", ABSX, 3, 7, 0 };

    // --- INX/INY: Increment Registers ---
    opTable[0xE8] = { "INX", IMP,  1, 2, 0 };
    opTable[0xC8] = { "INY", IMP,  1, 2, 0 };

    // --- JMP/JSR: Jumps and Calls ---
    opTable[0x4C] = { "JMP", ABS,  3, 3, 0 };
    opTable[0x6C] = { "JMP", IND,  3, 5, 0 };
    opTable[0x20] = { "JSR", ABS,  3, 6, 0 };

    // --- LDA: Load Accumulator ---
    opTable[0xA9] = { "LDA", IMM,  2, 2, 0 };
    opTable[0xA5] = { "LDA", ZP,   2, 3, 0 };
    opTable[0xB5] = { "LDA", ZPX,  2, 4, 0 };
    opTable[0xAD] = { "LDA", ABS,  3, 4, 0 };
    opTable[0xBD] = { "LDA", ABSX, 3, 4, 1 }; // +1
    opTable[0xB9] = { "LDA", ABSY, 3, 4, 1 }; // +1
    opTable[0xA1] = { "LDA", INDX, 2, 6, 0 };
    opTable[0xB1] = { "LDA", INDY, 2, 5, 1 }; // +1

    // --- LDX: Load X Register ---
    opTable[0xA2] = { "LDX", IMM,  2, 2, 0 };
    opTable[0xA6] = { "LDX", ZP,   2, 3, 0 };
    opTable[0xB6] = { "LDX", ZPY,  2, 4, 0 };
    opTable[0xAE] = { "LDX", ABS,  3, 4, 0 };
    opTable[0xBE] = { "LDX", ABSY, 3, 4, 1 }; // +1

    // --- LDY: Load Y Register ---
    opTable[0xA0] = { "LDY", IMM,  2, 2, 0 };
    opTable[0xA4] = { "LDY", ZP,   2, 3, 0 };
    opTable[0xB4] = { "LDY", ZPX,  2, 4, 0 };
    opTable[0xAC] = { "LDY", ABS,  3, 4, 0 };
    opTable[0xBC] = { "LDY", ABSX, 3, 4, 1 }; // +1

    // --- LSR: Logical Shift Right ---
    opTable[0x4A] = { "LSR", IMP,  1, 2, 0 };
    opTable[0x46] = { "LSR", ZP,   2, 5, 0 };
    opTable[0x56] = { "LSR", ZPX,  2, 6, 0 };
    opTable[0x4E] = { "LSR", ABS,  3, 6, 0 };
    opTable[0x5E] = { "LSR", ABSX, 3, 7, 0 };

    // --- NOP: No Operation ---
    opTable[0xEA] = { "NOP", IMP,  1, 2, 0 };

    // --- ORA: Logical Inclusive OR ---
    opTable[0x09] = { "ORA", IMM,  2, 2, 0 };
    opTable[0x05] = { "ORA", ZP,   2, 3, 0 };
    opTable[0x15] = { "ORA", ZPX,  2, 4, 0 };
    opTable[0x0D] = { "ORA", ABS,  3, 4, 0 };
    opTable[0x1D] = { "ORA", ABSX, 3, 4, 1 }; // +1
    opTable[0x19] = { "ORA", ABSY, 3, 4, 1 }; // +1
    opTable[0x01] = { "ORA", INDX, 2, 6, 0 };
    opTable[0x11] = { "ORA", INDY, 2, 5, 1 }; // +1

    // --- PUSH/PULL (Stack) ---
    opTable[0x48] = { "PHA", IMP,  1, 3, 0 };
    opTable[0x08] = { "PHP", IMP,  1, 3, 0 };
    opTable[0x68] = { "PLA", IMP,  1, 4, 0 };
    opTable[0x28] = { "PLP", IMP,  1, 4, 0 };

    // --- ROL: Rotate Left ---
    opTable[0x2A] = { "ROL", IMP,  1, 2, 0 };
    opTable[0x26] = { "ROL", ZP,   2, 5, 0 };
    opTable[0x36] = { "ROL", ZPX,  2, 6, 0 };
    opTable[0x2E] = { "ROL", ABS,  3, 6, 0 };
    opTable[0x3E] = { "ROL", ABSX, 3, 7, 0 };

    // --- ROR: Rotate Right ---
    opTable[0x6A] = { "ROR", IMP,  1, 2, 0 };
    opTable[0x66] = { "ROR", ZP,   2, 5, 0 };
    opTable[0x76] = { "ROR", ZPX,  2, 6, 0 };
    opTable[0x6E] = { "ROR", ABS,  3, 6, 0 };
    opTable[0x7E] = { "ROR", ABSX, 3, 7, 0 };

    // --- RTI/RTS: Returns ---
    opTable[0x40] = { "RTI", IMP,  1, 6, 0 };
    opTable[0x60] = { "RTS", IMP,  1, 6, 0 };

    // --- SBC: Subtract with Carry ---
    opTable[0xE9] = { "SBC", IMM,  2, 2, 0 };
    opTable[0xE5] = { "SBC", ZP,   2, 3, 0 };
    opTable[0xF5] = { "SBC", ZPX,  2, 4, 0 };
    opTable[0xED] = { "SBC", ABS,  3, 4, 0 };
    opTable[0xFD] = { "SBC", ABSX, 3, 4, 1 }; // +1
    opTable[0xF9] = { "SBC", ABSY, 3, 4, 1 }; // +1
    opTable[0xE1] = { "SBC", INDX, 2, 6, 0 };
    opTable[0xF1] = { "SBC", INDY, 2, 5, 1 }; // +1

    // --- STA: Store Accumulator ---
    opTable[0x85] = { "STA", ZP,   2, 3, 0 };
    opTable[0x95] = { "STA", ZPX,  2, 4, 0 };
    opTable[0x8D] = { "STA", ABS,  3, 4, 0 };
    opTable[0x9D] = { "STA", ABSX, 3, 5, 0 }; // Stores never get bonus
    opTable[0x99] = { "STA", ABSY, 3, 5, 0 }; // Stores never get bonus
    opTable[0x81] = { "STA", INDX, 2, 6, 0 };
    opTable[0x91] = { "STA", INDY, 2, 6, 0 }; // Stores never get bonus

    // --- STX: Store X Register ---
    opTable[0x86] = { "STX", ZP,   2, 3, 0 };
    opTable[0x96] = { "STX", ZPY,  2, 4, 0 };
    opTable[0x8E] = { "STX", ABS,  3, 4, 0 };

    // --- STY: Store Y Register ---
    opTable[0x84] = { "STY", ZP,   2, 3, 0 };
    opTable[0x94] = { "STY", ZPX,  2, 4, 0 };
    opTable[0x8C] = { "STY", ABS,  3, 4, 0 };

    // --- TRANSFER INSTRUCTIONS ---
    opTable[0xAA] = { "TAX", IMP,  1, 2, 0 };
    opTable[0xA8] = { "TAY", IMP,  1, 2, 0 };
    opTable[0xBA] = { "TSX", IMP,  1, 2, 0 };
    opTable[0x8A] = { "TXA", IMP,  1, 2, 0 };
    opTable[0x9A] = { "TXS", IMP,  1, 2, 0 };
    opTable[0x98] = { "TYA", IMP,  1, 2, 0 };



}

void NESLogger::openJson() {
    jsonFile.open("ROM/emulator_trace.json");
    jsonFile << "[\n";
}

std::string NESLogger::getLogStep(NES* nes) {
    uint16_t pc = nes->regPC;
    uint8_t opcode = nes->logRead(pc);
    InstructionInfo info = opTable[opcode];

    uint8_t b1 = 0, b2 = 0;
    char hexStr[15] = { 0 };
    char asmStr[40] = { 0 };

    // --- logRead BYTES ---
    if (info.bytes == 1) {
        snprintf(hexStr, sizeof(hexStr), "%02X      ", opcode);
    }
    else if (info.bytes == 2) {
        b1 = nes->logRead(pc + 1);
        snprintf(hexStr, sizeof(hexStr), "%02X %02X   ", opcode, b1);
    }
    else if (info.bytes == 3) {
        b1 = nes->logRead(pc + 1);
        b2 = nes->logRead(pc + 2);
        snprintf(hexStr, sizeof(hexStr), "%02X %02X %02X", opcode, b1, b2);
    }

    // --- FORMAT ASSEMBLY STRING ---
    uint16_t addr = 0;
    uint16_t ptr = 0; // Used for indirect addressing

    switch (info.mode) {
    case IMP:
        if (opcode == 0x0A || opcode == 0x4A || opcode == 0x2A || opcode == 0x6A) {
            snprintf(asmStr, sizeof(asmStr), "%s A", info.name);
        }
        else {
            snprintf(asmStr, sizeof(asmStr), "%s", info.name);
        }
        break;

    case IMM:
        snprintf(asmStr, sizeof(asmStr), "%s #$%02X", info.name, b1);
        break;

    case ZP:
        snprintf(asmStr, sizeof(asmStr), "%s $%02X = %02X", info.name, b1, nes->logRead(b1));
        break;

    case ZPX:
        addr = (b1 + nes->regX) & 0xFF;
        snprintf(asmStr, sizeof(asmStr), "%s $%02X,X @ %02X = %02X", info.name, b1, addr, nes->logRead(addr));
        break;

    case ZPY:
        addr = (b1 + nes->regY) & 0xFF;
        snprintf(asmStr, sizeof(asmStr), "%s $%02X,Y @ %02X = %02X", info.name, b1, addr, nes->logRead(addr));
        break;

    case REL:
        addr = pc + 2 + (int8_t)b1;
        snprintf(asmStr, sizeof(asmStr), "%s $%04X", info.name, addr);
        break;

    case ABS:
        addr = b1 | (b2 << 8);
        if (opcode == 0x4C || opcode == 0x20) { // JMP and JSR
            snprintf(asmStr, sizeof(asmStr), "%s $%04X", info.name, addr);
        }
        else {
            snprintf(asmStr, sizeof(asmStr), "%s $%04X = %02X", info.name, addr, nes->logRead(addr));
        }
        break;

    case ABSX:
        addr = (b1 | (b2 << 8)) + nes->regX;
        snprintf(asmStr, sizeof(asmStr), "%s $%04X,X @ %04X = %02X", info.name, b1 | (b2 << 8), addr, nes->logRead(addr));
        break;

    case ABSY:
        addr = (b1 | (b2 << 8)) + nes->regY;
        snprintf(asmStr, sizeof(asmStr), "%s $%04X,Y @ %04X = %02X", info.name, b1 | (b2 << 8), addr, nes->logRead(addr));
        break;

    case IND: // Only used by JMP Indirect
        ptr = b1 | (b2 << 8);
        // Replicate the famous 6502 page boundary hardware bug!
        if (b1 == 0xFF) {
            addr = nes->logRead(ptr) | (nes->logRead(ptr & 0xFF00) << 8);
        }
        else {
            addr = nes->logRead(ptr) | (nes->logRead(ptr + 1) << 8);
        }
        snprintf(asmStr, sizeof(asmStr), "%s ($%04X) = %04X", info.name, ptr, addr);
        break;

    case INDX:
        ptr = (b1 + nes->regX) & 0xFF;
        addr = nes->logRead(ptr) | (nes->logRead((ptr + 1) & 0xFF) << 8);
        snprintf(asmStr, sizeof(asmStr), "%s ($%02X,X) @ %02X = %04X = %02X", info.name, b1, ptr, addr, nes->logRead(addr));
        break;

    case INDY:
        ptr = b1;
        addr = (nes->logRead(ptr) | (nes->logRead((ptr + 1) & 0xFF) << 8)) + nes->regY;
        snprintf(asmStr, sizeof(asmStr), "%s ($%02X),Y = %04X @ %04X = %02X", info.name, b1, addr - nes->regY, addr, nes->logRead(addr));
        break;

    default:
        snprintf(asmStr, sizeof(asmStr), "???");
        break;
    }

    // --- PRINT TO FINAL BUFFER ---
    char finalLog[150] = { 0 };
    snprintf(finalLog, sizeof(finalLog),
        "%04X  %-8s  %-32s  A:%02X X:%02X Y:%02X P:%02X SP:%02X CYC:%llu\n",
        pc, hexStr, asmStr,
        (int)nes->regA, (int)nes->regX, (int)nes->regY, (int)nes->regP, (int)nes->regSP,
        nes->currentCycles);

    // Safely construct a C++ std::string and return it!
    return std::string(finalLog);
}

void NESLogger::flush() {
    if (jsonLogBuffer.empty()) return;

    for (size_t i = 0; i < jsonLogBuffer.size(); i++) {

        json j = jsonLogBuffer[i];


        static bool firstLog = true;
        if (!firstLog) jsonFile << ",\n";
        firstLog = false;

        jsonFile << j.dump();
    }

    jsonLogBuffer.clear(); 
}


int NESLogger::jsonlogStep(NES* nes, uint64_t totalCpuCycles, uint64_t totalPpuCycles, uint64_t totalApuCycles, bool last){
    EmuStep state;
    state.pc = nes->regPC;
    state.opcode = nes->logRead(nes->regPC); 
    state.regA = nes->regA;
    state.regX = nes->regX;
    state.regY = nes->regY;
    state.regP = nes->regP;
    state.regSP = nes->regSP;
    state.totalCpuCycles = totalCpuCycles;
    state.totalPpuCycles = totalPpuCycles;
    state.totalApuCycles = totalApuCycles;
    state.ppuScanline = nes->ppu->scanline;
    state.ppuCycle = nes->ppu->cycle;

    jsonLogBuffer.push_back(state);

    if (jsonLogBuffer.size() >= 50) {
        flush();
       
    }

    if (last) {
        flush();
        jsonFile << "\n]";
        jsonFile.close();
    }

    return 0;
}
