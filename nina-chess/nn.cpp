#include "nn.h"

#include <cmath>
#include <fstream>
#include <mutex>

std::mutex reading_weights_mutex;

namespace nn
{
#if use_nn
float accumulator_weights[input_size][accumulator::layer_size];
float accumulator_biases[accumulator::layer_size];

float first_layer_weights[accumulator::layer_size][layer_sizes[FIRST_LAYER]];
float first_layer_biases[layer_sizes[FIRST_LAYER]];

//float second_layer_weights[layer_sizes[FIRST_LAYER]][layer_sizes[SECOND_LAYER]];
//float second_layer_biases[layer_sizes[SECOND_LAYER]];

float output_layer_weights[layer_sizes[FIRST_LAYER]][layer_sizes[OUTPUT_LAYER]];
float output_layer_biases[layer_sizes[OUTPUT_LAYER]];

nn_evaluator::nn_evaluator(const std::string& filename)
{
	read_weights(filename);
}


void nn::nn_evaluator::read_weights(const std::string& filename)
{
	reading_weights_mutex.lock();
	std::ifstream weights(filename, std::ios::binary);


	weights.read((char*) accumulator_weights, sizeof(accumulator_weights));
	weights.read((char*)accumulator_biases, sizeof(accumulator_biases));
	nn::accumulator::read_weights(accumulator_weights, accumulator_biases);

	weights.read((char*)first_layer_weights, sizeof(first_layer_weights));
	weights.read((char*)first_layer_biases, sizeof(first_layer_biases));
	first_layer.read_weights(first_layer_weights, first_layer_biases);

	weights.read((char*)output_layer_weights, sizeof(output_layer_weights));
	weights.read((char*)output_layer_biases, sizeof(output_layer_biases));
	output_layer.read_weights(output_layer_weights, output_layer_biases);


	weights.close();
	reading_weights_mutex.unlock();
}

float nn::nn_evaluator::evaluate_board(const board& b)
{
	const float* accumulator_output = b.get_current_accumulator().get_output(b.get_side_to_move());

	for (int i = 0; i < 32; i++)
	{
		///std::cout << accumulator_output[i] << " ";
	}
	//std::cout << "\n";

	const float* first_layer_output = first_layer.forward(accumulator_output);

	for (int i = 0; i < 32; i++)
	{
		//std::cout << first_layer_output[i] << " ";
	}
	//std::cout << "\n\n";

	//const float* second_layer_output = second_layer.forward(first_layer_output);
	for (int i = 0; i < 32; i++)
	{
	//	std::cout << second_layer_output[i] << " ";
	}
	//std::cout << "\n\n";

	const float output = *output_layer.forward(first_layer_output);

	//std::cout << std::tanh(output) << "\n\n\n";

	return std::tanh(output);
}

#endif
}