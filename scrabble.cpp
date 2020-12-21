#include "scrabble.h"

#include "formatting.h"
#include <iomanip>
#include <iostream>
#include <map>

using namespace std;

// Given to you. this does not need to be changed
Scrabble::Scrabble(const ScrabbleConfig& config)
        : hand_size(config.hand_size),
          minimum_word_length(config.minimum_word_length),
          tile_bag(TileBag::read(config.tile_bag_file_path, config.seed)),
          board(Board::read(config.board_file_path)),
          dictionary(Dictionary::read(config.dictionary_file_path)) {}

// Game Loop should cycle through players and get and execute that players move
// until the game is over.
void Scrabble::game_loop() {
    // We initialize some variables to keep track of when we should end the game
    size_t passed_in_row = 0;
    bool all_players_have_tiles = true;

    // We clear the string stream for our first move.
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    // We want to keep getting moves from the players until either all players have
    // passed in a row or one of the players is out of tiles.
    while (passed_in_row < (this->players.size() - non_human_players) && all_players_have_tiles) {
        // We get the current player's index based on the number of moves that have occured.
        size_t p = board.get_move_index() % this->players.size();

        // We get the move from the curent player and test the place result with it.
        if (players[p]->is_human()) {
            Move curr_move = players[p]->get_move(board, dictionary);
            PlaceResult result = board.test_place(curr_move);

            // If we got an invalid place result, we ask the user to give us a correct one.
            while (!result.valid) {
                cout << "Error in move: " << result.error << endl << endl;
                curr_move = players[p]->get_move(board, dictionary);
                result = board.test_place(curr_move);
            }

            if (curr_move.kind == MoveKind::PASS) {
                // If the move is pass, we increment the turn index and passed in row variable,
                // and output our generic output.
                board.place(curr_move);
                if (players[p]->is_human())
                    passed_in_row++;
                cout << "Your current score: " << SCORE_COLOR << players[p]->get_points() << rang::style::reset << endl;
                cout << "Press [enter] to confirm." << endl;
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
            } else if (curr_move.kind == MoveKind::EXCHANGE) {
                // If the move is exchange, we increment the turn index, remove the tiles from the player's
                // collection, add them back to the bag, and replace the missing tiles with random ones
                // from the bag.
                // We also set pass_in_row back to zero.
                board.place(curr_move);
                passed_in_row = 0;
                players[p]->remove_tiles(curr_move.tiles);
                for (size_t n = 0; n < curr_move.tiles.size(); ++n) {
                    tile_bag.add_tile(curr_move.tiles[n]);
                }
                vector<TileKind> draw = tile_bag.remove_random_tiles(curr_move.tiles.size());
                players[p]->add_tiles(draw);

                // We output our generic output.
                cout << "Your current score: " << SCORE_COLOR << players[p]->get_points() << rang::style::reset << endl;
                cout << "Press [enter] to confirm." << endl;
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
            } else if (curr_move.kind == MoveKind::PLACE) {
                // If the move is place, we place the move and remove the used tiles from the player's hand.
                // We also set pass_in_row back to zero and refill the players hand with random tiles.
                // Finally, we print out the number of points gained and the generic output.
                passed_in_row = 0;
                board.place(curr_move);
                players[p]->remove_tiles(curr_move.tiles);
                vector<TileKind> draw = tile_bag.remove_random_tiles(curr_move.tiles.size());
                players[p]->add_tiles(draw);
                if (curr_move.tiles.size() == hand_size) {
                    players[p]->add_points(result.points);
                    players[p]->add_points(EMPTY_HAND_BONUS);
                    cout << "You gained " << SCORE_COLOR << result.points + EMPTY_HAND_BONUS << rang::style::reset
                         << " points!" << endl;
                } else {
                    players[p]->add_points(result.points);
                    cout << "You gained " << SCORE_COLOR << result.points << rang::style::reset << " points!" << endl;
                }
                cout << "Your current score: " << SCORE_COLOR << players[p]->get_points() << rang::style::reset << endl
                     << endl;
                cout << "Press [enter] to continue." << endl;
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
            }
        } else {
            // if the player isn't human, get_move is called and placed (it doesn't need to be checked
            // because the returned move is guaranteed to be valid)
            Move curr_move = players[p]->get_move(board, dictionary);
            PlaceResult result = board.place(curr_move);

            // the tiles used are removed from the hand and the hand is refilled
            players[p]->remove_tiles(curr_move.tiles);
            vector<TileKind> draw = tile_bag.remove_random_tiles(curr_move.tiles.size());
            players[p]->add_tiles(draw);

            // points are added based on the words created
            if (curr_move.tiles.size() == hand_size) {
                players[p]->add_points(result.points + EMPTY_HAND_BONUS);
                cout << "You gained " << SCORE_COLOR << result.points + EMPTY_HAND_BONUS << rang::style::reset
                     << " points!" << endl;
            } else if (curr_move.kind == MoveKind::PLACE) {
                players[p]->add_points(result.points);
                cout << "You gained " << SCORE_COLOR << result.points << rang::style::reset << " points!" << endl;
            }

            // the generic score output is presented for the computer
            cout << "Your current score: " << SCORE_COLOR << players[p]->get_points() << rang::style::reset << endl
                 << endl;
            cout << "Press [enter] to continue." << endl;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
        }

        if (players[p]->count_tiles() == 0) {
            // If the current player has no tiles left, we set the flag so the loop ends next turn.
            all_players_have_tiles = false;
        }
    }
}

