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
#include <SDL3/SDL.h>
#include "SDLNTSC.h"

void CompareFiles(const std::string& file1Path, const std::string& file2Path, const std::string& outputPath);

int main(){



    NESLogger* logger = new NESLogger();


    SDLNTSC* sdl = new SDLNTSC(1280, 720);

    //RiskVProcessor* processor = new RiskVProcessor(0x10000);
    NES* nes = new NES();
    NESROM* rom = new NESROM("ROM/AccuracyCoin.nes");
    nes->ppu->videoBuffer = sdl->nesBuffer.data();

    nes->loadRom(rom);
  //  nes->reset();

   

    std::cout << "vPRGMemory size: " << rom->vPRGMemory.size() << "!" << std::endl;
    std::cout << "vCHRMemory size: " << rom->vCHRMemory.size() << "!" << std::endl;
    
    
    //nestest
    //int instructionCount = 0;

   // std::ofstream logFile("ROM/DQ3Risc.log");

    //if (!logFile.is_open()) {
      //  std::cout << "Error: falha ao criar log file!" << std::endl;
       // return -1;
   // }
    

    const std::chrono::nanoseconds frameTarget(16639260);
   
    auto frameStart = std::chrono::steady_clock::now();
    auto targetTime = frameStart + frameTarget;

    auto lastFpsTime = std::chrono::steady_clock::now();
    int frames = 0;

    uint64_t totalCpuCycles = 0;
    int totalPpuCycles = 0;
    int totalApuCycles = 0;
    bool isCISC = false;
    bool logNext = true;
    while (nes->on) {
        
        if (!sdl->poll(nes->controller, nes->apu->audioBuffer)) {
            nes->on = false;
            sdl->quit();
        }

 
           
        //if(nes->opQueue[nes->queueIndex] == OP_FETCH_OPCODE)
       //     logFile << logger->getLogStep(nes);

        // if (logger) {
        //     if (logger->jsonlogStep(nes, totalCpuCycles, totalPpuCycles, totalApuCycles, false)) {

            //   }
        // }
        if (isCISC) {

            int cpuCycles = nes->step(logger, isCISC);

            totalCpuCycles += cpuCycles;

            bool sleep = false;
            bool isGet = (totalCpuCycles % 2 == 0);
            for (int i = 0; i < cpuCycles; i++) {
                nes->apu->step(isGet);
                totalApuCycles++;

            }

            for (int i = 0; i < (cpuCycles * 3); i++) {
                if (nes->ppu->step(logger)) {
                    //   sleep = true;
                    frames++;
                    sdl->playAudio(nes->apu->audioBuffer);
                    sdl->draw();
                    if (nes->wannaSave) {
                        nes->saveGame();
                    }

                    // logger->jsonlogStep(nes, totalCpuCycles, totalPpuCycles, totalApuCycles, true);
                    //  nes->on = false;

                }
                totalPpuCycles++;

            }


            auto now = std::chrono::steady_clock::now();

            if (now >= lastFpsTime + std::chrono::seconds(1)) {
                std::cout << std::dec << "FPS: " << frames << std::endl;
                frames = 0;
                lastFpsTime += std::chrono::seconds(1);
            }


            if (sleep) {


                while (std::chrono::steady_clock::now() < targetTime) {
                    std::this_thread::yield();
                }


                targetTime += frameTarget;
            }


        }
        else {


           
            bool isGet = (nes->currentCycles % 2 == 0);

         
            if (nes->apu->dpcm->dmaPending && !nes->dpcmActive) {
                if (nes->traceCPU) {
                   
                    printf("[Cycle %llu] dmaPending! |  isWritingMemory: %s \n",
                            nes->currentCycles, nes->isWritingMemory ? "true" : "false");
                    
                    //	traceCPU = true;
                    //	LOGGO = true;
                }
                
                if (!nes->isWritingMemory) {

                    if (nes->apu->dpcm->enabled || nes->apu->dpcm->implicitAbortFlag) {
                        nes->dpcmActive = true;
                        nes->dpcmHaltCycles = isGet ? 4 : 3;
                    }

                 
                    nes->apu->dpcm->dmaPending = false;
                    nes->apu->dpcm->implicitAbortFlag = false;
                }
                else {
                    if (nes->apu->dpcm->implicitAbortFlag) {
                        nes->apu->dpcm->implicitAbortFlag = false;
                    }
                }
            }

          
            if (nes->dpcmActive) {
             
                if (nes->dpcmHaltCycles == 1) {
                    if (nes->traceCPU) {

                        printf("[Cycle %llu] Last Halt Cycle! \n",
                            nes->currentCycles);

                        //	traceCPU = true;
                        //	LOGGO = true;
                    }
                    uint8_t sampleByte = nes->read(nes->apu->dpcm->currentAddress);
                    nes->apu->dpcm->sampleBuffer = sampleByte; 
                    nes->apu->dpcm->hasBuffer = true;

                    if (nes->apu->dpcm->currentAddress == 0xFFFF) {
                        nes->apu->dpcm->currentAddress = 0x8000;
                    }
                    else {
                        nes->apu->dpcm->currentAddress++;
                    }

                
                    if (nes->apu->dpcm->currentBytesRemaining > 0) {
                        nes->apu->dpcm->currentBytesRemaining--;
                        if (nes->apu->dpcm->currentBytesRemaining == 0) {
                            if (nes->apu->dpcm->loop) {
                                nes->apu->dpcm->currentAddress = nes->apu->dpcm->sampleAddress;
                                nes->apu->dpcm->currentBytesRemaining = nes->apu->dpcm->sampleLength;
                            }
                            else if (nes->apu->dpcm->irqEnable) {
                                nes->apu->dpcm->irqPending = true;
                            }
                        }
                    }
                    nes->dpcmActive = false; 
                }
                else {
                  
                    if (nes->addressBus != 0x2007) {
                        nes->read(nes->addressBus);
                    }
                }
                nes->dpcmHaltCycles--;
            }

            else if (nes->dmaWaiting) {

                if (nes->oamDmaState == 0) {
                    nes->oamDmaState = isGet ? 1 : 2;
                }
                else if (nes->oamDmaState == 1) {
                    nes->oamDmaState = 2;
                }
                else if (nes->oamDmaState == 2) {
                    if (isGet) {
                        nes->dmaData = nes->read((nes->dmaPage << 8) | nes->dmaAddress);
                        nes->oamDmaState = 3;
                    }
                }
                else if (nes->oamDmaState == 3) {
                    if (!isGet) { 
                        nes->ppu->oam[nes->ppu->oamAddress++] = nes->dmaData;
                        nes->dmaAddress++;
                        if (nes->dmaAddress == 0) {
                            nes->dmaWaiting = false;
                        }
                        else {
                            nes->oamDmaState = 2;
                        }
                    }
                }
            }
            else {

                nes->RISCStep(logger);
                
            }

            for (int i = 0; i < 3; i++) {
                if (nes->ppu->step(logger)) {
                    frames++;
                    sdl->playAudio(nes->apu->audioBuffer);
                    sdl->draw();
                    if (nes->wannaSave) nes->saveGame();
                }
                totalPpuCycles++;
            }

            nes->apu->step(isGet);
            nes->currentCycles++;


            auto now = std::chrono::steady_clock::now();

            if (now >= lastFpsTime + std::chrono::seconds(1)) {
                std::cout << std::dec << "FPS: " << frames << std::endl;
                frames = 0;
                lastFpsTime += std::chrono::seconds(1);
            }


            /*     if (nes->apu->debugTest7) {
                         printf("CPU:%llu | Parity:%s | DMA:%d | PC:%04X | frameIRQ:%d | Delay:%d\n",
                             nes->currentCycles,
                             (nes->currentCycles % 2 == 0) ? "GET" : "PUT",
                             nes->dmaWaiting,
                             nes->regPC,
                             nes->apu->frameIRQ,
                             nes->apu->framIrqDelay);
                 }*/




            totalCpuCycles++;
            totalApuCycles++;

           

        }

    }


   // logFile.close();

    CompareFiles("ROM/nestestLog.log", "ROM/nestest.log", "ROM/compareLog.txt");
    return 0;
}




