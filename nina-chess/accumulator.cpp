#include "accumulator.h"

alignas(64) float nn::accumulator::weights[COLOR_NONE][PIECE_NONE][COLOR_NONE][num_board_squares][layer_size][times_input_repeated];
alignas(64) float nn::accumulator::biases[layer_size];
alignas(64) float nn::accumulator::en_passant_weights[COLOR_NONE][num_en_passant_squares][layer_size][times_input_repeated];
alignas(64) float nn::accumulator::castling_weights[COLOR_NONE][4][layer_size][times_input_repeated];
alignas(64) float nn::accumulator::side_to_move_weights[COLOR_NONE][layer_size][times_input_repeated];


void nn::accumulator::read_weights(float weights_from_file[][layer_size], float biases_from_file[layer_size])
{
	static bool weights_read = false;
	if(!weights_read)
	{
		weights_read = true;
		
			for (int output_neuron = 0; output_neuron < layer_size; output_neuron++)
			{
				int curr_input_index = 0;
				for (int perspective = 0; perspective < COLOR_NONE; perspective++)
				{
					for (int current_piece = 0; current_piece < PIECE_NONE; current_piece++)
					{
						for (int current_color = 0; current_color < COLOR_NONE; current_color++)
						{
							for (int current_square = 0; current_square < num_board_squares; current_square++)
							{
								for (int repetition = 0; repetition < nn::times_input_repeated; repetition++)
								{
									weights[perspective][current_piece][current_color][current_square][output_neuron][repetition]
										= weights_from_file[curr_input_index++][output_neuron];
									
								}

								//std::cout << weights_from_file[curr_input_index++][output_neuron] << "\n";
							}
						}
					}
					for (int repetition = 0; repetition < nn::times_input_repeated; repetition++)
					{
						side_to_move_weights[perspective][output_neuron][repetition] = weights_from_file[curr_input_index++][output_neuron];
					}
					for (int ep_square = 0; ep_square < num_en_passant_squares; ep_square++)
					{
						for (int repetition = 0; repetition < nn::times_input_repeated; repetition++)
						{
							en_passant_weights[perspective][ep_square][output_neuron][repetition] = weights_from_file[curr_input_index++][output_neuron];
						}
					}

					for (int curr_castling = 0; curr_castling < 4; curr_castling++)
					{
						for (int repetition = 0; repetition < nn::times_input_repeated; repetition++)
						{
							castling_weights[perspective][curr_castling][output_neuron][repetition] = weights_from_file[curr_input_index++][output_neuron];
						}
					}
				}
			}
			
		
		
		std::memcpy(biases, biases_from_file, sizeof(biases));
	}
}

void nn::accumulator::apply_move(move m, piece moving_piece, piece captured_piece, const int ep_square,
	const color side_to_move, const int castling_rights, const accumulator& previous_state)

{
	accumulated_castling_rights = previous_state.accumulated_castling_rights;
	accumulated_en_passant_square = previous_state.accumulated_en_passant_square;
	accumulated_side_to_move = previous_state.accumulated_side_to_move;
	int square_from = move_from(m);
	int square_to = move_to(m);
	int flag = move_flag(m);

	// castling isn't handled as king moves require full refresh
	for (int output_neuron = 0; output_neuron < layer_size; output_neuron++)
	{
		output[WHITE][output_neuron] = previous_state.output[WHITE][output_neuron];
		output[BLACK][output_neuron] = previous_state.output[BLACK][output_neuron];
		color piece_color = get_color_from_piece(moving_piece);
		piece_type type = get_piece_type_from_piece(moving_piece);
		if (m == null_move)
		{
			continue;
		}

		// remove the piece's features from square_from
		for (int repetition = 0; repetition < nn::times_input_repeated; repetition++)
		{
			output[WHITE][output_neuron] -= weights[WHITE][type][piece_color][square_from][output_neuron][repetition];
			output[BLACK][output_neuron] -= weights[BLACK][type][piece_color][square_from][output_neuron][repetition];
			// add the features in destination, possibly promoted
			if (flag != promotion)
			{
				output[WHITE][output_neuron] += weights[WHITE][type][piece_color][square_to][output_neuron][repetition];
				output[BLACK][output_neuron] += weights[BLACK][type][piece_color][square_to][output_neuron][repetition];
			}
			else
			{
				piece_type piece_promoted_to = piece_from_promotion_piece[move_promotion_piece(m)];
				output[WHITE][output_neuron] += weights[WHITE][piece_promoted_to][piece_color][square_to][output_neuron][repetition];
				output[BLACK][output_neuron] += weights[BLACK][piece_promoted_to][piece_color][square_to][output_neuron][repetition];
			}
		}

		// if we captured something, remove its features
		if (captured_piece != COLOR_PIECE_NONE)
		{
			int captured_piece_index = square_to;
			int captured_piece_type = get_piece_type_from_piece(captured_piece);
			int captued_piece_color = get_color_from_piece(captured_piece);
			if (flag == en_passant)
			{
				captured_piece_index += pawn_push_offset[opposite_side_lookup[piece_color]];
			}
			for (int repetition = 0; repetition < nn::times_input_repeated; repetition++)
			{
				output[WHITE][output_neuron] -=
					weights[WHITE][captured_piece_type][captued_piece_color][captured_piece_index][output_neuron][repetition];
				output[BLACK][output_neuron] -=
					weights[BLACK][captured_piece_type][captued_piece_color][captured_piece_index][output_neuron][repetition];
			}
		}

		
		if (accumulated_castling_rights != castling_rights)
		{
			// we can only lose castling rights as the game progresses
			bitboard castling_rights_difference = ((bitboard)~castling_rights & accumulated_castling_rights);
			while (castling_rights_difference)
			{
				int castling_index = pop_bit(castling_rights_difference);
				for (int repetition = 0; repetition < nn::times_input_repeated; repetition++)
				{
					output[WHITE][output_neuron] -= castling_weights[WHITE][castling_index][output_neuron][repetition];
					output[BLACK][output_neuron] -= castling_weights[BLACK][castling_index][output_neuron][repetition];
				}
			}
			
		}
		if (accumulated_en_passant_square != ep_square)
		{
			if (accumulated_en_passant_square != no_en_passant)
			{
				int ep_square_index = en_passant_index_lookup[accumulated_en_passant_square];
				for (int repetition = 0; repetition < nn::times_input_repeated; repetition++)
				{
					output[WHITE][output_neuron] -= en_passant_weights[WHITE][ep_square_index][output_neuron][repetition];
					output[BLACK][output_neuron] -= en_passant_weights[BLACK][ep_square_index][output_neuron][repetition];
				}
			}
			if (ep_square != no_en_passant)
			{
				int ep_square_index = en_passant_index_lookup[ep_square];
				for (int repetition = 0; repetition < nn::times_input_repeated; repetition++)
				{
					output[WHITE][output_neuron] += en_passant_weights[WHITE][ep_square_index][output_neuron][repetition];
					output[BLACK][output_neuron] += en_passant_weights[BLACK][ep_square_index][output_neuron][repetition];
				}
			}
		}
	}
	accumulated_en_passant_square = ep_square;
	accumulated_castling_rights = castling_rights;
	accumulated_side_to_move = side_to_move;
}

