#include <chrono>
#include <openssl/sha.h>
#include "block.h"
#include "proofofwork.h"
#include "util.h"

// 创建新的区块
Block* new_block(string data, string pre_block_hash) {
    Block *block = new Block;
    block->timestamp = current_timestamp();
    block->data = data;
    block->pre_block_hash = pre_block_hash;
    // 计算区块哈希
    ProofOfWork pow = ProofOfWork(block);
    pair<long, string> ans = pow.run();
    block->nonce = ans.first;
    block->hash = ans.second;
    return block;
}

// 创建创世区块
Block* new_genesis_block() {
    return new_block("Genesis Block", "");
}

