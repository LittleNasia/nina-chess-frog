#pragma once
#include "board.h"

// 68 bytes per position
struct position_entry
{
	// more compact encoding than bitboards
	// could be more compact but I cba
	piece board_array[num_board_squares];
	// the flags structure is as follows:
	// bits 0-6 - en_passant_square
	// bits 7-10 - castling rights
	// bit 11 - side_to_move
	uint16_t flags;
	uint16_t score;
	static const uint16_t create_flag(int en_passant_square, int castling_rights, color side_to_move)
	{
		return (((uint16_t)en_passant_square)&63) | (castling_rights << 6) | (side_to_move << 10);
	}
};



namespace game_generation
{

	inline position_entry create_entry_from_pos(const board& b)
	{
		position_entry position;
		std::memcpy(position.board_array, b.get_board_array(), sizeof(position.board_array));
		//std::cout << b.get_en_passant_square() << "\n";
		position.flags = position_entry::create_flag(b.get_en_passant_square(),
			b.get_castling_rights(), b.get_side_to_move());
		position.score = 1;
		return position;
	}
	inline position_entry create_vertical_symmetry_entry(const board& b)
	{
		position_entry position;
		const auto board_array = b.get_board_array();
		for (int square = 0; square < num_board_squares; square++)
		{
			position.board_array[square] = board_array[vertical_symmetry_lookup[square]];
		}
		int ep_square = -1;
		if (b.get_en_passant_square() != -1)
			ep_square = vertical_symmetry_lookup[b.get_en_passant_square()];
		position.flags = position_entry::create_flag(ep_square,
			b.get_castling_rights(), b.get_side_to_move());
		position.score = 1;
		return position;
	}
	inline position_entry create_horizontal_symmetry_entry(const board& b)
	{
		position_entry position;
		const auto board_array = b.get_board_array();
		for (int square = 0; square < num_board_squares; square++)
		{
			// horizontal symmetry requires flipping the color of pieces
			// and the side to move
			position.board_array[square] = get_opposite_color_piece(board_array[horizontal_symmetry_lookup[square]]);
		}
		int ep_square = -1;
		if (b.get_en_passant_square() != -1)
			ep_square = horizontal_symmetry_lookup[b.get_en_passant_square()];
		position.flags = position_entry::create_flag(ep_square,
			b.get_castling_rights(), opposite_side_lookup[b.get_side_to_move()]);
		position.score = -1;
		return position;
	}
	inline position_entry create_vertical_horizontal_symmetry_entry(const board& b)
	{
		position_entry position;
		const auto board_array = b.get_board_array();
		for (int square = 0; square < num_board_squares; square++)
		{
			// horizontal symmetry requires flipping the color of pieces
			// and the side to move
			position.board_array[square] = get_opposite_color_piece(board_array[vertical_horizontal_symmetry_lookup[square]]);
		}
		int ep_square = -1;
		if (b.get_en_passant_square() != -1)
			ep_square = vertical_horizontal_symmetry_lookup[b.get_en_passant_square()];
		position.flags = position_entry::create_flag(ep_square,
			b.get_castling_rights(), opposite_side_lookup[b.get_side_to_move()]);
		position.score = -1;
		return position;
	}
	// total number of games is this times number of threads
	static inline constexpr int games_per_thread = 1500;
	// adjudicate as draw if game ever goes this long
	static inline constexpr int max_game_length = 250;
	// min value is 0, max value is 255
	static inline constexpr int random_move_chance = 40;
	// no more random moves after this many random moves
	static inline constexpr int max_random_moves = 6;
	// no more random moves after this ply
	static inline constexpr int max_random_move_ply = 15;

	void generate_games(int num_threads);
};

