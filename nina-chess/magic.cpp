#include "magic.h"
#include "bitmasks.h"
#include "board.h"

#include <iostream>

inline magic bishop_magics[num_board_squares];
inline magic rook_magics[num_board_squares];

// this value has been calculated by checking how many combinations of possible blockers exist
// for all the possible rook placements
inline constexpr int rook_attacks_table_size = 102400;
inline bitboard rook_attacks_table[rook_attacks_table_size];

// this value has been calculated by checking how many combinations of possible blockers exist
// for all the possible bishop placements
inline constexpr int bishop_attacks_table_size = 5248;
inline bitboard bishop_attacks_table[bishop_attacks_table_size];



bitboard generate_sliding_attacks(const int square, bitboard occupied, const point* move_offsets)
{
    bitboard result = 0ULL;

    for (int direction = 0; direction < num_directions; direction++)
    {
        int row = square_row[square];
        int col = square_col[square];
        while (true)
        {
            int curr_row = row + move_offsets[direction].row;
            int curr_col = col + move_offsets[direction].col;
            //squares outside the board can't really be attacked
            if ((curr_row >= 8 || curr_row < 0) || (curr_col >= 8 || curr_col < 0))
            {
                break;
            }

            int curr_index = curr_row * board_rows + curr_col;
            result |= (1ULL << curr_index);

            //we stop on first attacked piece
            if (occupied & (1ULL << curr_index))
            {
                break;
            }
            row = curr_row;
            col = curr_col;
        }
    }
    return result;
}

inline constexpr int magic_index(bitboard occupied, bitboard mask, uint64_t magic_num, int shift)
{
    // if I had pext, this magic trick wouldn't be needed
    return ((occupied & mask) * magic_num) >> shift;
}

void initialize_sliding_magics(const point* move_offsets, bitboard* attacks_table, const uint64_t* magic_numbers, magic* table_with_magics)
{
    int table_offset = 0;
    for (int square = 0; square < num_board_squares; square++)
    {
        // the edges aren't included in the attacking mask
        // the existence of a piece on the edges doesn't stop us from attacking it
        // we can't go behind such piece on the edge, but there's kinda no board behind it
        // the lack of existence of a piece on an edge also doesn't stop us from attacking the empty square
        // the exception is the case in which our current piece lies on the edge
        // in that case we need to be able to attack pieces alongside that edge 
        uint64_t edges = ((row_bitmasks[0] | row_bitmasks[board_rows - 1]) & ~row_bitmasks[square_row[square]])
            | ((col_bitmasks[0] | col_bitmasks[board_cols - 1]) & ~col_bitmasks[square_col[square]]);

        // the magic numbers are taken from Andy Grant's Ethereal
        // I trust him they work 
        // which they dooooooo
        uint64_t magic_num = magic_numbers[square];

        // get all attacked squares, cut the edges for the reasons explained before
        bitboard mask = generate_sliding_attacks(square, empty_bitboard, move_offsets) & ~edges;

        // shifting by this number leaves popcnt(mask) most significant bits after shifting
        // this magic number is confirmed to give the same most significant bits after multiplication
        // for all numbers that should have them be the same, the name "magic" is approriate 
        int shift = 64 - popcnt(mask);

        // if we could use one entry for each combination of bits (as is done with pext instruction)
        // we'd have pow(2,popcnt(mask)) possible numbers represented by the mask
        // that's why the table for the current square and mask is exactly this size
        // magic numbers cause some collisions and so the number of used entries is even smaller
        // the magic numbers I stole from Ethereal work only for the given shift
        // making the size to be smaller could (and should) go outside the bounds sometimes
        // making the size to be bigger makes no sense, shifting guarantees this size is enough
        bitboard* curr_table = attacks_table + table_offset;

        // this is equivalent to the pow(2,popcnt(mask)) thingie
        // table_offset makes sure the table for the next square starts right after this one
        table_offset += (1 << popcnt(mask));

        bitboard occupied = 0ULL;
        // this do-while loop iterates over all possible combinations of bits in the mask
        // this is very smart
        do
        {
            // get the index for our table with already calculated attacked squares
            int index = magic_index(occupied, mask, magic_num, shift);
            // put the result for the given square and combination of attackers into the table
            curr_table[index] = generate_sliding_attacks(square, occupied, move_offsets);
            // get next combination of attackers using this neat trick
            occupied = (occupied - mask) & mask;
        } while (occupied);

        table_with_magics[square] = { magic_num, mask, curr_table, shift };
    }
}

void init_magics()
{
    initialize_sliding_magics(rook_moves, rook_attacks_table, rook_magic_numbers, rook_magics);
    initialize_sliding_magics(bishop_moves, bishop_attacks_table, bishop_magic_numbers, bishop_magics);
}

bitboard generate_bishop_attacks(const bitboard occupied, int square)
{
    const auto mask = bishop_magics[square].mask;
    const auto magic_num = bishop_magics[square].magic_num;
    const auto shift = bishop_magics[square].shift;
    const int index = magic_index(occupied, mask, magic_num, shift);
    return bishop_magics[square].hash_table[index];
}

bitboard generate_rook_attacks(const bitboard occupied, int square)
{
    const auto mask = rook_magics[square].mask;
    const auto magic_num = rook_magics[square].magic_num;
    const auto shift = rook_magics[square].shift;
    const int index = magic_index(occupied, mask, magic_num, shift);
    return rook_magics[square].hash_table[index];
}