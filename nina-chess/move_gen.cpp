#include "move_gen.h"
#include "magic.h"
#include "bitmasks.h"

inline static constexpr bitboard generate_pawn_advances(bitboard pawns, bitboard occupied, color side_to_move)
{
	bitboard destinations = ((side_to_move == WHITE) ? pawns << 8 : pawns >> 8);
	bitboard legal_moves = destinations & ~occupied;
	return legal_moves;
}

inline static constexpr bitboard generate_left_pawn_captures(bitboard pawns, bitboard enemy, color side_to_move)
{
	// captures to the left
	bitboard left_captures = ((side_to_move == WHITE) ? pawns << 9 : pawns >>7);
	// right now pawns in first column will also have tried to capture and their move will be in last column
	// removing that column from the list of the captures just gets rid of that problem
	left_captures = left_captures & ~col_bitmasks[0];
	// we can only capture enemy pieces (or the en-passant square)
	left_captures &= enemy;

	return left_captures;
}

inline static constexpr bitboard generate_right_pawn_captures(bitboard pawns, bitboard enemy, color side_to_move)
{
	// captures to the right, logic is almost the same as in the other direction
	bitboard right_captures = ((side_to_move == WHITE) ? pawns << 7 : pawns >> 9);
	right_captures = right_captures & ~col_bitmasks[board_cols - 1];
	
	right_captures &= enemy;

	return right_captures;
}

// square offset is the offset between the target square, and the square from which move is made
// for example pushes forward increase the from_square index either by 8 or -8, depending on side
template<int flags>
inline static constexpr void create_pawn_moves(bitboard pawn_moves, move_list_container& move_list, int square_offset)
{
	while (pawn_moves)
	{
		int square_to = pop_bit(pawn_moves);
		int square_from = square_to - square_offset;
		if (flags == promotion)
		{
			for (int promotion_piece = PROMOTION_KNIGHT; promotion_piece < PROMOTION_NONE; promotion_piece++)
			{
				move m = construct_move(square_from, square_to, promotion_piece, flags);
				move_list.moves[move_list.num_moves++] = m;
			}
			continue;
		}
		move m = construct_move(square_from, square_to, 0, flags);
		move_list.moves[move_list.num_moves++] = m;
	}
}

inline static constexpr void create_moves(bitboard move_mask, int square_from, move_list_container& move_list)
{
	while (move_mask)
	{
		int square_to = pop_bit(move_mask);
		move m = construct_move(square_from, square_to, default_piece, no_flag);
		move_list.moves[move_list.num_moves++] = m;
	}
}


inline static void create_castling_moves(const board& b, int square_from, int castling_type , move_list_container& move_list)
{
	int side_to_move = b.get_side_to_move();
	const bitboard occupied = b.get_occupied_bitboard();

	int rook_pos			= (castling_type == QUEENSIDE_CASTLING) ? (board_cols - 1) : 0;
	int king_destination	= (castling_type == QUEENSIDE_CASTLING) ? queenside_king_dest : kingside_king_dest;
	rook_pos			+= castling_dest_offset[side_to_move];
	king_destination	+= castling_dest_offset[side_to_move];

	// can't castle if there are pieces between king and rook
	bitboard between_pieces_mask =
		bitmask_between_coords[square_col[rook_pos]][square_col[square_from]];
	bitboard kings_route_mask = bitmask_between_coords[square_col[square_from]][square_col[king_destination]];
	

	// the masks are always from white perspective
	if (side_to_move == BLACK)
	{
		kings_route_mask <<= ((board_rows - 1) * board_cols);
		between_pieces_mask <<= ((board_rows - 1) * board_cols);
	}
	kings_route_mask |= (1ULL << king_destination);
	kings_route_mask |= (1ULL << square_from);
	// if there are pieces between king and rook, leave
	if (between_pieces_mask & occupied)
	{
		return;
	}
	// check if the squares on king's route (and destination) are attacked
	while (kings_route_mask)
	{
		int square_index = pop_bit(kings_route_mask);
		if (square_is_attacked(square_index, b))
		{
			// this castling can't be done because the squares on king's route are attacked
			return;
		}
	}
	// (finally) add the move to the list
	// castling is "king takes rook", even though none of my kings does that
	move m = construct_move(square_from, rook_pos, default_piece, castling);
	move_list.moves[move_list.num_moves++] = m;
}


