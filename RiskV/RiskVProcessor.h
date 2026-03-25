#pragma once
#include <vector>


class RiskVProcessor {

private:
	std::vector<uint8_t> memory;
	uint32_t reg[32] = {};

public:
	RiskVProcessor(uint32_t memorySize);



}; 
