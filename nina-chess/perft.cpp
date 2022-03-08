#include "perft.h"
#include "move_gen.h"


long long nodes = 0;
long long checks = 0;
long long castlings = 0;
long long en_passants = 0;

void print()
{
	std::cout << nodes << " " << checks << " " << castlings << " " << en_passants << "\n";
}

int perft(board& b, int depth)
{
	if (depth == 0)
	{
		nodes++;
		return 0;
	}
	
	move_list_container curr_moves;
	generate_legal_moves(curr_moves,b);
	if (depth == 1)
	{
		//b.print_board();
		//std::cout << "\n";
		/*b.print_board();
		for (int move = 0; move < curr_moves.num_moves; move++)
		{
			const auto m = curr_moves.moves[move];
			std::cout << move+1 << ": " << square_names[move_from(m)] << square_names[move_to(m)] << "\n";
		}
		int x;
		std::cin >> x;*/
	}
	for (int move_index = 0; move_index < curr_moves.num_moves; move_index++)
	{
		const move m = curr_moves.moves[move_index];
		/*int square_from = move_from(m);
		int square_to = move_to(m);
		if (square_from > 63 || square_from < 0)
		{
			std::cout << "invalid square from\n";
		}
		if (square_to > 63 || square_to < 0)
		{
			std::cout << "invalid square to\n";
		}*/

		b.make_move(m);
		//std::cout <<"p: " << square_names[move_from(curr_moves.moves[move_index])] << square_names[move_to(curr_moves.moves[move_index])] << "\n";
		perft(b, depth - 1);
		b.undo_move();
	}
	return nodes;
}