#include "board.h"

#include "board_square.h"
#include "exceptions.h"
#include "formatting.h"
#include <fstream>
#include <iomanip>

using namespace std;
char Board::letter_at(Position p) const {
    if (at(p).get_tile_kind().letter == '?')
        return at(p).get_tile_kind().assigned;
    return at(p).get_tile_kind().letter;
}

bool Board::Position::operator==(const Board::Position& other) const {
    return this->row == other.row && this->column == other.column;
}

bool Board::Position::operator!=(const Board::Position& other) const {
    return this->row != other.row || this->column != other.column;
}

Board::Position Board::Position::translate(Direction direction) const { return this->translate(direction, 1); }

Board::Position Board::Position::translate(Direction direction, ssize_t distance) const {
    if (direction == Direction::DOWN) {
        return Board::Position(this->row + distance, this->column);
    } else {
        return Board::Position(this->row, this->column + distance);
    }
}

// Read reads the inputted file to create the board.
Board Board::read(const string& file_path) {
    // We specify the file and create an ifstream with it.
    // If the file doesn't exist or is of wrong format, we throw
    // a file exception.
    ifstream file(file_path);
    if (!file) {
        throw FileException("cannot open board file!");
    }

    // We initialize values for the variables we are getting from the file.
    size_t rows;
    size_t columns;
    size_t starting_row;
    size_t starting_column;

    // Using the input from the file, we create our board.
    file >> rows >> columns >> starting_row >> starting_column;
    Board board(rows, columns, starting_row, starting_column);

    // This char will be used to read in one character at a time for each board space.
    char x;

    // For each row, we will read in each value for each column.
    for (size_t i = 0; i < rows; ++i) {
        // We create a new row to put our values into.
        vector<BoardSquare> newList;
        board.squares.push_back(newList);

        // For each column per row, we will enter in one value at a time.
        for (size_t j = 0; j < columns; ++j) {
            // We read in the next character from the file.
            file >> x;

            // We create generic board square which is standard
            // (with letter multiplier 1 and word multiplier 1).
            BoardSquare b_s(1, 1);

            // Based on the character we read in, we will update the
            // values of our board_squre to reflect the multipliers we want.
            switch (x) {
            case '.':
                // . is a standard board square so we just return.
                break;

            case '2':
                // 2 means double letter, so we update the letter multiplier.
                b_s = BoardSquare(2, 1);
                break;

            case '3':
                // 3 means triple letter, so we update the letter multiplier.
                b_s = BoardSquare(3, 1);
                break;

            case 'd':
                // d means double word, so we update the word multiplier.
                b_s = BoardSquare(1, 2);
                break;

            case 't':
                // t means triple word, so we update the word multiplier.
                b_s = BoardSquare(1, 3);
                break;

            default:
                // If we get anything else, we throw an exception.
                throw FileException("Invalid Board");
                break;
            }

            // We insert the board_square into the correct location on the board.
            board.squares[i].push_back(b_s);
        }
    }

    // Finally, we return the board.
    return board;
}

// This is a simple getter for move_index
size_t Board::get_move_index() const { return this->move_index; }

