#ifndef COMMON_H
#define COMMON_H

#include <iostream>
#include <vector>
#include <map>
#include <sstream>
#include <memory>

using namespace std;

// 交易输入
struct TXInput {
    string txid; // 一个交易输入引用了之前一笔交易的一个输出, ID 表示是之前的哪一笔交易
    int vout; // 输出的索引
    string script_sig; // 解锁脚本

    // 锁定检查
    bool can_unlock_output_with(string unlocking_data);
};

// 交易输出
struct TXOutput {
    int value; // 交易金额
    string script_pub_key; // 锁定脚本

    // 解锁检查
    bool can_be_unlocked_with(string unlocking_data);
};

// 交易
struct Transaction {
    string id; // 交易 ID
    vector<TXInput> vin; // 交易输入
    vector<TXOutput> vout; // 交易输出

    // 交易序列化为字节数组
    std::vector<char> serialize_transaction();

    // 从字节数组序列化为交易
    void deserialize_transaction(std::vector<char> data);

    // 判断是否是 coinbase 交易
    bool is_coinbase();
};

// 区块结构体
struct Block {
   long   timestamp; // 时间戳
   string pre_block_hash; // 前一个区块的 hash
   string hash; // 当前区块的 hash
   vector<Transaction*> transactions; // 交易数据
   long   nonce; // 随机数

    // 对象序列化
    string to_json();

    // 对象反序列化
    void from_json(string block_str);

    // 析构函数
    ~Block();
};

#endif // COMMON_H
