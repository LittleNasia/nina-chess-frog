#pragma once
#include <cstdint>
#include <intrin.h>
#include <iostream>
#include <limits>

using bitboard	= uint64_t;
using color		= int32_t;

inline constexpr int max_ply = 1024;

inline constexpr int board_rows = 8;
inline constexpr int board_cols = 8;
inline constexpr int num_board_squares = board_rows * board_cols;
#define should_generate_games true

inline void print_bitboard(const bitboard bb)
{
	int index = 63;
	for (int row = 0; row < 8; row++)
	{
		for (int col = 0; col < 8; col++)
		{
			std::cout << "[";
			if ((1ULL << index) & bb)
			{
				std::cout << "X";
			}
			else
			{
				std::cout << " ";
			}
			std::cout << "]";
			index--;
		}
		std::cout << "\n";
	}
}

using move = uint16_t;
inline constexpr int move_to_offset = 6;
inline constexpr int promotion_offset = 12;
inline constexpr int flag_offset = 14;

inline constexpr int no_flag = 0;
inline constexpr int promotion = 1;
inline constexpr int en_passant = 2;
inline constexpr int castling = 3;

enum
{
	WHITE,
	BLACK,
	COLOR_NONE
};

enum piece_type
{
	KING,
	QUEEN,
	ROOK,
	BISHOP,
	KNIGHT,
	PAWN,
	PIECE_NONE
};

// to get piece type, divide each of these by 2
// odd numbers are for black pieces
enum piece : uint8_t
{
	W_KING = 0,
	B_KING = 1,
	W_QUEEN = 2,
	B_QUEEN = 3, 
	W_ROOK = 4,
	B_ROOK = 5,
	W_BISHOP = 6,
	B_BISHOP = 7,
	W_KNIGHT = 8,
	B_KNIGHT = 9,
	W_PAWN = 10,
	B_PAWN = 11,
	COLOR_PIECE_NONE
};

inline const char* piece_names[COLOR_PIECE_NONE + 1] =
{
	"K","k","Q","q","R","r","B","b","N","n","P","p", " "
};

inline constexpr piece get_piece_from_char(char piece)
{
	switch (piece)
	{
	case 'p':
		return B_PAWN;
	case 'b':
		return B_BISHOP;
	case 'n':
		return B_KNIGHT;
	case 'r':
		return B_ROOK;
	case 'k':
		return B_KING;
	case 'q':
		return B_QUEEN;
	case 'P':
		return W_PAWN;
	case 'B':
		return W_BISHOP;
	case 'N':
		return W_KNIGHT;
	case 'R':
		return W_ROOK;
	case 'K':
		return W_KING;
	case 'Q':
		return W_QUEEN;
	default:
		return COLOR_PIECE_NONE;
	}
}

inline constexpr piece get_piece_from_piece_type(piece_type pt, color c)
{
	return (piece)((pt * 2) + c);
}

inline constexpr color get_color_from_piece(piece pt)
{
	return (color)pt & 1;
}

inline constexpr piece_type get_piece_type_from_piece(piece pt)
{
	return (piece_type)((pt / 2));
}

inline constexpr piece get_opposite_color_piece(piece p)
{
	if (p != COLOR_PIECE_NONE)
	{
		return (piece)((uint8_t)p ^ 1);
	}
	else
		return COLOR_PIECE_NONE;
}

enum
{
	PROMOTION_KNIGHT,
	PROMOTION_BISHOP,
	PROMOTION_ROOK,
	PROMOTION_QUEEN,
	PROMOTION_NONE
};

inline constexpr char promotion_piece_to_char[PROMOTION_NONE] =
{
	'n','b','r','q'
};

inline constexpr piece_type piece_from_promotion_piece[PROMOTION_NONE] =
{
	KNIGHT, BISHOP, ROOK, QUEEN
};

inline constexpr int promotion_piece_from_piece_type[PIECE_NONE] =
{
	0,
	PROMOTION_QUEEN,
	PROMOTION_ROOK,
	PROMOTION_BISHOP,
	PROMOTION_KNIGHT,
	0,
};

enum
{
	NO_CASTLING,

	WHITE_KINGSIDE = 1,
	WHITE_QUEENSIDE = 2,
	BLACK_KINGSIDE = 4,
	BLACK_QUEENSIDE = 8,

	BLACK_CASTLING = BLACK_KINGSIDE | BLACK_QUEENSIDE,
	WHITE_CASTLING = WHITE_KINGSIDE | WHITE_QUEENSIDE,
	QUEENSIDE_CASTLING  = WHITE_QUEENSIDE | BLACK_QUEENSIDE,
	KINGSIDE_CASTLING = BLACK_QUEENSIDE | BLACK_KINGSIDE,
	ALL_CASTLING = BLACK_CASTLING | WHITE_CASTLING
};

inline constexpr int castling_rights_mask_lookup[COLOR_NONE] =
{
	WHITE_CASTLING, BLACK_CASTLING
};

inline constexpr int kingside_castling_lookup[COLOR_NONE] =
{
	WHITE_KINGSIDE, BLACK_KINGSIDE
};

