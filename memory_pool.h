#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H

#include <map>
#include "transaction.h"

class MemoryPool {
public:

    // 创建交易池
    static MemoryPool* new_memory_pool();

    // 检查交易
    bool containes(const string& txid);

    // 添加交易
    void add(Transaction* tx);

    // 获取交易
    Transaction* get(const string& txid);

    // 删除交易
    void remove(const string& txid);

    // 池中交易数量
    size_t len();

    // 获取池中所有交易
    vector<Transaction*> get_all();

private:
    map<string, Transaction*> txs;
};

#endif // MEMORY_POOL_H
