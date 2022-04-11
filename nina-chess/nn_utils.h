#pragma once
#include <xmmintrin.h>

namespace nn
{
	enum activation_function
	{
		ACTIVATION_TANH,
		ACTIVATION_RELU
	};
	enum
	{
		ACCUMULATOR_LAYER,
		FIRST_LAYER,
		OUTPUT_LAYER,
		LAYER_NONE
	};
	inline constexpr int moving_side_index = 0;
	inline constexpr int opposite_side_index = 1;

	inline constexpr int times_input_repeated = 1;
	inline constexpr int input_size = ((64 * 2 * 6 + 4));
	inline constexpr int num_layers = LAYER_NONE;
	inline constexpr int layer_sizes[num_layers] = { 64,64,1 };
	inline constexpr activation_function activation_functions[num_layers] = { ACTIVATION_RELU, ACTIVATION_RELU, ACTIVATION_TANH };
	inline constexpr int floats_per_register = sizeof(__m128) / sizeof(float);

	struct simd_pack
	{
		float values[floats_per_register];
	};

#define use_nn true
}