// test_place essentially tests what would happen if we were to put down the
// move given by the player. It checks to make sure that putting the move down
// is valid and calculates the points and words that would be created.
PlaceResult Board::test_place(const Move& move) const {
    // Since the PlaceResult can be "invalid" we won't be checking for exceptions.
    // Any error will be contained in the returned Place Result.

    // We initialize flags for errors and ints for multipliers that we will be
    // using.
    bool touching_flag = false;
    bool start_flag = false;
    unsigned int total_points = 0;
    unsigned int main_points = 0;
    unsigned int total_multiplier = 1;

    // We create a variable for the direction that the word is going (the tiles will be placed),
    // and an antidirection for checking the words that can be created to the sides of each
    // tile placed. Based on the inputted direction, we infer the antidirection.
    Direction dir;
    Direction antidir;
    switch (move.direction) {
    case Direction::ACROSS:
        dir = Direction::ACROSS;
        antidir = Direction::DOWN;
        break;

    case Direction::DOWN:
        dir = Direction::DOWN;
        antidir = Direction::ACROSS;
        break;

    default:
        break;
    }

    // We initialize our first position with the row and column specified in the move.
    // We also create a hold position which we will use later to keep track of a specific position.
    // We initialize it with the starting position.
    Position curr(move.row, move.column);
    Position hold(curr);

    // We initialize a vector of strings to hold the words we create, as well as a
    // main word string which will hold the word created with the tiles placed.
    vector<string> words;
    string main_word;

    // Our index for our main loop!
    size_t i = 0;

    // If the move is a Pass or Exchange, we return empty word set and 0 points.
    if (move.kind == MoveKind::PASS)
        return PlaceResult(words, 0);

    if (move.kind == MoveKind::EXCHANGE) {
        return PlaceResult(words, 0);
    }

    // If the starting location is out of bounds or is on top of an exsiting tile,
    // we return an invalid move.
    if (!is_in_bounds(curr))
        return PlaceResult("Starting position must be in bounds");
    if (at(curr).has_tile())
        return PlaceResult("Cannot place a tile on top of another.");

    // If there is no tile on the starting tile, this must be the first move, and
    // if the first move does not have a tile on the starting location, we return
    // an invalid move.
    if (!at(start).has_tile()) {
        touching_flag = true;
        for (size_t n = 0; n < move.tiles.size(); ++n) {
            if (curr.translate(dir, n) == start)
                start_flag = true;
        }
        if (!start_flag)
            return PlaceResult("First move must start on start spot");
    }

    // If we are adding onto an existing word (for example, grand -> grandmother),
    // we will need to traverse into the existing word to add the letters to the
    // main word and add the points to the main points.
    while (this->in_bounds_and_has_tile(curr.translate(dir, -1))) {
        // We move into the old word in reverse, and add the letter and points
        curr = curr.translate(dir, -1);
        if (at(curr).get_tile_kind().letter == TileKind::BLANK_LETTER) {
            main_word.push_back(at(curr).get_tile_kind().assigned);
        } else {
            main_word.push_back(at(curr).get_tile_kind().letter);
        }
        main_points += at(curr).get_tile_kind().points;

        // If this loop runs, we know the tiles placed are touching an existing word.
        touching_flag = true;
    }
    // We reverse our main_word string because we added the letters in reverse order.
    reverse(main_word.begin(), main_word.end());

    // We then move our position back to the starting position.
    curr = hold;

    // In our main loop, we will act as if we are placing each tile down.
    while (i < move.tiles.size()) {
        // If we ever try to place a tile in  position that is not in bounds,
        // we will immediately return an invalid Place Result.
        if (!is_in_bounds(curr)) {
            return PlaceResult("Move must be in bounds");
        }

        // We create variables for the "side" words (words that are created
        // by branching off the main word tiles.)
        string word = "";
        unsigned int multiplier = 1;
        unsigned int side_points = 0;

        // If there is a letter in the next position, we skip over it, adding the
        // letters and points to the main word.
        while (this->in_bounds_and_has_tile(curr)) {
            // If this loop runs, we know the tiles are touching an existing tile.
            touching_flag = true;
            main_points += at(curr).get_tile_kind().points;
            if (at(curr).get_tile_kind().letter == TileKind::BLANK_LETTER) {
                main_word.push_back(at(curr).get_tile_kind().assigned);
            } else {
                main_word.push_back(at(curr).get_tile_kind().letter);
            }

            // We move to the next position we would add a tile.
            curr = curr.translate(dir);
        }

        // At each location, we want to add the current next tile into our
        // main word and add the points to our main points.
        if (move.tiles[i].letter == TileKind::BLANK_LETTER) {
            main_word.push_back(move.tiles[i].assigned);
        } else {
            main_word.push_back(move.tiles[i].letter);
        }
        if (this->is_in_bounds(curr)) {
            main_points += at(curr).letter_multiplier * move.tiles[i].points;

            // We increase the multiplier for our main word by the multiplier of
            // the current location, as well as a multiplier for any side words we create
            // from the current tile.
            total_multiplier *= at(curr).word_multiplier;
            multiplier *= at(curr).word_multiplier;
        }
        // If there is a tile in the antidiretion, we have side word formed,
        // so we know the tiles are touching an existing tile, and we want to
        // add our the points and multiplier of our placed tile to the side word
        // variables.
        if (this->in_bounds_and_has_tile(curr.translate(antidir, -1))
            || this->in_bounds_and_has_tile(curr.translate(antidir, 1))) {
            touching_flag = true;
            side_points += at(curr).letter_multiplier * move.tiles[i].points;
        }

        // We set a holding point at our current location so we can come back to where
        // we are in the main direction.
        hold = curr;

        // While there is still a tile in the negative antidirection, we continue to
        // traverse in that direction, adding points to our side point variable and adding
        // the letter to our side word string.
        while (this->in_bounds_and_has_tile(curr.translate(antidir, -1))) {
            curr = curr.translate(antidir, -1);
            side_points += at(curr).get_tile_kind().points;
            if (at(curr).get_tile_kind().letter == TileKind::BLANK_LETTER) {
                word.push_back(at(curr).get_tile_kind().assigned);
            } else {
                word.push_back(at(curr).get_tile_kind().letter);
            }
        }
        // Since we traversed the word in opposite order, we reverse the string holding the
        // word.
        reverse(word.begin(), word.end());

        // We go back to the location in the main direction
        curr = hold;

        // We add the letter that would be placed in that spot to the side word string
        if (move.tiles[i].letter == TileKind::BLANK_LETTER) {
            word.push_back(move.tiles[i].assigned);
        } else {
            word.push_back(move.tiles[i].letter);
        }

        // We repeat the above loop, but going in the positive antidirection.
        while (this->in_bounds_and_has_tile(curr.translate(antidir, 1))) {
            curr = curr.translate(antidir, 1);
            side_points += at(curr).get_tile_kind().points;
            if (at(curr).get_tile_kind().letter == TileKind::BLANK_LETTER) {
                word.push_back(at(curr).get_tile_kind().assigned);
            } else {
                word.push_back(at(curr).get_tile_kind().letter);
            }
        }

        // We go back to the spot in the main direction.
        curr = hold;

        // If the side word string is only one character, there wasn't
        // actually a new word formed, but otherwise, we add the side points
        // to the main total points after multiplying them by the correct multiplier.
        // We also add the word to our vector of words created.
        if (word.size() > 1) {
            side_points *= multiplier;
            total_points += side_points;
            words.push_back(word);
        }

        // Finally, we increase our letter so we move to the next placed tile in our vector,
        // and traverse one in the main direction.
        i++;
        curr = curr.translate(dir);
    }

    // Finally, we set a new hold at the current location
    hold = curr;

    // If we added to the beginning of an existing word (mother -> grandmother),
    // we need to add the letters and points from the existing word to our variables.
    // Since we translated at the end of our last loop, this loop will also traverse at the end,
    // unlike our side word loops.
    while (this->in_bounds_and_has_tile(curr)) {
        touching_flag = true;
        main_points += at(curr).get_tile_kind().points;
        if (at(curr).get_tile_kind().letter == TileKind::BLANK_LETTER) {
            main_word.push_back(at(curr).get_tile_kind().assigned);
        } else {
            main_word.push_back(at(curr).get_tile_kind().letter);
        }
        curr = curr.translate(dir);
    }

    // If we never ended up connecting to an existing word, we return an invalid place result
    if (!touching_flag)
        return PlaceResult("Word must be touching existing word.");

    // If the main word is only one character, it must be because only one tile
    // was placed down, so our "main word" could have been in the antidirection.
    // Otherwise, we push the main word to our vector and multiply and add the points
    // to the total points.
    if (main_word.size() > 1) {
        words.push_back(main_word);
        main_points *= total_multiplier;
        total_points += main_points;
    }

    // If we made it here, it's a valid place result, so we return such.
    return PlaceResult(words, total_points);
}

