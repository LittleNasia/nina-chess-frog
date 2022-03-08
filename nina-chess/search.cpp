#include "move_picker.h"
#include "move_gen.h"
#include "evaluate.h"



namespace search
{


// function is only to be called if the hash of the stored position matches
// returns value_inf if can't return a value, otherwise value 
inline constexpr int probeEntry(const tt_entry& entry, const int depth, const int alpha, const int beta)
{
	if (entry.depth >= depth)
	{
		//the score is exact, we can just return it 
		if (entry.flag == tt_entry::flag_exact)
		{
			return entry.score;
		}
		else if ((entry.flag == tt_entry::flag_alpha)
			&& (entry.score <= alpha))
		{
			return alpha;
		}
		else if ((entry.flag == tt_entry::flag_beta)
			&& (entry.score >= beta))
		{
			return beta;
		}
	}
	return value_inf;
}
constexpr int root_node = 0;
constexpr int normal_node = 1;

template<int NodeType>
value search(board& b, int alpha, int beta, int depth, search_info& si, bool do_null_move = true)
{
	si.nodes++;
	if ((si.nodes & 0xFFF) == 0xFFF)
	{
		// get the time count
		auto curr_time = std::chrono::steady_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>
			(curr_time - si.start_search_timestamp).count();
		if (duration >= si.time_for_search)
		{
			si.interrupted = true;
		}
		
	}

	if ((b.is_drawn() && (NodeType != root_node)))
	{
		return 0;
	}
	// check if we are in a leaf node
	if (depth <= 0)
	{
		return evaluate(b);
	}

	move best_move;
	value best_score = -value_inf;
	uint64_t pos_key = b.get_hash();
	// by default we haven't beaten alpha
	// if we beat it just once, the value will be exact
	// if we get a beta cutoff, it will be flag_beta
	int move_flag = tt_entry::flag_alpha;

	// try to get info from the transposition table
	bool found = false;
	const auto& position_entry = si.transposition_table->get(pos_key, found);

	// check if we can return the value from the position we just found
	if (found && (probeEntry(position_entry, depth, alpha, beta) != value_inf))
	{
		return probeEntry(position_entry, depth, alpha, beta);
	}

	// generate all moves
	move_list_container available_moves;
	generate_legal_moves(available_moves, b);

	// see if the game is over
	// the game is over if a player can't move
	if (available_moves.num_moves == 0)
	{
		return  (b.in_check() ? -10000 + si.ply : 0); //; ;//evaluate(b);// 
	}

	// null move pruning
	/*if ((do_null_move) && (depth > 2) && NodeType != root_node)
	{
		b.make_move(null_move);
		value score = -search<normal_node>(b, -beta, -beta+1, depth - 1 - null_move_reduction, si, false);
		b.undo_move();
		if (score >= beta)
		{
			return beta;
		}
	}*/

	//value static_eval = evaluate(b);
	// reverse futility pruning
	/*if (
		NodeType != root_node
		&& (depth <= 6)
		&& ((static_eval - reverse_futility_margin * depth) >= beta)
		&& (std::abs(beta) < (value_mate - max_ply))
		)
	{
		return static_eval;
	}*/
		
	
	
	// move picker object for sweet move ordering (cute)
	move_picker mp(available_moves, position_entry, b,si);
	// iterate over all moves since the game is not over
	int score;
	move current_move;
	bool searching_pv = true;
	while(current_move = mp.next_move())
	{
		const move m = current_move;
		int extension = 0;
		// play and search the move 
		b.make_move(m);
		extension += b.in_check();

		si.ply++;
		// principal variation search
		// the PV needs exact value, and so the windows are full
		if (searching_pv)
		{
			score = -search<normal_node>(b, -beta, -alpha, depth - 1 + extension, si);
		}
		else
		{
			// assumption that since we already found the PV, this move will be worse
			// we do a test that checks whether the move will actually be worse
			score = -search<normal_node>(b, -alpha-1, -alpha, depth - 1 + extension, si);
			//we have to research in case our assumption was incorrect
			if (score > alpha && score < beta)
			{
				score = -search<normal_node>(b, -beta, -alpha, depth - 1 + extension, si);
			}
		}
		//score = -search<normal_node>(b, -beta, -alpha, depth - 1 + extension, si);
		si.ply--;
		b.undo_move();

		if (si.interrupted)
		{
			return 0;
		}

		// keeping info about best score, so that we can get the best move in a position
		if (score > best_score)
		{
			best_move = m;
			best_score = score;

			// if we beat alpha, many funny things happen
			if (score > alpha)
			{
				searching_pv = false;
				// let's check for a cutoff
				if (score >= beta)
				{
					si.killer_moves[1][si.ply] = si.killer_moves[0][si.ply];
					si.killer_moves[0][si.ply] = m;
					si.transposition_table->store({ pos_key, best_move, beta, depth, tt_entry::flag_beta }, NodeType == root_node);
					return beta;
				}
				// we beat alpha and didn't cutoff, and so the score is exact and the node will be searched further
				move_flag = tt_entry::flag_exact;
				alpha = score;

				si.history_heuristic[b.get_piece_at(move_from(m))][move_to(m)] += depth;
			}
		}
	}
	si.transposition_table->store({ pos_key, best_move, alpha, depth, move_flag }, NodeType == root_node);
	return alpha;
}

void print_bestmoves(const tt* transposition_table, uint64_t root_pos_key, board& b)
{
	// we'll be making moves to compare the hash
	// we'll need to undo those moves afterwards
	int move_count = 0;
	root_pos_key = b.get_hash();
	
	
	bool found = false;
	std::cout << "score cp " << transposition_table->get(root_pos_key, found).score << " ";
	std::cout << "depth " << transposition_table->get(root_pos_key, found).depth << " ";
	std::cout << "pv ";
	int index = 0;
	while (true)
	{
		bool found = false;
		uint64_t hash = b.get_hash();
		const auto& tt_entry = transposition_table->get(hash, found);
		if (found)
		{
			move_count++;
			move m = tt_entry.move;
			std::cout << square_names[move_from(m)] << square_names[move_to(m)] << " ";
			b.make_move(m);
		}
		else
		{
			break;
		}
		if (b.is_drawn())
		{
			break;
		}
	}
	for (int undo_move = 0; undo_move < move_count; undo_move++)
	{
		b.undo_move();
	}
}




move search_move(board& b, value& score, int depth, long long time,bool print)
{
	int64_t time_for_move = (time ? time * 0.2 : time_inf);
	
	search_info si;
	si.start_search_timestamp = std::chrono::steady_clock::now();
	si.time_for_search = time_for_move;
	si.transposition_table = &transposition_table;
	si.interrupted = false;
	si.nodes = 0;
	std::memset(si.killer_moves, 0, sizeof(si.killer_moves));
	std::memset(si.history_heuristic, 0, sizeof(si.history_heuristic));
	// iterative deepening
	for (int curr_depth = 1; curr_depth <= depth; curr_depth++)
	{
		if (curr_depth == 1)
		{
			si.time_for_search = time_inf;
		}
		else
		{
			si.time_for_search = time_for_move;
		}
		si.ply = 0;
		value alpha = -value_inf;
		value beta = value_inf;
		auto search_start = std::chrono::steady_clock::now();
		//b.print_board();
		search<root_node>(b, alpha, beta, curr_depth, si);
		if (si.interrupted)
		{
			break;
		}
		auto search_stop = std::chrono::steady_clock::now();
		// calculate the duration and nodes per second 
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(search_stop - search_start);
		if (print)
		{
			std::cout << "info ";
			std::cout << "nodes " << si.nodes << " ";
			std::cout << "time " << duration.count() << " ";

			// find best move from the transposition table
			uint64_t root_pos_key = b.get_hash();
			//
			print_bestmoves(si.transposition_table, root_pos_key, b);
			//b.print_board();
			//std::cout << "--------------\n";
			std::cout << std::endl;
		}
	}

	bool found = false;
	const auto root_entry = si.transposition_table->get(b.get_hash(), found);
	move best_move = root_entry.move;
	score = root_entry.score;
	if (print)
	{
		std::cout << "bestmove " << square_names[move_from(best_move)] << square_names[move_to(best_move)];
		if (move_flag(best_move) == promotion)
		{
			int promotion_piece = move_promotion_piece(best_move);
			std::cout << promotion_piece_to_char[promotion_piece];
		}

		std::cout << std::endl;
	}
	
	return best_move;
}


}