inline constexpr int queenside_castling_lookup[COLOR_NONE] =
{
	WHITE_QUEENSIDE, BLACK_QUEENSIDE
};

// index of square king and rook end up on after castling kingside
inline constexpr int kingside_king_dest = 1;
inline constexpr int kingside_rook_dest = 2;
// index of square king and rook end up on after castling queenside
inline constexpr int queenside_king_dest = 5;
inline constexpr int queenside_rook_dest = 4;

inline constexpr int default_piece = PROMOTION_KNIGHT;

inline constexpr int castling_dest_offset[COLOR_NONE] =
{
	// the castling destination indices are from white's perspective
	// black's destinations are on the opposite side of the board
	0,board_cols * (board_rows - 1)
};

inline constexpr int pawn_push_offset[COLOR_NONE] =
{
	board_cols,
	-board_cols
};

inline constexpr int pawn_left_capture_offset[COLOR_NONE] =
{
	board_cols + 1,
	-board_cols + 1
};

inline constexpr int pawn_right_capture_offset[COLOR_NONE] =
{
	board_cols - 1,
	-board_cols - 1
};


constexpr inline move construct_move(int square_from, int square_to, int promotion_piece, int flag)
{
	move m = 0;
	m |= square_from;
	m |= square_to		 << move_to_offset;
	m |= promotion_piece << promotion_offset;
	m |= flag			 << flag_offset;
	return m;
}

inline constexpr move null_move = 0;

constexpr inline int move_from(move m)
{
	return m & 0x3F;
}

constexpr inline int move_to(move m)
{
	return (m>> move_to_offset) & 0x3F;
}

constexpr inline int move_promotion_piece(move m)
{
	return (m >> promotion_offset) & 3;
}

constexpr inline int move_flag(move m)
{
	return (m >> flag_offset) & 3;
}

inline constexpr int no_en_passant = -1;




struct point
{
	int row;
	int col;
};

inline constexpr int two_d_to_one_d(int row, int col)
{
	return row * board_cols + col;
}


inline int popcnt(bitboard bb)
{
	return __popcnt64(bb);
}

inline unsigned int bsf(const bitboard& bb)
{
	unsigned long index;
	_BitScanForward64(&index, bb);
	return index;
}

inline unsigned int pop_bit(bitboard& bb)
{
	unsigned int index = bsf(bb);
	bb ^= (1ULL << index);
	return index;
}



inline constexpr color opposite_side_lookup[COLOR_NONE] = { BLACK, WHITE };


inline const char* square_names[num_board_squares] =
{
	"h1","g1","f1","e1","d1","c1","b1","a1",
	"h2","g2","f2","e2","d2","c2","b2","a2",
	"h3","g3","f3","e3","d3","c3","b3","a3",
	"h4","g4","f4","e4","d4","c4","b4","a4",
	"h5","g5","f5","e5","d5","c5","b5","a5",
	"h6","g6","f6","e6","d6","c6","b6","a6",
	"h7","g7","f7","e7","d7","c7","b7","a7",
	"h8","g8","f8","e8","d8","c8","b8","a8",
};

inline constexpr int get_col_from_file(const char file)
{
	int col = file - 'a';
	// "h" file is actually column number 0 internally
	col = board_cols - (col + 1);
	return col;
}

inline constexpr int get_row_from_rank(const char rank)
{
	return rank - '1';
}

inline constexpr int square_index_from_square_name(const char* square_name)
{
	int row = get_row_from_rank(square_name[1]);
	int col = get_col_from_file(square_name[0]);
	return two_d_to_one_d(row, col);
}




inline constexpr int horizontal_symmetry_lookup[num_board_squares] =
{
	56,57,58,59,60,61,62,63,
	48,49,50,51,52,53,54,55,
	40,41,42,43,44,45,46,47,
	32,33,34,35,36,37,38,39,
	24,25,26,27,28,29,30,31,
	16,17,18,19,20,21,22,23,
	8,9,10,11,12,13,14,15,
	0,1,2,3,4,5,6,7,
};



inline constexpr int vertical_symmetry_lookup[num_board_squares] =
{
	7,6,5,4,3,2,1,0,
	15,14,13,12,11,10,9,8,
	23,22,21,20,19,18,17,16,
	31,30,29,28,27,26,25,24,
	39,38,37,36,35,34,33,32,
	47,46,45,44,43,42,41,40,
	55,54,53,52,51,50,49,48,
	63,62,61,60,59,58,57,56,
};

inline constexpr int vertical_horizontal_symmetry_lookup[num_board_squares] =
{
	63,62,61,60,59,58,57,56,
	55,54,53,52,51,50,49,48,
	47,46,45,44,43,42,41,40,
	39,38,37,36,35,34,33,32,
	31,30,29,28,27,26,25,24,
	23,22,21,20,19,18,17,16,
	15,14,13,12,11,10,9,8,
	7,6,5,4,3,2,1,0,
};

inline constexpr int num_en_passant_squares = 16;

inline constexpr int en_passant_index_lookup[num_board_squares] =
{
	-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,
	0,1,2,3,4,5,6,7,
	-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,
	8,9,10,11,12,13,14,15,
	-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,
};