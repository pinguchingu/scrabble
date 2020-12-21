#include "player.h"

#include <iostream>

using namespace std;

// add_points simply increments the member variable.
void Player::add_points(size_t points) { this->points += points; }

// Subtracts points from player's score
void Player::subtract_points(size_t points) {
    // we first create a copy of the player's points that
    // is signed, so we can keep track of whether the points
    // become negative. Otherwise, we may get overflow.
    int curr_points = this->points;
    curr_points -= (int)points;

    // If the points become negative, we just set player points to 0.
    // Otherwise, we just subtract from the member variable.
    if (curr_points < 0)
        this->points = 0;
    else
        this->points -= points;
}

// get_points is just a getter for points.
size_t Player::get_points() const { return this->points; }

// get_name is just a getter for name.
const std::string& Player::get_name() const { return this->name; }

// count tiles calls the count from tile collection and returns it.
size_t Player::count_tiles() const { return this->tiles.count_tiles(); }

// remove_tiles removes tiles from player's hand.
void Player::remove_tiles(const std::vector<TileKind>& tiles) {
    // for every tile in the tiles vector, we call the tile collection
    // remove to remove it.
    // If we try to remove a tile that doesnt exist, we throw an exception.
    for (size_t i = 0; i < tiles.size(); ++i) {
        try {
            this->tiles.remove_tile(tiles[i]);
        } catch (const exception& e) {
            throw out_of_range("No tile to remove.");
        }
    }
}

// add_tiles Adds tiles to player's hand.
void Player::add_tiles(const std::vector<TileKind>& tiles) {
    // for every tile in the tiles vector, we add that tile to
    // the tile collection that serves as the player's hand.
    for (size_t i = 0; i < tiles.size(); ++i) {
        this->tiles.add_tile(tiles[i]);
    }
}

// Checks if player has a matching tile.
bool Player::has_tile(TileKind tile) { return (this->tiles.count_tiles(tile)); }

// Returns the total points of all tiles in the players hand.
unsigned int Player::get_hand_value() const { return this->tiles.total_points(); }

// get_hand_size is just a getter for hand size.
size_t Player::get_hand_size() const { return this->hand_size; }