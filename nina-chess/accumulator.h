#pragma once
#include "nn_utils.h"
#include "utils.h"

namespace nn
{
	class alignas(64) accumulator
	{
	public:
		static constexpr int layer_size = layer_sizes[ACCUMULATOR_LAYER];
		constexpr static int calculate_config (int white_king_pos, int black_king_pos)
		{
			return 0;
		}

		accumulator()
		{

		}

		static void read_weights(float weights_from_file[][layer_size], float biases_from_file[layer_size]);

		const float* get_output(color c) const { return output[c]; }
		void apply_move(move m, piece moving_piece, piece captured_piece,const int ep_square,
			const color side_to_move, const int castling_rights, const accumulator& previous_state);
		

		void refresh_accumulator(const bitboard bitboards[][PIECE_NONE], int ep_square, int castling_rights, color side_to_move);
		
		void refresh(const float* nn_input);

		bool needs_refresh = false;
	private:
		static constexpr int max_config = 64*64;
		static constexpr int input_size = 64 * 2 * 6 + 1 + 64 + 4;

		alignas(64) static float weights[PIECE_NONE][COLOR_NONE][num_board_squares][layer_size];
		alignas(64) static float biases[layer_size];
		//alignas(64) static float side_to_move_weights[COLOR_NONE][layer_size][times_input_repeated];
		//alignas(64) static float en_passant_weights[COLOR_NONE][num_en_passant_squares][layer_size][times_input_repeated];
		alignas(64) static float castling_weights[4][layer_size];
		

		alignas(64) float output[COLOR_NONE][layer_size];

		color accumulated_side_to_move;
		int accumulated_castling_rights;
		int accumulated_en_passant_square;
		};
}
