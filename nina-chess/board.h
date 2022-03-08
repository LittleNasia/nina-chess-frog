#pragma once
#include "utils.h"
#include "accumulator.h"


struct position_entry;

struct move_info
{
	// position hash to check for repetition
	uint64_t hash;
	// move which was made, can get all info from it basically
	// whether it's enpassant or castling all is here
	move m;
	// castling permissions before the move so we for example know
	int castling_rights;
	int en_passant_square;
	int fifty_rule;
	// captured piece if one was captured
	// if wasn't we're just gonna put back an empty piece
	piece captured_piece = COLOR_PIECE_NONE;
};


class board
{
public:
	board();
	void print_board();
	void make_move(move m);
	void make_uci_move(const std::string& move);
	void undo_move();
	void new_game();
	static void init_zoribst();
	move parse_move(const std::string& m);

	// checks if bitboards align with the board array 
	bool check_board();
	bool is_legal() const;
	bool is_drawn() const;
	bool in_check() const;


	void randomize_position();


	// does a few simple checks if a move is legal
	// may fail sometimes, but is better than generating all moves
	// and then iterating through all of them
	bool move_pseudo_legal(move m) const;

	void set_position(const std::string& fen);
	void set_position(const position_entry& entry);
	void set_frc(bool frc) { is_frc = frc; }
	
	int get_pawns_count(color c) { return popcnt(bitboards[c][PAWN]); }
	// we get the pieces minus pawns by xoring with pawns, -1 because king is also counted
	int get_pieces_count(color c) { return popcnt(color_bitboards[c] ^ bitboards[c][PAWN]) - 1; }

	

	const nn::accumulator& get_current_accumulator() const { return acc_history[ply]; }
	uint64_t get_hash() const { return hash; }
	int get_ply() const { return ply; }
	bool is_capture(move m) const { return board_array[move_to(m)] != COLOR_PIECE_NONE; }
	const piece* get_board_array() const { return board_array; }
	color get_side_to_move() const { return side_to_move; }
	int get_castling_rights() const { return castling_rights; }
	int get_en_passant_square() const { return en_passant_square; }
	piece get_piece_at(int square) const { return board_array[square]; }

	const bitboard get_occupied_bitboard() const { return occupied; }
	const bitboard get_color_bitboard(color c) const { return color_bitboards[c]; }
	const bitboard get_piece_bitboard(piece_type p) const { return piece_bitboards[p]; }
	const bitboard get_bitboard(color c, piece_type p) const { return bitboards[c][p]; }

	//float nn_input[nn::input_size];

	void prepare_input();
private:
	inline constexpr void flip_piece(int square, color c, piece_type p);
	inline uint64_t calculate_hash();

	inline static uint64_t zoribst_values[COLOR_NONE][PIECE_NONE][num_board_squares];
	inline static uint64_t zoribst_castling[ALL_CASTLING];
	inline static uint64_t zoribst_en_passant[num_board_squares];
	inline static uint64_t zoribst_side_to_move;
	
	uint64_t hash;
	color side_to_move;
	int en_passant_square;
	int castling_rights;
	int ply;
	int fifty_rule;
	bool is_frc;

	// board array for fast lookup of pieces on square
	piece board_array[board_rows * board_cols];
	// "normal" bitboards for each color and type
	bitboard bitboards[COLOR_NONE][PIECE_NONE];
	// bitboards independent of piece type
	bitboard color_bitboards[COLOR_NONE];
	// bitboards independent of color
	bitboard piece_bitboards[PIECE_NONE];
	// bitboard of all taken squares
	bitboard occupied;

	move_info move_history[max_ply];

	

#if use_nn
	nn::accumulator acc_history[max_ply];
#endif
};