char* logStep(NES* nes, InstructionInfo opTable[256]) {
  
    uint16_t pc = nes->regPC;
    uint8_t opcode = nes->read(pc);
    InstructionInfo info = opTable[opcode];

    uint8_t b1 = 0, b2 = 0;
    char hexStr[15] = { 0 };
    char asmStr[40] = { 0 };


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

    uint16_t addr = 0;
    switch (info.mode) {
    case IMP:
        snprintf(asmStr, sizeof(asmStr), "%s", info.name);
       
        if (opcode == 0x0A || opcode == 0x4A || opcode == 0x2A || opcode == 0x6A) {
            snprintf(asmStr, sizeof(asmStr), "%s A", info.name);
        }
        break;

    case IMM:
        snprintf(asmStr, sizeof(asmStr), "%s #$%02X", info.name, b1);
        break;

    case ZP:
        
        snprintf(asmStr, sizeof(asmStr), "%s $%02X = %02X", info.name, b1, nes->read(b1));
        break;

    case ZPX:
        addr = (b1 + nes->regX) & 0xFF;
        snprintf(asmStr, sizeof(asmStr), "%s $%02X,X @ %02X = %02X", info.name, b1, addr, nes->read(addr));
        break;

    case ABS:
        addr = b1 | (b2 << 8);
       
        if (opcode == 0x4C || opcode == 0x20) {
            snprintf(asmStr, sizeof(asmStr), "%s $%04X", info.name, addr);
        }
        else {
            snprintf(asmStr, sizeof(asmStr), "%s $%04X = %02X", info.name, addr, nes->read(addr));
        }
        break;

    case REL:
        
        addr = pc + 2 + (int8_t)b1;
        snprintf(asmStr, sizeof(asmStr), "%s $%04X", info.name, addr);
        break;



    default:
        snprintf(asmStr, sizeof(asmStr), "???");
        break;
    }

  
    char finalLog[150];
  
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
        std::cerr << "Error abrindo os arquivos de log." << std::endl;
        return;
    }

    std::string line1, line2;


    while (std::getline(file1, line1) && std::getline(file2, line2)) {
      
        std::string prefix1 = line1.substr(0, 4);
        std::string prefix2 = line2.substr(0, 4);

    
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
