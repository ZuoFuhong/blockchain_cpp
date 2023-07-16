#ifndef BLOCK_H
#define BLOCK_H

#include <iostream>

using namespace std;

// 区块结构体
struct Block {
   long   timestamp; // 时间戳
   string data; // 区块数据
   string pre_block_hash; // 前一个区块的 hash
   string hash; // 当前区块的 hash
};

// 创建新的区块
Block* new_block(string data, string pre_block_hash);

// 创建创世区块
Block* new_genesis_block();

#endif // BLOCK_H
