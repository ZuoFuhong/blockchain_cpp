#include "blockchain.h"
#include "block.h"
#include "util.h"
#include "wallet.h"

using ROCKSDB_NAMESPACE::DB;
using ROCKSDB_NAMESPACE::Options;
using ROCKSDB_NAMESPACE::ReadOptions;
using ROCKSDB_NAMESPACE::Status;
using ROCKSDB_NAMESPACE::WriteBatch;
using ROCKSDB_NAMESPACE::WriteOptions;

const string kDBPath = "/tmp/blockchain/blocks";
const string tipBlockHashKey = "tip_block_hash";

// 构造函数
Blockchain::Blockchain(DB* db, string tip) {
    this->tip = tip;
    this->db = db;
}

// 创建新的区块链
Blockchain* Blockchain::new_blockchain() {
    // 创建目录
    if(!create_directory(kDBPath)) {
        std::cerr << "Failed to create blocks directory: " << kDBPath << std::endl;
        exit(1);
    }
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

    string tip;
    db->Get(ReadOptions(), tipBlockHashKey, &tip);
    if (tip == "") {
        // 本地没有联网, 手动同步创世块的钱包
        unique_ptr<Wallet> genesis_wallet(Wallet::new_wallet());
        // 创建创世区块
        auto coinbase_tx = Transaction::new_coinbase_tx(genesis_wallet->get_address());
        unique_ptr<Block> block(generate_genesis_block(coinbase_tx));
        string block_hash = block->hash;
        // 序列化 
        string block_str = block->to_json();

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
    return new Blockchain(db, tip);
}

// 挖矿新区块
Block* Blockchain::mine_block(vector<Transaction*> transactions) {
    Block* block = new_block(this->tip, transactions);
    string block_hash = block->hash; 
    // 序列化
    string block_str = block->to_json();

    WriteBatch batch;
    batch.Put(block_hash, block_str);
    batch.Put(tipBlockHashKey, block_hash);
    Status s = db->Write(WriteOptions(), &batch);
    if (!s.ok()) {
        std::cerr << "Failed to write database: " << s.ToString() << std::endl; 
        exit(1);
    }
    this->tip = block_hash;
    return block;
}

// 找到足够的未花费输出
pair<int, map<string, vector<int>>> Blockchain::find_spendable_outputs(vector<unsigned char>& pub_key_hash, int amount) {
    int accumulated = 0;
    map<string, vector<int>> unspent_outputs;
    vector<Transaction*> unspent_txs = this->find_unspent_transactions(pub_key_hash);
    for (auto tx : unspent_txs) {
        string txid = tx->id;
        for (int out_idx = 0; out_idx < tx->vout.size(); out_idx++) {
            auto txout = tx->vout[out_idx];
            if (txout.is_locked_with_key(pub_key_hash) && accumulated < amount) {
                accumulated += txout.value;
                unspent_outputs[txid].push_back(out_idx);
                // 金额足够
                if (accumulated >= amount) {
                    goto endloop;
                }
            }
        }
    }
    endloop:
    return make_pair(accumulated, unspent_outputs);
}

// 找到未花费支出的交易
// 1.有一些输出并没有被关联到某个输入上，如 coinbase 挖矿奖励。
// 2.一笔交易的输入可以引用之前多笔交易的输出。
// 3.一个输入必须引用一个输出。
vector<Transaction*> Blockchain::find_unspent_transactions(vector<unsigned char> pub_key_hash) {
    vector<Transaction*> unspent_txs;
    map<string, vector<int>> spent_txos; 
    // 指针自动释放
    unique_ptr<BlockchainIterator> iter(this->iterator());

    while (true) {
        unique_ptr<Block> block(iter->next());
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
                if (txout.is_locked_with_key(pub_key_hash)) {
                    // 拷贝交易
                    unspent_txs.push_back(tx->clone());
                }
            endloop:
                continue;
            }
            if (tx->is_coinbase()) {
                continue;
            }
            // 在输入中找到未花费的输出
            for (auto txin : tx->vin) {
                if (txin.uses_key(pub_key_hash)) {
                    string txid = txin.txid;
                    spent_txos[txid].push_back(txin.vout);
                }
            }
        }
    }
    return unspent_txs;
}

// 查找所有未花费的交易输出 k -> txid, v -> vector<TXOutput>
map<string, vector<TXOutput>> Blockchain::find_utxo() {
    map<string, vector<TXOutput>> utxo;
    map<string, vector<int>> spent_txos;
    unique_ptr<BlockchainIterator> iter(this->iterator());
    while (true) {
        unique_ptr<Block> block(iter->next());
        if (block == nullptr) {
            break;
        }
        for (auto tx : block->transactions) {
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
                utxo[tx_id].push_back(txout);
            endloop:
                continue;
            }
            if (tx->is_coinbase()) {
                continue;
            }
            // 在输入中找到已花费输出
            for (auto txin : tx->vin) {
                string txid = txin.txid;
                spent_txos[txid].push_back(txin.vout);
            }
        }
    }
    return utxo; 
}

// 找到未花费支出的交易输出
vector<TXOutput> Blockchain::find_utxo(vector<unsigned char> pub_key_hash) {
    auto transactions = find_unspent_transactions(pub_key_hash);
    vector<TXOutput> utxos;
    for (auto tx : transactions) {
        for (auto out : tx->vout) {
            if (out.is_locked_with_key(pub_key_hash)) {
                utxos.push_back(out);
            }
        }
        // 释放内存
        delete tx;
    }
    return utxos;
}

// 从区块链中查找交易
Transaction* Blockchain::find_transaction(string txid) {
    unique_ptr<BlockchainIterator> iter(this->iterator());
    while (true) {
        unique_ptr<Block> block(iter->next());
        if (block == nullptr) {
            break;
        }
        for (auto tx : block->transactions) {
            if (tx->id == txid) {
                // 拷贝交易
                return tx->clone();
            }
        }
    }
    return nullptr;
}

// 清空数据
void Blockchain::clear_data() {
    Status status = rocksdb::DestroyDB(kDBPath, Options());
    if (!status.ok()) {
        std::cerr << "Failed to destroy database: " << status.ToString() << std::endl; 
        exit(1);
    }
}

// 区块链迭代器
BlockchainIterator* Blockchain::iterator() {
    return new BlockchainIterator(this->db, this->tip);
}

// 析构函数
Blockchain::~Blockchain() {
    // 关闭数据库
    Status status = db->Close();
    if (!status.ok()) {
        std::cerr << "Failed to close database: " << status.ToString() << std::endl; 
        exit(1);
    }
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
    Block *block = Block::from_json(block_str);
    current_block_hash = block->pre_block_hash;
    return block;
}

