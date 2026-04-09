#include "APU.h"
#include "NES.h"


APU::APU(NES* nes) {
    pulse1 = new Pulse();
    pulse1->isPulse1 = true;
    pulse2 = new Pulse();
    pulse2->isPulse1 = false;
    noise = new Noise();
    triangle = new Triangle();
    dpcm = new DPCM();

    filterHP90 = new Filter(44100.0f, 90.0f, false);
    filterHP440 = new Filter(44100.0f, 440.0f, false);
    filterLP14k = new Filter(44100.0f, 14000.0f, true);
    this->nes = nes;
}

void APU::write(uint16_t address, uint8_t data){
    switch (address) {
            case 0x4000:
           
            //  DDLC VVVV
            pulse1->dutyMode = (data & 0xC0) >> 6;
            pulse1->lengthCounterHalt = (data & 0x20) > 0; 
            pulse1->constantVolume = (data & 0x10) > 0;
            pulse1->volume = (data & 0x0F);
            if (debugTest7) {
                printf("[Cycle %llu] $4000 Write: Pulse 1 Halt Flag is now %s\n",
                    nes->currentCycles, pulse1->lengthCounterHalt ? "TRUE" : "FALSE");
            }

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

            if (debugTest7) {
                if (pulse1->enabled) {
                    printf("[Cycle %llu] $4003 Write: Pulse 1 Length is now %d\n",
                        nes->currentCycles, pulse1->lengthCounter);
                }
            }

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
            noise->lengthCounterHalt = (data & 0x20) > 0;

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

            if (noise->enabled) {
                noise->lengthCounter = (data & 0xF8) >> 3;
            }
           
            noise->envelopeStart = true;
            break;

        case 0x4015:

            if (debugTest7) {
                printf("[Cycle %llu | %s] APU::write($4015). Returning bit6=%d. Setting Delay to %d\n",
                    nes->currentCycles,
                    (nes->currentCycles % 2 == 0) ? "GET" : "PUT",
                    (data & 0x40) >> 6,
                    framIrqDelay);

                if (!pulse1->enabled) {
                    pulse1->lengthCounter = 0;
                    printf("[Cycle %llu] Pulse 1 DISABLED via $4015. Length forced to 0.\n", nes->currentCycles);
                }
            }

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

            noise->enabled = (data & 0x08) > 0;
            if (!noise->enabled) noise->lengthCounter = 0;
           
           
            dpcm->enabled = (data & 0x10) > 0;
            if (dpcm->enabled) {
                if (dpcm->currentBytesRemaining == 0) {
                  
                    dpcm->currentAddress = dpcm->sampleAddress;
                    dpcm->currentBytesRemaining = dpcm->sampleLength;
                }
            }
            else {
              
                dpcm->currentBytesRemaining = 0;
            }

            dpcm->irqPending = false;
            break;

        case 0x4017:
            // MI-- ----
            // M = Mode (Bit 7), I = IRQ Inhibit (Bit 6)
            cycleOfLast4017Write = nes->currentCycles;

            if (debugTest7) {
                printf("\n[Cycle %llu | %s] CPU Wrote %02X to $4017.\n",
                    nes->currentCycles,
                    (nes->currentCycles % 2 == 0) ? "GET" : "PUT",
                    data);
            }

      
            pending4017Value = data;


            

            if (nes->currentCycles % 2 == 1) {
                // PUT cycle -> 3 (+1)
                frameCounterResetDelay = 4;
            }
            else {
                // GET cycle -> 4 (+1)
                frameCounterResetDelay = 5;
            }

            if (data == 0x80) {
                frameCounterResetDelay--;
            }
            

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

        case 0x4010:
            // IL-- RRRR (IRQ Enable, Loop, Rate/Frequency)
            dpcm->irqEnable = (data & 0x80) > 0;
            dpcm->loop = (data & 0x40) > 0;
            dpcm->timer = dpcm->dpcmPeriodTable[data & 0x0F];

          
            if (!dpcm->irqEnable) {
                dpcm->irqPending = false;
            }

            break;

        case 0x4011:
            
            dpcm->currentOutput = data & 0x7F;
            break;

        case 0x4012:
           
            dpcm->sampleAddress = 0xC000 + (data * 64);
            break;

        case 0x4013:
           
            dpcm->sampleLength = (data * 16) + 1;
            break;
    }
  
}

uint8_t APU::read(uint16_t address){
    if (address == 0x4015) {

        uint8_t data = 0;
      
   
        if (pulse1->lengthCounter > 0)   data |= 0x01;
        if (pulse2->lengthCounter > 0)   data |= 0x02;
        if (triangle->lengthCounter > 0) data |= 0x04;
        if (noise->lengthCounter > 0)    data |= 0x08;

        if (dpcm->currentBytesRemaining > 0) data |= 0x10;

   
        if (frameIRQ) data |= 0x40;
        if (dpcm->irqPending) data |= 0x80;


        if ((nes->currentCycles) % 2 == 0) {
            // Read on GET cycle: Takes 2 CPU cycles to reach the next GET cycle
            framIrqDelay = 2;
        }
        else {
            // Read on PUT cycle: Takes 1 CPU cycle to reach the next GET cycle
            framIrqDelay = 1;
        }

        pendingFrameIRQClear = true;

       
        if (debugTest7) {
            printf("[Cycle %llu] CPU READ $4015. Pulse1 Length: %d. Returning Data: %02X\n",
                nes->currentCycles, pulse1->lengthCounter, data);
        }


        return data;
    }


    
    return 0;
}

