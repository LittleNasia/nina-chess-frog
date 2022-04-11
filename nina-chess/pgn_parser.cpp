#include "pgn_parser.h"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <tuple>
#include <vector>

#include "bitmasks.h"
#include "board.h"
#include "game_generation.h"
#include "move_gen.h"
#include "uci.h"




inline const std::tuple<bool, int> parse_game_result(const std::string& line)
{
	bool found = false;
	int score = 0;
	if (line.find("1-0") != std::string::npos)
	{
		score = 1;
		found = true;
	}
	else if (line.find("0-1") != std::string::npos)
	{
		score = -1;
		found = true;
	}
	else if (line.find("1/2-1/2") != std::string::npos)
	{
		score = 0;
		found = true;
	}
	else if (line.find("*") != std::string::npos)
	{
		score = 0;
		found = true;
	}
	return { found, score };
}


inline move parse_chess_move(board& b, std::string& move_str)
{
	move_list_container moves;
	generate_legal_moves(moves, b);
	//std::cout << "parsing the move " << move_str << "\n";

	// get rid of all "check", "checkmate" and "captures" info from the string, it's useless
	move_str.erase(std::remove(std::begin(move_str), std::end(move_str), 'x'), std::end(move_str));
	move_str.erase(std::remove(std::begin(move_str), std::end(move_str), '#'), std::end(move_str));
	move_str.erase(std::remove(std::begin(move_str), std::end(move_str), '+'), std::end(move_str));
	// while promotion "=" might be useful, there are other ways of finding the promotion move
	move_str.erase(std::remove(std::begin(move_str), std::end(move_str), '='), std::end(move_str));

	//std::cout << "move after removing useless characters: " << move_str << "\n";

	// special case, if the string is castling, it's easy to do
	if (move_str == "O-O" || move_str == "O-O-O")
	{
		bool is_kingside = true;
		// we are castling
		// let's check the type of castling based on the length of the string
		if (move_str.length() == 3)
		{
			is_kingside = true;
		}
		else
		{
			is_kingside = false;
		}

		//std::cout << "the move turned out to be a " << (is_kingside ? "kingside castling\n" : "queenside castling\n");


		// iterate over all legal moves and look for castling
		for (int move_index = 0; move_index < moves.num_moves; move_index++)
		{
			const auto current_move = moves.moves[move_index];
			if (move_flag(current_move) == castling)
			{
				// we distinguish kingside castling from queenside castling
				// by the square_to's column
				int square_to = move_to(current_move);
				int target_column = square_col[square_to];
				// column 0 is h file, which means kingside castling
				if (target_column == 0 && is_kingside)
				{
					//std::cout << "found a move that checks out\n";
					return current_move;
				}
				else if (target_column == board_cols - 1 && !is_kingside)
				{
					//std::cout << "found a move that checks out\n";
					return current_move;
				}
			}
		}
		std::cout << "didn't find a legal move that checks out\n";
		return 0;
	}

	std::vector<move> candidate_moves;
	// another special case, promotion
	// the only time the last character will be uppercase is during a promotion
	if (isupper(move_str.back()))
	{
		//std::cout << "move is a promotion\n";
		int promotion_piece;
		if (move_str.back() == 'Q')
		{
			promotion_piece = PROMOTION_QUEEN;
		}
		else if (move_str.back() == 'R')
		{
			promotion_piece = PROMOTION_ROOK;
		}
		else if (move_str.back() == 'B')
		{
			promotion_piece = PROMOTION_BISHOP;
		}
		else
		{
			promotion_piece = PROMOTION_KNIGHT;
		}
		// remove the promotion piece for further pruning
		move_str = move_str.substr(0, move_str.length() - 1);
		// now let's get the square_to from the string
		// now that's the promoted piece is removed from the string
		// the square_to is the last two characters
		std::string square_to_str = move_str.substr(move_str.length() - 2);
		// get integer value of the square_to
		int square_to = square_index_from_square_name(square_to_str.c_str());

		//std::cout << "promoting to " << move_str.back() << "\n";
		// let's find a promotion move that promotes to this piece
		for (int move_index = 0; move_index < moves.num_moves; move_index++)
		{
			const auto current_move = moves.moves[move_index];
			if (move_flag(current_move) == promotion 
				&&
				move_to(current_move) == square_to)
			{
				int current_promotion_piece = move_promotion_piece(current_move);
				if (current_promotion_piece == promotion_piece)
				{
					//std::cout << "found a promotion move " << current_move << "\n";
					candidate_moves.push_back(current_move);
				}
			}
		}
		
	}
	else
	{
		// check which square the move is to
		// square_to is always two last characters
		std::string square_to_str = move_str.substr(move_str.length() - 2);
		// get integer value of the square_to
		int square_to = square_index_from_square_name(square_to_str.c_str());
		//std::cout << "the move is to square " << square_to << " with name " <<
		//	square_names[square_to] << "\n";

		// iterate over all legal moves and check how many moves can end on that square
		for (int move_index = 0; move_index < moves.num_moves; move_index++)
		{
			const auto current_move = moves.moves[move_index];
			if (move_to(current_move) == square_to)
			{
				candidate_moves.push_back(current_move);
			}
		}
	}
	
	// if there's just one legal move, return it
	if (candidate_moves.size() == 1)
	{
	//	std::cout << "found move that works " << candidate_moves.front() << "\n";
		return candidate_moves.front();
	}
	//std::cout << "after pruning square_to, there are " << candidate_moves.size() << " remaining moves\n";
	//std::cout << "the moves are: \n";
	for (const auto cand_move : candidate_moves)
	{
	//	std::cout << square_names[move_from(cand_move)] << square_names[move_to(cand_move)]
	//		<< " " << move_promotion_piece(cand_move) << " " << move_flag(cand_move) << "\n";
	}

	// now let's prune based on the type of the piece that's moving
	// the chess notation moves always start with the piece name if it's not a pawn
	// if the piece is a pawn, the first letter is lowercase, otherwise not
	piece_type moving_piece;
	if (islower(move_str[0]))
	{
	//	std::cout << "the move is a pawn move\n";
		moving_piece = PAWN;
	}
	else
	{
		// get_piece_from_char returns the exact piece including color
		// for example uppercase 'N' is white knight, lowercase 'p' is black pawn
		// we don't care about the color, only the piece type
		moving_piece = get_piece_type_from_piece(
			get_piece_from_char(move_str[0])
		);
	//	std::cout << "the move is not a pawn move and is a piece " <<
	//		piece_names[ get_piece_from_piece_type(moving_piece, WHITE)] << "\n";
	}

	// prepare a lambda that checks the moving piece type
	const auto piece_type_checker =
	[moving_piece, &b](const move m)
	{ 
		piece piece_at_square_from = b.get_piece_at(move_from(m));
		piece_type piece_type_at_square_from = get_piece_type_from_piece(piece_at_square_from);
		return piece_type_at_square_from != moving_piece;
	};

	// remove all candidate moves that don't satisfy the condition
	candidate_moves.erase(
		std::remove_if(std::begin(candidate_moves), std::end(candidate_moves), piece_type_checker),
		std::end(candidate_moves)
	);

	// if there's just one legal move, return it
	if (candidate_moves.size() == 1)
	{
	//	std::cout << "found move that works " << candidate_moves.front() << "\n";
		return candidate_moves.front();
	}


	// at this point two or more pieces of the same type can move to a given square
	// the move either gives us information about the row or column the move is made from
	// the only exception is a queen case where you might need to specify exactly which queen it is
	// let's check whether we have a row information or a column information

	bool checking_columns = false;
	// value holds either the column index or the row index
	int value;
	// pawns are only ever distinguished by their column
	if (moving_piece == PAWN)
	{
		// in case of pawns, first letter is always the file
		int col = get_col_from_file(move_str[0]);
		checking_columns = true;
		value = col;
	}
	else
	{
		// the information about the row/column is after the piece name
		char info = move_str[1];
		// check whether the info is a column
		if (info >= 'a' && info <= 'h')
		{
			checking_columns = true;
			value = get_col_from_file(info);
		}
		// if not a column, then it's a row
		else
		{
			value = get_row_from_rank(info);
		}
	}

	//std::cout << "we are looking for " 
	//	<< (checking_columns ? "column" : "row") 
	//	<< " with value " << value << "\n";

	// let's look for moves that satisfy the criteria
	const auto location_checker =
		[=](const move m)
	{
		int square_from = move_from(m);
		// it's the row of the col of the square, depending on needs
		int square_value;
		if (checking_columns)
		{
			square_value = square_col[square_from];
		}
		else
		{
			square_value = square_row[square_from];
		}
		return square_value != value;
	};

	// remove all candidate moves that don't satisfy the condition
	candidate_moves.erase(
		std::remove_if(std::begin(candidate_moves), std::end(candidate_moves), location_checker),
		std::end(candidate_moves)
	);

	// if there's just one legal move, return it
	if (candidate_moves.size() == 1)
	{
	//	std::cout << "found move that works " << candidate_moves.front() << "\n";
		return candidate_moves.front();
	}
	b.print_board();
	std::cout << move_str << "\n";
	std::cout << "didn't find a move\n";
	std::cout << "value is " << value << "\n";
	std::cout << "info is " << checking_columns << "\n";
	std::cout << "there are " << candidate_moves.size() << " candidate moves\n";
	std::cout << "here they all are:\n";
	for (const auto cand_move : candidate_moves)
	{
			std::cout << square_names[move_from(cand_move)] << square_names[move_to(cand_move)]
				<< " " << move_promotion_piece(cand_move) << " " << move_flag(cand_move) << "\n";
	}
	std::cout << "for reference, the move list is: \n";
	for (int move_index = 0; move_index < moves.num_moves; move_index++)
	{
		const auto current_move = moves.moves[move_index];
		std::cout << square_names[move_from(current_move)] << square_names[move_to(current_move)]
			<< " " << move_promotion_piece(current_move) << " " << move_flag(current_move) << "\n";
	}

	return 0;
}


