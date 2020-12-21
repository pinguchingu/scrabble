
#include "computer_player.h"

#include <algorithm>
#include <iostream>
#include <map>
#include <memory>
#include <string>

// left part finds all the possible prefixes of the given anchor that
// can be made from the letters in hand, then calls extend right on each.
void ComputerPlayer::left_part(
        Board::Position anchor_pos,
        std::string partial_word,
        Move partial_move,
        std::shared_ptr<Dictionary::TrieNode> node,
        size_t limit,
        TileCollection& remaining_tiles,
        std::vector<Move>& legal_moves,
        const Board& board) const {

    // an iterator to use with the node maps
    std::map<char, std::shared_ptr<Dictionary::TrieNode>>::iterator it;

    // extend right from the starting position
    extend_right(anchor_pos, partial_word, partial_move, node, remaining_tiles, legal_moves, board);

    // if there are possibilities for prefixes, create the prefixes that can be made from letters in
    // the current player's hand.
    if (limit > 0) {
        for (it = node->nexts.begin(); it != node->nexts.end(); it++) {
            if (remaining_tiles.has_tile(it->first)) {
                // for each extend_right call, a new partial move is created
                // with the relevant information
                Move newMove(partial_move);
                if (partial_move.direction == Direction::ACROSS)
                    newMove.column--;
                else
                    newMove.row--;

                // the specific tile that is being used in the prefix is
                // removed from the hand and added to the partial move
                TileKind curr = remaining_tiles.lookup_tile(it->first);
                remaining_tiles.remove_tile(curr);
                newMove.tiles.push_back(curr);
                left_part(
                        anchor_pos,
                        (partial_word + it->first),
                        newMove,
                        it->second,
                        limit - 1,
                        remaining_tiles,
                        legal_moves,
                        board);

                // the tile that was added is removed so that additional
                // left_part calls can be made with different prefixes.
                newMove.tiles.pop_back();
                remaining_tiles.add_tile(curr);
            }

            // if there are blank tiles in hand, the same is called as above, except
            // with the blank tile (in which case a call can be made with every single
            // child node)
            if (remaining_tiles.has_tile('?')) {
                Move newMove(partial_move);
                if (partial_move.direction == Direction::ACROSS)
                    newMove.column--;
                else
                    newMove.row--;
                TileKind curr = remaining_tiles.lookup_tile('?');
                remaining_tiles.remove_tile(curr);
                // there is a difference here - the blank tile must be assigned
                // the value of the letter being considered
                curr.assigned = it->first;
                newMove.tiles.push_back(curr);
                left_part(
                        anchor_pos,
                        (partial_word + it->first),
                        newMove,
                        it->second,
                        limit - 1,
                        remaining_tiles,
                        legal_moves,
                        board);
                remaining_tiles.add_tile(curr);
            }
        }
    }
}

// extend right creates all possible moves at the given anchor and with given
// prefix, and adds them to the legal_moves vector
void ComputerPlayer::extend_right(
        Board::Position square,
        std::string partial_word,
        Move partial_move,
        std::shared_ptr<Dictionary::TrieNode> node,
        TileCollection& remaining_tiles,
        std::vector<Move>& legal_moves,
        const Board& board) const {

    // iterator to use throughout the function for map of next nodes
    std::map<char, std::shared_ptr<Dictionary::TrieNode>>::iterator it;

    // if there is a tile already on the board, the current node has its
    // children searched for the letter, and if it is found extend_right is
    // called with no change in placed moves but using the node that is found
    if (board.in_bounds_and_has_tile(square)) {
        it = node->nexts.find(board.letter_at(square));
        if (it != node->nexts.end()) {
            extend_right(
                    square.translate(partial_move.direction),
                    (partial_word + it->first),
                    partial_move,
                    it->second,
                    remaining_tiles,
                    legal_moves,
                    board);
        }
    } else {
        // otherwise, there is a blank space, and the tiles in hand are used
        // to determine moves that can be made

        // if what has been made so far is a word, add the move to the list
        if (node->is_final) {
            legal_moves.push_back(partial_move);
        }

        // for every possible next letter, call extend right
        for (it = node->nexts.begin(); it != node->nexts.end(); it++) {

            // if the hand contains the letter, call extend right on it
            // after doing necessary steps, and then backtrack
            if (remaining_tiles.has_tile(it->first)) {
                // get the tile
                TileKind curr = remaining_tiles.lookup_tile(it->first);

                // remove the tile from hand and add it to the partial move
                remaining_tiles.remove_tile(curr);
                partial_move.tiles.push_back(curr);

                // recursive call on next board space, with updated
                // move and node and hand
                extend_right(
                        square.translate(partial_move.direction),
                        partial_word + it->first,
                        partial_move,
                        it->second,
                        remaining_tiles,
                        legal_moves,
                        board);

                // to backtrack, add tile back to hand and remove from partial move
                remaining_tiles.add_tile(curr);
                partial_move.tiles.pop_back();
            }

            // in the case that there is a blank tile in hand, every next letter
            // is possible
            // the only difference between this code and the code above is that
            // since the tile is blank, the letter is assigned to the assigned member
            if (remaining_tiles.has_tile('?')) {
                TileKind curr = remaining_tiles.lookup_tile('?');
                curr.assigned = it->first;
                partial_move.tiles.push_back(curr);
                remaining_tiles.remove_tile(curr);
                extend_right(
                        square.translate(partial_move.direction),
                        partial_word + it->first,
                        partial_move,
                        it->second,
                        remaining_tiles,
                        legal_moves,
                        board);
                remaining_tiles.add_tile(curr);
                partial_move.tiles.pop_back();
            }
        }
    }
}

