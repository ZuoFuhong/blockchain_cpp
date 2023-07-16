#include <chrono>
#include <openssl/sha.h>
#include "block.h"

// 获取当前时间戳
long get_now_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    return timestamp;
}

// 计算字符串的sha256
string sha256(const string str) {
    char buf[3];
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, str.c_str(), str.size());
    SHA256_Final(hash, &sha256);
    string output = "";
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        snprintf(buf, sizeof(buf), "%02x", hash[i]);
        output += buf;
    }
    return output;
}

// 创建新的区块
Block* new_block(string data, string pre_block_hash) {
    Block *block = new Block;
    block->timestamp = get_now_timestamp();
    block->data = data;
    block->pre_block_hash = pre_block_hash;
    block->hash = sha256(data + pre_block_hash + to_string(block->timestamp));
    return block;
}

// 创建创世区块
Block* new_genesis_block() {
    return new_block("Genesis Block", "");
}

