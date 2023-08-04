#include <cstddef>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <vector>
#include <map>
#include "blockchain.h"
#include "openssl/ossl_typ.h"
#include "transaction.h"
#include "wallet.h"
#include "util.h"
#include "utxo_set.h"

// 挖矿奖励金
const int SUBSIDY = 10;

// 检查公钥哈希是否能够解锁输出
bool TXInput::uses_key(vector<unsigned char>& pub_key_hash) {
    vector<unsigned char> locking_hash = hash_pub_key(pub_key);
    return are_vectors_equal(locking_hash, pub_key_hash);
}

// 构建输出
TXOutput::TXOutput(int value, vector<unsigned char> pub_key_hash) {
    this->value = value;
    this->pub_key_hash = pub_key_hash;
}

// 构造输出
TXOutput::TXOutput(int value, const string& address) {
    this->value = value;
    // 锁定输出
    this->lock(address);
}

// 输出锁定
void TXOutput::lock(const string& address) {
    vector<unsigned char> payload;
    bool result = decode_base58(address, payload);
    if (!result) {
        std::cerr << "ERROR: Address is not valid!" << std::endl;
        exit(1);
    }
    // 去掉版本号和校验码
    this->pub_key_hash = vector<unsigned char>(payload.begin() + 1, payload.end() - ADDRESS_CHECK_SUM_LEN);
}

// 解锁检查
bool TXOutput::is_locked_with_key(vector<unsigned char> &pub_key_hash) {
    return are_vectors_equal(this->pub_key_hash, pub_key_hash);
}

// 判断是否是 coinbase 交易
bool Transaction::is_coinbase() {
    return this->vin.size() == 1 && this->vin[0].pub_key.size() == 0;
}

// 交易哈希
string Transaction::hash() {
    if (this->id != "") {
        return this->id;
    }
    // 交易 ID 不参与哈希计算
    vector<unsigned char> tx_bytes = this->serialize_transaction();
    return sha256_digest_hex(tx_bytes);
}

// 创建一个修剪后的交易副本
Transaction* Transaction::trimmed_copy() {
    vector<TXInput> new_vin;
    vector<TXOutput> new_vout;
    for (auto vin : this->vin) {
        new_vin.push_back(TXInput{vin.txid, vin.vout, vector<unsigned char>{}, vector<unsigned char>{}});
    }
    for (auto vout : this->vout) {
        new_vout.push_back(TXOutput{vout.value, vout.pub_key_hash});
    }
    return new Transaction{this->id, new_vin, new_vout};
}

// 克隆交易
Transaction* Transaction::clone() {
    return new Transaction{this->id, this->vin, this->vout};
}

// 对交易的每个输入进行签名
void Transaction::sign(Blockchain* bc, EC_KEY* ec_key) {
    if (this->is_coinbase()) {
        return;
    }
    // 交易副本
    unique_ptr<Transaction> tx_copy(this->trimmed_copy());
    
    for (int idx = 0; idx < this->vin.size(); idx++) {
        auto vin = this->vin[idx];
        // 查询输入引用的交易
        unique_ptr<Transaction> prev_tx(bc->find_transaction(vin.txid));
        if (prev_tx == nullptr) {
            std::cerr << "ERROR: Previous transaction not found!" << std::endl;
            exit(1);
        }
        // 交易副本的输入
        tx_copy->vin[idx].signature.clear();
        tx_copy->vin[idx].pub_key = prev_tx->vout[vin.vout].pub_key_hash;
        tx_copy->id = tx_copy->hash();
        tx_copy->vin[idx].pub_key.clear();
        // 使用私钥签名
        auto tx_bytes = tx_copy->serialize_transaction();
        vin.signature = ecdsa_p256_sha256_sign_digest(ec_key, tx_bytes);
    }
}

// 对交易的每个输入进行验证
bool Transaction::verify(Blockchain* bc) {
    if (this->is_coinbase()) {
        return true;
    }
    unique_ptr<Transaction> tx_copy(this->trimmed_copy());
    for (int idx = 0; idx < this->vin.size(); idx++) {
        auto vin = this->vin[idx];
        // 智能指针
        unique_ptr<Transaction> prev_tx(bc->find_transaction(vin.txid));
        if (prev_tx == nullptr) {
            std::cerr << "ERROR: Previous transaction not found!" << std::endl;
            exit(1);
        }
        tx_copy->vin[idx].signature.clear();
        tx_copy->vin[idx].pub_key = prev_tx->vout[vin.vout].pub_key_hash;
        tx_copy->id = tx_copy->hash();
        tx_copy->vin[idx].pub_key.clear();

        // 使用公钥验证签名
        auto tx_bytes = tx_copy->serialize_transaction();
        if (!ecdsa_p256_sha256_sign_verify(vin.pub_key, vin.signature, tx_bytes)) {
            return false;
        }
    }
    return true;
}

