#ifndef WALLET_H
#define WALLET_H

#include <map>
#include "util.h"

// checksum 长度
const size_t ADDRESS_CHECK_SUM_LEN = 4;

struct Wallet {
    EC_KEY* ec_key;

    // 获取 DER 格式公钥
    vector<unsigned char> get_public_key();

    // 创建钱包
    static Wallet* new_wallet();

    // 加载钱包
    static Wallet* load_wallet(string address);

    // 获取钱包地址
    string get_address();

    // 析构函数
    ~Wallet();
};

// 获取所有钱包
vector<string> get_addresses();

// 验证地址有效
bool validate_address(const string& address);

// 计算公钥的哈希值
vector<unsigned char> hash_pub_key(vector<unsigned char> public_key);

// 通过公钥哈希值反推钱包地址
string pub_key_hash_to_address(vector<unsigned char> pub_key_hash);

#endif // WALLET_H
