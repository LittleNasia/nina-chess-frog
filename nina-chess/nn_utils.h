#pragma once
#include <xmmintrin.h>

namespace nn
{
	enum
	{
		ACCUMULATOR_LAYER,
		FIRST_LAYER,
		OUTPUT_LAYER,
		LAYER_NONE
	};
	inline constexpr int times_input_repeated = 1;
	inline constexpr int input_size = ((64 * 2 * 6 + 1 + 16 + 4) * times_input_repeated) * 2;
	inline constexpr int num_layers = LAYER_NONE;
	inline constexpr int layer_sizes[num_layers] = { 32,32,1 };
	inline constexpr int floats_per_register = sizeof(__m128) / sizeof(float);

	struct simd_pack
	{
		float values[floats_per_register];
	};

#define use_nn true
}

