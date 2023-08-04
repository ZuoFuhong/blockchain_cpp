#ifndef UTXO_SET_H
#define UTXO_SET_H

#include "blockchain.h"

// UTXO 集
class UTXOSet {
public:
    // 构造函数
    UTXOSet(Blockchain* bc, DB* db);

    // 析构函数
    ~UTXOSet();
 
    // 创建 UTXO 集
    static UTXOSet* new_utxo_set(Blockchain *bc);

    // 清空数据
    static void clear_data();

    // 找到未花费的输出
    pair<int, map<string, vector<int>>> find_spendable_outputs(vector<unsigned char>& pub_key_hash, int amount); 

    // 通过公钥哈希查找 UTXO 集
    vector<TXOutput> find_utxo(vector<unsigned char>& pub_key_hash);

    // 统计 UTXO 集合中的交易数量
    int count_transactions();

    // 重建 UTXO 集
    void reindex();

    // 使用来自区块的交易更新 UTXO 集
    void update(Block *block);

    // 区块链
    Blockchain* blockchain();
private:
    Blockchain *bc;
    DB* db; 
};

#endif // UTXO_SET_H
