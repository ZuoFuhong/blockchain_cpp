#ifndef BLOCK_H
#define BLOCK_H

#include "common.h"

// 创建新的区块
Block* new_block(string data, string pre_block_hash);

// 创建创世区块
Block* new_genesis_block();

#endif // BLOCK_H
