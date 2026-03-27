// RiskV.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#pragma once
#include <iostream>
#include <iomanip>
#include <fstream>
#include <thread>
#include <chrono>
#include "RiskVProcessor.h"
#include "NES.h"
#include "NESROM.h"
#include "NESLogger.h"

void CompareFiles(const std::string& file1Path, const std::string& file2Path, const std::string& outputPath);

int main(){



    NESLogger* logger = new NESLogger();


    //RiskVProcessor* processor = new RiskVProcessor(0x10000);
    NES* nes = new NES();
    NESROM* rom = new NESROM("ROM/nestest.nes");

    nes->loadRom(rom);
    nes->reset();


    std::cout << "vPRGMemory size: " << rom->vPRGMemory.size() << "!" << std::endl;
    std::cout << "vCHRMemory size: " << rom->vCHRMemory.size() << "!" << std::endl;
    
    //nestest
    int instructionCount = 0;

    std::ofstream logFile("ROM/nestestLog.log");

    if (!logFile.is_open()) {
        std::cout << "Error: falha ao criar log file!" << std::endl;
        return -1;
    }


    const std::chrono::nanoseconds frameTarget(16639260);
   
    auto frameStart = std::chrono::steady_clock::now();
    auto targetTime = frameStart + frameTarget;

    auto lastFpsTime = std::chrono::steady_clock::now();
    int frames = 0;
    while (nes->on) {
       
        


        //logFile << logger->getLogStep(nes);

        uint8_t cpuCycles = nes->step(logger);
        bool sleep = false;

        for (int i = 0; i < (cpuCycles * 3); i++) {
            if (nes->ppu->step(logger)) {
                sleep = true;
                frames++;

            }
            
        }


        if (sleep) {

            auto now = std::chrono::steady_clock::now();
            if (now >= lastFpsTime + std::chrono::seconds(1)) {
                std::cout << "FPS: " << frames << std::endl;
                frames = 0;
                lastFpsTime += std::chrono::seconds(1);
            }


            while (std::chrono::steady_clock::now() < targetTime) {
                std::this_thread::yield();
            }


            targetTime += frameTarget;
        }



    }


    logFile.close();

    CompareFiles("ROM/nestestLog.log", "ROM/nestest.log", "ROM/compareLog.txt");
    return 0;
}






char* logStep(NES* nes, InstructionInfo opTable[256]) {
    // --- A. PEEK AT THE NEXT INSTRUCTION ---
    uint16_t pc = nes->regPC;
    uint8_t opcode = nes->read(pc);
    InstructionInfo info = opTable[opcode];

    uint8_t b1 = 0, b2 = 0;
    char hexStr[15] = { 0 };
    char asmStr[40] = { 0 };

    // --- B. READ THE BYTES AND FORMAT THE HEX COLUMN ---
    if (info.bytes == 1) {
        snprintf(hexStr, sizeof(hexStr), "%02X      ", opcode);
    }
    else if (info.bytes == 2) {
        b1 = nes->read(pc + 1);
        snprintf(hexStr, sizeof(hexStr), "%02X %02X   ", opcode, b1);
    }
    else if (info.bytes == 3) {
        b1 = nes->read(pc + 1);
        b2 = nes->read(pc + 2);
        snprintf(hexStr, sizeof(hexStr), "%02X %02X %02X", opcode, b1, b2);
    }

    // --- C. FORMAT THE ASSEMBLY STRING BASED ON ADDRESSING MODE ---
    uint16_t addr = 0; // Used for dynamic peeking
    switch (info.mode) {
    case IMP:
        snprintf(asmStr, sizeof(asmStr), "%s", info.name);
        // Special case for Accumulator instructions (like ASL A)
        if (opcode == 0x0A || opcode == 0x4A || opcode == 0x2A || opcode == 0x6A) {
            snprintf(asmStr, sizeof(asmStr), "%s A", info.name);
        }
        break;

    case IMM:
        snprintf(asmStr, sizeof(asmStr), "%s #$%02X", info.name, b1);
        break;

    case ZP:
        // Nintendulator dynamically peeks at the RAM value!
        snprintf(asmStr, sizeof(asmStr), "%s $%02X = %02X", info.name, b1, nes->read(b1));
        break;

    case ZPX:
        addr = (b1 + nes->regX) & 0xFF;
        snprintf(asmStr, sizeof(asmStr), "%s $%02X,X @ %02X = %02X", info.name, b1, addr, nes->read(addr));
        break;

    case ABS:
        addr = b1 | (b2 << 8);
        // JMP and JSR don't print the RAM value in Nintendulator
        if (opcode == 0x4C || opcode == 0x20) {
            snprintf(asmStr, sizeof(asmStr), "%s $%04X", info.name, addr);
        }
        else {
            snprintf(asmStr, sizeof(asmStr), "%s $%04X = %02X", info.name, addr, nes->read(addr));
        }
        break;

    case REL:
        // Branch instructions calculate the destination PC
        addr = pc + 2 + (int8_t)b1;
        snprintf(asmStr, sizeof(asmStr), "%s $%04X", info.name, addr);
        break;

        // ... (You will add ABSX, ABSY, INDX, INDY here following the same pattern) ...

    default:
        snprintf(asmStr, sizeof(asmStr), "???");
        break;
    }

    // --- D. PRINT THE FINAL NINTENDULATOR STRING ---
    char finalLog[150];
    // %-32s left-aligns the assembly string and pads it with spaces until column 48
    snprintf(finalLog, sizeof(finalLog),
        "%04X  %-8s  %-32s  A:%02X X:%02X Y:%02X P:%02X SP:%02X\n",
        pc, hexStr, asmStr,
        nes->regA, nes->regX, nes->regY, nes->regP, nes->regSP);

    return finalLog;
}


void CompareFiles(const std::string& file1Path, const std::string& file2Path, const std::string& outputPath) {
    std::ifstream file1(file1Path);
    std::ifstream file2(file2Path);
    std::ofstream outFile(outputPath);

    if (!file1.is_open() || !file2.is_open() || !outFile.is_open()) {
        std::cerr << "Error: Could not open one or more files." << std::endl;
        return;
    }

    std::string line1, line2;

    // Read both files line by line simultaneously
    while (std::getline(file1, line1) && std::getline(file2, line2)) {
        // Extract the first 4 characters (or less if the line is short)
        std::string prefix1 = line1.substr(0, 4);
        std::string prefix2 = line2.substr(0, 4);

        // Compare and write to the output file
        outFile << prefix1 << " " << prefix2 << " ";

        if (prefix1 == prefix2) {
            outFile << "EQUAL" << std::endl;
        }
        else {
            outFile << "NOT EQUAL" << std::endl;
        }
    }

    file1.close();
    file2.close();
    outFile.close();
}


// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