int generate_pseudolegal_moves(move_list_container& move_list, const board& b)
{
	move_list.num_moves = 0;

	const color side_to_move = b.get_side_to_move();
	const color opposite_color = opposite_side_lookup[side_to_move];

	const int en_passant_square = b.get_en_passant_square();
	const int castling_rights = b.get_castling_rights();

	const bitboard occupied = b.get_occupied_bitboard();
	const bitboard opposite_pieces = b.get_color_bitboard(opposite_color);
	const bitboard curr_player_pieces = b.get_color_bitboard(side_to_move);

	bitboard curr_player_kings = b.get_bitboard(side_to_move, KING);
	bitboard curr_player_pawns = b.get_bitboard(side_to_move, PAWN);
	bitboard curr_player_rooks = b.get_bitboard(side_to_move, ROOK);
	bitboard curr_player_queens = b.get_bitboard(side_to_move, QUEEN);
	bitboard curr_player_knights = b.get_bitboard(side_to_move, KNIGHT);
	bitboard curr_player_bishops = b.get_bitboard(side_to_move, BISHOP);

	// generate pawn moves
	bitboard simple_pawn_advances = generate_pawn_advances(curr_player_pawns, occupied, side_to_move);
	// special case: pawns can move up by 2 squares on their first move
	// the pawns that can do that will be on third/fifth row after the first advance (depending on side to move)
	bitboard double_advance_candidates = simple_pawn_advances & ((side_to_move == WHITE) ? row_bitmasks[2] : row_bitmasks[5]);
	// generate another push with the candidates, these will generate ep_square later
	bitboard double_advance_moves = generate_pawn_advances(double_advance_candidates, occupied, side_to_move);
	// generate pawn captures
	// they are split so it's easy to get the square_from in each case
	bitboard pawn_left_captures = generate_left_pawn_captures(curr_player_pawns, opposite_pieces, side_to_move);
	bitboard pawn_right_captures = generate_right_pawn_captures(curr_player_pawns, opposite_pieces, side_to_move);

	// en passant is also separate, so that we can set the template flag argument for the pawn function easily
	bitboard en_passant_left = 0;
	bitboard en_passant_right = 0;
	if (en_passant_square != -1)
	{
		en_passant_left = generate_left_pawn_captures(curr_player_pawns, (1ULL << en_passant_square), side_to_move);
		en_passant_right = generate_right_pawn_captures(curr_player_pawns, (1ULL << en_passant_square), side_to_move);
	}
	

	//promotion pawn moves
	bitboard promotion_forward = simple_pawn_advances & promotion_row_bitmasks[side_to_move];
	simple_pawn_advances &= ~promotion_row_bitmasks[side_to_move];;
	bitboard promotion_left_captures = pawn_left_captures & promotion_row_bitmasks[side_to_move];
	pawn_left_captures &= ~promotion_row_bitmasks[side_to_move];
	bitboard promotion_right_captures = pawn_right_captures & promotion_row_bitmasks[side_to_move];
	pawn_right_captures &= ~promotion_row_bitmasks[side_to_move];

	create_pawn_moves<no_flag>(simple_pawn_advances, move_list, pawn_push_offset[side_to_move]);
	create_pawn_moves<no_flag>(double_advance_moves, move_list, pawn_push_offset[side_to_move] * 2);

	create_pawn_moves<no_flag>(pawn_left_captures, move_list, pawn_left_capture_offset[side_to_move]);
	create_pawn_moves<no_flag>(pawn_right_captures, move_list, pawn_right_capture_offset[side_to_move]);

	create_pawn_moves<en_passant>(en_passant_left, move_list, pawn_left_capture_offset[side_to_move]);
	create_pawn_moves<en_passant>(en_passant_right, move_list, pawn_right_capture_offset[side_to_move]);

	create_pawn_moves<promotion>(promotion_forward, move_list, pawn_push_offset[side_to_move]);
	create_pawn_moves<promotion>(promotion_left_captures, move_list, pawn_left_capture_offset[side_to_move]);
	create_pawn_moves<promotion>(promotion_right_captures, move_list, pawn_right_capture_offset[side_to_move]);

	// generate bishop and rook moves using magics
	// iterate over bits in bishop and rook masks
	while (curr_player_bishops)
	{
		int curr_bishop_square = pop_bit(curr_player_bishops);
		bitboard curr_bishop_moves = generate_bishop_attacks(occupied, curr_bishop_square);
		curr_bishop_moves &= ~curr_player_pieces;
		create_moves(curr_bishop_moves, curr_bishop_square, move_list);
	}
	// same story for rooks
	while (curr_player_rooks)
	{
		int curr_rook_square = pop_bit(curr_player_rooks);
		bitboard curr_rook_moves = generate_rook_attacks(occupied, curr_rook_square);
		curr_rook_moves &= ~curr_player_pieces;
		create_moves(curr_rook_moves, curr_rook_square, move_list);
	}

	// queens are basically just bishops and rooks in one
	while (curr_player_queens)
	{
		int curr_queen_square = pop_bit(curr_player_queens);
		bitboard curr_queen_moves = generate_bishop_attacks(occupied, curr_queen_square) | generate_rook_attacks(occupied, curr_queen_square);
		curr_queen_moves &= ~curr_player_pieces;
		create_moves(curr_queen_moves, curr_queen_square, move_list);
	}

	// knights are super simple, they can just move wherever this player's pieces aren't 
	while (curr_player_knights)
	{
		int curr_knight_square = pop_bit(curr_player_knights);
		bitboard curr_knight_moves = knight_moves[curr_knight_square] & ~curr_player_pieces;
		create_moves(curr_knight_moves, curr_knight_square, move_list);
	}
	
	// generate king moves
	// as simple as knights
	// normally there'd be only one king
	// but I wanna test multiple king gamemodes in the future
	while (curr_player_kings)
	{
		int curr_king_square = pop_bit(curr_player_kings);
		bitboard curr_king_moves = king_moves[curr_king_square] & ~curr_player_pieces;
		create_moves(curr_king_moves, curr_king_square, move_list);
	}


	// super special case: castling onee-sama
	// ara ara 
	
	// check if the current player can even castle
	if (castling_rights & castling_rights_mask_lookup[side_to_move])
	{
		// we need the kings again for that 
		// only the kings in the first row can castle
		// our first row is the opponent's promotion row
		curr_player_kings = b.get_bitboard(side_to_move, KING) & promotion_row_bitmasks[opposite_color];
		// iterate over current player's kings, see if at least one of them can maybe castle
		while (curr_player_kings)
		{
			// kings need rooks to castle, find a suitable rook
			int curr_king_square = pop_bit(curr_player_kings);
			
			// let's see if queenside castle is possible
			if (queenside_castling_lookup[side_to_move] & castling_rights)
			{
				create_castling_moves(b, curr_king_square, QUEENSIDE_CASTLING, move_list);
			}
			if (kingside_castling_lookup[side_to_move] & castling_rights)
			{
				create_castling_moves(b, curr_king_square, KINGSIDE_CASTLING, move_list);
			}
		}
	}

	//return the move count
	return move_list.num_moves;
}

