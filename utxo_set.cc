#include <json/json.h>
#include "blockchain.h"
#include "rocksdb/db.h"
#include "util.h"
#include "utxo_set.h"

using ROCKSDB_NAMESPACE::DB;
using ROCKSDB_NAMESPACE::Options;
using ROCKSDB_NAMESPACE::ReadOptions;
using ROCKSDB_NAMESPACE::Status;
using ROCKSDB_NAMESPACE::WriteBatch;
using ROCKSDB_NAMESPACE::WriteOptions;
using ROCKSDB_NAMESPACE::Slice;

const string utxoDBPath = "/tmp/blockchain/chainstate";

// 交易输出序列化
string txouts_to_json(vector<TXOutput> txouts);

// 交易输出反序列化
vector<TXOutput> txouts_from_json(string json_str);

// 构造函数
UTXOSet::UTXOSet(Blockchain* bc, DB* db) {
    this->bc = bc;
    this->db = db;
}

// 打开 UTXO 数据库
DB* open_utxo_db() {
    DB* db;
    Options options;
    options.IncreaseParallelism();
    options.OptimizeLevelStyleCompaction();
    options.create_if_missing = true;
    Status status = DB::Open(options, utxoDBPath, &db);
    if (!status.ok()) {
        std::cerr << "Failed to open database: " << status.ToString() << std::endl; 
        exit(1);
    }
    return db;
}

// 创建 UTXO 集
UTXOSet* UTXOSet::new_utxo_set(Blockchain *bc) {
    // 创建目录
    if(!create_directory(utxoDBPath)) {
        std::cerr << "Failed to create utxo_set directory: " << utxoDBPath << std::endl;
        exit(1);
    }
    return new UTXOSet(bc, open_utxo_db());
}

// 找到未花费的输出
pair<int, map<string, vector<int>>> UTXOSet::find_spendable_outputs(vector<unsigned char>& pub_key_hash, int amount) {
    map<string, vector<int>> unspent_outputs;
    int accumulated = 0;
    unique_ptr<rocksdb::Iterator> it(db->NewIterator(ReadOptions()));
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        string txid = it->key().ToString();
        string value = it->value().ToString();
        vector<TXOutput> txouts = txouts_from_json(value);
        for (int out_idx = 0; out_idx < txouts.size(); out_idx++) {
            TXOutput txout = txouts[out_idx];
            if (txout.is_locked_with_key(pub_key_hash) && accumulated < amount) {
                accumulated += txout.value;
                unspent_outputs[txid].push_back(out_idx);
            }
            if (accumulated >= amount) {
                goto endloop;
            }
        }
    }
    endloop:
    return make_pair(accumulated, unspent_outputs);
}

// 通过公钥哈希查找 UTXO 集
vector<TXOutput> UTXOSet::find_utxo(vector<unsigned char>& pub_key_hash) {
    unique_ptr<rocksdb::Iterator> it(db->NewIterator(ReadOptions()));
    vector<TXOutput> utxos;
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        string txid = it->key().ToString();
        string value = it->value().ToString();
        vector<TXOutput> txouts = txouts_from_json(value);
        for (int out_idx = 0; out_idx < txouts.size(); out_idx++) {
            TXOutput txout = txouts[out_idx];
            if (txout.is_locked_with_key(pub_key_hash)) {
                utxos.push_back(txout);
            }
        }
    }
    return utxos;
}

// 统计 UTXO 集合中的交易数量
int UTXOSet::count_transactions() {
    unique_ptr<rocksdb::Iterator> it(db->NewIterator(ReadOptions()));
    int count = 0;
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        count++;
    }
    return count;
}

// 重建 UTXO 集
void UTXOSet::reindex() {
    unique_ptr<rocksdb::Iterator> iter(db->NewIterator(ReadOptions()));
    // 清空数据库
    WriteBatch batch;
    for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
        batch.Delete(iter->key());
    }
    Status status = db->Write(WriteOptions(), &batch);
    if (!status.ok()) {
        std::cerr << "Failed to write database: " << status.ToString() << std::endl; 
        exit(1);
    }
    // 建立新的 UTXO 集
    auto utxo_map = bc->find_utxo();
    WriteBatch batch_write;
    for (auto it = utxo_map.begin(); it != utxo_map.end(); it++) {
        string txid = it->first;
        string value = txouts_to_json(it->second);
        batch_write.Put(txid, value);
    }
    status = db->Write(WriteOptions(), &batch_write);
    if (!status.ok()) {
        std::cerr << "Failed to write database: " << status.ToString() << std::endl; 
        exit(1);
    } 
}

// 使用来自区块的交易更新 UTXO 集
void UTXOSet::update(Block *block) {
    WriteBatch batch;
    for (auto tx : block->transactions) {
        if (!tx->is_coinbase()) {
            for (auto vin : tx->vin) {
                string txouts_bytes;
                Status status = db->Get(ReadOptions(), vin.txid, &txouts_bytes);
                if (!status.ok()) {
                    std::cerr << "Failed to get txid: " << vin.txid << std::endl; 
                    exit(1);
                }
                vector<TXOutput> txouts = txouts_from_json(txouts_bytes);
                // 删除已经花费的输出
                vector<TXOutput> updated_outs;
                for (int idx = 0; idx < txouts.size(); idx++) {
                    if (idx != vin.vout) {
                        updated_outs.push_back(txouts[idx]);
                    }
                }
                if (updated_outs.empty()) {
                    batch.Delete(vin.txid);
                } else {
                    batch.Put(vin.txid, txouts_to_json(updated_outs));
                }
            }
        }
        batch.Put(tx->id, txouts_to_json(tx->vout));
    }
    Status status = db->Write(WriteOptions(), &batch);
    if (!status.ok()) {
        std::cerr << "Failed to write database: " << status.ToString() << std::endl; 
        exit(1);
    }
}

// 清空数据
void UTXOSet::clear_data() {
    Status status = rocksdb::DestroyDB(utxoDBPath, Options());
    if (!status.ok()) {
        std::cerr << "Failed to destroy database: " << status.ToString() << std::endl; 
        exit(1);
    }
}

// 区块链
Blockchain* UTXOSet::blockchain() {
    return bc;
}

// 将 TXOutput 转换为 JSON
string txouts_to_json(vector<TXOutput> txouts) {
    Json::Value root;
    for (auto vout : txouts) {
        Json::Value txout;
        txout["value"] = vout.value;
        txout["pub_key_hash"] = encode_base64(vout.pub_key_hash);
        root.append(txout); 
    }
    Json::FastWriter writer;
    return writer.write(root);
}

// 将 JSON 转换为 TXOutput
vector<TXOutput> txouts_from_json(string json_str) {
    Json::Reader reader;
    Json::Value root;
    if (!reader.parse(json_str, root)) {
        std::cerr << "Failed to parse json string: " << json_str << std::endl;
        exit(1);
    }
    vector<TXOutput> txouts;
    for (auto txout : root) {
        TXOutput vout;
        vout.value = txout["value"].asInt();
        decode_base64(txout["pub_key_hash"].asString(), vout.pub_key_hash);
        txouts.push_back(vout);
    }
    return txouts;
}

// 析构函数
UTXOSet::~UTXOSet() {
    // 关闭数据库
    delete db;
    db = nullptr;
}

