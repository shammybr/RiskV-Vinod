#pragma once
#include "lib/nes_ntsc/nes_ntsc.h"
#include "SDL3/SDL_events.h"
#include <vector>  
#include <cstdint>
#include "imgui.h"
#include "imgui_memory_editor.h"
#include <unordered_map>
#include <string>
#include <list>


enum EMemoryWindow {
	STACK,
	ZEROPAGE,
	GENERAL,
	IO


};

struct MacroOp {
    const char* opName;
    int opArgs[3];
    int argAmount;

};


struct NoWire {
	SDL_FRect wire;
	NoWire* prox = NULL;

	NoWire(SDL_FRect line) {
		wire = line;

	}
};


struct SWire {
	int iWires = 0;

	NoWire* firstLine = NULL;

	void insertLine(SDL_FRect line) {

		iWires++;

		if (firstLine == NULL) {
			firstLine = new NoWire(line);
			
		}
		else {
			NoWire* prox = firstLine;

			while (prox->prox != NULL) {
			
				prox = prox->prox;
			}	

			prox->prox = new NoWire(line);
		}


	}

};


class SDLNTSC {


public:
	static const int NES_WIDTH = 256;
	static const int NES_HEIGHT = 240;


	static const int NTSC_OUT_WIDTH = NES_NTSC_OUT_WIDTH(NES_WIDTH);
	static const int NTSC_OUT_HEIGHT = NES_HEIGHT;
	
