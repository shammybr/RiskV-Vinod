#include "APU.h"

APU::APU() {
    pulse1 = new Pulse();
    pulse1->isPulse1 = true;
    pulse2 = new Pulse();
    pulse2->isPulse1 = false;
    noise = new Noise();
    triangle = new Triangle();

}

void APU::write(uint16_t address, uint8_t data){
    switch (address) {
        case 0x4000:
           
            //  DDLC VVVV
            pulse1->dutyMode = (data & 0xC0) >> 6;
            pulse1->lengthCounterHalt = (data & 0x20) > 0; 
            pulse1->constantVolume = (data & 0x10) > 0;
            pulse1->volume = (data & 0x0F);


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
            pulse1->timer = (pulse1->timer & 0x0700) | data;
            break;

        case 0x4003:
            // high 3 bits 
            pulse1->timer = (pulse1->timer & 0x00FF) | ((data & 0x07) << 8);
            if (pulse1->enabled) {
                pulse1->lengthCounter = lengthTable[(data & 0xF8) >> 3];
            }
            pulse1->envelopeStart = true;
            pulse1->dutyStep = 0; 


            break;

        case 0x4004:

            //  DDLC VVVV
            pulse2->dutyMode = (data & 0xC0) >> 6;
            pulse2->lengthCounterHalt = (data & 0x20) > 0;
            pulse2->constantVolume = (data & 0x10) > 0;
            pulse2->volume = (data & 0x0F);

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
            pulse2->timer = (pulse2->timer & 0x0700) | data;
            break;

        case 0x4007:
            // high 3 bits 
            pulse2->timer = (pulse2->timer & 0x00FF) | ((data & 0x07) << 8);
            if (pulse2->enabled) {
                pulse2->lengthCounter = lengthTable[(data & 0xF8) >> 3];
            }
            pulse2->envelopeStart = true;
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

        case 0x4015:

            // Bit 0
            pulse1->enabled = (data & 0x01) > 0;
            if (!pulse1->enabled) {
                pulse1->lengthCounter = 0;
            }

            // Bit 1
            pulse2->enabled = (data & 0x02) > 0;
            if (!pulse2->enabled) {
                pulse2->lengthCounter = 0;
            }


            triangle->enabled = (data & 0x04) > 0;
            if (!triangle->enabled) triangle->lengthCounter = 0;

            //noise->enabled = (data & 0x08) > 0;
            //if (!noise->enabled) noise->lengthCounter = 0;
           
            break;

        case 0x4008:

            triangle->lengthCounterHalt = (data & 0x80) > 0;
            triangle->linearCounterReload = data & 0x7F;

            break;

        case 0x400A:
            // Low 8 bits
            triangle->timer = (triangle->timer & 0xFF00) | data;
            break;

        case 0x400B:
            // LLLL LTTT
            // high 3 bits (TTT)
            triangle->timer = (triangle->timer & 0x00FF) | ((data & 0b00000111) << 8);

            // LLLLL
            if (triangle->enabled) {
                uint8_t lengthIndex = (data & 0xF8) >> 3;
                triangle->lengthCounter = lengthTable[lengthIndex]; 
            }

           
            triangle->linearCounterReloadFlag = true;
            break;
    }
  
}

uint8_t APU::read(uint16_t address){
    if (address == 0x4015) {
        uint8_t status = 0;

      
        if (pulse1->lengthCounter > 0) status |= 0x01;
        if (pulse2->lengthCounter > 0) status |= 0x02;

        if (triangle->lengthCounter > 0) status |= 0x04;
        // if (noise->lengthCounter > 0)    status |= 0x08;

        return status;
    }

    
    return 0;
}

void APU::step() {
    evenCycle = !evenCycle;

    if (evenCycle) {
       
        pulse1->tick();
        pulse2->tick();

        noise->tick();
    }

    triangle->tick();

    frameCounter++;

    // 1/4 Frames: Clock  (~240Hz)
    if (frameCounter == 7457 || frameCounter == 14913 || frameCounter == 22371 || frameCounter == 29829) {
        pulse1->clockEnvelope();
        pulse2->clockEnvelope();
        triangle->clockLinearCounter();

        noise->clockEnvelope();
    }

    // 1/2 Frames: Clock  (~120Hz)
    if (frameCounter == 14913 || frameCounter == 29829) {
        pulse1->clockLengthCounter();
        pulse2->clockLengthCounter();

        pulse1->clockSweep();
        pulse2->clockSweep();

        triangle->clockLengthCounter();

        noise->clockLengthCounter();
    }


    if (frameCounter == 29830) {
        frameCounter = 0;
    }
  


    audioCycleCounter++;



    if (audioCycleCounter >= cyclesPerSample) {
        audioCycleCounter -= cyclesPerSample;

        float p1 = (float)pulse1->currentOutput;
        float p2 = (float)pulse2->currentOutput;
        float pulseOut = 0.0f;
        if (p1 + p2 > 0) {
            pulseOut = 95.88f / ((8128.0f / (p1 + p2)) + 100.0f);
        }

    
        float t = (float)triangle->currentOutput;
        float n = (float)noise->currentOutput;
        float d = 0.0f;
        float tndOut = 0.0f;

        if (t + n + d > 0) {
            tndOut = 159.79f / ((1.0f / ((t / 8227.0f) + (n / 12241.0f) + (d / 22638.0f))) + 100.0f);
        }

       
        float rawSample = pulseOut + tndOut;

       
        float filteredSample = rawSample - prevRawSample + (0.995f * prevFilteredSample);
        prevRawSample = rawSample;
        prevFilteredSample = filteredSample;

        audioBuffer.push_back(filteredSample * 0.2f);
    }


}