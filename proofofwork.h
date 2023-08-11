#pragma once

#include <gmp.h>
#include "block.h"

// 工作量证明
class ProofOfWork {
public:
    ProofOfWork(Block* block);
    ~ProofOfWork();

    // 运行挖矿
    pair<long, string> run();
private:
    Block* block;
    mpz_t  target;

    // 调整随机数
    vector<unsigned char> prepare_data(long nonce);
};

