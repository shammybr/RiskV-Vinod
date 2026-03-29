#pragma once
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>

#pragma pack(push, 1)
struct iNES_Header {
    char name[4];
    uint8_t prgChunks;
    uint8_t chrChunks;
    uint8_t mapper1;
    uint8_t mapper2;
    uint8_t prgRamSize;
    uint8_t tvSystem1;
    uint8_t tvSystem2;
    char unused[5];



};
#pragma pack(pop)

enum EMirrorMode {
    MVERTICAL,
    MHORIZONTAL,
    MONESCREENLO,
    MONESCREENHI
};


class NESROM {
public:
    std::vector<uint8_t> vPRGMemory;
    std::vector<uint8_t> vCHRMemory;
    
    iNES_Header header;
    uint8_t mapperID = 0;
    EMirrorMode mirrorMode;
    std::string romName;

    NESROM(const std::string& fileName);

    uint8_t read(uint16_t address);
    void write(uint16_t address, uint8_t data);
};