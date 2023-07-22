#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include <rocksdb/db.h>
#include <vector>
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

    // 挖矿新区块
    void mine_block(vector<Transaction*> transactions);

    // 找到未花费支出的交易
    vector<Transaction*> find_unspent_transactions( string address);

    // 找到未花费支出的交易输出
    vector<TXOutput> find_utxo(string address);

    // 清空数据
    void clear_data();

    // 区块链迭代器
    BlockchainIterator* iterator();

private:
    DB* db;
    string tip;
};

#endif // BLOCKCHAIN_H
