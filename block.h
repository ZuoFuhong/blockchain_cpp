#ifndef BLOCK_H
#define BLOCK_H

#include "transaction.h"

// 区块
struct Block {
   long   timestamp; // 时间戳
   string pre_block_hash; // 前一个区块的 hash
   string hash; // 当前区块的 hash
   vector<Transaction*> transactions; // 交易数据
   long   nonce; // 随机数

    // 对象序列化
    string to_json();

    // 对象反序列化
    static Block* from_json(string block_str);

    // 析构函数
    ~Block();
};

// 创建新的区块
Block* new_block(string pre_block_hash, vector<Transaction*> transactions);

// 生成创世区块
Block* generate_genesis_block(Transaction* coinbase_tx);

#endif // BLOCK_H
