#include "human_player.h"

#include "exceptions.h"
#include "formatting.h"
#include "move.h"
#include "place_result.h"
#include "rang.h"
#include "tile_kind.h"
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <vector>

using namespace std;

// This method is fully implemented.
inline string& to_upper(string& str) {
    transform(str.begin(), str.end(), str.begin(), ::toupper);
    return str;
}

// This method prints the board and hand of the current user and asks them to input a move.
// It then parses the move and returns it if it is valid. Otherwise, it asks the player
// to try again with a valid move.
Move HumanPlayer::get_move(const Board& board, const Dictionary& dictionary) const {
    // Prints the board and hand
    board.print(cout);
    print_hand(cout);
    cout << endl;
    cout << FG_COLOR_HEADING << "Your move, " << PLAYER_NAME_COLOR << get_name() << FG_COLOR_HEADING << ": "
         << rang::style::reset;

    // Initializing an error string to hold an error if one occurs.
    // Initializing a flag which keeps track of whether all the words
    // created by the move are in the dictionary.
    // Initializing a string to hold the input of the user so we can pass it
    // to our parse_move method.
    string error;
    bool not_all_words = false;
    string to_parse;

    // We get a line in from cin, which we assume is the user's inputted move.
    // We parse it, and pass it to test_place on our board. Both of these methods
    // should check if the inputted move is of correct format, and if the placement
    // on the board is valid, respectively.
    getline(cin, to_parse);
    Move curr_move = parse_move(to_parse);
    PlaceResult result = board.test_place(curr_move);

    // We check to make sure every word created by the user's move is in the dictionary,
    // and set our flag to true otherwise.
    for (size_t i = 0; i < result.words.size(); ++i) {
        if (!dictionary.is_word(result.words[i])) {
            error = result.words[i];
            not_all_words = true;
        }
    }

    // If we got an invalid PlaceResult from our test_place or we found a word that is
    // not in the dictionary, we should print the error to the user and ask them to try
    // again.
    while (!result.valid || not_all_words) {
        if (not_all_words) {
            cout << "Error in move: " << error << " is not a word." << endl << endl;
        } else {
            cout << "Error in move: " << result.error << endl << endl;
        }
        cout << FG_COLOR_HEADING << "Your move, " << PLAYER_NAME_COLOR << get_name() << FG_COLOR_HEADING << ": "
             << rang::style::reset;

        not_all_words = false;
        getline(cin, to_parse);
        curr_move = parse_move(to_parse);
        result = board.test_place(curr_move);
        for (size_t i = 0; i < result.words.size(); ++i) {
            if (!dictionary.is_word(result.words[i])) {
                error = result.words[i];
                not_all_words = true;
            }
        }
    }

    // If we make it here, we have a completely valid move, so we just return it.
    return curr_move;
}

// parse_tiles takes the inputted letters from the user and tests if the letters used
// are valid and converts them to a vector of tiles.
vector<TileKind> HumanPlayer::parse_tiles(string& letters) const {
    // Initializing a vector of TileKinds to be our output.
    vector<TileKind> output;

    // For every character in the letters string, we add it to the output vector
    // by using the lookup_tile method from our tilecollection.
    // If the character is '?' we don't add the next tile, and instead add it as
    // the 'assigned' character to the blank letter tile.
    for (size_t i = 0; i < letters.size(); ++i) {
        if (letters[i] == TileKind::BLANK_LETTER) {
            output.push_back(TileKind(TileKind::BLANK_LETTER, 0, letters[i + 1]));
            i++;
        } else {
            output.push_back(tiles.lookup_tile(letters[i]));
        }
    }

    // Testing to make sure the user has all the tiles that they are trying to use.
    // We create a duplicate of our output vector because we are going to sort it,
    // and the order of the output vector matters.
    vector<TileKind> test = output;
    sort(test.begin(), test.end());

    // We use the curr int to keep track of how many of the same tile the user is trying
    // to use.
    // Since the vector is sorted, as soon as the letter changes, we check to make sure the
    // user has curr of the last letter. If not we throw an exception.
    unsigned int curr = 0;
    for (size_t i = 0; i < test.size(); i++) {
        if (i > 0) {
            if (test[i].letter != test[i - 1].letter || i == test.size() - 1) {
                if (this->tiles.count_tiles(test[i - 1]) < curr)
                    throw MoveException("Tile not found.");
                curr = 0;
            }
        }
        curr++;
    }

    // If we made it here, the parsed letters are valid, so we return the output vector.
    return output;
}