void generate_pseudolegal_moves_bitmasks(move_list_container& move_list, const board& b)
{
	const color side_to_move = b.get_side_to_move();
	const color opposite_color = opposite_side_lookup[side_to_move];

	const int en_passant_square = b.get_en_passant_square();
	const int castling_rights = b.get_castling_rights();

	const bitboard occupied = b.get_occupied_bitboard();
	const bitboard opposite_pieces = b.get_color_bitboard(opposite_color);
	const bitboard curr_player_pieces = b.get_color_bitboard(side_to_move);

	bitboard curr_player_kings = b.get_bitboard(side_to_move, KING);
	bitboard curr_player_pawns = b.get_bitboard(side_to_move, PAWN);
	bitboard curr_player_rooks = b.get_bitboard(side_to_move, ROOK);
	bitboard curr_player_queens = b.get_bitboard(side_to_move, QUEEN);
	bitboard curr_player_knights = b.get_bitboard(side_to_move, KNIGHT);
	bitboard curr_player_bishops = b.get_bitboard(side_to_move, BISHOP);

	// generate pawn moves
	bitboard simple_pawn_advances = generate_pawn_advances(curr_player_pawns, occupied, side_to_move);
	// special case: pawns can move up by 2 squares on their first move
	// the pawns that can do that will be on third/fifth row after the first advance (depending on side to move)
	bitboard double_advance_candidates = simple_pawn_advances & ((side_to_move == WHITE) ? row_bitmasks[2] : row_bitmasks[5]);
	// generate another push with the candidates, these will generate ep_square later
	bitboard double_advance_moves = generate_pawn_advances(double_advance_candidates, occupied, side_to_move);
	// generate pawn captures
	// they are split so it's easy to get the square_from in each case
	bitboard pawn_left_captures = generate_left_pawn_captures(curr_player_pawns, opposite_pieces, side_to_move);
	bitboard pawn_right_captures = generate_right_pawn_captures(curr_player_pawns, opposite_pieces, side_to_move);

	bitboard total_bishop_moves = 0ULL;
	while (curr_player_bishops)
	{
		int curr_bishop_square = pop_bit(curr_player_bishops);
		bitboard curr_bishop_moves = generate_bishop_attacks(occupied, curr_bishop_square);
		curr_bishop_moves &= ~curr_player_pieces;
		total_bishop_moves |= curr_bishop_moves;
	}
	// same story for rooks
	bitboard total_rook_moves = 0ULL;
	while (curr_player_rooks)
	{
		int curr_rook_square = pop_bit(curr_player_rooks);
		bitboard curr_rook_moves = generate_rook_attacks(occupied, curr_rook_square);
		curr_rook_moves &= ~curr_player_pieces;
		total_rook_moves |= curr_rook_moves;
	}

	bitboard total_queen_moves = 0ULL;
	// queens are basically just bishops and rooks in one
	while (curr_player_queens)
	{
		int curr_queen_square = pop_bit(curr_player_queens);
		bitboard curr_queen_moves = generate_bishop_attacks(occupied, curr_queen_square) | generate_rook_attacks(occupied, curr_queen_square);
		curr_queen_moves &= ~curr_player_pieces;
		total_queen_moves |= curr_queen_moves;
	}
	bitboard total_knight_moves = 0ULL;;
	// knights are super simple, they can just move wherever this player's pieces aren't 
	while (curr_player_knights)
	{
		int curr_knight_square = pop_bit(curr_player_knights);
		bitboard curr_knight_moves = knight_moves[curr_knight_square] & ~curr_player_pieces;
		total_knight_moves |= curr_knight_moves;
	}
	
	// generate king moves
	// as simple as knights
	// normally there'd be only one king
	// but I wanna test multiple king gamemodes in the future
	bitboard total_king_moves = 0ULL;
	while (curr_player_kings)
	{
		int curr_king_square = pop_bit(curr_player_kings);
		bitboard curr_king_moves = king_moves[curr_king_square] & ~curr_player_pieces;
		total_king_moves |= curr_king_moves;
	}
}

