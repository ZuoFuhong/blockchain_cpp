#ifndef BLOCK_H
#define BLOCK_H

#include "common.h"

// 创建新的区块
Block* new_block(string data, string pre_block_hash);

// 创建创世区块
Block* new_genesis_block();

// 对象序列化
string block_to_string(Block *block);

// 对象反序列化
Block* string_to_block(string block_str);

#endif // BLOCK_H
