#ifndef PROOFOFWORK_H
#define PROOFOFWORK_H

#include <gmp.h>
#include "common.h"

// 工作量证明
class ProofOfWork {
public:
    ProofOfWork(Block* block);
    ~ProofOfWork();

    // 调整随机数
    string prepare_data(long nonce);
    
    // 运行挖矿
    pair<long, string> run();
private:
    Block* block;
    mpz_t  target;
};

#endif // PROOFOFWORK_H