// Add players begins the game and sets up the players
void Scrabble::add_players() {
    // Asks the players to enter the number of players
    cout << "Please enter number of players: ";
    size_t num_players;
    cin >> num_players;

    // If the number of players is too large, we just start over.
    if (num_players > 8) {
        cout << "Error: Maximum 8 players." << endl;
        return add_players();
    }

    // We confirm the number of players.
    cout << num_players << " players confirmed." << endl;

    // For each player, we ask the players to enter their name,
    // create a player object for them based on whether they are a
    // computer or not, and initialize their hand
    // and add them to the vector of players.
    string name;  // string to hold name
    char c;       // char to get response on whether player is computer or not
    for (size_t i = 0; i < num_players; ++i) {
        cout << "Please enter name for player " << i + 1 << ": ";
        cin >> name;
        cout << "Is " << name << " a computer? (y/n)" << endl;
        cin >> c;
        shared_ptr<Player> newPlayer;

        // simple if statement for making player a computer or human
        if (c == 'y') {
            newPlayer = make_shared<ComputerPlayer>(name, this->hand_size);
            non_human_players++;
        } else {
            newPlayer = make_shared<HumanPlayer>(name, this->hand_size);
        }

        // initialize the player's hand
        vector<TileKind> draw = tile_bag.remove_random_tiles(hand_size);
        newPlayer->add_tiles(draw);
        this->players.push_back(newPlayer);
        cout << "Player " << i + 1 << ", named \"" << players[i]->get_name() << "\" has been added." << endl;
    }
}

// Performs final score subtraction. Players lose points for each tile in their
// hand. The player who cleared their hand receives all the points lost by the
// other players.
void Scrabble::final_subtraction(vector<shared_ptr<Player>>& plrs) {
    // We set up an int to hold how many points the cleared hand player gets
    // as well as a flag to keep track of if a player has cleared their hand
    // and an index to save which player it was.
    int total_lost = 0;
    bool hand_gone = false;
    size_t winning_player_index = 0;

    // For each player, we subtract from their points the total value of their hand
    // and check to see if the player has run out of tiles. We then set the correct
    // values in lost points and winning index.
    for (size_t i = 0; i < plrs.size(); ++i) {
        plrs[i]->subtract_points(plrs[i]->get_hand_value());
        total_lost += plrs[i]->get_hand_value();
        if (plrs[i]->count_tiles() == 0) {
            winning_player_index = i;
            hand_gone = true;
        }
    }

    // If a player has lost their entire hand, we give them all the points
    // lost by the other players.
    if (hand_gone) {
        plrs[winning_player_index]->add_points(total_lost);
    }
}

// You should not need to change this function.
void Scrabble::print_result() {
    // Determine highest score
    size_t max_points = 0;
    for (auto player : this->players) {
        if (player->get_points() > max_points) {
            max_points = player->get_points();
        }
    }

    // Determine the winner(s) indexes
    vector<shared_ptr<Player>> winners;
    for (auto player : this->players) {
        if (player->get_points() >= max_points) {
            winners.push_back(player);
        }
    }

    cout << (winners.size() == 1 ? "Winner:" : "Winners: ");
    for (auto player : winners) {
        cout << SPACE << PLAYER_NAME_COLOR << player->get_name();
    }
    cout << rang::style::reset << endl;

    // now print score table
    cout << "Scores: " << endl;
    cout << "---------------------------------" << endl;

    // Justify all integers printed to have the same amount of character as the high score, left-padding with spaces
    cout << setw(static_cast<uint32_t>(floor(log10(max_points) + 1)));

    for (auto player : this->players) {
        cout << SCORE_COLOR << player->get_points() << rang::style::reset << " | " << PLAYER_NAME_COLOR
             << player->get_name() << rang::style::reset << endl;
    }
}

// You should not need to change this.
void Scrabble::main() {
    add_players();
    game_loop();
    final_subtraction(this->players);
    print_result();
}