// place checks to make sure a move is valid using the test place and then
// if it is, actually places the tiles on the board.
PlaceResult Board::place(const Move& move) {
    // If the move is a pass or exchange, we just want to increment the move_index
    // (which we use to keep track of which player's turn it is), so we pass an "invalid"
    // place result just to exit the loop.
    if (move.kind == MoveKind::PASS || move.kind == MoveKind::EXCHANGE) {
        this->move_index++;
        return PlaceResult("Successful Pass/Exchange.");
    }

    // We test the move with test_place
    PlaceResult result = test_place(move);

    // If the result is invalid we just return it, because we won't be
    // putting any tiles down.
    if (result.valid == false)
        return result;

    // We set our starting position to the inputted position.
    // We initialize our index at zero.
    Position curr(move.row, move.column);
    size_t i = 0;

    // We determine the direction we are tranversing from the move.
    Direction dir;
    switch (move.direction) {
    case Direction::ACROSS:
        dir = Direction::ACROSS;
        break;

    case Direction::DOWN:
        dir = Direction::DOWN;
        break;

    default:
        break;
    }

    // We just traverse while there are still tiles to place down.
    while (i < move.tiles.size()) {
        // If there's already a tile at the location, we move to the next location.
        while (at(curr).has_tile()) {
            curr = curr.translate(dir);
        }

        // We set the tile with the tile from our move
        at(curr).set_tile_kind(move.tiles[i]);

        // we increment the index and traverse to the next tile.
        ++i;
        curr = curr.translate(dir);
    }

    // We increment the move index and return the place result.
    move_index++;
    return result;
}