    const MacroOp macroOps[151] = {
        // ==========================================
        // A
        // ==========================================
        {"ADC_ABS",  {0x6D, -1, -1}, 3}, {"ADC_ABX",  {0x7D, -1, -1}, 3}, {"ADC_ABY",  {0x79, -1, -1}, 3}, {"ADC_IMM",  {0x69, -1, -1}, 2},
        {"ADC_INDX", {0x61, -1, -1}, 2}, {"ADC_INDY", {0x71, -1, -1}, 2}, {"ADC_ZP",   {0x65, -1, -1}, 2}, {"ADC_ZPX",  {0x75, -1, -1}, 2},
        {"AND_ABS",  {0x2D, -1, -1}, 3}, {"AND_ABX",  {0x3D, -1, -1}, 3}, {"AND_ABY",  {0x39, -1, -1}, 3}, {"AND_IMM",  {0x29, -1, -1}, 2},
        {"AND_INDX", {0x21, -1, -1}, 2}, {"AND_INDY", {0x31, -1, -1}, 2}, {"AND_ZP",   {0x25, -1, -1}, 2}, {"AND_ZPX",  {0x35, -1, -1}, 2},
        {"ASL_ABS",  {0x0E, -1, -1}, 3}, {"ASL_ABX",  {0x1E, -1, -1}, 3}, {"ASL_IMP",  {0x0A, -1, -1}, 1}, {"ASL_ZP",   {0x06, -1, -1}, 2},
        {"ASL_ZPX",  {0x16, -1, -1}, 2},

        // ==========================================
        // B
        // ==========================================
        {"BCC_REL",  {0x90, -1, -1}, 2}, {"BCS_REL",  {0xB0, -1, -1}, 2}, {"BEQ_REL",  {0xF0, -1, -1}, 2}, {"BIT_ABS",  {0x2C, -1, -1}, 3},
        {"BIT_ZP",   {0x24, -1, -1}, 2}, {"BMI_REL",  {0x30, -1, -1}, 2}, {"BNE_REL",  {0xD0, -1, -1}, 2}, {"BPL_REL",  {0x10, -1, -1}, 2},
        {"BRK_IMP",  {0x00, -1, -1}, 1}, {"BVC_REL",  {0x50, -1, -1}, 2}, {"BVS_REL",  {0x70, -1, -1}, 2},

        // ==========================================
        // C
        // ==========================================
        {"CLC_IMP",  {0x18, -1, -1}, 1}, {"CLD_IMP",  {0xD8, -1, -1}, 1}, {"CLI_IMP",  {0x58, -1, -1}, 1}, {"CLV_IMP",  {0xB8, -1, -1}, 1},
        {"CMP_ABS",  {0xCD, -1, -1}, 3}, {"CMP_ABX",  {0xDD, -1, -1}, 3}, {"CMP_ABY",  {0xD9, -1, -1}, 3}, {"CMP_IMM",  {0xC9, -1, -1}, 2},
        {"CMP_INDX", {0xC1, -1, -1}, 2}, {"CMP_INDY", {0xD1, -1, -1}, 2}, {"CMP_ZP",   {0xC5, -1, -1}, 2}, {"CMP_ZPX",  {0xD5, -1, -1}, 2},
        {"CPX_ABS",  {0xEC, -1, -1}, 3}, {"CPX_IMM",  {0xE0, -1, -1}, 2}, {"CPX_ZP",   {0xE4, -1, -1}, 2},
        {"CPY_ABS",  {0xCC, -1, -1}, 3}, {"CPY_IMM",  {0xC0, -1, -1}, 2}, {"CPY_ZP",   {0xC4, -1, -1}, 2},

        // ==========================================
        // D
        // ==========================================
        {"DEC_ABS",  {0xCE, -1, -1}, 3}, {"DEC_ABX",  {0xDE, -1, -1}, 3}, {"DEC_ZP",   {0xC6, -1, -1}, 2}, {"DEC_ZPX",  {0xD6, -1, -1}, 2},
        {"DEX_IMP",  {0xCA, -1, -1}, 1}, {"DEY_IMP",  {0x88, -1, -1}, 1},

        // ==========================================
        // E
        // ==========================================
        {"EOR_ABS",  {0x4D, -1, -1}, 3}, {"EOR_ABX",  {0x5D, -1, -1}, 3}, {"EOR_ABY",  {0x59, -1, -1}, 3}, {"EOR_IMM",  {0x49, -1, -1}, 2},
        {"EOR_INDX", {0x41, -1, -1}, 2}, {"EOR_INDY", {0x51, -1, -1}, 2}, {"EOR_ZP",   {0x45, -1, -1}, 2}, {"EOR_ZPX",  {0x55, -1, -1}, 2},

        // ==========================================
        // I
        // ==========================================
        {"INC_ABS",  {0xEE, -1, -1}, 3}, {"INC_ABX",  {0xFE, -1, -1}, 3}, {"INC_ZP",   {0xE6, -1, -1}, 2}, {"INC_ZPX",  {0xF6, -1, -1}, 2},
        {"INX_IMP",  {0xE8, -1, -1}, 1}, {"INY_IMP",  {0xC8, -1, -1}, 1},

        // ==========================================
        // J
        // ==========================================
        {"JMP_ABS",  {0x4C, -1, -1}, 3}, {"JMP_IND",  {0x6C, -1, -1}, 3}, {"JSR_ABS",  {0x20, -1, -1}, 3},

        // ==========================================
        // L
        // ==========================================
        {"LDA_ABS",  {0xAD, -1, -1}, 3}, {"LDA_ABX",  {0xBD, -1, -1}, 3}, {"LDA_ABY",  {0xB9, -1, -1}, 3}, {"LDA_IMM",  {0xA9, -1, -1}, 2},
        {"LDA_INDX", {0xA1, -1, -1}, 2}, {"LDA_INDY", {0xB1, -1, -1}, 2}, {"LDA_ZP",   {0xA5, -1, -1}, 2}, {"LDA_ZPX",  {0xB5, -1, -1}, 2},
        {"LDX_ABS",  {0xAE, -1, -1}, 3}, {"LDX_ABY",  {0xBE, -1, -1}, 3}, {"LDX_IMM",  {0xA2, -1, -1}, 2}, {"LDX_ZP",   {0xA6, -1, -1}, 2},
        {"LDX_ZPY",  {0xB6, -1, -1}, 2},
        {"LDY_ABS",  {0xAC, -1, -1}, 3}, {"LDY_ABX",  {0xBC, -1, -1}, 3}, {"LDY_IMM",  {0xA0, -1, -1}, 2}, {"LDY_ZP",   {0xA4, -1, -1}, 2},
        {"LDY_ZPX",  {0xB4, -1, -1}, 2},
        {"LSR_ABS",  {0x4E, -1, -1}, 3}, {"LSR_ABX",  {0x5E, -1, -1}, 3}, {"LSR_IMP",  {0x4A, -1, -1}, 1}, {"LSR_ZP",   {0x46, -1, -1}, 2},
        {"LSR_ZPX",  {0x56, -1, -1}, 2},

        // ==========================================
        // N
        // ==========================================
        {"NOP_IMP",  {0xEA, -1, -1}, 1},

        // ==========================================
        // O
        // ==========================================
        {"ORA_ABS",  {0x0D, -1, -1}, 3}, {"ORA_ABX",  {0x1D, -1, -1}, 3}, {"ORA_ABY",  {0x19, -1, -1}, 3}, {"ORA_IMM",  {0x09, -1, -1}, 2},
        {"ORA_INDX", {0x01, -1, -1}, 2}, {"ORA_INDY", {0x11, -1, -1}, 2}, {"ORA_ZP",   {0x05, -1, -1}, 2}, {"ORA_ZPX",  {0x15, -1, -1}, 2},

        // ==========================================
        // P
        // ==========================================
        {"PHA_IMP",  {0x48, -1, -1}, 1}, {"PHP_IMP",  {0x08, -1, -1}, 1}, {"PLA_IMP",  {0x68, -1, -1}, 1}, {"PLP_IMP",  {0x28, -1, -1}, 1},

        // ==========================================
        // R
        // ==========================================
        {"ROL_ABS",  {0x2E, -1, -1}, 3}, {"ROL_ABX",  {0x3E, -1, -1}, 3}, {"ROL_IMP",  {0x2A, -1, -1}, 1}, {"ROL_ZP",   {0x26, -1, -1}, 2},
        {"ROL_ZPX",  {0x36, -1, -1}, 2},
        {"ROR_ABS",  {0x6E, -1, -1}, 3}, {"ROR_ABX",  {0x7E, -1, -1}, 3}, {"ROR_IMP",  {0x6A, -1, -1}, 1}, {"ROR_ZP",   {0x66, -1, -1}, 2},
        {"ROR_ZPX",  {0x76, -1, -1}, 2},
        {"RTI_IMP",  {0x40, -1, -1}, 1}, {"RTS_IMP",  {0x60, -1, -1}, 1},

        // ==========================================
        // S
        // ==========================================
        {"SBC_ABS",  {0xED, -1, -1}, 3}, {"SBC_ABX",  {0xFD, -1, -1}, 3}, {"SBC_ABY",  {0xF9, -1, -1}, 3}, {"SBC_IMM",  {0xE9, -1, -1}, 2},
        {"SBC_INDX", {0xE1, -1, -1}, 2}, {"SBC_INDY", {0xF1, -1, -1}, 2}, {"SBC_ZP",   {0xE5, -1, -1}, 2}, {"SBC_ZPX",  {0xF5, -1, -1}, 2},
        {"SEC_IMP",  {0x38, -1, -1}, 1}, {"SED_IMP",  {0xF8, -1, -1}, 1}, {"SEI_IMP",  {0x78, -1, -1}, 1},
        {"STA_ABS",  {0x8D, -1, -1}, 3}, {"STA_ABX",  {0x9D, -1, -1}, 3}, {"STA_ABY",  {0x99, -1, -1}, 3}, {"STA_INDX", {0x81, -1, -1}, 2},
        {"STA_INDY", {0x91, -1, -1}, 2}, {"STA_ZP",   {0x85, -1, -1}, 2}, {"STA_ZPX",  {0x95, -1, -1}, 2},
        {"STX_ABS",  {0x8E, -1, -1}, 3}, {"STX_ZP",   {0x86, -1, -1}, 2}, {"STX_ZPY",  {0x96, -1, -1}, 2},
        {"STY_ABS",  {0x8C, -1, -1}, 3}, {"STY_ZP",   {0x84, -1, -1}, 2}, {"STY_ZPX",  {0x94, -1, -1}, 2},

        // ==========================================
        // T
        // ==========================================
        {"TAX_IMP",  {0xAA, -1, -1}, 1}, {"TAY_IMP",  {0xA8, -1, -1}, 1}, {"TSX_IMP",  {0xBA, -1, -1}, 1}, {"TXA_IMP",  {0x8A, -1, -1}, 1},
        {"TXS_IMP",  {0x9A, -1, -1}, 1}, {"TYA_IMP",  {0x98, -1, -1}, 1}
    };

