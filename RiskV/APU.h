#pragma once
#include <cstdint>
#include <vector>

struct Pulse {
    bool isPulse1 = true; 
    bool enabled = false;

    // Sweep
    bool sweepEnabled = false;
    uint8_t sweepPeriod = 0;
    bool sweepNegate = false;
    uint8_t sweepShift = 0;
    bool sweepReload = false;
    uint8_t sweepDivider = 0;

    // Envelope
    bool constantVolume = false;
    bool envelopeStart = false;
    uint8_t volume = 0;
    uint8_t envelopeVolume = 0;
    uint8_t envelopeDivider = 0;

    // Core
    uint8_t dutyMode = 0;
    uint8_t dutyStep = 0;
    uint16_t timer = 0;
    uint16_t timerValue = 0;
    uint8_t lengthCounter = 0;
    bool lengthCounterHalt = false;
    uint8_t currentOutput = 0;

    uint16_t targetPeriod() {
        uint16_t change = timer >> sweepShift;
        if (sweepNegate) {
            int32_t target = timer - change;
            if (isPulse1) target--;
            return (target < 0) ? 0 : (uint16_t)target;
        }
        else {
            return timer + change;
        }
    }

    void tick() {
        if (timerValue == 0) {
            timerValue = timer;
            dutyStep = (dutyStep - 1) & 0x07;
        }
        else {
            timerValue--;
        }

        static const uint8_t dutyTable[4][8] = {
            {0, 1, 0, 0, 0, 0, 0, 0},
            {0, 1, 1, 0, 0, 0, 0, 0},
            {0, 1, 1, 1, 1, 0, 0, 0},
            {1, 0, 0, 1, 1, 1, 1, 1}
        };

        // The absolute mute conditions
        if (lengthCounter == 0 || timer < 8 || targetPeriod() > 0x07FF) {
            currentOutput = 0;
        }
        else if (dutyTable[dutyMode][dutyStep] == 0) {
            currentOutput = 0;
        }
        else {
            currentOutput = constantVolume ? volume : envelopeVolume;
        }
    }

    void clockSweep() {
        uint16_t target = targetPeriod();

        if (sweepDivider == 0 && sweepEnabled && sweepShift > 0 && timer >= 8 && target <= 0x07FF) {
            timer = target;
        }

        if (sweepDivider == 0 || sweepReload) {
            sweepDivider = sweepPeriod;
            sweepReload = false;
        }
        else {
            sweepDivider--;
        }
    }

    void clockLengthCounter() {
        if (!lengthCounterHalt && lengthCounter > 0) {
            lengthCounter--;
        }
    }

    void clockEnvelope() {
        if (envelopeStart) {
            envelopeStart = false;
            envelopeVolume = 15;
            envelopeDivider = volume;
        }
        else {
            if (envelopeDivider > 0) {
                envelopeDivider--;
            }
            else {
                envelopeDivider = volume;
                if (envelopeVolume > 0) {
                    envelopeVolume--;
                }
                else if (lengthCounterHalt) {
                    envelopeVolume = 15;
                }
            }
        }
    }
};

struct Noise {

    uint16_t shiftRegister = 1; 
    uint16_t timerValue = 0;
    uint8_t currentOutput = 0;

	bool    envelopeLoop = false;
	bool    constantVolume = false;
    uint8_t envelopeVolume = 0;         
    uint8_t envelopeDivider = 0;
	uint8_t volume = 0;          // 0 - 15
	bool    mode = false;        // 0 = long  1 = short 
	uint8_t periodIndex = 0;     // 0 - 15
	uint16_t timer = 0;          
	uint8_t lengthCounter = 0;   
	bool    envelopeStart = false;
    bool lengthCounterHalt = false;


	void tick() {
        if (timerValue == 0) {
            timerValue = timer;


            uint8_t tapBit = mode ? ((shiftRegister >> 6) & 1) : ((shiftRegister >> 1) & 1);
            uint8_t bit0 = shiftRegister & 1;

         
            uint16_t feedback = bit0 ^ tapBit;

           
            shiftRegister >>= 1;

       
            shiftRegister |= (feedback << 14);

        }
        else {
            timerValue--;
        }

  
        if (lengthCounter == 0 || (shiftRegister & 1) == 1) {
            currentOutput = 0;
        }
        else {
           
            currentOutput = constantVolume ? volume : envelopeVolume;
        }
	}

    void clockLengthCounter() {
        if (!lengthCounterHalt && lengthCounter > 0) {
            lengthCounter--;
        }
    }

    void clockEnvelope() {
        if (envelopeStart) {
            envelopeStart = false;
            envelopeVolume = 15;
            envelopeDivider = volume;
        }
        else {
            if (envelopeDivider > 0) {
                envelopeDivider--;
            }
            else {
                envelopeDivider = volume;
                if (envelopeVolume > 0) {
                    envelopeVolume--;
                }
                else if (lengthCounterHalt) {
                    envelopeVolume = 15;
                }
            }
        }
    }
};


struct Triangle {
	bool enabled = false;

	
	uint16_t timer = 0;
	uint16_t timerValue = 0;

	
	uint8_t lengthCounter = 0;
	bool lengthCounterHalt = false; //ctrl flag

	uint8_t linearCounter = 0;
	uint8_t linearCounterReload = 0;
	bool linearCounterReloadFlag = false;


	uint8_t sequenceStep = 0;
	uint8_t currentOutput = 0;


	const uint8_t triangleSequence[32] = {
		15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
	};

	void tick() {
		if (timerValue == 0) {
			timerValue = timer;


			if (lengthCounter > 0 && linearCounter > 0 && timer >= 2) {
				sequenceStep = (sequenceStep + 1) & 0x1F; 
			}
		}
		else {
			timerValue--;
		}

		
		currentOutput = triangleSequence[sequenceStep];
	}

	void clockLengthCounter() {
		if (!lengthCounterHalt && lengthCounter > 0) {
			lengthCounter--;
		}
	}

	void clockLinearCounter() {
		if (linearCounterReloadFlag) {
			linearCounter = linearCounterReload;
		}
		else if (linearCounter > 0) {
			linearCounter--;
		}

		if (!lengthCounterHalt) {
			linearCounterReloadFlag = false;
		}
	}
};


class APU {
public:

	APU();

	
	const double cyclesPerSample = 29780.5 / 735.0;

	const uint8_t triangleSequence[32] = {
	15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
	};

	const uint8_t lengthTable[32] = {
	10, 254, 20, 2, 40, 4, 80, 6, 160, 8, 60, 10, 14, 12, 26, 14,
	12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30
	};

	double audioCycleCounter = 0;
	bool     evenCycle = false;  

	float prevRawSample = 0.0f;
	float prevFilteredSample = 0.0f;
	uint16_t frameCounter = 0;

	Pulse* pulse1 = nullptr;
	Pulse* pulse2 = nullptr;
	Noise* noise = nullptr;
	Triangle* triangle = nullptr;

	const double audioClockAccumulator = 44100.0 / 1789773.0; 
	std::vector<float> audioBuffer;


	void write(uint16_t address, uint8_t data);
	uint8_t read(uint16_t address);


	void step();

};