// finds all anchors of the current board
std::vector<Board::Anchor> Board::get_anchors() const {
    std::vector<Anchor> output;  // vector of anchors to output
    size_t marker = 0;           // a marker to store the last anchor in the given column/row

    // if it is the first move, the only anchors are the starting location
    if (move_index == 0) {
        output.push_back(Anchor(start, Direction::ACROSS, start.column));
        output.push_back(Anchor(start, Direction::DOWN, start.row));
        return output;
    }

    // iterate through every column for every row
    for (size_t row = 0; row < this->rows; row++) {
        // marker is set to 0 at the beginning of each row
        marker = 0;
        for (size_t col = 0; col < this->columns; col++) {
            Position curr(row, col);  // a position to use to iterate through places on board

            // if the given place is adjacent to a tile, then it is an anchor
            if (in_bounds_and_has_tile(curr.translate(Direction::ACROSS))
                || in_bounds_and_has_tile(curr.translate(Direction::ACROSS, -1))
                || in_bounds_and_has_tile(curr.translate(Direction::DOWN))
                || in_bounds_and_has_tile(curr.translate(Direction::DOWN, -1))) {

                // if there is no tile, then it is an anchor, and add it to the vector
                // here, col - marker is used, because the marker is the last anchor
                // or if no anchors in the row have been found, the left edge of the board
                if (!in_bounds_and_has_tile(curr)) {
                    output.push_back(Anchor(curr, Direction::ACROSS, col - marker));
                }

                // marker is updated to the most recent anchor
                marker = col + 1;
            }

            // if there is a tile, then marker is still updated
            if (in_bounds_and_has_tile(curr)) {
                marker = col + 1;
            }
        }
    }

    // the same idea is used going in the column direction
    for (size_t col = 0; col < this->columns; col++) {
        marker = 0;
        for (size_t row = 0; row < this->rows; row++) {
            Position curr(row, col);
            if (in_bounds_and_has_tile(curr.translate(Direction::DOWN))
                || in_bounds_and_has_tile(curr.translate(Direction::DOWN, -1))
                || in_bounds_and_has_tile(curr.translate(Direction::ACROSS))
                || in_bounds_and_has_tile(curr.translate(Direction::ACROSS, -1))) {
                if (!in_bounds_and_has_tile(curr)) {
                    output.push_back(Anchor(curr, Direction::DOWN, row - marker));
                }
                marker = row + 1;
            }
            if (in_bounds_and_has_tile(curr)) {
                marker = row + 1;
            }
        }
    }

    // after iterating through every location, return the output vector
    return output;
}

// The rest of this file is provided for you. No need to make changes.

BoardSquare& Board::at(const Board::Position& position) { return this->squares.at(position.row).at(position.column); }

const BoardSquare& Board::at(const Board::Position& position) const {
    return this->squares.at(position.row).at(position.column);
}

bool Board::is_in_bounds(const Board::Position& position) const {
    return position.row < this->rows && position.column < this->columns;
}

bool Board::in_bounds_and_has_tile(const Position& position) const {
    return is_in_bounds(position) && at(position).has_tile();
}

