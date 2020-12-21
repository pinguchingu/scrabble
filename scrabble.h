#ifndef SCRABBLE_H
#define SCRABBLE_H

#include "board.h"
#include "colors.h"
#include "computer_player.h"
#include "dictionary.h"
#include "exceptions.h"
#include "human_player.h"
#include "move.h"
#include "rang.h"
#include "scrabble_config.h"
#include "tile_bag.h"
#include <cmath>
#include <memory>

class Scrabble {
public:
    Scrabble(const ScrabbleConfig& config);

    void main();

    static const size_t EMPTY_HAND_BONUS = 50;

    static void final_subtraction(std::vector<std::shared_ptr<Player>>& players);  // public for testing

private:
    // TODO: you may add more private members.

    size_t hand_size;
    size_t minimum_word_length;
    size_t non_human_players;
    TileBag tile_bag;
    Board board;
    Dictionary dictionary;
    std::vector<std::shared_ptr<Player>> players;

    void add_players();
    void game_loop();
    void print_result();
};

#endif
