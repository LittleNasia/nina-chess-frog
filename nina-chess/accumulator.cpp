#include "accumulator.h"

#if use_nn

alignas(64) float nn::accumulator::weights[PIECE_NONE][COLOR_NONE][num_board_squares][layer_size];
alignas(64) float nn::accumulator::biases[layer_size];
alignas(64) float nn::accumulator::castling_weights[4][layer_size];


static constexpr color own_pieces = WHITE;
static constexpr color opposite_pieces = BLACK;


void nn::accumulator::read_weights(float weights_from_file[][layer_size], float biases_from_file[layer_size])
{
	static bool weights_read = false;
	if(!weights_read)
	{
		weights_read = true;
		
		for (int output_neuron = 0; output_neuron < layer_size; output_neuron++)
		{
			int curr_input_index = 0;
			for (int current_color = 0; current_color < COLOR_NONE; current_color++)
			{
				for (int current_piece = 0; current_piece < PIECE_NONE; current_piece++)
				{
					for (int current_square = 0; current_square < num_board_squares; current_square++)
					{
						weights[current_piece][current_color][current_square][output_neuron]
								= weights_from_file[curr_input_index++][output_neuron];
						//std::cout << weights_from_file[curr_input_index++][output_neuron] << "\n";
					}
				}
			}

			for (int curr_castling = 0; curr_castling < 4; curr_castling++)
			{
				castling_weights[curr_castling][output_neuron] = weights_from_file[curr_input_index++][output_neuron];
			}
		}
		std::memcpy(biases, biases_from_file, sizeof(biases));
	}
}

void nn::accumulator::apply_move(move m, piece moving_piece, piece captured_piece, const int ep_square,
	const color side_to_move, const int castling_rights, const accumulator& previous_state)

{
	accumulated_castling_rights = previous_state.accumulated_castling_rights;
	//accumulated_en_passant_square = previous_state.accumulated_en_passant_square;
	//accumulated_side_to_move = previous_state.accumulated_side_to_move;

	// opposite side is the side that made the move
	const color side_that_moved = opposite_side_lookup[side_to_move];
	const color opposite_side = side_to_move;



	int square_from = move_from(m);
	int square_to = move_to(m);
	int flag = move_flag(m);
	int mirrored_from = horizontal_symmetry_lookup[square_from];
	int mirrored_to = horizontal_symmetry_lookup[square_to];

	// black's figures are flipped horizontally in the input 
	// black king on e8 is identical to white king on e1
	// and so from black's perspective their e8 king "is" on e1
	// meanwhile white's e1 king "is" on e8
	// for white everything works as it should
	// that means depending on who was the side that made the move,
	// their squares_from and squares_to are mirrored
	int moving_side_square_to   = ((side_that_moved == WHITE) ? square_to : mirrored_to);
	int moving_side_square_from = ((side_that_moved == WHITE) ? square_from : mirrored_from);

	int opposite_side_square_to   = ((side_that_moved != WHITE) ? square_to : mirrored_to);
	int opposite_side_square_from = ((side_that_moved != WHITE) ? square_from : mirrored_from);

	// castling isn't handled as king moves require full refresh
	for (int output_neuron = 0; output_neuron < layer_size; output_neuron++)
	{
		output[WHITE][output_neuron] = previous_state.output[WHITE][output_neuron];
		output[BLACK][output_neuron] = previous_state.output[BLACK][output_neuron];

		piece_type moving_piece_type = get_piece_type_from_piece(moving_piece);
		color moving_piece_color = get_color_from_piece(moving_piece);
		if (m == null_move)
		{
			continue;
		}

		// remove the piece's features from square_from

		// side that moved "lost" their piece from square_from
		output[side_that_moved][output_neuron] -= 
			weights[moving_piece_type][own_pieces][moving_side_square_from][output_neuron];
		// from the opposite perspective it was the opponent who "lost" the piece
		output[opposite_side][output_neuron] -=
			weights[moving_piece_type][opposite_pieces][opposite_side_square_from][output_neuron];

		// add the features in destination, possibly promoted
		if (flag != promotion)
		{
			// for side that moved, they "gain" their own piece in the square_to
			output[side_that_moved][output_neuron] += 
				weights[moving_piece_type][own_pieces][moving_side_square_to][output_neuron];
			// likewise, for the opponent it was the opponent who gained a piece
			output[opposite_side][output_neuron] +=
				weights[moving_piece_type][opposite_pieces][opposite_side_square_to][output_neuron];
		}
		else
		{
			piece_type piece_promoted_to = piece_from_promotion_piece[move_promotion_piece(m)];

			output[side_that_moved][output_neuron] += 
				weights[piece_promoted_to][own_pieces][moving_side_square_to][output_neuron];
			output[opposite_side][output_neuron] +=
				weights[piece_promoted_to][opposite_pieces][opposite_side_square_to][output_neuron];
		}
		

		// if we captured something, remove its features
		if (captured_piece != COLOR_PIECE_NONE)
		{
			int moving_side_captured_index   = moving_side_square_to;
			int opposite_side_captured_index = opposite_side_square_to;

			int captured_piece_type = get_piece_type_from_piece(captured_piece);
			int captured_piece_color = get_color_from_piece(captured_piece);

			if (flag == en_passant)
			{
				moving_side_captured_index   += pawn_push_offset[opposite_side];
				opposite_side_captured_index += pawn_push_offset[side_that_moved];
			}
			output[side_that_moved][output_neuron] -=
				weights[captured_piece_type][opposite_pieces][moving_side_captured_index][output_neuron];

			output[opposite_side][output_neuron] -=
				weights[captured_piece_type][own_pieces][opposite_side_captured_index][output_neuron];
				
		}
		
		if (accumulated_castling_rights != castling_rights)
		{
			// we can only lose castling rights as the game progresses
			bitboard castling_rights_difference = ((bitboard)~castling_rights & accumulated_castling_rights);
			while (castling_rights_difference)
			{
				int castling_index = pop_bit(castling_rights_difference);
				output[WHITE][output_neuron] -= castling_weights[castling_index][output_neuron];

				static constexpr int black_persp_castling_lookup[4] =
				{ 2,3,0,1 };

				output[BLACK][output_neuron] -= 
					castling_weights[black_persp_castling_lookup[castling_index]][output_neuron];
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
					const color white_color_perspective = ((c == WHITE) ? own_pieces : opposite_pieces);
					const color black_color_perspective = ((c == BLACK) ? own_pieces : opposite_pieces);

					const int black_perspective_index = horizontal_symmetry_lookup[feature_index];

					output[WHITE][output_neuron] += weights[pt][white_color_perspective][feature_index][output_neuron];
					output[BLACK][output_neuron] += weights[pt][black_color_perspective][black_perspective_index][output_neuron];
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
				output[WHITE][output_neuron] += castling_weights[castling][output_neuron];
				static constexpr int black_persp_castling_lookup[4] =
				{ 2,3,0,1 };

				output[BLACK][output_neuron] +=
					castling_weights[black_persp_castling_lookup[castling]][output_neuron];
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
#endif