// nina-chess.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include  <iomanip>
#include "utils.h"
#include "magic.h"
#include "bitmasks.h"
#include "board.h"
#include "move_gen.h"
#include "perft.h"
#include "search.h"
#include "dense_layer.h"
#include "accumulator.h"
#include "nn.h"
#include <fstream>
#include "game_generation.h"
#include "uci.h"



void check_move_generation_of_dump()
{
    board b;
    std::ifstream dump("board.bin", std::ios::binary);
    dump.read((char*)&b, sizeof(board));
    dump.close();
    b.print_board();
    move_list_container move_list;
    generate_legal_moves(move_list, b);
    for (int move = 0; move < move_list.num_moves; move++)
    {
        const auto m = move_list.moves[move];
        std::cout << move << " " << m << " " << move_from(m) << " " << move_to(m) << " "
            << square_names[move_from(m)] << square_names[move_to(m)] 
            << " " << move_promotion_piece(m) << " " << move_flag(m) << "\n";
    }
    int score;
    move m = search::search_move(b, score, 2, 0, false);
    std::cout << "search move " << m << " " << move_from(m) << " " << move_to(m) << " "
        << square_names[move_from(m)] << square_names[move_to(m)] << "\n";
    b.print_board();
}


void print_evaluations()
{
    std::ifstream games_file("games.bin", std::ios::binary);
    position_entry positions[100];
    games_file.read((char*)positions, sizeof(positions));
    board b;
    for (const auto& position:positions)
    {
        b.set_position(position);
        std::cout << " " << nn::network.evaluate_board(b) << "\n";
    }
}


int main()
{
    init_magics();
    board::init_zoribst();

   // print_evaluations();
    //check_move_generation_of_dump();
    //
#if should_generate_games
    std::ofstream file_with_games("games.bin", std::ios::binary);
    file_with_games.close();
    for (int i = 0; i < 100; i++)
    {
        std::cout << "run " << i << "\n";
        game_generation::generate_games(4);
    }
    exit(0);
#else
    uci::loop();
#endif
    
    ////b.randomize_position();
    //b.print_board();
    //uci::loop();
    

    
    /*b.new_game();
    b.print_board();
    
    std::cout << " " << nn::network.evaluate_board(b) << "\n";

    int value;
    search::search_move(b, value, 5, 0);

    std::cout << " " << nn::network.evaluate_board(b) << "\n";*/

    board b;
    //std::ofstream file_with_games("games.bin", std::ios::binary);
    //for (move m : {1153, 2745, 1739, 2933, 1292, 2556, 1350, 1770, 706, 999, 967, 3070, 49155, 1371, 1358, 2868, 1812, 2096, 2130})
    //{
    //    b.prepare_input();
    //    //b.print_board();
    //    
    //    position_entry current_pos = game_generation::create_entry_from_pos(b);
    //    position_entry horizontal_pos = game_generation::create_horizontal_symmetry_entry(b);
    //    position_entry vertical_pos = game_generation::create_vertical_symmetry_entry(b);
    //    position_entry vertical_horizontal_pos = game_generation::create_vertical_horizontal_symmetry_entry(b);

    //    //for (auto board_array : { current_pos.board_array, horizontal_pos.board_array, vertical_pos.board_array, vertical_horizontal_pos.board_array })
    //    //{
    //    //    for (int row = 7; row >= 0; row--)
    //    //    {
    //    //        for (int col = 7; col >= 0; col--)
    //    //        {
    //    //            std::cout << "[";
    //    //            //std::cout <<(int) board_array[two_d_to_one_d(row, col)];
    //    //            std::cout << piece_names[board_array[two_d_to_one_d(row, col)]];
    //    //            std::cout << "]";
    //    //        }
    //    //        std::cout << " " << row + 1 << "\n";
    //    //    }
    //    //    std::cout << "--------------------------\n\n";
    //    //}
    //    
    //    /*for (int i = 0; i < nn::input_size; i++)
    //    {
    //        std::cout << b.nn_input[i] << " ";
    //    }
    //    std::cout << "\n\n";*/

    //    std::cout << b.get_ply() << " " << m << " " << move_from(m) << " " << move_to(m) << " "
    //        << square_names[move_from(m)] << square_names[move_to(m)]
    //        << " " << move_promotion_piece(m) << " " << move_flag(m) << "\n";
    //    position_entry p = game_generation::create_entry_from_pos(b);
    //    file_with_games.write((char*)&p, sizeof(p));
    //    std::cout << nn::network.evaluate_board(b) << "\n\n\n";
    //    b.make_move(m);
    //}
    //file_with_games.close();
    
    uci::loop();
    
    alignas(64) float input[32];
    for (int i = 0; i < 32; i++)
    {
        input[i] = 1;
    }
    nn::dense_layer<32, 8> layer;
    nn::dense_layer<8, 1> layer2;
    float* first_out = layer.forward(input);

    float out = *layer2.forward(first_out);

    /*search::value score = 0;
    search::search_move(b, score, 20, 0);*/
    
    //b.print_board();
    //b.set_position("rnbq1bnr/pppp1pp1/8/4P2p/k1BQP3/5PPP/PPP5/RNB1K1NR b KQ - 6 10");
    /*for (auto move : { 1568,512,2673,776,2153,524,1633 })
    {
        b.make_move(move);
        std::cout << square_names[move_from(move)] << square_names[move_to(move)] << " " << move_promotion_piece(move) << " " << move_flag(move) << "\n";
        b.print_board();
    }*/
    move_list_container move_list;
    //generate_legal_moves(move_list, b);
    
    //generate_pseudolegal_moves(move_list, b);
    //for (int move = 0; move < move_list.num_moves; move++)
    //{
    //    const auto m = move_list.moves[move];
    //    std::cout << move << " " << m << " " << move_from(m) << " " << move_to(m) << " "
    //        << square_names[move_from(m)] << square_names[move_to(m)] 
    //        << " " << move_promotion_piece(m) << " " << move_flag(m) << "\n";
    //}
    //
    //for (int i = 0; i < 7; i++)
    //{
    //    int x;
    //    std::cin >> x;
    //    b.make_move(x);
    //    //std::ofstream board_dump("board.bin", std::ios::binary);
    //    //board_dump.write((char*)&b, sizeof(board));
    //    //board_dump.close();
    //    //std::cin >> x;
    //    b.print_board();
    //}
    

 //   move_list_container move_list;
    
    generate_legal_moves(move_list, b);
    
    b.print_board();
    for (int move = 0; move < move_list.num_moves; move++)
    {
        const auto m = move_list.moves[move];
        std::cout << move + 1 << ": " << square_names[move_from(m)] << square_names[move_to(m)] << " " 
            << move_flag(m) << "\n";
        move_list_container sub_moves;
    }
    
    std::cout << perft(b,5) << "\n";
    b.print_board();
    print();
    
}
