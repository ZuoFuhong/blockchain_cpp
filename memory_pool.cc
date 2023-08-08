#include "memory_pool.h"

// 创建交易池
MemoryPool* MemoryPool::new_memory_pool() {
    return new MemoryPool();
}

// 检查交易
bool MemoryPool::containes(const string& txid) {
    return txs.find(txid) != txs.end();
}

// 添加交易
void MemoryPool::add(Transaction* tx) {
    txs[tx->id] = tx;
}

// 获取交易
Transaction* MemoryPool::get(const string& txid) {
    return txs[txid];
}

// 删除交易
void MemoryPool::remove(const string& txid) {
    if (txs.find(txid) == txs.end()) {
        return;
    }
    txs.erase(txid);
}

// 池中交易数量
size_t MemoryPool::len() {
    return txs.size();
}

// 获取池中所有交易
vector<Transaction*> MemoryPool::get_all() {
    vector<Transaction*> txs;
    for (auto& kv : this->txs) {
        txs.push_back(kv.second);
    }
    return txs;
}

