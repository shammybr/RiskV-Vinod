#pragma once

class APU {
public:

	APU();

	// pulse 1 
	uint8_t  dutyMode = 0;       // 0-3
	uint8_t  dutyStep = 0;       // 0-7
	uint16_t timer = 0;          // 11 bit
	uint16_t timerValue = 0;     
	uint8_t  volume = 0;         // 0-15
	uint8_t  currentOutput = 0;  
	bool     evenCycle = false;  


};