// 创建一笔 coinbase 交易, 该交易没有输入, 仅有一个输出
Transaction* Transaction::new_coinbase_tx(const string& to) {
    TXInput txin;
    txin.txid = "None";
    txin.vout = 0;
    string uuid = generateUUID();
    txin.signature = vector<unsigned char>(uuid.begin(), uuid.end());
    TXOutput txout(SUBSIDY, to);

    // 生成交易 hash
    Transaction* tx = new Transaction{"", {txin}, {txout}};
    // 生成交易 ID
    tx->id = tx->hash();
    return tx;
}

// 创建一笔 UTXO 交易 
Transaction* Transaction::new_utxo_transaction(const string& from, const string& to, int amount, UTXOSet* utxo_set) {
    // 查找钱包
    unique_ptr<Wallet> wallet(Wallet::load_wallet(from));
    if (wallet == nullptr) {
        std::cerr << "ERROR: Sender's wallet not found!" << std::endl;
        exit(1);
    }
    // 公钥哈希
    vector<unsigned char> pub_key_hash = hash_pub_key(wallet->get_public_key());
    // 找到足够的未花费输出
    pair<int, map<string, vector<int>>> spendable_outputs = utxo_set->find_spendable_outputs(pub_key_hash, amount);
    int accumulated = spendable_outputs.first;
    if (accumulated < amount) {
        std::cerr << "ERROR: Not enough funds!" << std::endl;
        exit(1);
    }
    // 交易数据
    map<string, vector<int>> valid_outputs = spendable_outputs.second;
    vector<TXInput> inputs;
    for (auto tx : valid_outputs) {
        string txid = tx.first;
        for (auto out : tx.second) {
            TXInput txin = {txid, out, {}, wallet->get_public_key()};
            inputs.push_back(txin);
        }
    }
    // 交易的输出
    vector<TXOutput> outputs;
    outputs.push_back(TXOutput(amount, to));

    // 如果 UTXO 总数超过所需, 则产生找零
    if (accumulated > amount) {
        outputs.push_back(TXOutput(accumulated - amount, from)); 
    }
    Transaction* tx = new Transaction{"", inputs, outputs};
    // 生成交易 ID
    tx->id = tx->hash();
    // 交易中的 TXInput 签名
    tx->sign(utxo_set->blockchain(), wallet->ec_key);
    return tx;
}

// 交易序列化为字节数组
std::vector<unsigned char> Transaction::serialize_transaction() {
    // 计算字节数组大小
    size_t size = 0;
    // 数组长度
    size += sizeof(size_t);
    for (auto& txin : this->vin) {
        // 字符串长度
        size += sizeof(size_t);
        // 加上 null 终止符
        size += txin.txid.size() + 1;
        // vout
        size += sizeof(txin.vout);
        // signature
        size += sizeof(size_t);
        size += txin.signature.size();
        // pub_key
        size += sizeof(size_t);
        size += txin.pub_key.size();
    }
    size += sizeof(size_t);
    for (auto& txout : this->vout) {
        // value
        size += sizeof(txout.value);
        // script_pub_key
        size += sizeof(size_t);
        size += txout.pub_key_hash.size();
    }
    // 构造字节数组
    std::vector<unsigned char> bytes(size);
    unsigned char* ptr = bytes.data();
    size_t vin_size = this->vin.size();
    memcpy(ptr, &vin_size, sizeof(vin_size));
    ptr += sizeof(vin_size);
    for (auto& txin : this->vin) {
        // 字符串长度
        size_t txid_size = txin.txid.size();
        memcpy(ptr, &txid_size, sizeof(txid_size));
        ptr += sizeof(txid_size);
        // 字符串内容
        memcpy(ptr, txin.txid.c_str(), txin.txid.size() + 1);
        // 字符长度 + null 终止符
        ptr += txin.txid.size() + 1;
        // vout
        memcpy(ptr, &txin.vout, sizeof(txin.vout));
        ptr += sizeof(txin.vout);
        // signature
        size_t signature_size = txin.signature.size();
        memcpy(ptr, &signature_size, sizeof(signature_size));
        ptr += sizeof(signature_size);
        memcpy(ptr, txin.signature.data(), txin.signature.size());
        ptr += txin.signature.size();
        // pub_key
        size_t pub_key_size = txin.pub_key.size();
        memcpy(ptr, &pub_key_size, sizeof(pub_key_size));
        ptr += sizeof(pub_key_size);
        memcpy(ptr, txin.pub_key.data(), txin.pub_key.size());
        ptr += txin.pub_key.size();
    }
    size_t vout_size = this->vout.size();
    memcpy(ptr, &vout_size, sizeof(vout_size));
    ptr += sizeof(vout_size);
    for (auto& txout : this->vout) {
        memcpy(ptr, &txout.value, sizeof(txout.value));
        ptr += sizeof(txout.value);
        // script_pub_key
        size_t script_pub_key_size = txout.pub_key_hash.size();
        memcpy(ptr, &script_pub_key_size, sizeof(script_pub_key_size));
        ptr += sizeof(script_pub_key_size);
        memcpy(ptr, txout.pub_key_hash.data(), txout.pub_key_hash.size());
        ptr += txout.pub_key_hash.size();
    }
    return bytes;
}

