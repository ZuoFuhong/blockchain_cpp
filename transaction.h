#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <openssl/ecdsa.h>
#include "iostream"

using namespace std;

// 前置声明
class Blockchain;

// 交易输入
struct TXInput {
    string txid; // 一个交易输入引用了之前一笔交易的一个输出, ID 表示是之前的哪一笔交易
    int vout; // 输出的索引
    vector<unsigned char> signature; // 签名
    vector<unsigned char> pub_key; // 公钥

    // 检查公钥哈希是否能够解锁输出
    bool uses_key(vector<unsigned char>& pub_key_hash);
};

// 交易输出
struct TXOutput {
    int value; // 交易金额
    vector<unsigned char> pub_key_hash; // 公钥哈希

    // 构造函数
    TXOutput() = default;

    // 构建输出
    TXOutput(int value, vector<unsigned char> pub_key_hash);
    
    // 构造输出
    TXOutput(int value, const string& address);

    // 输出锁定
    void lock(const string& address);

    // 解锁检查
    bool is_locked_with_key(vector<unsigned char>& pub_key_hash);
};

// 交易
struct Transaction {
    string id; // 交易 ID
    vector<TXInput> vin; // 交易输入
    vector<TXOutput> vout; // 交易输出

    // 创建 coinbase 交易, 该交易没有输入, 只有一个输出
    static Transaction* new_coinbase_tx(const string& to);

    // 创建一笔 UTXO 交易 
    static Transaction* new_utxo_transaction(const string& from, const string& to, int amount, Blockchain* bc);

    // 从字节数组序列化为交易
    static Transaction* deserialize_transaction(std::vector<unsigned char> data);

    // 交易序列化为字节数组
    std::vector<unsigned char> serialize_transaction();

    // 判断是否是 coinbase 交易
    bool is_coinbase();

    // 交易哈希(16进制)
    string hash();

    // 创建一个修剪后的交易副本
    Transaction* trimmed_copy();

    // 克隆交易
    Transaction* clone();

    // 对交易的每个输入进行签名
    void sign(Blockchain* bc, EC_KEY* ec_key);

    // 对交易的每个输入进行验证
    bool verify(Blockchain* bc);
};

#endif // TRANSACTION_H
