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
string ProofOfWork::prepare_data(long nonce) {
    block->nonce = nonce;
    return block->pre_block_hash + block->data + to_hex(block->timestamp) + to_hex(targetBit) + to_hex(nonce);
}

// 运行挖矿
pair<long, string> ProofOfWork::run() {
    mpz_t hashInt;
    mpz_init(hashInt);
    string hash;
    long nonce = 0;

    std::cout << "Mining the block containing " << block->data << std::endl;
    while (nonce < LONG_MAX) {
        string data = prepare_data(nonce);
        hash = sha256_digest(data);
        // std::cout << "nonce: " << nonce << ", hash: " << hash << std::endl;
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

