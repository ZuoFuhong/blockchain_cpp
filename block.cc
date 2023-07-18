#include <chrono>
#include <openssl/sha.h>
#include <json/json.h>
#include "block.h"
#include "proofofwork.h"
#include "util.h"

// 创建新的区块
Block* new_block(string data, string pre_block_hash) {
    Block *block = new Block;
    block->timestamp = current_timestamp();
    block->data = data;
    block->pre_block_hash = pre_block_hash;
    // 计算区块哈希
    ProofOfWork pow = ProofOfWork(block);
    pair<long, string> ans = pow.run();
    block->nonce = ans.first;
    block->hash = ans.second;
    return block;
}

// 创建创世区块
Block* new_genesis_block() {
    return new_block("Genesis Block", "");
}

// 对象序列化
string block_to_string(Block *block) {
    if (block == nullptr) {
        return "";
    }
    // 使用 jsoncpp 序列化 block 对象
    Json::Value root;
    root["timestamp"] = int64_t(block->timestamp);
    root["data"] = block->data;
    root["pre_block_hash"] = block->pre_block_hash;
    root["hash"] = block->hash;
    root["nonce"] = int64_t(block->nonce);
    Json::FastWriter writer;
    return writer.write(root);
}

// 对象反序列化
Block* string_to_block(string block_str) {
    Block *block = new Block();
    Json::Reader reader; 
    Json::Value root;
    reader.parse(block_str, root);
    block->timestamp = root["timestamp"].asInt64();
    block->data = root["data"].asString();
    block->pre_block_hash = root["pre_block_hash"].asString();
    block->hash = root["hash"].asString();
    block->nonce = root["nonce"].asInt64();
    return block;
}

