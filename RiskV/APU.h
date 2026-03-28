#pragma once
#include <cstdint>
#include <vector>

struct Pulse {
	bool    sweepEnabled = false;
	uint8_t sweepPeriod = 0;
	bool    sweepNegate = false;
	uint8_t sweepShift = 0;
	bool    sweepReload = false;


	uint8_t  dutyMode = 0;       // 0-3
	uint8_t  dutyStep = 0;       // 0-7
	uint16_t timer = 0;          // 11 bit
	uint16_t timerValue = 0;
	uint8_t  volume = 0;         // 0-15
	uint8_t  currentOutput = 0;

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

		if (timer < 8) {
			currentOutput = 0;
		}
		else {
			currentOutput = dutyTable[dutyMode][dutyStep] ? volume : 0;
		}
	}

};

struct Noise {
	bool    envelopeLoop = false;
	bool    constantVolume = false;
	uint8_t volume = 0;          // 0 - 15
	bool    mode = false;        // 0 = long  1 = short 
	uint8_t periodIndex = 0;     // 0 - 15
	uint16_t timer = 0;          
	uint8_t lengthCounter = 0;   
	bool    envelopeStart = false;

	void tick() {
		volume = 0;
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

	double audioCycleCounter = 0;
	bool     evenCycle = false;  

	float prevRawSample = 0.0f;
	float prevFilteredSample = 0.0f;

	Pulse* pulse1 = nullptr;
	Pulse* pulse2 = nullptr;
	Noise* noise = nullptr;

	const double audioClockAccumulator = 44100.0 / 1789773.0; 
	std::vector<float> audioBuffer;


	void write(uint16_t address, uint8_t data);

	void step();

};