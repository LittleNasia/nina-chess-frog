#include "game_generation.h"
#include "move_gen.h"
#include "search.h"

#include <vector>
#include <future>
#include <fstream>

#include <algorithm>
#include <random>


namespace game_generation
{
	using game = std::vector<position_entry>;

	


	std::vector<game> game_generation_worker()
	{
		move_list_container move_list;
		board b;
		std::vector<game> games;
		for (int game_index = 0; game_index < games_per_thread; game_index++)
		{
			if (!(game_index % (games_per_thread/100)))
			{
				std::cout << "finished " << game_index << "/" << games_per_thread << " games!\n";
			}
			game current_game;
			int score;
			int random_moves = 0;
			bool previous_was_random = false;
			while (true)
			{
				// save position to the current game
				position_entry&& current_pos = create_entry_from_pos(b);
				position_entry&& horizontal_pos = create_horizontal_symmetry_entry(b);
				//position_entry&& vertical_pos = create_vertical_symmetry_entry(b);
				//position_entry&& vertical_horizontal_pos = create_vertical_horizontal_symmetry_entry(b);
				current_game.emplace_back(current_pos);
				current_game.emplace_back(horizontal_pos);
				//current_game.emplace_back(vertical_pos);
				//current_game.emplace_back(vertical_horizontal_pos);
				move m;
				// check if game is over
				generate_legal_moves(move_list, b);
				if (move_list.num_moves == 0)
				{
					score = (b.in_check() ? ((b.get_side_to_move() == WHITE) ? -1 : 1) : 0);
					//b.print_board();
					break;
				}
				else if (b.is_drawn() || (b.get_ply() == max_game_length))
				{
					score = 0;
					break;
				}
				
				
				// search the move
				// we make a normal move if the previous move wasn't random
				// or if the conditions are met
				if (!previous_was_random &&
					(((rng::xorshift64()&255)>random_move_chance) || (b.get_ply() > max_random_move_ply) || (random_moves >= max_random_moves)))
				{
					int score;
					//determine search depth based on a few factors

					// if this is early into the game, there usually isn't a tactic
					// search is also slower and it's harder to hit higher depths

					// there are some tactics *sometimes*, with a small probablity try to look for one
					if ((rng::xorshift64() & 255) < 16)
					{
						m = search::search_move(b, score, 10, 15, false);
					}
					else
					{
						// we're in the "not looking for tactic" territory
						// get number of pieces on the board to decide what's next
						const int num_pieces = popcnt(b.get_occupied_bitboard());
						// if there are quite a bit of pieces on the board
						// we randomly decide between depth 1 and 2
						if (num_pieces > 9)
						{
							if ((rng::xorshift64() & 1))
							{
								m = search::search_move(b, score, 10, 15, false);
								
							}
							else
							{
								m = search::search_move(b, score, 10, 15, false);
							}
						}
						else
						{
							if ((rng::xorshift64() & 1))
							{
								m = search::search_move(b, score, 10, 15, false);
							}
							else
							{
								m = search::search_move(b, score, 10, 15, false);
							}
						}
					}
					
					b.make_move(m);
				}
				// random move
				else
				{
					// rng returns unsigned values, and so modulo is never negative
					int random_move_index = rng::xorshift64() % move_list.num_moves;
					b.make_move(move_list.moves[random_move_index]);
					random_moves++;
					// if the previous move wasn't a random move, mark it as random
					// if the previous move was random, we switch this flag off
					// this is to prevent making an infinite loop of random moves
					previous_was_random = !previous_was_random;
				}
			}
			//std::cout << score << "\n";
			// apply score to each position in the current game
			for (auto& position : current_game)
			{
				position.score *= score;
			}
			b.new_game();
			if (!b.check_board())
			{
				b.print_board();
			}
			games.emplace_back(std::move(current_game));
		}

		return games;
	}

	int remaining_val_data_positions = 256000;

	void generate_games(int num_threads)

	{
		std::vector<std::future<std::vector<game>>> thread_results(num_threads);
		std::vector<game> combined_games;
		for (int current_thread = 0; current_thread < num_threads; current_thread++)
		{
			thread_results[current_thread] = std::async(&game_generation_worker);
		}

		for (int current_thread = 0; current_thread < num_threads; current_thread++)
		{
			const auto& result = thread_results[current_thread].get();
			combined_games.insert(combined_games.end(), result.begin(), result.end());
		}
		
		std::vector<position_entry> positions;
		for (const auto& game : combined_games)
		{
			positions.insert(positions.end(), game.begin(), game.end());
		}

		auto rng = std::default_random_engine{};
		std::shuffle(std::begin(positions), std::end(positions), rng);
		//remaining_val_data_positions -= positions.size();
		if (remaining_val_data_positions < 0)
		{
			remaining_val_data_positions = 0;
		}
		std::ofstream file_with_positions("games.bin", std::ios::binary | std::ios::app);
		file_with_positions.write((char*)positions.data(), sizeof(position_entry) * positions.size());
		file_with_positions.close();

	}
}