#include "blockchain.h"
#include "block.h"
#include "transaction.h"
#include "util.h"

using ROCKSDB_NAMESPACE::DB;
using ROCKSDB_NAMESPACE::Options;
using ROCKSDB_NAMESPACE::ReadOptions;
using ROCKSDB_NAMESPACE::Status;
using ROCKSDB_NAMESPACE::WriteBatch;
using ROCKSDB_NAMESPACE::WriteOptions;

const string kDBPath = "/tmp/blockchain.db";
const string tipBlockHashKey = "tip_block_hash";
const string genesisAddress = "Genesis";
const string genesisCoinbaseData = "The Times 03/Jan/2009 Chancellor on brink of second bailout for banks";

// 创建新的区块链
Blockchain::Blockchain() {
    DB* db;
    Options options;
    options.IncreaseParallelism();
    options.OptimizeLevelStyleCompaction();
    options.create_if_missing = true;
    Status status = DB::Open(options, kDBPath, &db);
    if (!status.ok()) {
        std::cerr << "Failed to open database: " << status.ToString() << std::endl; 
        exit(1);
    }

    db->Get(ReadOptions(), tipBlockHashKey, &tip);
    if (tip == "") {
        auto coinbase_tx = new_coinbase_tx(genesisAddress, genesisCoinbaseData);
        // 创建创世区块
        Block *block = generate_genesis_block(coinbase_tx);
        string block_hash = block->hash;
        // 序列化 
        string block_str = block->to_json();
        // 释放内存 
        delete block;
        block = nullptr;

        WriteBatch batch;
        batch.Put(block_hash, block_str);
        batch.Put(tipBlockHashKey, block_hash);
        status = db->Write(WriteOptions(), &batch);
        if (!status.ok()) {
            std::cerr << "Failed to write database: " << status.ToString() << std::endl; 
            exit(1);
        }
        tip = block_hash;
    }
    this->db = db;
}

// 挖矿新区块
void Blockchain::mine_block(vector<Transaction*> transactions) {
    Block *block = new_block(this->tip, transactions);
    string block_hash = block->hash; 
    // 序列化
    string block_str = block->to_json();
    // 释放内存
    delete block;
    block = nullptr;

    WriteBatch batch;
    batch.Put(block_hash, block_str);
    batch.Put(tipBlockHashKey, block_hash);
    Status s = db->Write(WriteOptions(), &batch);
    if (!s.ok()) {
        std::cerr << "Failed to write database: " << s.ToString() << std::endl; 
        exit(1);
    }
    this->tip = block_hash;
}

// 找到未花费支出的交易
// 1.有一些输出并没有被关联到某个输入上，如 coinbase 挖矿奖励。
// 2.一笔交易的输入可以引用之前多笔交易的输出。
// 3.一个输入必须引用一个输出。
vector<Transaction*> Blockchain::find_unspent_transactions(string address) {
    vector<Transaction*> unspent_txs;
    map<string, vector<int>> spent_txos; 
    // 指针自动释放
    unique_ptr<BlockchainIterator> iter(this->iterator());

    while (true) {
        auto block = iter->next();
        if (block == nullptr) {
            break;
        }
        for (auto tx : block->transactions) {
            // 未花费输出
            string tx_id = tx->id;
            vector<TXOutput> txouts = tx->vout;
            for (int idx = 0; idx < txouts.size(); idx++) {
                auto txout = txouts[idx]; 
                // 过滤掉已经花费的输出
                if (spent_txos.find(tx_id) != spent_txos.end()) {
                    auto outs = spent_txos[tx_id];
                    for (auto out : outs) {
                        if (out == idx) {
                            goto endloop;
                        }
                    }
                }
                if (txout.can_be_unlocked_with(address)) {
                    unspent_txs.push_back(tx);
                }
            endloop:
                continue;
            }
            if (tx->is_coinbase()) {
                continue;
            }
            // 在输入中找到未花费的输出
            for (auto txin : tx->vin) {
                if (txin.can_unlock_output_with(address)) {
                    string txid = txin.txid;
                    spent_txos[txid].push_back(txin.vout);
                }
            }
        }
    }
    return unspent_txs;
}

// 找到未花费支出的交易输出
vector<TXOutput> Blockchain::find_utxo(string address) {
    auto transactions = find_unspent_transactions(address);
    vector<TXOutput> utxos;
    for (auto tx : transactions) {
        for (auto out : tx->vout) {
            if (out.can_be_unlocked_with(address)) {
                utxos.push_back(out);
            }
        }
    }
    return utxos;
}

// 清空数据
void Blockchain::clear_data() {
    // 关闭数据库
    delete db;
    // 删除数据库文件
    delete_directory(kDBPath);
}

// 区块链迭代器
BlockchainIterator* Blockchain::iterator() {
    return new BlockchainIterator(this->db, this->tip);
}

// 析构函数
Blockchain::~Blockchain() {
    // 关闭数据库
    delete db;
    db = nullptr;
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
        return nullptr;
    }
    // 反序列化
    Block *block = new Block();
    block->from_json(block_str);
    current_block_hash = block->pre_block_hash;
    return block;
}

