#pragma once
#include "dense_layer.h"
#include "board.h"



namespace nn
{
	

#if use_nn
	class nn_evaluator
	{
	public:

		using first_layer_type = dense_layer<accumulator::layer_size, layer_sizes[FIRST_LAYER], activation_functions[FIRST_LAYER]>;
		using output_layer_type = dense_layer<accumulator::layer_size, layer_sizes[OUTPUT_LAYER], activation_functions[OUTPUT_LAYER]>;
		
		nn_evaluator(const std::string& filename);

		void read_weights(const std::string& filename);

		float evaluate_board(const board& b);


	private:
		first_layer_type first_layer;
		output_layer_type output_layer;
	};

	inline thread_local nn_evaluator network("chess.nnue");
#endif
}


