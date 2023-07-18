#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include <rocksdb/db.h>
#include "block.h"

using ROCKSDB_NAMESPACE::DB;

// 迭代器
class BlockchainIterator {
public:
    BlockchainIterator(DB* db, string tip);
    Block* next();
private:
    DB* db;
    string current_block_hash;
};

// 区块链
class Blockchain {
public:
    // 创建新的区块链
    Blockchain();
    
    // 析构函数
    ~Blockchain();

    // 添加区块
    void add_block(string data);

    // 区块链迭代器
    BlockchainIterator* iterator();

private:
    DB* db;
    string tip;
};

#endif // BLOCKCHAIN_H