void nn::accumulator::refresh_accumulator(const bitboard bitboards[][PIECE_NONE]
	, int ep_square, int castling_rights, color side_to_move)
{
	// get the new config based on the king bitboards
	bitboard white_kings = bitboards[WHITE][KING];
	bitboard black_kings = bitboards[BLACK][KING];
	current_config = calculate_config(pop_bit(white_kings), pop_bit(black_kings));
	int curr_feature = 0;

	std::memcpy(output[WHITE], biases, sizeof(biases));
	std::memcpy(output[BLACK], biases, sizeof(biases));
	for (color c = 0; c < COLOR_NONE; c++)
	{
		for (int pt = 0; pt < PIECE_NONE; pt++)
		{
			// get all features from bitboards
			bitboard curr_bb = bitboards[c][pt];
			while (curr_bb)
			{
				int feature_index = pop_bit(curr_bb);
				for (int output_neuron = 0; output_neuron < layer_size; output_neuron++)
				{
					for (int repetition = 0; repetition < times_input_repeated; repetition++)
					{
						output[WHITE][output_neuron] += weights[WHITE][pt][c][feature_index][output_neuron][repetition];
						output[BLACK][output_neuron] += weights[BLACK][pt][c][feature_index][output_neuron][repetition];
					}
				}
			}
		}
	}

	accumulated_castling_rights = castling_rights;
	for (int castling = 0; castling < 4; castling++)
	{
		if (castling_rights & (1ULL << castling))
		{
			for (int output_neuron = 0; output_neuron < layer_size; output_neuron++)
			{
				for (int repetition = 0; repetition < times_input_repeated; repetition++)
				{
					output[WHITE][output_neuron] += castling_weights[WHITE][castling][output_neuron][repetition];
					output[BLACK][output_neuron] += castling_weights[BLACK][castling][output_neuron][repetition];
					
				}
			}
		}
	}
	accumulated_side_to_move = side_to_move;
	for (int output_neuron = 0; output_neuron < layer_size; output_neuron++)
	{
		for (int repetition = 0; repetition < times_input_repeated; repetition++)
		{
			output[BLACK][output_neuron] += side_to_move_weights[BLACK][output_neuron][repetition];
		}
	}


	if (ep_square != no_en_passant)
	{
		int ep_square_index = en_passant_index_lookup[ep_square];
		for (int output_neuron = 0; output_neuron < layer_size; output_neuron++)
		{
			for (int repetition = 0; repetition < times_input_repeated; repetition++)
			{
				output[WHITE][output_neuron] += en_passant_weights[WHITE][ep_square_index][output_neuron][repetition];
				output[BLACK][output_neuron] += en_passant_weights[BLACK][ep_square_index][output_neuron][repetition];
				
			}
		}
	}

	accumulated_en_passant_square = ep_square;
	accumulated_castling_rights = castling_rights;
	accumulated_side_to_move = side_to_move;
}

void nn::accumulator::refresh(const float* nn_input)
{	
	/*for (int output_neuron = 0; output_neuron < layer_size; output_neuron++)
	{
		output[output_neuron] = 0;
		int index = 0;
		for (int current_piece = 0; current_piece < PIECE_NONE; current_piece++)
		{
			for (int current_color = 0; current_color < COLOR_NONE; current_color++)
			{
				for (int current_square = 0; current_square < num_board_squares; current_square++)
				{
					 output[output_neuron] += weights[current_piece][current_color][current_square][output_neuron] *
						 nn_input[index++];
				}
			}
		}
		output[output_neuron] += side_to_move_weights[output_neuron] * nn_input[index++];
		for (int current_square = 0; current_square < num_board_squares; current_square++)
		{
			output[output_neuron] += en_passant_weights[current_square][output_neuron] * nn_input[index++];
		}

		for (int castling = 0; castling < 4; castling++)
		{
			output[output_neuron] += castling_weights[castling][output_neuron] * nn_input[index++];
		}

	}*/
}