// parse_move parses the entire move string inputted by the user.
Move HumanPlayer::parse_move(string& move_string) const {
    // We put the whole thing in a try-catch block because we will have to
    // deal with exceptions from parse_tiles and if the move is of unrecognized format/
    try {
        // We create a string stream of our inputted move so that we can deal with
        // one word at a time, which will be our to_parse string.
        stringstream ss(move_string);
        string to_parse;

        // The first word we get should be the command type. If it isn't one of the three
        // valid commands, we throw a move exception, which should start the loop over and
        // ask the user for a valid command.
        ss >> to_parse;
        if (to_parse == "PASS" || to_parse == "pass")
            // If the move is Pass, we just return the "Pass" move.
            return Move();
        else if (to_parse == "EXCHANGE" || to_parse == "exchange") {
            // If the move is Exchange, we get the next string in the stream
            // since it should be the letters the player wants to exchange.
            // We parse the tiles from the string and return the Move with the
            // vector of tiles.
            ss >> to_parse;
            return Move(this->parse_tiles(to_parse));
        } else if (to_parse == "PLACE" || to_parse == "place") {
            // If the move is Place, we will need to get the row, column,
            // direction, and string of tiles to place.
            size_t row;
            size_t column;
            Direction dir = Direction::NONE;

            // We set the direction based on the next string
            ss >> to_parse;
            if (to_parse == "-") {
                dir = Direction::ACROSS;
            } else if (to_parse == "|") {
                dir = Direction::DOWN;
            }

            // We set the row and column from the next 2
            ss >> row;
            ss >> column;

            // Finally we get the string of tiles from the player and
            // return a move with all the values we've gotten, as well as the
            // vector of tiles from parsing the tile string.
            ss >> to_parse;
            return Move(this->parse_tiles(to_parse), row - 1, column - 1, dir);
        } else {
            throw MoveException("Unrecognized move.");
        }
    } catch (const exception& e) {
        // We just ask the user for a new move and call parse_move on it again.
        string to_parse;
        std::cerr << "Error in move: " << e.what() << endl << endl;
        cout << FG_COLOR_HEADING << "Your move, " << PLAYER_NAME_COLOR << get_name() << FG_COLOR_HEADING << ": "
             << rang::style::reset;
        getline(cin, to_parse);
        return parse_move(to_parse);
    }
}

// This function is fully implemented.
void HumanPlayer::print_hand(ostream& out) const {
    const size_t tile_count = tiles.count_tiles();
    const size_t empty_tile_count = this->get_hand_size() - tile_count;
    const size_t empty_tile_width = empty_tile_count * (SQUARE_OUTER_WIDTH - 1);

    for (size_t i = 0; i < HAND_TOP_MARGIN - 2; ++i) {
        out << endl;
    }

    out << repeat(SPACE, HAND_LEFT_MARGIN) << FG_COLOR_HEADING << "Your Hand: " << endl << endl;

    // Draw top line
    out << repeat(SPACE, HAND_LEFT_MARGIN) << FG_COLOR_LINE << BG_COLOR_NORMAL_SQUARE;
    print_horizontal(tile_count, L_TOP_LEFT, T_DOWN, L_TOP_RIGHT, out);
    out << repeat(SPACE, empty_tile_width) << BG_COLOR_OUTSIDE_BOARD << endl;

    // Draw middle 3 lines
    for (size_t line = 0; line < SQUARE_INNER_HEIGHT; ++line) {
        out << FG_COLOR_LABEL << BG_COLOR_OUTSIDE_BOARD << repeat(SPACE, HAND_LEFT_MARGIN);
        for (auto it = tiles.cbegin(); it != tiles.cend(); ++it) {
            out << FG_COLOR_LINE << BG_COLOR_NORMAL_SQUARE << I_VERTICAL << BG_COLOR_PLAYER_HAND;

            // Print letter
            if (line == 1) {
                out << repeat(SPACE, 2) << FG_COLOR_LETTER << (char)toupper(it->letter) << repeat(SPACE, 2);

                // Print score in bottom right
            } else if (line == SQUARE_INNER_HEIGHT - 1) {
                out << FG_COLOR_SCORE << repeat(SPACE, SQUARE_INNER_WIDTH - 2) << setw(2) << it->points;

            } else {
                out << repeat(SPACE, SQUARE_INNER_WIDTH);
            }
        }
        if (tiles.count_tiles() > 0) {
            out << FG_COLOR_LINE << BG_COLOR_NORMAL_SQUARE << I_VERTICAL;
            out << repeat(SPACE, empty_tile_width) << BG_COLOR_OUTSIDE_BOARD << endl;
        }
    }

    // Draw bottom line
    out << repeat(SPACE, HAND_LEFT_MARGIN) << FG_COLOR_LINE << BG_COLOR_NORMAL_SQUARE;
    print_horizontal(tile_count, L_BOTTOM_LEFT, T_UP, L_BOTTOM_RIGHT, out);
    out << repeat(SPACE, empty_tile_width) << rang::style::reset << endl;
}
