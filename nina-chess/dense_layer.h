#pragma once
#include "nn_utils.h"
#include "utils.h"


namespace nn
{
	template<int in_neurons, int out_neurons, activation_function func=ACTIVATION_TANH>
	class dense_layer
	{
	public:
		static constexpr int num_input_packs = in_neurons / floats_per_register;
		alignas(64) simd_pack weights[num_input_packs][out_neurons];
		alignas(64) float biases[out_neurons];
		alignas(64) float output[out_neurons];

		dense_layer()
		{
			for (int input_pack = 0, input_neuron = 0; input_pack < num_input_packs; input_pack++, input_neuron += 4)
			{
				for (int output_neuron = 0; output_neuron < out_neurons; output_neuron++)
				{
					weights[input_pack][output_neuron].values[0] = 1 + output_neuron;
					weights[input_pack][output_neuron].values[1] = 1 + output_neuron;
					weights[input_pack][output_neuron].values[2] = 1 + output_neuron;
					weights[input_pack][output_neuron].values[3] = 1 + output_neuron;
				}
			}
			std::memset(biases, 0, sizeof(biases));
		}

		void read_weights(float weights_from_file[in_neurons][out_neurons], float biases_from_file[out_neurons])
		{
			for (int input_pack = 0, curr_input_index=0; input_pack < num_input_packs; input_pack++, curr_input_index+=4)
			{
				for (int output_neuron = 0; output_neuron < out_neurons; output_neuron++)
				{
					for (int val_in_pack = 0; val_in_pack < floats_per_register; val_in_pack++)
					{
						weights[input_pack][output_neuron].values[val_in_pack]
							= weights_from_file[curr_input_index + val_in_pack][output_neuron];
					}
				}
			}
			std::memcpy(biases, biases_from_file, sizeof(biases));
		}

		float* forward(const float* input)
		{
			std::memcpy(output, biases, sizeof(output));
			if (out_neurons == 1)
			{
				return forward_single(input);
			}
			else
			{
				return forward_multi(input);
			}
			
		}


	private:
		// in case the output layer has 4 neurons, we can optimize in a neat little way
		float * forward_multi(const float* input)
		{
			for (int input_pack = 0, input_neuron = 0; input_pack < num_input_packs; input_pack++, input_neuron += 4)
			{
				// load 4 inputs into one register
				__m128 curr_input_pack = _mm_load_ps(input + input_neuron);
				// apply activation function on input first
				if (func == ACTIVATION_RELU)
				{
					curr_input_pack = _mm_max_ps(curr_input_pack, _mm_setzero_ps());
				}
				else if (func == ACTIVATION_TANH)
				{
					curr_input_pack = _mm_tanh_ps(curr_input_pack);
				}
				
				// 4 output values are calculated in one iteration 
				static constexpr int num_output_packs = out_neurons / 4;
				for (int output_pack = 0, output_neuron = 0; output_pack < num_output_packs; output_pack++, output_neuron += 4)
				{
					// weights for each of the packs
					__m128 weight_packs[4];
					for (int weight_pack = 0; weight_pack < 4; weight_pack++)
					{
						// load the weights
						weight_packs[weight_pack] = _mm_load_ps((float*)&weights[input_pack][output_neuron + weight_pack]);
						// multiply each of the weights by the input
						weight_packs[weight_pack] = _mm_mul_ps(weight_packs[weight_pack], curr_input_pack);
					}

					// sum the weights corresponding to the first two output neurons
					__m128 first_two_sums = _mm_hadd_ps(weight_packs[0], weight_packs[1]);
					first_two_sums = _mm_hadd_ps(first_two_sums, first_two_sums);

					// identical logic for the next two thingies
					__m128 next_two_sums = _mm_hadd_ps(weight_packs[2], weight_packs[3]);
					next_two_sums = _mm_hadd_ps(next_two_sums, next_two_sums);

					// now the first vector has sums of input*weight for the first two output neurons
					// and the second vector has just that for the next two output neurons
					// if 1 is the value we need to add to the first output neuron, 2 to the second etc.
					// then the vectors look as follows:
					// first_two_sums = [ 1, 2, 1, 2]
					// second_two_sums = [ 3, 4, 3, 4]
					// we want a vector in form of:
					// [1, 2, 3, 4]
					// to which we then add current values in output, and store it where it needs to bes

					next_two_sums = _mm_movehl_ps(next_two_sums, first_two_sums);

					// now we need to add the values to the ones currently present in output
					__m128 curr_output_part = _mm_load_ps(output + output_neuron);
					next_two_sums = _mm_add_ps(next_two_sums, curr_output_part);

					// store the result
					_mm_store_ps(output + output_neuron, next_two_sums);
				}

			}
			return output;
		}
		// in case the output layer has only 1 neuron, we can't optimize it in that neat little way
		float* forward_single(const float* input)
		{
			// there is only one output neuron
			constexpr int output_neuron = 0;
			float& output_value = output[output_neuron];
			
			// we load two input packs so that two additions at once can be performed using haddps
			for (int input_pack = 0, input_neuron = 0; input_pack < num_input_packs; input_pack+=2, input_neuron += 8)
			{
				// load 4 inputs into one register twice
				__m128 first_input_pack = _mm_load_ps(input + input_neuron);
				__m128 second_input_pack = _mm_load_ps(input + input_neuron + 4);

				// apply activation function on input first
			    //first_input_pack = _mm_tanh_ps(first_input_pack);
				//second_input_pack = _mm_tanh_ps(second_input_pack);
				if (func == ACTIVATION_RELU)
				{
					first_input_pack = _mm_max_ps(first_input_pack, _mm_setzero_ps());
					second_input_pack = _mm_max_ps(second_input_pack, _mm_setzero_ps());
				}
				else if (func == ACTIVATION_TANH)
				{
					first_input_pack = _mm_tanh_ps(first_input_pack);
					second_input_pack = _mm_tanh_ps(second_input_pack);
				}

				// load weights of these packs
				__m128 first_input_pack_weights = _mm_load_ps((float*)&weights[input_pack][output_neuron]);
				__m128 second_input_pack_weights = _mm_load_ps((float*)&weights[input_pack + 1][output_neuron]);

				first_input_pack_weights = _mm_mul_ps(first_input_pack, first_input_pack_weights);
				second_input_pack_weights = _mm_mul_ps(second_input_pack, second_input_pack_weights);

				// sum all the values
				first_input_pack_weights = _mm_hadd_ps(first_input_pack_weights, second_input_pack_weights);
				first_input_pack_weights = _mm_hadd_ps(first_input_pack_weights, first_input_pack_weights);
				first_input_pack_weights = _mm_hadd_ps(first_input_pack_weights, first_input_pack_weights);

				output_value += _mm_cvtss_f32(first_input_pack_weights);
			}
			return output;
		}
	};

};