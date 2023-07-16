#include "blockchain.h"

// 创建新的区块链
Blockchain::Blockchain() {
    blocks.push_back(new_genesis_block());
}

// 添加区块
void Blockchain::add_block(string data) {
    auto prev_block = blocks.back();
    Block *block = new_block(data, prev_block->hash);
    blocks.push_back(block);
}

// 获取所有区块
vector<Block*> Blockchain::get_blocks() {
    return blocks;
}

