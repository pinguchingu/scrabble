#include "dictionary.h"

#include "exceptions.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <memory>
#include <vector>

using namespace std;

string lower(string str) {
    transform(str.cbegin(), str.cend(), str.begin(), ::tolower);
    return str;
}

// Implemented for you to read dictionary file and
// construct dictionary trie graph for you
Dictionary Dictionary::read(const std::string& file_path) {
    ifstream file(file_path);
    if (!file) {
        throw FileException("cannot open dictionary file!");
    }
    std::string word;
    Dictionary dictionary;
    dictionary.root = make_shared<TrieNode>();

    while (!file.eof()) {
        file >> word;
        if (word.empty()) {
            break;
        }
        dictionary.add_word(lower(word));
    }

    return dictionary;
}

bool Dictionary::is_word(const string& word) const {
    shared_ptr<TrieNode> cur = find_prefix(word);  // the node of the word is found
    if (cur == nullptr)
        return false;

    // if the current node is marked as a final word,
    // return true
    if (cur->is_final == true)
        return true;
    else
        return false;
}

shared_ptr<Dictionary::TrieNode> Dictionary::find_prefix(const string& prefix) const {
    shared_ptr<TrieNode> cur = this->get_root();   // the beginning node is the root node
    map<char, shared_ptr<TrieNode>>::iterator it;  // an iterator to use

    // for every letter, the corresponding node is found in the children of the
    // current node
    for (char letter : prefix) {
        // if there is no child of cur using `letter`
        //     return nullptr
        // set cur to the child of cur that uses `letter`
        it = cur->nexts.find(letter);
        if (it == cur->nexts.end()) {
            return nullptr;
        } else {
            cur = it->second;
        }
    }

    // at the end of the prefix, return the node that is reached
    return cur;
}

// Implemented for you to build the dictionary trie
void Dictionary::add_word(const string& word) {
    shared_ptr<TrieNode> cur = root;
    for (char letter : word) {
        if (cur->nexts.find(letter) == cur->nexts.end()) {
            cur->nexts.insert({letter, make_shared<TrieNode>()});
        }
        cur = cur->nexts.find(letter)->second;
    }
    cur->is_final = true;
}

vector<char> Dictionary::next_letters(const std::string& prefix) const {
    // find the current node with the given prefix
    shared_ptr<TrieNode> cur = find_prefix(prefix);
    vector<char> nexts;  // a vector of characters to output
    if (cur == nullptr)
        return nexts;
    // add all letters in cur->nexts to `nexts`
    for (map<char, shared_ptr<TrieNode>>::iterator it = cur->nexts.begin(); it != cur->nexts.end(); it++) {
        nexts.push_back(it->first);
    }
    // return nexts
    return nexts;
}
