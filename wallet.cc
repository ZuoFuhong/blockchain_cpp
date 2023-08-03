#include <cstdlib>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <iostream>
#include <sys/stat.h>
#include "wallet.h"
#include "util.h"

// 钱包文件夹
const string WALLET_DIR = "/tmp/wallets/";

// 版本号
const char VERSION = 0x00;

// 计算公钥的哈希值
vector<unsigned char> hash_pub_key(vector<unsigned char> public_key) {
    if (public_key.size() == 0) {
        return vector<unsigned char>();
    }
    vector<unsigned char> public_key_hash = sha256_digest(public_key);
    return ripemd160_digest(public_key_hash);
}

// 计算校验和
vector<unsigned char> checksum(vector<unsigned char> payload) {
    vector<unsigned char> first_hash = sha256_digest(payload);
    vector<unsigned char> second_hash = sha256_digest(first_hash);
    return vector<unsigned char>(second_hash.begin(), second_hash.begin() + ADDRESS_CHECK_SUM_LEN);
}

// 验证地址有效
bool validate_address(const string& address) {
    // 解码 base58
    vector<unsigned char> payload;
    if (!decode_base58(address, payload)) {
        return false;
    }
    // 边界值
    if (payload.size() < ADDRESS_CHECK_SUM_LEN) {
        return false;
    }
    vector<unsigned char> check_sum = vector<unsigned char>(payload.end() - ADDRESS_CHECK_SUM_LEN, payload.end());
    // 计算校验和
    vector<unsigned char> curr_check_sum = checksum(vector<unsigned char>(payload.begin(), payload.end() - ADDRESS_CHECK_SUM_LEN));
    return are_vectors_equal(check_sum, curr_check_sum);
}

// 通过公钥哈希值反推钱包地址
string pub_key_hash_to_address(vector<unsigned char> pub_key_hash) {
    if (pub_key_hash.size() == 0) {
        return "";
    }
    vector<unsigned char> payload;
    payload.push_back(VERSION);
    payload.insert(payload.end(), pub_key_hash.begin(), pub_key_hash.end());
    // 计算校验和
    vector<unsigned char> check_sum = checksum(payload);
    payload.insert(payload.end(), check_sum.begin(), check_sum.end());
    return encode_base58(payload);
}

// 创建钱包
Wallet* Wallet::new_wallet() {
    Wallet* wallet = new Wallet();
    EC_KEY* ec_key = new_ecdsa_key_pair();
    wallet->ec_key = ec_key;

    // 钱包文件夹
    string address = wallet->get_address();
    string wallet_dir = WALLET_DIR + wallet->get_address();
    if(!create_directory(wallet_dir)) {
        std::cerr << "Failed to create wallet directory: " << wallet_dir << std::endl;
        exit(1);
    }
    // 将私钥保存到文件
    string priv_path = wallet_dir + "/private.pem";
    FILE* fp = fopen(priv_path.c_str(), "w"); 
    PEM_write_ECPrivateKey(fp, ec_key, NULL, NULL, 0, NULL, NULL);
    fclose(fp);

    // 将公钥保存到文件
    string pub_path = wallet_dir + "/public.pem";
    fp = fopen(pub_path.c_str(), "w");
    PEM_write_EC_PUBKEY(fp, ec_key);
    fclose(fp);
    return wallet;
}

// 加载钱包
Wallet* Wallet::load_wallet(string address) {
    string wallet_dir = WALLET_DIR + address;
    struct stat st;
    if (stat(wallet_dir.c_str(), &st) != 0) {
        return nullptr;
    }
    // 读取私钥
    string priv_path = wallet_dir + "/private.pem";
    // 检查文件是否存在
    if (stat(priv_path.c_str(), &st) != 0) {
        std::cerr << "ERROR: Address is not valid!" << std::endl;
        exit(1);
    }
    FILE* fp = fopen(priv_path.c_str(), "r");
    // 将 PEM 格式私钥转换为 EC_KEY
    EC_KEY* ec_key = PEM_read_ECPrivateKey(fp, NULL, NULL, NULL);
    fclose(fp);

    Wallet* wallet = new Wallet();
    wallet->ec_key = ec_key;
    return wallet;
}

// 获取所有钱包
vector<string> get_addresses() {
    vector<string> addresses;
    DIR* dir = opendir(WALLET_DIR.c_str());
    if (dir == nullptr) {
        return addresses;
    }
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type == DT_DIR) {
            string filename = entry->d_name;
            if (filename == "." || filename == "..") {
                continue;
            }
            addresses.push_back(filename);
        }
    }
    // 关闭目录
    closedir(dir);
    return addresses;
}

// 获取 DER 格式公钥
vector<unsigned char> Wallet::get_public_key() {
    vector<unsigned char> public_key_vec;
    int public_key_len = i2o_ECPublicKey(this->ec_key, NULL);
    public_key_vec.resize(public_key_len);
    unsigned char* public_key_ptr = public_key_vec.data();
    i2o_ECPublicKey(this->ec_key, &public_key_ptr);
    return public_key_vec;
}

// 获取钱包地址
// 这里得到了一个真实的BTC地址，可以在 (Tokenview)[https://tokenview.com/cn/search/173EuX6KuB1EiWYEKyaQud6x91VNjkM3Vu] 查询它的余额.
// 不过我可以负责任地说，无论生成一个新的地址多少次，检查它的余额都是 0。这就是为什么选择一个合适的公钥加密算法是如此重要：考虑到私钥是随机数，生成
// 同一个数字的概率必须是尽可能地低。理想情况下，必须是低到“永远”不会重复。
// 另外，注意：你并不需要连接到一个比特币节点来获得一个地址。地址生成算法使用的多种开源算法可以通过很多编程语言和库实现。
string Wallet::get_address() {
    vector<unsigned char> pub_key_hash = hash_pub_key(this->get_public_key());
    vector<unsigned char> payload;
    payload.push_back(VERSION);
    payload.insert(payload.end(), pub_key_hash.begin(), pub_key_hash.end());
    vector<unsigned char> check_sum = checksum(payload); 
    payload.insert(payload.end(), check_sum.begin(), check_sum.end());
    // version + pub_key_hash + check_sum
    return encode_base58(payload);
}

// 析构函数
Wallet::~Wallet() {
    EC_KEY_free(ec_key);
}