void APU::step(bool isGet) {
    evenCycle = !evenCycle;


    frameCounter++;


    if (pendingFrameIRQClear) {
        if (framIrqDelay > 0) {
            framIrqDelay--;
            if (debugTest7) {
                printf("  -> [Cycle %llu | %s] APU::step ticked delay down to %d\n",
                    nes->currentCycles,
                    isGet ? "GET" : "PUT",
                    framIrqDelay);
            }

            if (framIrqDelay == 0) {
                frameIRQ = false;
                pendingFrameIRQClear = false;
                if (debugTest7) {
                    printf("  ***> [Cycle %llu | %s] FLAG CLEARED! <***\n",
                        nes->currentCycles,
                        isGet ? "GET" : "PUT");
                }
            }
        }

    }


    if (frameCounterResetDelay > 0) {
        frameCounterResetDelay--;

        if (frameCounterResetDelay == 0) {

            if (debugTest7) {
                printf("[Cycle %llu] frameCounterResetDelay hit 0. Sequencer Reset! (Delta: %llu cycles)\n",
                    nes->currentCycles,
                    nes->currentCycles - cycleOfLast4017Write);
            }


         
            uint8_t data = pending4017Value;

            frameCounterMode = (data & 0x80) > 0;
            irqInhibit = (data & 0x40) > 0;

            if (irqInhibit) {
                frameIRQ = false;
                printf("[Cycle %llu] irqInhibit!. frameIRQ = false\n",
                    nes->currentCycles);
            }

            
            frameCounter = 0;

          
            if (frameCounterMode) {
                pulse1->clockEnvelope();
                pulse2->clockEnvelope();
                triangle->clockLinearCounter();
                noise->clockEnvelope();

                pulse1->clockLengthCounter();
                pulse2->clockLengthCounter();
                pulse1->clockSweep();
                pulse2->clockSweep();
                triangle->clockLengthCounter();
                noise->clockLengthCounter();
            }
        }
    }

    dpcm->tick(nes);



    if (!dpcm->hasBuffer && dpcm->currentBytesRemaining > 0 ) {
        
         

         dpcm->dmaPending = true;

    }





    if (isGet) {

        pulse1->tick();
        pulse2->tick();

        noise->tick();


    }
    if (frameCounterMode == 0) {

        // MODE 0 (4-Step Sequence) 
        // 1/4 Frames: 7457, 14913, 22371, 29829
        if (frameCounter == 7456 || frameCounter == 14912 || frameCounter == 22370 || frameCounter == 29828) {
         


            pulse1->clockEnvelope(); pulse2->clockEnvelope(); triangle->clockLinearCounter(); noise->clockEnvelope();
        }
        // 1/2 Frames: 14913, 29829
        if (frameCounter == 14912 || frameCounter == 29828) {

            if (debugTest7 && frameCounter == 14913) {
                printf("[Cycle %llu] Clocking Length Counters! Total Delta from write: %llu cycles\n",
                    nes->currentCycles,
                    nes->currentCycles - cycleOfLast4017Write);
            }
            pulse1->clockLengthCounter(); pulse2->clockLengthCounter(); triangle->clockLengthCounter(); noise->clockLengthCounter();
            pulse1->clockSweep(); pulse2->clockSweep();
        }
        if (frameCounter == 29827 || frameCounter == 29828 || frameCounter == 29829) {
            if (!irqInhibit) {
                frameIRQ = true;
            }
            else {
                // Silicon Leak
                if (frameCounter == 29827 || frameCounter == 29828) {
                    frameIRQ = true;  
                }
                else {
                    frameIRQ = false; 
                }
            }
        }

        if (frameCounter == 29830) {

            frameCounter = 0;
        }


    }
    else {
        //  MODE 1 (5-Step Sequence) 
        // 1/4 Frames: 7457, 14913, 22371, 37281
        if (frameCounter == 7457 || frameCounter == 14913 || frameCounter == 22371 || frameCounter == 37281) {
            pulse1->clockEnvelope(); pulse2->clockEnvelope(); triangle->clockLinearCounter(); noise->clockEnvelope();
        }
        // 1/2 Frames: 14913, 37281
        if (frameCounter == 14913 || frameCounter == 37281) {
            pulse1->clockLengthCounter(); pulse2->clockLengthCounter(); triangle->clockLengthCounter(); noise->clockLengthCounter();
            pulse1->clockSweep(); pulse2->clockSweep();
        }

        if (frameCounter == 37282) {
            frameCounter = 0;
        }



        }
    
    
    triangle->tick();


   





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
        float d = (float)dpcm->currentOutput;

        float tndOut = 0.0f;
        if (t + n + d > 0) {
            tndOut = 159.79f / ((1.0f / ((t / 8227.0f) + (n / 12241.0f) + (d / 22638.0f))) + 100.0f);
        }


       
  
        float rawSample = pulseOut + tndOut;

       
        float output = filterHP90->step(rawSample);
        output = filterHP440->step(output);
        output = filterLP14k->step(output);


        audioBuffer.push_back(output * 0.2f);
    }


}

void DPCM::tick(NES* nes){
    if (timerValue == 0) {
        timerValue = timer - 1;

   
        if (bitsRemaining > 0) {
            if (shiftRegister & 1) {
                if (currentOutput <= 125) currentOutput += 2;
            }
            else {
                if (currentOutput >= 2) currentOutput -= 2;
            }
            shiftRegister >>= 1;
            bitsRemaining--;
        }


        if (bitsRemaining == 0 && hasBuffer) {
            shiftRegister = sampleBuffer;
            hasBuffer = false;
            bitsRemaining = 8;
        }
    }
    else {
        timerValue--;
    }
    
}

