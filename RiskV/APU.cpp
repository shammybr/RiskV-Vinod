#include "APU.h"

APU::APU() {
    pulse1 = new Pulse();
    pulse2 = new Pulse();
    noise = new Noise();
}

void APU::write(uint16_t address, uint8_t data){
    switch (address) {
        case 0x4000:
            pulse1->dutyMode = (data & 0b11000000) >> 6;
            pulse1->volume = data & 0b00001111;
            break;

        case 0x4001: // 
            pulse1->sweepEnabled = (data & 0x80) > 0;
            pulse1->sweepPeriod = (data & 0x70) >> 4;
            pulse1->sweepNegate = (data & 0x08) > 0;
            pulse1->sweepShift = data & 0x07;
            pulse1->sweepReload = true; 
            break;

        case 0x4002:
            // low 8 bits
            pulse1->timer = (pulse1->timer & 0xFF00) | data;
            break;

        case 0x4003:
            // high 3 bits 
            pulse1->timer = (pulse1->timer & 0x00FF) | ((data & 0b00000111) << 8);

            pulse1->dutyStep = 0;
            break;

        case 0x4004:
            pulse2->dutyMode = (data & 0b11000000) >> 6;
            pulse2->volume = data & 0b00001111;
            break;

        case 0x4005: // 
            pulse2->sweepEnabled = (data & 0x80) > 0;
            pulse2->sweepPeriod = (data & 0x70) >> 4;
            pulse2->sweepNegate = (data & 0x08) > 0;
            pulse2->sweepShift = data & 0x07;
            pulse2->sweepReload = true;
            break;

        case 0x4006:
            // low 8 bits
            pulse2->timer = (pulse2->timer & 0xFF00) | data;
            break;

        case 0x4007:
            // high 3 bits 
            pulse2->timer = (pulse2->timer & 0x00FF) | ((data & 0b00000111) << 8);

            pulse2->dutyStep = 0;
            break;

        case 0x400C:

            noise->envelopeLoop = (data & 0x20) > 0;
            noise->constantVolume = (data & 0x10) > 0;
            noise->volume = data & 0x0F;
            break;

        case 0x400D:
            
            break;

        case 0x400E:

            noise->mode = (data & 0x80) > 0;
            noise->periodIndex = data & 0x0F;

            // NTSC Frequencies
            {
                static const uint16_t noisePeriodTable[16] = {
                    4, 8, 16, 32, 64, 96, 128, 160, 202,
                    254, 380, 508, 762, 1016, 2034, 4068
                };
                noise->timer = noisePeriodTable[noise->periodIndex];
            }
            break;

        case 0x400F:

            noise->lengthCounter = (data & 0xF8) >> 3;

           
            noise->envelopeStart = true;
            break;

    }
  
}

void APU::step() {
    evenCycle = !evenCycle;

    if (evenCycle) {
       
        pulse1->tick();
        pulse2->tick();

        noise->tick();
    }

    audioCycleCounter++;



    if (audioCycleCounter >= cyclesPerSample) {
        audioCycleCounter -= cyclesPerSample;

        float p1 = (float)pulse1->currentOutput;
        float p2 = (float)pulse2->currentOutput;

        float pulseOut = 0.0f;
        if (p1 + p2 > 0) {
            // Standard NES Pulse Mixer formula
            pulseOut = 95.88f / ((8128.0f / (p1 + p2)) + 100.0f);
        }

       
        float filteredSample = pulseOut - prevRawSample + (0.995f * prevFilteredSample);
        prevRawSample = pulseOut;
        prevFilteredSample = filteredSample;


 
        audioBuffer.push_back(filteredSample * 0.2f);
    }


}