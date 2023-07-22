#include "proofofwork.h"
#include "util.h"

// 难度值, 这里表示哈希的前 20 位必须是 0
const int targetBit = 8;

ProofOfWork::ProofOfWork(Block* block) {
    mpz_init(target); 
    // target 等于 1 左移 256 - targetBit 位
    mpz_ui_pow_ui(target, 2, 256 - targetBit);

    this->block = block;
}

// 调整随机数
vector<char> ProofOfWork::prepare_data(long nonce) {
    block->nonce = nonce;
    vector<char> bytes;
    bytes.insert(bytes.end(), std::begin(block->pre_block_hash), std::end(block->pre_block_hash));
    // tx_hash
    for (auto tx : block->transactions) {
        bytes.insert(bytes.end(), std::begin(tx->id), std::end(tx->id));
    }
    // timestamp
    string timestamp_str = to_string(block->timestamp);
    bytes.insert(bytes.end(), std::begin(timestamp_str), std::end(timestamp_str));
    // targetBit
    string targetbit_str = to_hex(targetBit);
    bytes.insert(bytes.end(), std::begin(targetbit_str), std::end(targetbit_str));
    // nonce
    string nonce_str = to_hex(nonce);
    bytes.insert(bytes.end(), std::begin(nonce_str), std::end(nonce_str));
    return bytes;     
}

// 运行挖矿
pair<long, string> ProofOfWork::run() {
    mpz_t hashInt;
    mpz_init(hashInt);
    string hash;
    long nonce = 0;

    while (nonce < LONG_MAX) {
        auto data = prepare_data(nonce);
        hash = sha256_digest(data);
        // 将 hash 转换为 GMP 数字
        mpz_set_str(hashInt, hash.c_str(), 16);
        if (mpz_cmp(hashInt, target) < 0) {
            break;
        } else {
            nonce++;
        }
    }
    // 释放内存
    mpz_clear(hashInt);
    return make_pair(nonce, hash);
}

ProofOfWork::~ProofOfWork() {
    mpz_clear(target);
}

