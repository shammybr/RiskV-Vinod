#pragma once
#include "NESROM.h"

NESROM::NESROM(const std::string& fileName) {

    std::ifstream file(fileName, std::ios::binary);
    if (!file.is_open()) {
        std::cout << "Arquivo inexistente!" << std::endl;
        return;
    }

    file.read((char*)&header, sizeof(iNES_Header));

    if (header.mapper1 & 0x04) {
        file.seekg(512, std::ios_base::cur);
    }

    std::cout << "Name: " << header.name << " " << std::endl;

    // ID = 4 bits da flag 6 e flag 7
    mapperID = ((header.mapper2 >> 4) << 4) | (header.mapper1 >> 4);


    //CPU
    uint32_t prgSize = header.prgChunks * 16384;
    vPRGMemory.resize(prgSize);
    file.read((char*)vPRGMemory.data(), vPRGMemory.size());

    //PPU
    uint32_t chrSize = header.chrChunks * 8192;

    // jogos que usam RAM para graficos
    if (chrSize == 0) {
        vCHRMemory.resize(8192); // Create 8KB of empty RAM for them
    }
    else {
        vCHRMemory.resize(chrSize);
        file.read((char*)vCHRMemory.data(), vCHRMemory.size());
    }

    file.close();
}


uint8_t NESROM::read(uint16_t address){
    return 0;
}

void NESROM::write(uint16_t address, uint8_t data)
{

}