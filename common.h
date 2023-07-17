#ifndef COMMON_H
#define COMMON_H

#include <iostream>

using namespace std;

// 区块结构体
struct Block {
   long   timestamp; // 时间戳
   string data; // 区块数据
   string pre_block_hash; // 前一个区块的 hash
   string hash; // 当前区块的 hash
   long   nonce; // 随机数
};

#endif // COMMON_H