void pgn_parser::parse_pgns(const std::string& dir)
{
	std::vector<position_entry> positions;
	std::vector<position_entry> val_data;

	std::ofstream games_file("games.bin", std::ios::app | std::ios::binary);
	std::ofstream val_data_file("valdata.bin", std::ios::app | std::ios::binary);

	for (const auto file : std::filesystem::recursive_directory_iterator(dir))
	{
		if (file.is_directory())
		{
			continue;
		}
		std::ifstream pgn_file(file.path());

		std::cout << "parsing the pgn file " << file.path() << "\n\n";
		std::string line;
		board b;

		int score;
		bool looking_for_moves = false;
		bool looking_for_comment = false;
		bool is_val_data = false;
		static rng prng;
		while (pgn_file >> line)
		{
			if (!looking_for_moves)
			{
				if (line.find("Event") != std::string::npos)
				{
					b.new_game();
					// 5% chance a game is a validation data game instead
					std::uniform_int_distribution<int> dist(1, 100);
					is_val_data = dist(prng) <= 10;
				}
				else if (line.find("Result") != std::string::npos)
				{
					pgn_file >> line;
					if (line.find("1-0") != std::string::npos)
					{
						score = 1;
					}
					else if (line.find("0-1") != std::string::npos)
					{
						score = -1;
					}
					else
					{
						score = 0;
					}
				}
				else if (line == "1.")
				{
					looking_for_moves = true;
					//std::cout << line << "\n\n";
				}
				else if (line.find("FEN") != std::string::npos)
				{
					std::string fen;
					pgn_file >> fen;
					fen += " ";
					fen = fen.substr(1);

					pgn_file >> line;
					while (line.back() != ']')
					{
						fen += line + " ";
						pgn_file >> line;
					}
					fen += line.substr(0, line.length() - 2);
					// remove the quotations at the begining and the last characters at the end
					//std::cout << fen << "\n";
					b.set_position(fen);
				}
			}
			else
			{
				// ignore comments
				
				if (line[0] == '{')
				{
					looking_for_comment = true;
				}
				if (looking_for_comment)
				{
					if (line.back() == '}')
					{
						looking_for_comment = false;
					}
					continue;
				}
				// ignore move numbers
				else if (line.back() == '.')
				{
					continue;
				}

				// the string is the move or the game result, do things with it
				// check if it's the game result
				auto [found, temp_score] = parse_game_result(line);
				if (found)
				{
					//std::cout << temp_score << " " << score << "\n";
					looking_for_moves = false;
					continue;
				}
				//if (!score)
				//{
				//	continue;
				//}
				// if the string is not the game result or anything else, it's the move
				// try to parse the move
				const auto move = parse_chess_move(b,line);
				//std::cout << "\n\n";

				auto curr_entry = game_generation::create_entry_from_pos(b);
				
				if (b.get_side_to_move() == BLACK)
				{
					curr_entry.score = -score;
				}
				else
				{
					curr_entry.score = score;
				}
				if(!is_val_data)
					positions.push_back(curr_entry);
				else
					val_data.push_back(curr_entry);
					//positions.push_back(flipped_entry);



				// check whether we occupy a significant portion of the containers
				if (positions.size() > 100000)
				{
					auto rng = std::default_random_engine{};
					std::shuffle(std::begin(positions), std::end(positions), rng);
					games_file.write((char*)positions.data(), sizeof(position_entry) * positions.size());
					positions.clear();
				}
				if (val_data.size() > 100000)
				{
					auto rng = std::default_random_engine{};
					std::shuffle(std::begin(val_data), std::end(val_data), rng);
					val_data_file.write((char*)val_data.data(), sizeof(position_entry) * val_data.size());
					val_data.clear();
				}
				b.make_move(move);
			}
		}
	}

	auto rng = std::default_random_engine{};
	std::shuffle(std::begin(positions), std::end(positions), rng);
	games_file.write((char*)positions.data(), sizeof(position_entry) * positions.size());
	std::shuffle(std::begin(val_data), std::end(val_data), rng);
	val_data_file.write((char*)val_data.data(), sizeof(position_entry)* val_data.size());
}