    std::unordered_map<std::string, uint8_t> assemblerDictionary = {

        // ==========================================
        // 1. LOAD & STORE (Memory to Registers)
        // ==========================================
        // LDA - Load Accumulator
        {"LDA_IMM",  0xA9}, {"LDA_ZP",   0xA5}, {"LDA_ZPX",  0xB5}, {"LDA_ABS",  0xAD},
        {"LDA_ABX",  0xBD}, {"LDA_ABY",  0xB9}, {"LDA_INDX", 0xA1}, {"LDA_INDY", 0xB1},
        // LDX - Load X Register
        {"LDX_IMM",  0xA2}, {"LDX_ZP",   0xA6}, {"LDX_ZPY",  0xB6}, {"LDX_ABS",  0xAE}, {"LDX_ABY",  0xBE},
        // LDY - Load Y Register
        {"LDY_IMM",  0xA0}, {"LDY_ZP",   0xA4}, {"LDY_ZPX",  0xB4}, {"LDY_ABS",  0xAC}, {"LDY_ABX",  0xBC},

        // STA - Store Accumulator
        {"STA_ZP",   0x85}, {"STA_ZPX",  0x95}, {"STA_ABS",  0x8D}, {"STA_ABX",  0x9D},
        {"STA_ABY",  0x99}, {"STA_INDX", 0x81}, {"STA_INDY", 0x91},
        // STX - Store X Register
        {"STX_ZP",   0x86}, {"STX_ZPY",  0x96}, {"STX_ABS",  0x8E},
        // STY - Store Y Register
        {"STY_ZP",   0x84}, {"STY_ZPX",  0x94}, {"STY_ABS",  0x8C},

        // ==========================================
        // 2. REGISTER TRANSFERS (Implied Mode)
        // ==========================================
        {"TAX_IMP",  0xAA}, // Transfer A to X
        {"TAY_IMP",  0xA8}, // Transfer A to Y
        {"TXA_IMP",  0x8A}, // Transfer X to A
        {"TYA_IMP",  0x98}, // Transfer Y to A
        {"TSX_IMP",  0xBA}, // Transfer Stack Pointer to X
        {"TXS_IMP",  0x9A}, // Transfer X to Stack Pointer

        // ==========================================
        // 3. STACK OPERATIONS (Implied Mode)
        // ==========================================
        {"PHA_IMP",  0x48}, // Push Accumulator
        {"PLA_IMP",  0x68}, // Pull Accumulator
        {"PHP_IMP",  0x08}, // Push Processor Status (Flags)
        {"PLP_IMP",  0x28}, // Pull Processor Status

        // ==========================================
        // 4. MATH & ARITHMETIC
        // ==========================================
        // ADC - Add with Carry
        {"ADC_IMM",  0x69}, {"ADC_ZP",   0x65}, {"ADC_ZPX",  0x75}, {"ADC_ABS",  0x6D},
        {"ADC_ABX",  0x7D}, {"ADC_ABY",  0x79}, {"ADC_INDX", 0x61}, {"ADC_INDY", 0x71},
        // SBC - Subtract with Carry
        {"SBC_IMM",  0xE9}, {"SBC_ZP",   0xE5}, {"SBC_ZPX",  0xF5}, {"SBC_ABS",  0xED},
        {"SBC_ABX",  0xFD}, {"SBC_ABY",  0xF9}, {"SBC_INDX", 0xE1}, {"SBC_INDY", 0xF1},

        // INC / DEC - Increment / Decrement Memory
        {"INC_ZP",   0xE6}, {"INC_ZPX",  0xF6}, {"INC_ABS",  0xEE}, {"INC_ABX",  0xFE},
        {"DEC_ZP",   0xC6}, {"DEC_ZPX",  0xD6}, {"DEC_ABS",  0xCE}, {"DEC_ABX",  0xDE},

        // INX/INY/DEX/DEY - Increment / Decrement Registers (Implied)
        {"INX_IMP",  0xE8}, {"INY_IMP",  0xC8},
        {"DEX_IMP",  0xCA}, {"DEY_IMP",  0x88},

        // ==========================================
        // 5. BITWISE LOGIC
        // ==========================================
        // AND - Logical AND
        {"AND_IMM",  0x29}, {"AND_ZP",   0x25}, {"AND_ZPX",  0x35}, {"AND_ABS",  0x2D},
        {"AND_ABX",  0x3D}, {"AND_ABY",  0x39}, {"AND_INDX", 0x21}, {"AND_INDY", 0x31},
        // ORA - Logical Inclusive OR
        {"ORA_IMM",  0x09}, {"ORA_ZP",   0x05}, {"ORA_ZPX",  0x15}, {"ORA_ABS",  0x0D},
        {"ORA_ABX",  0x1D}, {"ORA_ABY",  0x19}, {"ORA_INDX", 0x01}, {"ORA_INDY", 0x11},
        // EOR - Logical Exclusive OR
        {"EOR_IMM",  0x49}, {"EOR_ZP",   0x45}, {"EOR_ZPX",  0x55}, {"EOR_ABS",  0x4D},
        {"EOR_ABX",  0x5D}, {"EOR_ABY",  0x59}, {"EOR_INDX", 0x41}, {"EOR_INDY", 0x51},

        // BIT - Bit Test
        {"BIT_ZP",   0x24}, {"BIT_ABS",  0x2C},

        // ==========================================
        // 6. SHIFTS & ROTATES
        // ==========================================
        // ASL - Arithmetic Shift Left
        {"ASL_IMP",  0x0A}, {"ASL_ZP",   0x06}, {"ASL_ZPX",  0x16}, {"ASL_ABS",  0x0E}, {"ASL_ABX",  0x1E},
        // LSR - Logical Shift Right
        {"LSR_IMP",  0x4A}, {"LSR_ZP",   0x46}, {"LSR_ZPX",  0x56}, {"LSR_ABS",  0x4E}, {"LSR_ABX",  0x5E},
        // ROL - Rotate Left
        {"ROL_IMP",  0x2A}, {"ROL_ZP",   0x26}, {"ROL_ZPX",  0x36}, {"ROL_ABS",  0x2E}, {"ROL_ABX",  0x3E},
        // ROR - Rotate Right
        {"ROR_IMP",  0x6A}, {"ROR_ZP",   0x66}, {"ROR_ZPX",  0x76}, {"ROR_ABS",  0x6E}, {"ROR_ABX",  0x7E},

        // ==========================================
        // 7. COMPARISONS
        // ==========================================
        // CMP - Compare Accumulator
        {"CMP_IMM",  0xC9}, {"CMP_ZP",   0xC5}, {"CMP_ZPX",  0xD5}, {"CMP_ABS",  0xCD},
        {"CMP_ABX",  0xDD}, {"CMP_ABY",  0xD9}, {"CMP_INDX", 0xC1}, {"CMP_INDY", 0xD1},
        // CPX - Compare X Register
        {"CPX_IMM",  0xE0}, {"CPX_ZP",   0xE4}, {"CPX_ABS",  0xEC},
        // CPY - Compare Y Register
        {"CPY_IMM",  0xC0}, {"CPY_ZP",   0xC4}, {"CPY_ABS",  0xCC},

        // ==========================================
        // 8. CONTROL FLOW (Jumps & Branches)
        // ==========================================
        // Jumps and Returns
        {"JMP_ABS",  0x4C}, {"JMP_IND",  0x6C},
        {"JSR_ABS",  0x20}, // Jump to Subroutine
        {"RTS_IMP",  0x60}, // Return from Subroutine
        {"RTI_IMP",  0x40}, // Return from Interrupt

        // Branches (Relative Mode)
        {"BCC_REL",  0x90}, // Branch on Carry Clear
        {"BCS_REL",  0xB0}, // Branch on Carry Set
        {"BNE_REL",  0xD0}, // Branch on Not Equal (Zero Clear)
        {"BEQ_REL",  0xF0}, // Branch on Equal (Zero Set)
        {"BPL_REL",  0x10}, // Branch on Plus (Negative Clear)
        {"BMI_REL",  0x30}, // Branch on Minus (Negative Set)
        {"BVC_REL",  0x50}, // Branch on Overflow Clear
        {"BVS_REL",  0x70}, // Branch on Overflow Set

        // ==========================================
        // 9. FLAG TOGGLES (Implied Mode)
        // ==========================================
        {"CLC_IMP",  0x18}, // Clear Carry
        {"SEC_IMP",  0x38}, // Set Carry
        {"CLI_IMP",  0x58}, // Clear Interrupt Disable
        {"SEI_IMP",  0x78}, // Set Interrupt Disable
        {"CLV_IMP",  0xB8}, // Clear Overflow
        {"CLD_IMP",  0xD8}, // Clear Decimal
        {"SED_IMP",  0xF8}, // Set Decimal

        // ==========================================
        // 10. SYSTEM CONTROLS (Implied Mode)
        // ==========================================
        {"BRK_IMP",  0x00}, // Force Interrupt
        {"NOP_IMP",  0xEA}  // No Operation
    };

