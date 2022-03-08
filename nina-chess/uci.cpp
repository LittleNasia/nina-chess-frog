#include "uci.h"

#include "board.h"
#include "move_gen.h"
#include "search.h"

#include <iostream>
#include <sstream>
#include <string>


void go(std::istringstream& input_stream, board& root_pos)
{
	std::string token;

	int depth = 256;
	long long time = 0;


	while (input_stream >> token)
	{
		if (token == "depth")
			input_stream >> depth;
		else if((token == "wtime") && (root_pos.get_side_to_move() == WHITE))
			input_stream >> time;
		else if ((token == "btime") && (root_pos.get_side_to_move() == BLACK))
			input_stream >> time;
	}
	value eval;
	std::cout << "time " << time << "\n";
	search::search_move(root_pos, eval, depth, time);
}

void position(std::istringstream& input_stream, board& root_pos)
{
	std::string token;

	// startpos or fen
	input_stream >> token;
	if (token == "startpos")
	{
		root_pos.new_game();
		// fen thingie ends whenever "moves" is encountered
		// make sure this also ends whenever "moves" is encountered
		input_stream >> token;
	}
	// fen has weird spaces in it so need a weird loop
	else if (token == "fen")
	{
		std::string fen;
		while (true)
		{
			input_stream >> token;
			if (token == "moves" || !input_stream)
			{
				break;
			}
			fen += token;
			fen += " ";
		}
		root_pos.set_position(fen);
	}

	// make all moves now
	while (true)
	{
		input_stream >> token;
		if (!input_stream)
		{
			break;
		}
		root_pos.make_uci_move(token);
	}
}

void uci::loop()
{
	board root_pos;
	
	while (true)
	{
		std::string cmd;
		std::getline(std::cin, cmd);
		std::istringstream input_stream(cmd);
		std::string token;
		input_stream >> token;

		if (token == "go")
		{
			go(input_stream, root_pos);
		}
		else if (token == "position")
		{
			position(input_stream, root_pos);
		}
		else if (token == "isready")
		{
			std::cout << "readyok" << std::endl;
		}
		else if (token == "print")
		{
			root_pos.print_board();
		}
		else if (token == "uci")
		{
			std::cout << "id name ninka :3 " << std::endl;
			std::cout << "id author nasia " << std::endl;
			std::cout << "uciok " << std::endl;
		}
		else if (token == "legal")
		{
			move_list_container move_list;
			generate_legal_moves(move_list, root_pos);

			root_pos.print_board();
			for (int move = 0; move < move_list.num_moves; move++)
			{
				const auto m = move_list.moves[move];
				std::cout << move + 1 << ": " << square_names[move_from(m)] << square_names[move_to(m)] << " "
					<< move_flag(m) << "\n";
				move_list_container sub_moves;
			}
		}
		else if (token == "quit")
		{
			exit(0);
		}

	}
}
