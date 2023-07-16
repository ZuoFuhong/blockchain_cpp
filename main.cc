#include <iostream>
#include <vector>
#include "block.h"
#include "blockchain.h"

int main() {
    Blockchain *blockchain = new Blockchain();
    blockchain->add_block("Send 1 BTC to Ivan");
    blockchain->add_block("Send 2 more BTC to Ivan");
    for (auto block : blockchain->get_blocks()) {
        std::cout << "Prev. hash: " << block->pre_block_hash << std::endl;
        std::cout << "Data: " << block->data << std::endl;
        std::cout << "Hash: " << block->hash << std::endl;
        std::cout << std::endl;
    }
}

