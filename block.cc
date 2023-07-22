#include <chrono>
#include <openssl/sha.h>
#include <json/json.h>
#include <sstream>
#include <vector>
#include "block.h"
#include "proofofwork.h"
#include "util.h"

// 创建新的区块
Block* new_block(string pre_block_hash, vector<Transaction*> transactions) {
    Block *block = new Block;
    block->timestamp = current_timestamp();
    block->transactions = transactions;
    block->pre_block_hash = pre_block_hash;
    // 计算区块哈希
    ProofOfWork pow = ProofOfWork(block);
    pair<long, string> ans = pow.run();
    block->nonce = ans.first;
    block->hash = ans.second;
    return block;
}

// 生成创世区块
Block* generate_genesis_block(Transaction* coinbase_tx) {
    return new_block("None", vector<Transaction*>{coinbase_tx});
}

// 对象序列化
string Block::to_json() {
    Json::Value root;
    root["timestamp"] = int64_t(this->timestamp);
    root["pre_block_hash"] = this->pre_block_hash;
    root["hash"] = this->hash;
    root["nonce"] = int64_t(this->nonce);
    Json::Value transactions;
    for (auto tx : this->transactions) {
        Json::Value tx_root;
        tx_root["id"] = tx->id;
        Json::Value tx_vin;
        for (auto vin : tx->vin) {
            Json::Value vin_root;
            vin_root["txid"] = vin.txid;
            vin_root["vout"] = vin.vout;
            vin_root["script_sig"] = vin.script_sig;
            tx_vin.append(vin_root);
        }
        tx_root["vin"] = tx_vin;
        Json::Value tx_vout;
        for (auto vout : tx->vout) {
            Json::Value vout_root;
            vout_root["value"] = vout.value;
            vout_root["script_pub_key"] = vout.script_pub_key;
            tx_vout.append(vout_root);
        }
        tx_root["vout"] = tx_vout;
        transactions.append(tx_root);
    }
    root["transactions"] = transactions;
    Json::FastWriter writer;
    return writer.write(root);
}

// 对象反序列化
void Block::from_json(string block_str) {
    Json::Reader reader; 
    Json::Value root;
    reader.parse(block_str, root);
    this->timestamp = root["timestamp"].asInt64();
    this->pre_block_hash = root["pre_block_hash"].asString();
    this->hash = root["hash"].asString();
    this->nonce = root["nonce"].asInt64();
    Json::Value transactions = root["transactions"];
    if (transactions.isArray()) {
        for (auto tx : transactions) {
            Transaction *transaction = new Transaction;
            transaction->id = tx["id"].asString();
            Json::Value vin = tx["vin"];
            if (vin.isArray()) {
                for (auto v : vin) {
                    TXInput input;
                    input.txid = v["txid"].asString();
                    input.vout = v["vout"].asInt();
                    input.script_sig = v["script_sig"].asString();
                    transaction->vin.push_back(input);
                }
            }
            Json::Value vout = tx["vout"];
            if (vout.isArray()) {
                for (auto v : vout) {
                    TXOutput output;
                    output.value = v["value"].asInt();
                    output.script_pub_key = v["script_pub_key"].asString();
                    transaction->vout.push_back(output);
                }
            }
            this->transactions.push_back(transaction);
        }
    }
}

// 析构函数
Block::~Block() {
    for (auto tx : transactions) {
        delete tx;
        tx = nullptr;
    }
    transactions.clear();
}
