#include "game_generation.h"
#include "move_gen.h"
#include "rng.h"
#include "search.h"
#include "tbprobe.h"

#include <vector>
#include <future>
#include <fstream>
#include <algorithm>
#include <exception>
#include <random>


namespace game_generation
{
	using game = std::vector<position_entry>;

	


	std::vector<game> game_generation_worker()
	{
		rng prng;
		move_list_container move_list;
		board b;
		std::vector<game> games;
		games.reserve(games_per_thread);
		for (int game_index = 0; game_index < games_per_thread; game_index++)
		{
			if (!(game_index % (games_per_thread/20)))
			{
				std::cout << "finished " << game_index << "/" << games_per_thread << " games!\n";
			}
			game current_game;
			current_game.reserve(256);
			int score;
			int random_moves = 0;
			bool previous_was_random = false;
			while (true)
			{
				// save position to the current game
				position_entry&& current_pos = create_entry_from_pos(b);
				// score of a position is just side to move
				current_pos.score = ((b.get_side_to_move() == WHITE) ? 1 : -1);
				//chess is vertically symmetric if there's no castling
				if (!b.get_castling_rights())
				{
					position_entry&& vertical_pos = create_vertical_symmetry_entry(b);
					vertical_pos.score = ((b.get_side_to_move() == WHITE) ? 1 : -1);
					current_game.emplace_back(vertical_pos);
				}
				current_game.emplace_back(current_pos);

				move m;

				// check if game is over
				generate_legal_moves(move_list, b);
				// checkmate or stalemate
				if (move_list.num_moves == 0)
				{
					score = (b.in_check() ? ((b.get_side_to_move() == WHITE) ? -1 : 1) : 0);
					break;
				}
				else if (b.is_drawn() || (b.get_ply() == max_game_length))
				{
					score = 0;
					break;
				}

				// probe the syzygy tablebases to get the result 
				if (popcnt(b.get_occupied_bitboard()) <= TB_LARGEST)
				{
						// get the result of the side to move
						auto tb_probe = tb_probe_wdl(
							b.get_color_bitboard(WHITE),
							b.get_color_bitboard(BLACK),
							b.get_piece_bitboard(KING),
							b.get_piece_bitboard(QUEEN),
							b.get_piece_bitboard(ROOK),
							b.get_piece_bitboard(BISHOP),
							b.get_piece_bitboard(KNIGHT),
							b.get_piece_bitboard(PAWN),
							0,
							b.get_castling_rights(),
							b.get_en_passant_square(),
							b.get_side_to_move() == WHITE
						);
						// cursed wins are still "winning" positions
						if (tb_probe == TB_WIN || tb_probe == TB_CURSED_WIN)
						{
							// the score is a win, white side to move perspective
							score = ((b.get_side_to_move() == WHITE) ? 1 : -1);
							break;
						}
						else if (tb_probe == TB_LOSS || tb_probe == TB_BLESSED_LOSS)
						{
							// the score is a loss, white side to move perspective
							score = ((b.get_side_to_move() == WHITE) ? -1 : 1);
							break;
						}
						else if (tb_probe == TB_DRAW)
						{
							score = 0;
							break;
						}
				}
				
				// calculate the real chance for a random move 
				// the chance depends on a number of factors
				// ply/2 gives the current move in the game
				int current_random_move_chance = random_move_chance -
					((b.get_ply()/2) * random_move_chance_decay);

				std::uniform_int_distribution<int> dist(0, 255);
				int rng_roll = dist(prng);

				if (min_random_move_chance >= current_random_move_chance)
				{
					current_random_move_chance = min_random_move_chance;
					
				}
				// search the move
				// we make a normal move if the previous move wasn't random
				// or if the conditions are met
				
				// check if the move has to be normal
				if (
					// previous move being random means we now also move randomly
					!previous_was_random &&
					// we don't make a random move if we don't roll for it
					// we're past maximum ply for random move
					// or we made too many of these already 
					((rng_roll > current_random_move_chance) ||
						(b.get_ply() > max_random_move_ply) || 
						(random_moves > max_random_moves))
					)
				{
					int search_score;
					//determine search depth based on a few factors

					// if this is early into the game, there usually isn't a tactic
					// search is also slower and it's harder to hit higher depths
					
					// get number of pieces on the board to decide what's next
					const int num_pieces = popcnt(b.get_occupied_bitboard());
					const bool is_low_piece_count = (num_pieces <= low_piece_count_threshold);

					// there are some tactics *sometimes*, with a small probablity try to look for one
					// if there are little pieces it might be easier to look for a tactic
					// so we put a higher chance of looking for tactic in those situations

					if (dist(prng) <=
						(tactic_search_chance + low_piece_tactic_chance_increase * is_low_piece_count))
					{
						m = search::search_move(b, search_score, 4, 0, false);
					}
					else
					{
						// we're in the "not looking for tactic" territory
						
						// if there are quite a bit of pieces on the board
						// randomly decide between depth 1 and 2
						m = search::search_move(b, search_score,
							// base depth
							2,
							// chance to increase by 1
							//+ (prng() & 1)
							0, false);
					}
					
					b.make_move(m);
				}
				// random move
				else
				{
					// rng returns unsigned values, and so modulo is never negative
					int random_move_index = prng() % move_list.num_moves;
					b.make_move(move_list.moves[random_move_index]);
					random_moves++;
					// if the previous move wasn't a random move, mark it as random
					// if the previous move was random, we switch this flag off
					// this is to prevent making an infinite loop of random moves
					previous_was_random = !previous_was_random;
				}
			}
			// apply score to each position in the current game
			for (auto& position : current_game)
			{
				// neural networks are from the perspective of the current moving player
				// position.score is already the perspective (-1 or 1), apply the score to it
				position.score *= score;
			}
			b.randomize_position(true);
			games.emplace_back(std::move(current_game));
		}

		return games;
	}

	void generate_games(int num_threads, const std::string& filename)
	{
		std::vector<std::future<std::vector<game>>> thread_results(num_threads);
		std::vector<game> combined_games;
		for (int current_thread = 0; current_thread < num_threads; current_thread++)
		{
			thread_results[current_thread] = std::async(&game_generation_worker);
		}

		// combine all the games from all threads
		for (int current_thread = 0; current_thread < num_threads; current_thread++)
		{
			const auto& result = thread_results[current_thread].get();
			combined_games.insert(combined_games.end(), result.begin(), result.end());
		}
		
		std::vector<position_entry> positions;
		// put all positions in one container
		for (const auto& game : combined_games)
		{
			positions.insert(positions.end(), game.begin(), game.end());
		}
		auto rng = std::default_random_engine{};
		std::shuffle(std::begin(positions), std::end(positions), rng);
		std::ofstream file_with_positions(filename, std::ios::binary | std::ios::app);
		file_with_positions.write((char*)positions.data(), sizeof(position_entry) * positions.size());
	}
}