// 从字节数组反序列化为交易
Transaction* Transaction::deserialize_transaction(std::vector<unsigned char> data) {
    Transaction* tx = new Transaction();
    unsigned char* ptr = data.data();
    // 数组长度
    size_t vin_size;
    memcpy(&vin_size, ptr, sizeof(vin_size));
    ptr += sizeof(vin_size);
    for (size_t i = 0; i < vin_size; i++) {
        TXInput txin;
        // 字符串长度
        size_t txid_size;
        memcpy(&txid_size, ptr, sizeof(txid_size));
        ptr += sizeof(txid_size);
        // 字符串内容
        char *txid = new char[txid_size + 1];
        memcpy(txid, ptr, txid_size + 1);
        txin.txid = txid;
        // 字符长度 + null 终止符
        ptr += txid_size + 1;
        // 释放内存
        delete [] txid;
        txid = nullptr;
        // vout
        memcpy(&txin.vout, ptr, sizeof(txin.vout));
        ptr += sizeof(txin.vout);
        // signature
        size_t signature_size;
        memcpy(&signature_size, ptr, sizeof(signature_size));
        ptr += sizeof(signature_size);
        char *signature = new char[signature_size];
        memcpy(signature, ptr, signature_size);
        txin.signature.insert(txin.signature.end(), signature, signature + signature_size);
        ptr += signature_size;
        // 释放内存
        delete [] signature;
        signature = nullptr;
        // pub_key
        size_t pub_key_size;
        memcpy(&pub_key_size, ptr, sizeof(pub_key_size));
        ptr += sizeof(pub_key_size);
        char *pub_key = new char[pub_key_size];
        memcpy(pub_key, ptr, pub_key_size);
        txin.pub_key.insert(txin.pub_key.end(), pub_key, pub_key + pub_key_size);
        ptr += pub_key_size;
        // 释放内存
        delete [] pub_key;
        pub_key = nullptr;
        tx->vin.push_back(txin);
    }
    size_t vout_size;
    memcpy(&vout_size, ptr, sizeof(vout_size));
    ptr += sizeof(vout_size);
    for (size_t i = 0; i < vout_size; i++) {
        TXOutput txout;
        // value
        memcpy(&txout.value, ptr, sizeof(txout.value)); 
        ptr += sizeof(txout.value);
        // script_pub_key
        size_t script_pub_key_size;
        memcpy(&script_pub_key_size, ptr, sizeof(script_pub_key_size));
        ptr += sizeof(script_pub_key_size);
        char *pub_key_hash = new char[script_pub_key_size];
        memcpy(pub_key_hash, ptr, script_pub_key_size);
        txout.pub_key_hash.insert(txout.pub_key_hash.end(), pub_key_hash, pub_key_hash + script_pub_key_size);
        ptr += script_pub_key_size + 1;
        // 释放内存
        delete [] pub_key_hash;
        pub_key_hash = nullptr;
        tx->vout.push_back(txout);
    }
    return tx;
}

