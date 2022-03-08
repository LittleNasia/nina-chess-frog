#pragma once
#include "utils.h"
#include "board.h"
#include "evaluate.h"
#include "tt.h"


#include <chrono>

namespace search
{
	struct search_info
	{
		std::chrono::steady_clock::time_point start_search_timestamp;
		int64_t time_for_search;
		bool interrupted;
		int ply;
		int64_t nodes;
		move killer_moves[2][256];
		int history_heuristic[COLOR_PIECE_NONE][num_board_squares];
		tt* transposition_table;
	};
	inline thread_local tt transposition_table;
	inline constexpr int value_inf = 16384;
	inline constexpr int value_mate = value_inf >>1;
	inline constexpr int value_mated = -value_mate;
	inline constexpr int max_eval_value = value_mate >> 1;

	inline constexpr int null_move_reduction = 3;

	inline constexpr int reverse_futility_margin = 130;

	// maximum possible 64-bit signed integer
	inline constexpr long long time_inf = (((uint64_t)-1) >> 2);

	move search_move(board& b, value& score, int depth, long long time, bool print = true);
};

