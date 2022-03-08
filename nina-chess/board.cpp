#include "board.h"
#include "move_gen.h"
#include "bitmasks.h"
#include "nn_utils.h"
#include "game_generation.h"

#include <sstream>

constexpr uint8_t startpos_board_array[num_board_squares] = {
	4, 8, 6, 0, 2, 6, 8, 4, 10, 10, 10, 10, 10, 10, 10, 10, 12, 12,
	12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
	12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
	12, 12, 11, 11, 11, 11, 11, 11, 11, 11, 5, 9, 7, 1, 3, 7, 9, 5
};

board::board()
{
	
	
	/*bitboards[WHITE][ROOK] = 129ULL;
	bitboards[WHITE][KNIGHT] = 66ULL;
	bitboards[WHITE][BISHOP] = 36ULL;
	bitboards[WHITE][QUEEN] = 16ULL;
	bitboards[WHITE][KING] = 8ULL;

	bitboards[BLACK][KING] = 8ULL << (board_cols * 7);
	bitboards[BLACK][ROOK] = 129ULL << (board_cols * 7);
	bitboards[BLACK][KNIGHT] = 66ULL << (board_cols * 7);
	bitboards[BLACK][BISHOP] = 36ULL << (board_cols * 7);
	bitboards[BLACK][QUEEN] = 16ULL << (board_cols * 7);
	bitboards[BLACK][KING] = 8ULL << (board_cols * 7);*/

	set_position("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
	
	ply = 0;
	hash = 0;
	fifty_rule = 0;
	is_frc = 0;
	calculate_hash();
	move_history[0].hash = hash;
	for (int row = 7; row >= 0; row--)
	{
		for (int col = 7; col >= 0; col--)
		{
			bitboard square_mask = 1ULL << (row * board_cols + col);
			if (bitboards[WHITE][PAWN] & square_mask)
			{
				board_array[row * board_cols + col] = W_PAWN;
			}
			else if (bitboards[WHITE][BISHOP] & square_mask)
			{
				board_array[row * board_cols + col] = W_BISHOP;
			}
			else if (bitboards[WHITE][KNIGHT] & square_mask)
			{
				board_array[row * board_cols + col] = W_KNIGHT;
			}
			else if (bitboards[WHITE][ROOK] & square_mask)
			{
				board_array[row * board_cols + col] = W_ROOK;
			}
			else if (bitboards[WHITE][QUEEN] & square_mask)
			{
				board_array[row * board_cols + col] = W_QUEEN;
			}
			else if (bitboards[WHITE][KING] & square_mask)
			{
				board_array[row * board_cols + col] = W_KING;
			}
			else if (bitboards[BLACK][PAWN] & square_mask)
			{
				board_array[row * board_cols + col] = B_PAWN;
			}
			else if (bitboards[BLACK][BISHOP] & square_mask)
			{
				board_array[row * board_cols + col] = B_BISHOP;
			}
			else if (bitboards[BLACK][KNIGHT] & square_mask)
			{
				board_array[row * board_cols + col] = B_KNIGHT;
			}
			else if (bitboards[BLACK][ROOK] & square_mask)
			{
				board_array[row * board_cols + col] = B_ROOK;
			}
			else if (bitboards[BLACK][QUEEN] & square_mask)
			{
				board_array[row * board_cols + col] = B_QUEEN;
			}
			else if (bitboards[BLACK][KING] & square_mask)
			{
				board_array[row * board_cols + col] = B_KING;
			}
			else
			{
				board_array[row * board_cols + col] = COLOR_PIECE_NONE;
			}
		}
	}

	
}

bool board::is_drawn() const
{
	// look for threefold repetition (or twofold in this case)
	for (int curr_ply = ply - 1; curr_ply >= 0; curr_ply--)
	{
		if (move_history[curr_ply].hash == hash)
		{
			return true;
		}
		// if the move reset 50 move rule, it can't be undone and so it can't be identical 
		if (move_history[curr_ply].fifty_rule == 0)
		{
			break;
		}
	}
	// check 50mr
	if (fifty_rule == 100)
	{
		return true;
	}
	return false;
}

void board::set_position(const std::string& fen)
{
	//std::cout << "setting a fen " << fen << "\n";
	int row = 7;
	int index = 0;
	std::memset(bitboards, 0, sizeof(bitboards));
	std::memset(piece_bitboards, 0, sizeof(piece_bitboards));
	std::memset(color_bitboards, 0, sizeof(color_bitboards));
	occupied = 0;
	for (auto& piece : board_array)
	{
		piece = COLOR_PIECE_NONE;
	}
	int col = 7;
	for (index = 0; index< fen.length(); index++)
	{
		// "col 0" is actually on the right of the board (where square 0 is)
		
		if (fen[index] == '/')
		{
			row--;
			col = 7;
			continue;
		}
		else if (fen[index] == ' ')
		{
			// we go parse side to move and other things afterwards
			break;
		}
		piece_type curr_piece_type = PIECE_NONE;
		color piece_color = COLOR_NONE;
		// put pieces
		switch (fen[index])
		{
		case 'p':
			curr_piece_type = PAWN;
			piece_color = BLACK;
			break;
		case 'r':
			curr_piece_type = ROOK;
			piece_color = BLACK;
			break;
		case 'b':
			curr_piece_type = BISHOP;
			piece_color = BLACK;
			break;
		case 'q':
			curr_piece_type = QUEEN;
			piece_color = BLACK;
			break;
		case 'k':
			curr_piece_type = KING;
			piece_color = BLACK;
			break;
		case 'n':
			curr_piece_type = KNIGHT;
			piece_color = BLACK;
			break;
		case 'P':
			curr_piece_type = PAWN;
			piece_color = WHITE;
			break;
		case 'R':
			curr_piece_type = ROOK;
			piece_color = WHITE;
			break;
		case 'B':
			curr_piece_type = BISHOP;
			piece_color = WHITE;
			break;
		case 'Q':
			curr_piece_type = QUEEN;
			piece_color = WHITE;
			break;
		case 'K':
			curr_piece_type = KING;
			piece_color = WHITE;
			break;
		case 'N':
			curr_piece_type = KNIGHT;
			piece_color = WHITE;
			break;
		// if it's not a piece, it's a number (new rows and end of pieces was already handled)
		default:
		{
			int num_of_empty_squares = fen[index] - '0';
			for (int empty_square = 0; empty_square < num_of_empty_squares; empty_square++)
			{
				col--;
			}
			continue;
		}

		}

		// put the current piece on the board
		flip_piece(row * board_cols + col, piece_color, curr_piece_type);
		// put the current piece on the board array
		board_array[row * board_cols + col] = get_piece_from_piece_type(curr_piece_type, piece_color);
		col--;
	}

	std::stringstream ss(fen.substr(index));

	// get side to move
	char fen_side_to_move;
	ss >> fen_side_to_move;
	switch (fen_side_to_move)
	{
	case 'w':
		side_to_move = WHITE;
		break;
	case 'b':
		side_to_move = BLACK;
		break;
	default:
		std::cout << "wrong fen lmao\n";
		break;
	}

	std::string fen_castling_rights;
	ss >> fen_castling_rights;

	castling_rights = 0;
	for (int castling_index = 0; castling_index < fen_castling_rights.length(); castling_index++)
	{
		if (fen_castling_rights[castling_index] == '-')
		{
			break;
		}
		switch (fen_castling_rights[castling_index])
		{
		case 'K':
			castling_rights |= WHITE_KINGSIDE;
			break;
		case 'Q':
			castling_rights |= WHITE_QUEENSIDE;
			break;
		case 'k':
			castling_rights |= BLACK_KINGSIDE;
			break;
		case 'q':
			castling_rights |= BLACK_QUEENSIDE;
			break;
		default:
			std::cout << "fen sucks\n";
			break;
		}
	}
	//std::cout << castling_rights << "\n";

	std::string fen_en_passant_square;
	ss >> fen_en_passant_square;
	if (fen_en_passant_square == "-")
	{
		en_passant_square = no_en_passant;
	}
	else
	{
		en_passant_square = square_index_from_square_name(fen_en_passant_square.c_str());
	}
	
	

	// my cols start counting from the other direction, and so h column is actually first
	
	//std::cout << en_passant_square << "\n";

	ss >> fifty_rule;
	//std::cout << fifty_rule << "\n";

	int fen_fullmoves;
	ss >> fen_fullmoves;
	ply = 0;
	//std::cout << ply << "\n";

	//prepare_input();
	acc_history[ply].refresh_accumulator(bitboards, en_passant_square, castling_rights, side_to_move);
}

void board::set_position(const position_entry& entry)
{
	std::memset(bitboards, 0, sizeof(bitboards));
	std::memset(piece_bitboards, 0, sizeof(piece_bitboards));
	std::memset(color_bitboards, 0, sizeof(color_bitboards));
	occupied = 0;
	for (int square = 0; square < num_board_squares; square++)
	{
		piece curr_piece = entry.board_array[square];
		color curr_piece_color = get_color_from_piece(curr_piece);
		piece_type curr_piece_type = get_piece_type_from_piece(curr_piece);
		if(curr_piece != COLOR_PIECE_NONE)
			flip_piece(square, curr_piece_color, curr_piece_type);
		board_array[square] = curr_piece;
	}
	int flags = entry.flags;
	int entry_ep_square = flags & 63;
	int entry_castling = (flags & 960) >> 6;
	int entry_side_to_move = (flags & 1024) >> 10;
	en_passant_square = entry_ep_square;
	if (entry_ep_square == 63)
	{
		en_passant_square = no_en_passant;
	}
	castling_rights = entry_castling;
	side_to_move = entry_side_to_move;
	ply = 0;
#if use_nn
	acc_history[ply].refresh_accumulator(bitboards, en_passant_square, castling_rights, side_to_move);
#endif
}

void board::print_board()
{
	std::cout << "bitboards data \n";
	for (int row = 7; row >= 0; row--)
	{
		for (int col = 7; col >= 0; col--)
		{
			std::cout << "[";
			bitboard square_mask = 1ULL << (row * board_cols + col);
			if (bitboards[WHITE][PAWN] & square_mask)
			{
				std::cout << "P";
			}
			else if (bitboards[WHITE][BISHOP] & square_mask)
			{
				std::cout << "B";
			}
			else if (bitboards[WHITE][KNIGHT] & square_mask)
			{
				std::cout << "N";
			}
			else if (bitboards[WHITE][ROOK] & square_mask)
			{
				std::cout << "R";
			}
			else if (bitboards[WHITE][QUEEN] & square_mask)
			{
				std::cout << "Q";
			}
			else if (bitboards[WHITE][KING] & square_mask)
			{
				std::cout << "K";
			}
			else if (bitboards[BLACK][PAWN] & square_mask)
			{
				std::cout << "p";
			}
			else if (bitboards[BLACK][BISHOP] & square_mask)
			{
				std::cout << "b";
			}
			else if (bitboards[BLACK][KNIGHT] & square_mask)
			{
				std::cout << "n";
			}
			else if (bitboards[BLACK][ROOK] & square_mask)
			{
				std::cout << "r";
			}
			else if (bitboards[BLACK][QUEEN] & square_mask)
			{
				std::cout << "q";
			}
			else if (bitboards[BLACK][KING] & square_mask)
			{
				std::cout << "k";
			}
			else
			{
				std::cout << " ";
			}
			std::cout << "]";
		}
		std::cout << " " << row+1 << "\n";
	}
	std::cout << " a  b  c  d  e  f  g  h \n";
	std::cout << ((castling_rights & WHITE_KINGSIDE) ? "K" : "");
	std::cout << ((castling_rights & WHITE_QUEENSIDE) ? "Q" : "");
	std::cout << ((castling_rights & BLACK_KINGSIDE) ? "k" : "");
	std::cout << ((castling_rights & BLACK_QUEENSIDE) ? "q" : "");
	std::cout << "\n" << hash << "\n";

	std::cout << "board_array data \n";
	for (int row = 7; row >= 0; row--)
	{
		for (int col = 7; col >= 0; col--)
		{
			std::cout << "[";
			std::cout << piece_names[board_array[two_d_to_one_d(row, col)]];
			std::cout << "]";
		}
		std::cout << " " << row + 1 << "\n";
	}
	std::cout << " a  b  c  d  e  f  g  h \n";
	std::cout << "ep " << en_passant_square << "\n";
	std::cout << "\n";
	print_bitboard(occupied);
	std::cout << "\n\n\n";
}

bool board::check_board()
{
	for (int row = 7; row >= 0; row--)
	{
		for (int col = 7; col >= 0; col--)
		{
			const char* current_bitboard_piece;
			bitboard square_mask = 1ULL << (row * board_cols + col);
			if (bitboards[WHITE][PAWN] & square_mask)
			{
				current_bitboard_piece = "P";
			}
			else if (bitboards[WHITE][BISHOP] & square_mask)
			{
				current_bitboard_piece = "B";
			}
			else if (bitboards[WHITE][KNIGHT] & square_mask)
			{
				current_bitboard_piece = "N";
			}
			else if (bitboards[WHITE][ROOK] & square_mask)
			{
				current_bitboard_piece = "R";
			}
			else if (bitboards[WHITE][QUEEN] & square_mask)
			{
				current_bitboard_piece = "Q";
			}
			else if (bitboards[WHITE][KING] & square_mask)
			{
				current_bitboard_piece = "K";
			}
			else if (bitboards[BLACK][PAWN] & square_mask)
			{
				current_bitboard_piece = "p";
			}
			else if (bitboards[BLACK][BISHOP] & square_mask)
			{
				current_bitboard_piece = "b";
			}
			else if (bitboards[BLACK][KNIGHT] & square_mask)
			{
				current_bitboard_piece = "n";
			}
			else if (bitboards[BLACK][ROOK] & square_mask)
			{
				current_bitboard_piece = "r";
			}
			else if (bitboards[BLACK][QUEEN] & square_mask)
			{
				current_bitboard_piece = "q";
			}
			else if (bitboards[BLACK][KING] & square_mask)
			{
				current_bitboard_piece = "k";
			}
			else
			{
				current_bitboard_piece = " ";
			}
			if (piece_names[board_array[two_d_to_one_d(row, col)]][0] != current_bitboard_piece[0])
			{
				return false;
			}
			bitboard total_bitboards = 0ULL;
			for (int color = 0; color < COLOR_NONE; color++)
			{
				for (int piece = 0; piece < PIECE_NONE; piece++)
				{
					total_bitboards |= bitboards[color][piece];
				}
			}
			if (total_bitboards != occupied)
			{
				return false;
			}
		}
	}
	return true;
}

void board::prepare_input()
{
	/*int offset = 0;
	if (side_to_move == BLACK)
	{
		offset = nn::input_size / 2;
	}
	std::memset(nn_input, 0, sizeof(nn_input));
	int index = 0;
	for (int current_piece = 0; current_piece < PIECE_NONE; current_piece++)
	{
		for (int current_color = 0; current_color < COLOR_NONE; current_color++)
		{
			for (int current_square = 0; current_square < num_board_squares; current_square++)
			{
				for (int repetition = 0; repetition < nn::times_input_repeated; repetition++)
				{
					nn_input[index++ + offset] = (bitboards[current_color][current_piece] & (1ULL << current_square)) ? 1 : 0;
				}
			}
		}
	}
	for (int repetition = 0; repetition < nn::times_input_repeated; repetition++)
	{
		nn_input[index++ + offset] = side_to_move;
	}

	if (en_passant_square != no_en_passant)
	{
		for (int repetition = 0; repetition < nn::times_input_repeated; repetition++)
		{
			nn_input[index + repetition + en_passant_index_lookup[en_passant_square] + offset] = 1;
		}
	}
	index += num_en_passant_squares * nn::times_input_repeated;
	
	for (int castling = 0; castling < 4; castling++)
	{
		for (int repetition = 0; repetition < nn::times_input_repeated; repetition++)
		{
			if (castling_rights & (1ULL << castling))
			{
				nn_input[index++ + offset] = 1;
			}
			else
			{
				nn_input[index++ + offset] = 0;
			}
		}
	}*/

	

}

inline constexpr void board::flip_piece(int square, color c, piece_type p)
{
	// get the bitboards of the captured piece type and remove it 
	bitboards[c][p] ^= (1ULL << square);
	color_bitboards[c] ^= (1ULL << square);
	piece_bitboards[p] ^= (1ULL << square);
	occupied ^= (1ULL << square);
	//hash ^= zoribst_values[c][p][square];
}

void board::make_move(move m)
{
	// save all this info before it gets overwritten by thingies later
	move_history[ply].m = m;
	move_history[ply].hash = hash;
	move_history[ply].castling_rights = castling_rights;
	move_history[ply].en_passant_square = en_passant_square;
	move_history[ply].fifty_rule = fifty_rule++;


	

	// get rid of en_passant thingie
	// if we enable the en passat it'll be enabled later
	en_passant_square = no_en_passant;
	if (m == null_move)
	{
		// null move is just skipping a turn
		// we skip the "moving the piece" part
		ply++;
		side_to_move = opposite_side_lookup[side_to_move];
		acc_history[ply].apply_move(m, COLOR_PIECE_NONE, COLOR_PIECE_NONE,
			-1, side_to_move, castling_rights, acc_history[ply - 1]);
		calculate_hash();
		return;
	}
	int square_from = move_from(m);
	int square_to = move_to(m);
	int flag = move_flag(m);
	color captured_piece_color = COLOR_NONE;
	color opposite_color = opposite_side_lookup[side_to_move];
	piece_type captured_piece_type = PIECE_NONE;
	piece captured_piece = COLOR_PIECE_NONE;
	// check if we capture anything, unless it's castling, then no capturing the rook
	if (((board_array[square_to] != COLOR_PIECE_NONE) || flag == en_passant) && (flag != castling))
	{
		// captures reset fifty move rule counter 
		fifty_rule = 0;
		// the captured piece type and color can be decoded easily
		// in normal chess you can only capture opponent's pieces
		// this however, is a general way of capturing any color pieces

		// if this is encroissant, the captured piece is in a different spot than the one we're moving to
		int square_offset = ((flag == en_passant) ? -pawn_push_offset[side_to_move] : 0);

		captured_piece = board_array[square_to + square_offset];
		captured_piece_color = get_color_from_piece(captured_piece);
		captured_piece_type = get_piece_type_from_piece(captured_piece);

		// remove the captured piece
		flip_piece(square_to + square_offset, captured_piece_color, captured_piece_type);

	}
	piece moving_piece = board_array[square_from];
	color moving_piece_color = get_color_from_piece(moving_piece);
	piece_type moving_piece_type = get_piece_type_from_piece(moving_piece);

	if (moving_piece_type == KING)
	{
		// king moves disable any castling, castling itself is a king move
		castling_rights &= ~castling_rights_mask_lookup[side_to_move];
	}
	// if we're not in frc, castling is to the rook on first and last row
	// if anything moves from there (a rook) or a captures there, we just disable castling
	if (!is_frc && (castling_rights))
	{
		if (0 + castling_dest_offset[side_to_move] == square_from)
		{
			castling_rights &= ~kingside_castling_lookup[side_to_move];
		}
		else if (board_cols - 1 + castling_dest_offset[side_to_move] == square_from)
		{
			castling_rights &= ~queenside_castling_lookup[side_to_move];
		}
		if (0 + castling_dest_offset[opposite_color] == square_to)
		{
			castling_rights &= ~kingside_castling_lookup[opposite_color];
		}
		else if (board_cols - 1 + castling_dest_offset[opposite_color] == square_to)
		{
			castling_rights &= ~queenside_castling_lookup[opposite_color];
		}
	}

	// rook moves are more common later into the game
	// and so doing all this logic just to disable already disabled castling is pointless
	// that's why in this case we check for castling rights, but not in the king case
	// rook moves disable castling, however only rooks that move in first row
	// if a game starts with a rook on any other row, it doesn't disable castling

	// TODO frc castling for any number of rooks


	if (moving_piece_type == PAWN)
	{
		// pawn moves also reset 50mr
		fifty_rule = 0;
		// see if en croissant should be allowed
		// if pawn moves by 2 rows and is a pawn, it allows en croissant
		if (std::abs(square_to - square_from) == board_cols * 2)
		{
			en_passant_square = square_from + pawn_push_offset[side_to_move];
		}
	}

	// remove the current piece from the bitboards
	flip_piece(square_from, moving_piece_color, moving_piece_type);

	// where we put the pieces depends on the flags of the move
	switch (flag)
	{
	case en_passant:
		// almost identical to normal, but also removing piece from the board array in the captured piece place
		// the captured piece color was set in the capture 
		board_array[square_to + pawn_push_offset[captured_piece_color]] = COLOR_PIECE_NONE;

		// this is so ugly whoever thought this is a good idea should be fired from making c++
		// or maybe I should just switch to C lol
		[[fallthrough]];
	case no_flag:
		// add the piece to the bitboards in the new location
		flip_piece(square_to, moving_piece_color, moving_piece_type);
		board_array[square_to] = board_array[square_from];
		board_array[square_from] = COLOR_PIECE_NONE;
		break;
	case castling:
	{
		// castling is onee-sama so it's ara ara
		// target square depends if it's queenside or kingside castling
		const bool is_kingside = square_from > square_to;
		int king_pos_after_castling = (is_kingside ? kingside_king_dest : queenside_king_dest) + castling_dest_offset[side_to_move];
		int rook_pos_after_castling = (is_kingside ? kingside_rook_dest : queenside_rook_dest) + castling_dest_offset[side_to_move];
		// we removed the current piece, but we also need to remove the rook that is castling
		// I can safely assume that castling with opposite color rooks will *never* be a thing
		flip_piece(square_to, side_to_move, ROOK);
		// clear the array with pieces
		board_array[square_to] = COLOR_PIECE_NONE;
		board_array[square_from] = COLOR_PIECE_NONE;

		// put the king and rook where they belong
		flip_piece(king_pos_after_castling, side_to_move, KING);
		flip_piece(rook_pos_after_castling, side_to_move, ROOK);
		// put W_KING OR B_KING depending on our needs, same with rooks
		board_array[king_pos_after_castling] = get_piece_from_piece_type(KING, side_to_move);
		board_array[rook_pos_after_castling] = get_piece_from_piece_type(ROOK, side_to_move);
	}
	break;

	case promotion:
		// promotion is almost identical, however we change the piece type 
		// the pawn has been removed from the bitboards, now we just replace it at the destination with what is desired
		// if it's a capture, it's already been handled
		// put a desired piece in place
		piece_type promotion_piece_type = piece_from_promotion_piece[move_promotion_piece(m)];
		// put the piece we're promoting to on the target square
		flip_piece(square_to, side_to_move, promotion_piece_type);
		// update the array
		piece promotion_piece = get_piece_from_piece_type(promotion_piece_type, side_to_move);
		board_array[square_to] = promotion_piece;
		board_array[square_from] = COLOR_PIECE_NONE;
		break;
	}
	//update move history so we can undo whatever we've done right now
	move_history[ply].captured_piece = captured_piece;
	ply++;
	side_to_move = opposite_side_lookup[side_to_move];
	calculate_hash();

#if use_nn
	if (flag != castling)
	{
		//acc_history[ply].refresh_accumulator(bitboards, en_passant_square, castling_rights, side_to_move);
		acc_history[ply].apply_move(m, moving_piece, captured_piece,
			en_passant_square,side_to_move,castling_rights, acc_history[ply - 1]);
		//acc_history[ply].refresh_accumulator(bitboards, en_passant_square, castling_rights, side_to_move);
	}
	else
	{
		//acc_history[ply].apply_move(m, moving_piece, captured_piece,
		//	en_passant_square, side_to_move, castling_rights, acc_history[ply - 1]);
		acc_history[ply].refresh_accumulator(bitboards, en_passant_square, castling_rights, side_to_move);
	}
#endif
	
}

void board::make_uci_move(const std::string& move_string)
{
	move m;
	const char* string_data = move_string.data();
	int square_from = square_index_from_square_name(string_data);
	int square_to = square_index_from_square_name(string_data + 2);
	// only promotion moves have this thingie
	if (move_string.length() == 5)
	{
		piece promotion_piece = get_piece_from_char(string_data[4]);
		int encoded_promotion_piece = promotion_piece_from_piece_type[
			get_piece_type_from_piece(promotion_piece)];
		m = construct_move(square_from, square_to, encoded_promotion_piece, promotion);
	}
	// a pawn can only move to an en_passant square through an en_passant
	else if (square_to == en_passant_square && (get_piece_type_from_piece(board_array[square_from]) == PAWN))
	{
		m = construct_move(square_from, square_to, default_piece, en_passant);
	}
	// if our king takes our rook, we're castling
	else if((get_piece_type_from_piece(board_array[square_from]) == KING) &&
		(get_piece_type_from_piece(board_array[square_to]) == ROOK) &&
		(get_color_from_piece(board_array[square_to]) == get_color_from_piece(board_array[square_from])))
	{
		m = construct_move(square_from, square_to, default_piece, castling);
	}
	// if our king moves to where it can't, we're castling
	else if ((get_piece_type_from_piece(board_array[square_from]) == KING) &&
		(std::abs(square_col[square_from] - square_col[square_to]) > 1))
	{
		// queenside castling
		if (square_from < square_to)
		{
			// in queenside castling we go to col 7
			square_to = 7 + castling_dest_offset[side_to_move];
		}
		// kingside castling
		else
		{
			// in kingside castling we go to col 0
			square_to = 0 + castling_dest_offset[side_to_move];
		}
		m = construct_move(square_from, square_to, default_piece, castling);
	}


	// normal move
	else
	{
		m = construct_move(square_from, square_to, default_piece, no_flag);
	}
	make_move(m);
}

bool board::is_legal() const
{
	// position is not legal, if the opposite king is being attacked
	bitboard opposite_kings = bitboards[opposite_side_lookup[side_to_move]][KING];
	// check if any of their kings is attacked by us 
	while (opposite_kings)
	{
		int king_square = pop_bit(opposite_kings);
		if (square_is_attacked(king_square, *this))
		{
			return false;
		}
	}
	return true;
}

bool board::in_check() const
{
	bitboard opposite_kings = bitboards[side_to_move][KING];
	// we're in check if any of our kings is attacked
	while (opposite_kings)
	{
		int king_square = pop_bit(opposite_kings);
		if (square_is_attacked(king_square, *this))
		{
			return true;
		}
	}
	return false;
}

bool board::move_pseudo_legal(move m) const
{
	// check if from and to squares make sense 
	int square_from = move_from(m);
	int square_to = move_to(m);
	int flag = move_flag(m);

	// can't move no piece
	if (get_piece_at(square_from) == COLOR_PIECE_NONE)
	{
		return false;
	}
	// can't capture own pieces
	if (get_color_from_piece(get_piece_at(square_to)) == side_to_move)
	{
		return false;
	}

	piece_type moving_piece_type = get_piece_type_from_piece(get_piece_at(square_from));
	bitboard square_to_bitmask = (1ULL << square_to);
	
	// check king moves
	if (moving_piece_type == KING)
	{
		if (flag == castling)
		{
			if (!is_frc)
			{
				// target square can only be A1 or A8 (or H1 and H8)
				constexpr int a1_square = square_index_from_square_name("a1");
				constexpr int h1_square = square_index_from_square_name("h1");
				if (square_to != (a1_square + castling_dest_offset[side_to_move])
					|| (square_to))
					return false;
			}
		}
		// move out of reach
		else if (!(square_to_bitmask & king_moves[square_from]))
		{
			return false;
		}
		// castling or the move don't seem illegal, possibly ok
		return true;
	}
}

void board::undo_move()
{
	ply--;
	hash = move_history[ply].hash;
	castling_rights = move_history[ply].castling_rights;
	fifty_rule = move_history[ply].fifty_rule;
	en_passant_square = move_history[ply].en_passant_square;
	//flip the side to move, keep track who was opponent when the move was made
	const color opposite_side = side_to_move;
	side_to_move = opposite_side_lookup[side_to_move];

	const move prev_move = move_history[ply].m;
	if (prev_move == null_move)
	{
		return;
	}
	const piece captured_piece = move_history[ply].captured_piece;
	const piece_type captured_piece_type = get_piece_type_from_piece(captured_piece);

	// determine what to do with the board depending on what the flag is
	int flag = move_flag(prev_move);
	int square_to = move_to(prev_move);
	int square_from = move_from(prev_move);




	switch (flag)
	{
	case en_passant:
	case promotion:
	case no_flag:
	{
		// if en_passant happened, captured piece will be in a different spot
		int square_offset = ((flag == en_passant) ? -pawn_push_offset[side_to_move] : 0);
		// only pawns can promote, so if it was a promotion then moved piece is a pawn
		// otherwise the moved piece is the same as the one in the destination
		piece moved_piece = ((flag == promotion) ? get_piece_from_piece_type(PAWN, side_to_move) : board_array[square_to]);
		// if it was a promotion, this is used and it's simply the piece that's on the board
		piece promotion_piece = board_array[square_to];
		piece_type moved_piece_type = get_piece_type_from_piece(moved_piece);
		piece_type promoted_piece_type = get_piece_type_from_piece(promotion_piece);

		// remove the current piece from the bitboards 
		// if it was promotion, we remove the promoted piece instead
		flip_piece(square_to, side_to_move, ((flag == promotion) ? promoted_piece_type : moved_piece_type));
		// put back the moving piece where it as
		// the moved piece is correct, even if it's promotion
		flip_piece(square_from, side_to_move, moved_piece_type);

		// put back the captured piece back where it was if it was a capture
		// if it was en_passant, captured piece was offset from the target square
		if (captured_piece != COLOR_PIECE_NONE)
		{
			color captured_piece_color = get_color_from_piece(captured_piece);
			flip_piece(square_to + square_offset, captured_piece_color, captured_piece_type);
		}

		// update the board array
		board_array[square_from] = moved_piece;
		board_array[square_to + square_offset] = captured_piece;
		if (flag == en_passant)
		{
			board_array[square_to] = COLOR_PIECE_NONE;
		}
	}
	break;
	case castling:
		//castling is onee-sama ara ara, and so of course is handled on its own
	{
		const bool is_kingside = square_from > square_to;
		const int king_pos_after_castling = (is_kingside ? kingside_king_dest : queenside_king_dest) + castling_dest_offset[side_to_move];
		const int rook_pos_after_castling = (is_kingside ? kingside_rook_dest : queenside_rook_dest) + castling_dest_offset[side_to_move];
		// we can now clear king and rook that was generated after castling
		flip_piece(king_pos_after_castling, side_to_move, KING);
		flip_piece(rook_pos_after_castling, side_to_move, ROOK);
		board_array[king_pos_after_castling] = COLOR_PIECE_NONE; //get_piece_from_piece_type();
		board_array[rook_pos_after_castling] = COLOR_PIECE_NONE;

		// put king and rook back where they were 
		// since it's king takes rook, it's super easy to know where they were
		flip_piece(square_from, side_to_move, KING);
		flip_piece(square_to, side_to_move, ROOK);
		board_array[square_from] = get_piece_from_piece_type(KING, side_to_move);
		board_array[square_to] = get_piece_from_piece_type(ROOK, side_to_move);
	}
	break;

	}

}

void board::randomize_position()
{
	// reset the current board
	std::memset(bitboards, 0, sizeof(bitboards));
	std::memset(color_bitboards, 0, sizeof(color_bitboards));
	std::memset(piece_bitboards, 0, sizeof(piece_bitboards));
	std::memset(board_array, COLOR_PIECE_NONE, sizeof(board_array));
	occupied = 0;

	// king always starts in the same position (for now)
	static constexpr int white_king_square = square_index_from_square_name("e1");
	static constexpr int black_king_square = square_index_from_square_name("e8");
	flip_piece(white_king_square, WHITE, KING);
	flip_piece(black_king_square, BLACK, KING);
	board_array[white_king_square] = W_KING;
	board_array[black_king_square] = B_KING;

	// the rooks you can castle with also start in the same position (for now)
	static constexpr int white_rooks_positions[2] = {
		square_index_from_square_name("a1"), square_index_from_square_name("h1")
	};
	static constexpr int black_rooks_positions[2] = {
		square_index_from_square_name("a8"), square_index_from_square_name("h8")
	};

	for (auto square_index : white_rooks_positions)
	{
		flip_piece(square_index, WHITE, ROOK);
		board_array[square_index] = W_ROOK;
	}
	for (auto square_index : black_rooks_positions)
	{
		flip_piece(square_index, BLACK, ROOK);
		board_array[square_index] = B_ROOK;
	}


	// generate remaining pieces
	const char* remaining_squares[board_cols - 3 + board_cols * 2] = {
		     "b1","c1","d1",     "f1","g1",
		"a2","b2","c2","d2","e2","f2","g2","h2",
		"a3","b3","c3","d3","e3","f3","g3","h3",
	};

	// there can be either 2 or 3 rows of pieces generated in a game
	bool generating_third_row = rng::xorshift64() & 1;

	for (const auto square_name : remaining_squares)
	{
		piece_type generated_piece_type;

		int white_index = square_index_from_square_name(square_name);
		// get the equivalent square from black's perspective
		const int white_index_row = square_row[white_index];
		const int white_index_col = square_col[white_index];
		int black_index = white_index_col + (board_rows - white_index_row - 1) * board_cols;


		// the last generated row is full of pawns
		const int row_of_pawns = 1 + generating_third_row;
		// if we reach row with this index, we stop
		const int last_row = 2 + generating_third_row;

		if (white_index_row == row_of_pawns)
		{
			generated_piece_type = PAWN;
		}
		else if (white_index_row == last_row)
		{
			break;
		}
		else
		{
			// king piece type is 0, get a piece that is not a king
			generated_piece_type = (piece_type)(rng::xorshift64() % (PIECE_NONE - 1) + 1);
		}
		
		flip_piece(white_index, WHITE, generated_piece_type);
		flip_piece(black_index, BLACK, generated_piece_type);
		board_array[white_index] = get_piece_from_piece_type(generated_piece_type, WHITE);
		board_array[black_index] = get_piece_from_piece_type(generated_piece_type, BLACK);
	}

	// get random castling rights for both sides
	int random_castling = rng::xorshift64() & 3;

	castling_rights = random_castling | (random_castling << 2);
	en_passant_square = no_en_passant;
	fifty_rule = 0;
	side_to_move = WHITE;
	ply = 0;
	calculate_hash();
#if use_nn
	acc_history[ply].refresh_accumulator(bitboards, en_passant_square, castling_rights, side_to_move);
#endif
}

void board::new_game()
{
	//set_position("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
	std::memcpy(board_array, startpos_board_array, sizeof(board_array));
	ply = 0;
	hash = 0;
	fifty_rule = 0;
	is_frc = 0;
	side_to_move = WHITE;
	en_passant_square = -1;
	castling_rights = 15;
	

	bitboards[WHITE][PAWN] = 65280ULL;
	bitboards[WHITE][ROOK] = 129ULL;
	bitboards[WHITE][BISHOP] = 36ULL;
	bitboards[WHITE][KING] = 8ULL;
	bitboards[WHITE][QUEEN] = 16ULL;
	bitboards[WHITE][KNIGHT] = 66ULL;

	bitboards[BLACK][PAWN] = 71776119061217280ULL;
	bitboards[BLACK][ROOK] = 9295429630892703744ULL;
	bitboards[BLACK][BISHOP] = 2594073385365405696ULL;
	bitboards[BLACK][KING] = 576460752303423488ULL;
	bitboards[BLACK][QUEEN] = 1152921504606846976ULL;
	bitboards[BLACK][KNIGHT] = 4755801206503243776ULL;

	piece_bitboards[PAWN] = 71776119061282560ULL;
	piece_bitboards[ROOK] = 9295429630892703873ULL;
	piece_bitboards[BISHOP] = 2594073385365405732ULL;
	piece_bitboards[KING] = 576460752303423496ULL;
	piece_bitboards[QUEEN] = 1152921504606846992ULL;
	piece_bitboards[KNIGHT] = 4755801206503243842ULL;

	color_bitboards[WHITE] = 65535ULL;
	color_bitboards[BLACK] = 18446462598732840960ULL;

	occupied = 18446462598732906495ULL;

	calculate_hash();
	acc_history[ply].refresh_accumulator(bitboards, en_passant_square, castling_rights, side_to_move);
}

void board::init_zoribst()
{
	static bool zoribst_initialized = false;
	if (!zoribst_initialized)
	{
		zoribst_initialized = true;
		for (int curr_color = 0; curr_color < COLOR_NONE; curr_color++)
		{
			for (int curr_piece_type = 0; curr_piece_type < PIECE_NONE; curr_piece_type++)
			{
				for (int square = 0; square < num_board_squares; square++)
				{
					zoribst_values[curr_color][curr_piece_type][square] = rng::xorshift64();
				}
			}
		}
		for (auto& val : zoribst_castling)
		{
			val = rng::xorshift64();
		}
		for (auto& val : zoribst_en_passant)
		{
			val = rng::xorshift64();
		}
		zoribst_side_to_move = rng::xorshift64();
	}
}

move board::parse_move(const std::string& m)
{
	int square_from = square_index_from_square_name(m.c_str());
	int square_to = square_index_from_square_name(m.c_str() + 2);
	piece_type promotion_piece = (piece_type)0;
	// check if we have a promotion piece
	if (m.length() == 5)
	{
		promotion_piece = get_piece_type_from_piece(get_piece_from_char(m[4]));
		return construct_move(square_from, square_to, promotion_piece_from_piece_type[promotion_piece], promotion);
	}
	// check if it's an en-passant
	if (square_to == en_passant_square)
	{
		return construct_move(square_from, square_to, 0, en_passant);
	}
	// maybe it is castling ??
	const bitboard& side_to_move_kings = bitboards[side_to_move][KING];
	const bitboard& side_to_move_rooks = bitboards[side_to_move][ROOK];
	if ((side_to_move_kings & (1ULL << square_from)) && side_to_move_rooks & (1ULL << square_to))
	{
		return construct_move(square_from, square_to, 0, castling);
	}

	// if it's not a weird move, it's a normal move
	return construct_move(square_from, square_to, 0, no_flag);
}

uint64_t board::calculate_hash()
{
	hash = side_to_move * zoribst_side_to_move;
	for (int curr_color = 0; curr_color < COLOR_NONE; curr_color++)
	{
		for (int curr_piece_type = 0; curr_piece_type < PIECE_NONE; curr_piece_type++)
		{
			bitboard curr_piece_bitboard = bitboards[curr_color][curr_piece_type];
			
			while (curr_piece_bitboard)
			{
				int index = pop_bit(curr_piece_bitboard);
				hash ^= zoribst_values[curr_color][curr_piece_type][index];
			}
		}
	}
	hash ^= zoribst_castling[castling_rights];
	if (en_passant_square != no_en_passant)
	{
		hash ^= zoribst_en_passant[en_passant_square];
	}
	return hash;
}