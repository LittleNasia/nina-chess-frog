#pragma once
#include "utils.h"

inline constexpr bitboard empty_bitboard = 0ULL;
inline constexpr bitboard full_bitboard = 0xffffffffffffffffULL;

inline constexpr bitboard row_bitmasks[board_rows] =
{
	255ULL,
	65280ULL,
	16711680ULL,
	4278190080ULL,
	1095216660480ULL,
	280375465082880ULL,
	71776119061217280ULL,
	18374686479671623680ULL,
};

inline constexpr int promotion_rows[COLOR_NONE] =
{
	7,0
};

inline constexpr bitboard promotion_row_bitmasks[COLOR_NONE] =
{
	row_bitmasks[promotion_rows[WHITE]], row_bitmasks[promotion_rows[BLACK]]
};

inline constexpr bitboard col_bitmasks[board_cols] =
{
	72340172838076673ULL,
	144680345676153346ULL,
	289360691352306692ULL,
	578721382704613384ULL,
	1157442765409226768ULL,
	2314885530818453536ULL,
	4629771061636907072ULL,
	9259542123273814144ULL,
};

inline constexpr int square_row[num_board_squares] =
{
	0, 0, 0, 0, 0, 0, 0, 0,
	1, 1, 1, 1, 1, 1, 1, 1,
	2, 2, 2, 2, 2, 2, 2, 2,
	3, 3, 3, 3, 3, 3, 3, 3,
	4, 4, 4, 4, 4, 4, 4, 4,
	5, 5, 5, 5, 5, 5, 5, 5,
	6, 6, 6, 6, 6, 6, 6, 6,
	7, 7, 7, 7, 7, 7, 7, 7,
};

inline constexpr int square_col[num_board_squares] =
{
	0, 1, 2, 3, 4, 5, 6, 7,
	0, 1, 2, 3, 4, 5, 6, 7,
	0, 1, 2, 3, 4, 5, 6, 7,
	0, 1, 2, 3, 4, 5, 6, 7,
	0, 1, 2, 3, 4, 5, 6, 7,
	0, 1, 2, 3, 4, 5, 6, 7,
	0, 1, 2, 3, 4, 5, 6, 7,
	0, 1, 2, 3, 4, 5, 6, 7,
};

// this lookup table returns a bitmask of all squares between the two column indices in a row
// for example, given pieces in the same row, one in column 1, another one in column 4
// [1][4] will return bitmask of all squares between these two
// in this case, it will be bits on index 2 and 3 set
inline constexpr bitboard bitmask_between_coords[board_cols][board_cols] =
{
	{0, 0, 2, 6, 14, 30, 62, 126, },
	{0, 0, 0, 4, 12, 28, 60, 124, },
	{2, 0, 0, 0, 8, 24, 56, 120, },
	{6, 4, 0, 0, 0, 16, 48, 112, },
	{14, 12, 8, 0, 0, 0, 32, 96, },
	{30, 28, 24, 16, 0, 0, 0, 64, },
	{62, 60, 56, 48, 32, 0, 0, 0, },
	{126, 124, 120, 112, 96, 64, 0, 0, },
};