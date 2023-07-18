#include "blockchain.h"
#include "block.h"
#include <cassert>
#include <cstdlib>

using ROCKSDB_NAMESPACE::DB;
using ROCKSDB_NAMESPACE::Options;
using ROCKSDB_NAMESPACE::ReadOptions;
using ROCKSDB_NAMESPACE::Status;
using ROCKSDB_NAMESPACE::WriteBatch;
using ROCKSDB_NAMESPACE::WriteOptions;

const string kDBPath = "/tmp/testdb";
const string tipBlockHashKey = "tip_block_hash";

// 创建新的区块链
Blockchain::Blockchain() {
    DB* db;
    Options options;
    options.IncreaseParallelism();
    options.OptimizeLevelStyleCompaction();
    options.create_if_missing = true;
    Status status = DB::Open(options, kDBPath, &db);
    if (!status.ok()) {
        std::cout << "Failed to open database: " << status.ToString() << std::endl; 
        exit(1);
    }

    db->Get(ReadOptions(), tipBlockHashKey, &tip);
    if (tip == "") {
        // 创建创世区块
        Block *block = new_genesis_block();
        string block_hash = block->hash;
        // 序列化 
        string block_str = block_to_string(block);

        WriteBatch batch;
        batch.Put(block_hash, block_str);
        batch.Put(tipBlockHashKey, block_hash);
        status = db->Write(WriteOptions(), &batch);
        if (!status.ok()) {
            std::cout << "Failed to write database: " << status.ToString() << std::endl; 
            exit(1);
        }
        tip = block_hash;
    }
    this->db = db;
}

// 添加区块
void Blockchain::add_block(string data) {
    Block *block = new_block(data, this->tip);
    string block_hash = block->hash; 
    // 序列化
    string block_str = block_to_string(block);

    WriteBatch batch;
    batch.Put(block_hash, block_str);
    batch.Put(tipBlockHashKey, block_hash);
    Status s = db->Write(WriteOptions(), &batch);
    if (!s.ok()) {
        std::cout << "Failed to write database: " << s.ToString() << std::endl; 
        exit(1);
    }
    this->tip = block_hash;
}

// 区块链迭代器
BlockchainIterator* Blockchain::iterator() {
    return new BlockchainIterator(this->db, this->tip);
}

// 析构函数
Blockchain::~Blockchain() {
    delete db;
}

// 迭代器
BlockchainIterator::BlockchainIterator(DB* db, string tip) {
    this->db = db;
    this->current_block_hash = tip;
}

// 下一个区块
Block* BlockchainIterator::next() {
    if (current_block_hash == "") {
        return nullptr;
    }
    string block_str;
    Status status = db->Get(ReadOptions(), current_block_hash, &block_str);
    if (!status.ok()) {
        std::cout << "Failed to read database: " << status.ToString() << std::endl; 
        exit(1);
    }
    Block *block = string_to_block(block_str);
    current_block_hash = block->pre_block_hash;
    return block;
}

