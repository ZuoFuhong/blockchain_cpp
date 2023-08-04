#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include <rocksdb/db.h>
#include <vector>
#include "transaction.h"
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
    // 构造函数
    Blockchain(DB* db, string tip);

    // 析构函数
    ~Blockchain();

    // 创建区块链
    static Blockchain* new_blockchain();

    // 清空数据
    static void clear_data();

    // 挖矿新区块
    Block* mine_block(vector<Transaction*> transactions);

    // 找到足够的未花费输出
    pair<int, map<string, vector<int>>> find_spendable_outputs(vector<unsigned char>& pub_key_hash, int amount);

    // 找到未花费支出的交易
    vector<Transaction*> find_unspent_transactions(vector<unsigned char> pub_key_hash);

    // 查找所有未花费的交易输出 k -> txid, v -> vector<TXOutput>
    map<string, vector<TXOutput>> find_utxo();

    // 找到未花费支出的交易输出
    vector<TXOutput> find_utxo(vector<unsigned char> pub_key_hash);

    // 从区块链中查找交易
    Transaction* find_transaction(string txid);

    // 区块链迭代器
    BlockchainIterator* iterator();

private:
    DB* db;
    string tip; 
};

#endif // BLOCKCHAIN_H