bool square_is_attacked(int square, const board& b, const color c)
{
	color side_to_move;
	// if we don't wish to check for specific color, we determine it based on two factors
	if (c == COLOR_NONE)
	{
		// if there is a piece at the square, we just check for opponents of that piece
		if (b.get_piece_at(square) != COLOR_PIECE_NONE)
		{
			// get the color of that piece
			side_to_move = b.get_piece_at(square) & 1;
		}
		// if the square is empty, we just assume it's the side_to_move that's checking it
		else
		{	
			side_to_move = b.get_side_to_move();
		}
	}
	else
	{
		// we have a color parameter we want to check
		side_to_move = c;
	}
	// we get the color of the square we're checking for being attacked
	// attacked counts only from the other perspective
	const color piece_at_color = b.get_piece_at(square) & 1;
	const color opposite_side = opposite_side_lookup[side_to_move];

	const bitboard occupied = b.get_occupied_bitboard();
	const bitboard square_bb = (1ULL << square);
	const bitboard pawn_attacks_from_square = generate_right_pawn_captures(square_bb, full_bitboard, side_to_move) | generate_left_pawn_captures(square_bb, full_bitboard, side_to_move);
	const bitboard bishop_attacks_from_square = generate_bishop_attacks(occupied, square);
	const bitboard rook_attacks_from_square = generate_rook_attacks(occupied, square);
	const bitboard knight_attacks_from_square = knight_moves[square];
	const bitboard king_attacks_from_square = king_moves[square];

	const bitboard& enemy_pawns = b.get_bitboard(opposite_side, PAWN);
	const bitboard& enemy_rooks = b.get_bitboard(opposite_side, ROOK);
	const bitboard& enemy_bishops = b.get_bitboard(opposite_side, BISHOP);
	const bitboard& enemy_queens = b.get_bitboard(opposite_side, QUEEN);
	const bitboard& enemy_kings = b.get_bitboard(opposite_side, KING);
	const bitboard& enemy_knights = b.get_bitboard(opposite_side, KNIGHT);

	// if we pretend we're a pawn, and try to attack opponent's pawns
	// that means they're also attacking us, so we check for that
	// same logic applies to all other piece types
	return (rook_attacks_from_square & enemy_rooks) ||
		((bishop_attacks_from_square | rook_attacks_from_square) & enemy_queens) ||
		(knight_attacks_from_square & enemy_knights) ||
		(bishop_attacks_from_square & enemy_bishops) ||
		(enemy_pawns & pawn_attacks_from_square) ||
		(king_attacks_from_square & enemy_kings);
}

#include <fstream>

int generate_legal_moves(move_list_container& move_list, board& b)
{
	generate_pseudolegal_moves(move_list, b);
	int move_index = 0;
	for (int pseudo_move_index = 0; pseudo_move_index < move_list.num_moves; pseudo_move_index++)
	{
		// make the move, see if it's legal, if it is then copy it otherwise not 
		move curr_pseudo_move = move_list.moves[pseudo_move_index];
		//board copy = b;
		b.make_move(curr_pseudo_move);
		/*if (!b.check_board())
		{
			std::cout << square_names[move_from(curr_pseudo_move)] << square_names[move_to(curr_pseudo_move)] 
				<< " " << move_promotion_piece(curr_pseudo_move) << " " << move_flag(curr_pseudo_move) << "\n";
  			b.print_board();
			std::ofstream board_dump("board.bin", std::ios::binary);
			board_dump.write((char*)&copy, sizeof(board));
			board_dump.close();
			copy.print_board();
		}*/
		if (b.is_legal())
		{
			move_list.moves[move_index++] = curr_pseudo_move;
		}
		b.undo_move();
	}
	move_list.num_moves = move_index;
	return move_list.num_moves;
}