// finds all possible moves with the given tiles and board, and returns the best one
// (the one that scores the highest)
Move ComputerPlayer::get_move(const Board& board, const Dictionary& dictionary) const {
    // print the board
    board.print(std::cout);

    // initialize a vector of legal moves and get the vector of anchors using
    // get_anchors
    std::vector<Move> legal_moves;
    std::vector<Board::Anchor> anchors = board.get_anchors();

    // create a copy of the hand to pass to the function
    TileCollection remaining(tiles);

    // create a blank Place move to pass to the function
    Move partial_move = Move();
    partial_move.kind = MoveKind::PLACE;

    // call left_part at every anchor
    for (size_t i = 0; i < anchors.size(); i++) {
        // set the direction of the partial move based on the info
        // from the anchor, as well as the row and column
        partial_move.direction = anchors[i].direction;
        partial_move.row = anchors[i].position.row;
        partial_move.column = anchors[i].position.column;

        // left_part only needs to be called if the anchor has a limit
        // larger than zero
        if (anchors[i].limit > 0)
            left_part(
                    anchors[i].position,
                    "",
                    partial_move,
                    dictionary.get_root(),
                    anchors[i].limit,
                    remaining,
                    legal_moves,
                    board);
        // if limit is zero, instead of calling left_part, if there are
        // tiles to the left (or above), the tiles are iterated through
        // to get the starting prefix, after which extend_right is called
        else {
            // creating a position to iterate on board
            Board::Position curr = anchors[i].position;
            curr = curr.translate(partial_move.direction, -1);
            std::string prefix;
            // while there are still tiles, add the letters to prefix
            while (board.in_bounds_and_has_tile(curr)) {
                prefix += board.letter_at(curr);
                curr = curr.translate(partial_move.direction, -1);
            }
            std::reverse(prefix.begin(), prefix.end());

            // get the node corresponding to the current prefix
            std::shared_ptr<Dictionary::TrieNode> node = dictionary.find_prefix(prefix);

            // call extend_right on it
            extend_right(anchors[i].position, prefix, partial_move, node, remaining, legal_moves, board);
        }
    }

    // after getting all the legal moves, we return the best one
    return get_best_move(legal_moves, board, dictionary);
}

// given a vector of moves, returns the highest scoring one
Move ComputerPlayer::get_best_move(
        std::vector<Move> legal_moves, const Board& board, const Dictionary& dictionary) const {

    Move best_move = Move();       // Pass if no move found
    unsigned int best_points = 0;  // stores the point value of the current best move

    // for every move in the vector, tests the move and updates best move if it is better
    for (size_t i = 0; i < legal_moves.size(); i++) {
        bool not_all_words = false;  // flag for if an invalid word is created

        // the move is tested with test_place
        PlaceResult result = board.test_place(legal_moves[i]);

        // We check to make sure every word created by the user's move is in the dictionary,
        // and set our flag to true otherwise.
        for (size_t j = 0; j < result.words.size(); ++j) {
            if (!dictionary.is_word(result.words[j])) {
                not_all_words = true;
            }
        }

        // if all the words are valid, the move itself is valid, and the move is not a pass,
        // the best move is updated
        if (!not_all_words && result.valid && !legal_moves[i].tiles.empty()) {
            if (legal_moves[i].tiles.size() == get_hand_size()) {
                if (result.points + 50 > best_points) {
                    best_move = legal_moves[i];
                    best_points = result.points + 50;
                }
            } else {
                if (result.points > best_points) {
                    best_move = legal_moves[i];
                    best_points = result.points;
                }
            }
        }
    }

    // the best move is returned
    return best_move;
}