    std::vector<MacroOp> customOps = { {"SEI_IMP",  {0x78, -1, -1}, 1}, {"LDA_IMM",  {0xA9, 0xFF, -1}, 2}, { "STA_ZP",   {0x85, 0x00, -1}, 2 }, {"JMP_ABS",  {0x4C, 0x01, 0x80}, 3} };

	std::vector<uint8_t> nesBuffer;
	std::vector<uint16_t> ntscOutput;
    std::vector<uint8_t> savedChrRom = {};
    uint8_t savedChrBanks = 0;
    uint8_t savedFlags6 = 0x00;
    uint8_t savedFlags7 = 0x00;


	class NES* nes;
	class PPU* ppu;

	struct SDL_Window* window;
	struct SDL_Renderer* renderer;
	struct SDL_Texture* texture;
	struct SDL_Texture* CPUtexture;
	struct SDL_AudioSpec spec;
	struct SDL_AudioStream* audioStream;
	SDL_Event event;
	nes_ntsc_t* ntsc;
	nes_ntsc_setup_t setup;
	SDL_FRect gameRect;
	SDL_FRect cpuRect;

	SDL_FRect cpuTopPins[20];
	SDL_FRect cpuBotPins[20];

	SWire* cpuTopWires[20];
	SWire* cpuBotWires[20];

	ImGuiWindowFlags regFlags;
	ImGuiWindowFlags regFlagsScroll;
	MemoryEditor mem_edit;
	SDL_FRect lineLeft;
	SDL_FRect lineRight;

	SDL_FRect lineTop;
	SDL_FRect lineBottom;


	//RAM
	SDL_FRect RAMlineLeft;
	SDL_FRect RAMlineRight;

	SDL_FRect RAMlineTop;
	SDL_FRect RAMlineBottom;


	SDL_FRect RAMTopPins[12];
	SDL_FRect RAMBotPins[12];

	SWire* RAMBotWires[12];


	float lineThicc = 4.0f;
	float wireThicc = 2.0f;

	int burst_phase;
	bool isCustomMode = false;
    bool isSaving = false;
    bool isLoading = false;

    std::string saveName;
    std::vector<std::string> files;

	SDLNTSC(int windowW, int windowH);

	void calculateLines();

	void quit();
	void draw(int frames);
	void DrawGame();
	void DrawCPU();
	bool poll(uint8_t* controller, std::vector<float>& audioBuffer);

	void playAudio(std::vector<float>& audioBuffer);

	void setNes(NES* _nes);
    std::vector<uint8_t> loadCustomCode(const std::string& filepath);
};