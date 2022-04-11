#pragma once
#include <cstdint>

#include "utils.h"

class rng
{
public:
	static constexpr int random_roll_max_val = 255;

	using result_type = uint64_t;

	static result_type max();
	static result_type min();

	rng();

	uint64_t operator()();
	color random_color();
	bool random_roll(int chance);

private:
	uint64_t rng_state;
};