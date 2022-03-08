#pragma once
#include "move_gen.h"
#include "search.h"

class move_picker
{
public:
	move_picker(const move_list_container& move_list, const tt_entry& pos, const board& b, const search::search_info& si);
	move next_move();
private:
	inline static constexpr int pv_move_weight = 10000;
	inline static constexpr int move_visited_weight = -10000;
	const move_list_container& moves;
	int moves_weights[max_moves];
};

