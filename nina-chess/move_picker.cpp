#include "move_picker.h"

inline constexpr int piece_values[COLOR_PIECE_NONE + 1] =
{
	// kings
	0,0,
	// queens
	9,9,
	// rooks 
	5,5,
	// bishops
	4,4,
	// knights
	3,3,
	// pawns
	1,1,
	// piece_none
	0
};

move_picker::move_picker(const move_list_container& move_list, const tt_entry& pos_entry, const board& b, const search::search_info& si) : moves(move_list)
{
	move pv_move = pos_entry.move;
	for (int current_move_index = 0; current_move_index < move_list.num_moves; current_move_index++)
	{
		const move curr_move = move_list.moves[current_move_index];
		// pv move always first 
		if (curr_move == pv_move)
		{
			moves_weights[current_move_index] = pv_move_weight;
			continue;
		}
		// capturing more valuable piece with less valuable piece next
		piece captured_piece = b.get_piece_at(move_to(curr_move));
		piece moving_piece = b.get_piece_at(move_to(curr_move));

		// bigger weight if we capture lower piece with bigger piece
		moves_weights[current_move_index] = piece_values[moving_piece] - piece_values[captured_piece] + ((captured_piece!=COLOR_PIECE_NONE)?1:0);
		moves_weights[current_move_index] *= 1000;

		if (curr_move == si.killer_moves[0][si.ply] ||
			curr_move == si.killer_moves[1][si.ply])
		{
			moves_weights[current_move_index] += 900;
		}
		else
		{
			moves_weights[current_move_index] += si.history_heuristic[b.get_piece_at(move_from(curr_move))][move_to(curr_move)];
		}
		
	}
}


move move_picker::next_move()
{
	move best_move = moves.moves[0];
	int best_weight = moves_weights[0];
	int best_move_index = 0;
	for (int current_move_index = 0; current_move_index < moves.num_moves; current_move_index++)
	{
		if (moves_weights[current_move_index] > best_weight)
		{
			best_weight = moves_weights[current_move_index];
			best_move = moves.moves[current_move_index];
			best_move_index = current_move_index;
		}
	}
	if (best_weight == move_visited_weight)
	{
		return 0;
	}
	// mark the move as visited
	moves_weights[best_move_index] = move_visited_weight;
	return best_move;
}