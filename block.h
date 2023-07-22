#ifndef BLOCK_H
#define BLOCK_H

#include "common.h"

// 创建新的区块
Block* new_block(string pre_block_hash, vector<Transaction*> transactions);

// 生成创世区块
Block* generate_genesis_block(Transaction* coinbase_tx);

#endif // BLOCK_H