void Board::print(ostream& out) const {
    // Draw horizontal number labels
    for (size_t i = 0; i < BOARD_TOP_MARGIN - 2; ++i) {
        out << std::endl;
    }
    out << FG_COLOR_LABEL << repeat(SPACE, BOARD_LEFT_MARGIN);
    const size_t right_number_space = (SQUARE_OUTER_WIDTH - 3) / 2;
    const size_t left_number_space = (SQUARE_OUTER_WIDTH - 3) - right_number_space;
    for (size_t column = 0; column < this->columns; ++column) {
        out << repeat(SPACE, left_number_space) << std::setw(2) << column + 1 << repeat(SPACE, right_number_space);
    }
    out << std::endl;

    // Draw top line
    out << repeat(SPACE, BOARD_LEFT_MARGIN);
    print_horizontal(this->columns, L_TOP_LEFT, T_DOWN, L_TOP_RIGHT, out);
    out << endl;

    // Draw inner board
    for (size_t row = 0; row < this->rows; ++row) {
        if (row > 0) {
            out << repeat(SPACE, BOARD_LEFT_MARGIN);
            print_horizontal(this->columns, T_RIGHT, PLUS, T_LEFT, out);
            out << endl;
        }

        // Draw insides of squares
        for (size_t line = 0; line < SQUARE_INNER_HEIGHT; ++line) {
            out << FG_COLOR_LABEL << BG_COLOR_OUTSIDE_BOARD;

            // Output column number of left padding
            if (line == 1) {
                out << repeat(SPACE, BOARD_LEFT_MARGIN - 3);
                out << std::setw(2) << row + 1;
                out << SPACE;
            } else {
                out << repeat(SPACE, BOARD_LEFT_MARGIN);
            }

            // Iterate columns
            for (size_t column = 0; column < this->columns; ++column) {
                out << FG_COLOR_LINE << BG_COLOR_NORMAL_SQUARE << I_VERTICAL;
                const BoardSquare& square = this->squares.at(row).at(column);
                bool is_start = this->start.row == row && this->start.column == column;

                // Figure out background color
                if (square.word_multiplier == 2) {
                    out << BG_COLOR_WORD_MULTIPLIER_2X;
                } else if (square.word_multiplier == 3) {
                    out << BG_COLOR_WORD_MULTIPLIER_3X;
                } else if (square.letter_multiplier == 2) {
                    out << BG_COLOR_LETTER_MULTIPLIER_2X;
                } else if (square.letter_multiplier == 3) {
                    out << BG_COLOR_LETTER_MULTIPLIER_3X;
                } else if (is_start) {
                    out << BG_COLOR_START_SQUARE;
                }

                // Text
                if (line == 0 && is_start) {
                    out << "  \u2605  ";
                } else if (line == 0 && square.word_multiplier > 1) {
                    out << FG_COLOR_MULTIPLIER << repeat(SPACE, SQUARE_INNER_WIDTH - 2) << 'W' << std::setw(1)
                        << square.word_multiplier;
                } else if (line == 0 && square.letter_multiplier > 1) {
                    out << FG_COLOR_MULTIPLIER << repeat(SPACE, SQUARE_INNER_WIDTH - 2) << 'L' << std::setw(1)
                        << square.letter_multiplier;
                } else if (line == 1 && square.has_tile()) {
                    char l = square.get_tile_kind().letter == TileKind::BLANK_LETTER ? square.get_tile_kind().assigned
                                                                                     : ' ';
                    out << repeat(SPACE, 2) << FG_COLOR_LETTER << square.get_tile_kind().letter << l
                        << repeat(SPACE, 1);
                } else if (line == SQUARE_INNER_HEIGHT - 1 && square.has_tile()) {
                    // I fixed this so tht the board would be able to handle double digit letter scores
                    if (square.get_points() > 9)
                        out << repeat(SPACE, SQUARE_INNER_WIDTH - 2) << FG_COLOR_SCORE << square.get_points();
                    else
                        out << repeat(SPACE, SQUARE_INNER_WIDTH - 1) << FG_COLOR_SCORE << square.get_points();
                } else {
                    out << repeat(SPACE, SQUARE_INNER_WIDTH);
                }
            }

            // Add vertical line
            out << FG_COLOR_LINE << BG_COLOR_NORMAL_SQUARE << I_VERTICAL << BG_COLOR_OUTSIDE_BOARD << std::endl;
        }
    }

    // Draw bottom line
    out << repeat(SPACE, BOARD_LEFT_MARGIN);
    print_horizontal(this->columns, L_BOTTOM_LEFT, T_UP, L_BOTTOM_RIGHT, out);
    out << endl << rang::style::reset << std::endl;
}
