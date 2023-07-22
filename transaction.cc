#include <cstddef>
#include <cstdio>
#include <cstring>
#include <vector>
#include <map>
#include "common.h"
#include "transaction.h"
#include "util.h"

// 挖矿奖励金
const int SUBSIDY = 10;

// 锁定检查
bool TXInput::can_unlock_output_with(string unlocking_data) {
    return this->script_sig == unlocking_data;
}

// 解锁检查
bool TXOutput::can_be_unlocked_with(string unlocking_data) {
    return script_pub_key == unlocking_data;
}

// 判断是否是 coinbase 交易
bool Transaction::is_coinbase() {
    return this->vin.size() == 1 && this->vin[0].txid == "" && this->vin[0].vout == -1;
}

// 创建一笔 UTXO 交易
Transaction* new_coinbase_tx(string to, string data) {
    if (data == "") {
        data = "Reward to '" + to + "'";
    }
    TXInput txin = {"None", -1, data};
    TXOutput txout = {SUBSIDY, to};

    // 生成交易 hash
    Transaction* tx = new Transaction{"", {txin}, {txout}};
    vector<char> bytes = tx->serialize_transaction();
    // 生成交易 ID
    tx->id = sha256_digest(bytes);
    return tx;
}

// 创建一笔 UTXO 交易 
Transaction* new_utxo_transaction(string from, string to, int amount, vector<Transaction*> &unspent_txs) {
    // 找到足够的未花费输出
    int accumulated = 0;
    map<string, vector<int>> valid_outputs;
    for (auto tx : unspent_txs) {
        string txid = tx->id;
        for (int idx = 0; idx < tx->vout.size(); idx++) {
            auto txout = tx->vout[idx];
            if (txout.can_be_unlocked_with(from)) {
                accumulated += txout.value;
                valid_outputs[txid].push_back(idx);
                if (accumulated >= amount) {
                    goto endloop;
                }
            }
        }
    }
    endloop:
    if (accumulated < amount) {
        std::cerr << "Error: not enough funds" << std::endl;
        exit(1);
    }
    // 交易的输入
    vector<TXInput> inputs;
    for (auto it = valid_outputs.begin(); it != valid_outputs.end(); it++) {
        string txid = it->first;
        vector<int> outs = it->second;
        for (auto out_idx : outs) {
            TXInput txin = {txid, out_idx, from};
            inputs.push_back(txin);
        }
    }
    // 交易的输出
    vector<TXOutput> outputs = { {amount, to} };
    // 如果 UTXO 总数超过所需, 则产生找零
    if (accumulated > amount) {
        outputs.push_back({accumulated - amount, from});
    }
    Transaction* tx = new Transaction{"", inputs, outputs};
    vector<char> bytes = tx->serialize_transaction();
    // 生成交易 ID
    tx->id = sha256_digest(bytes);
    return tx;
}

// 交易序列化为字节数组
std::vector<char> Transaction::serialize_transaction() {
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
        // script_sig
        size += sizeof(size_t);
        size += txin.script_sig.size() + 1;
    }
    size += sizeof(size_t);
    for (auto& txout : this->vout) {
        // value
        size += sizeof(txout.value);
        // script_pub_key
        size += sizeof(size_t);
        size += txout.script_pub_key.size() + 1;
    }
    // 构造字节数组
    std::vector<char> bytes(size);
    char* ptr = bytes.data();
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
        // script_sig
        size_t script_sig_size = txin.script_sig.size();
        memcpy(ptr, &script_sig_size, sizeof(script_sig_size));
        ptr += sizeof(script_sig_size);
        memcpy(ptr, txin.script_sig.c_str(), txin.script_sig.size() + 1);
        ptr += txin.script_sig.size() + 1;
    }
    size_t vout_size = this->vout.size();
    memcpy(ptr, &vout_size, sizeof(vout_size));
    ptr += sizeof(vout_size);
    for (auto& txout : this->vout) {
        memcpy(ptr, &txout.value, sizeof(txout.value));
        ptr += sizeof(txout.value);
        // script_pub_key
        size_t script_pub_key_size = txout.script_pub_key.size();
        memcpy(ptr, &script_pub_key_size, sizeof(script_pub_key_size));
        ptr += sizeof(script_pub_key_size);
        memcpy(ptr, txout.script_pub_key.c_str(), txout.script_pub_key.size() + 1);
        ptr += txout.script_pub_key.size() + 1;
    }
    return bytes;
}

// 从字节数组反序列化为交易
void Transaction::deserialize_transaction(std::vector<char> data) {
    char* ptr = data.data();
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
        // script_sig
        size_t script_sig_size;
        memcpy(&script_sig_size, ptr, sizeof(script_sig_size));
        ptr += sizeof(script_sig_size);
        char *script_sig = new char[script_sig_size + 1];
        memcpy(script_sig, ptr, script_sig_size + 1);
        txin.script_sig = script_sig; 
        ptr += script_sig_size + 1;
        // 释放内存
        delete [] script_sig;
        script_sig = nullptr;
        this->vin.push_back(txin);
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
        char *script_pub_key = new char[script_pub_key_size + 1];
        memcpy(script_pub_key, ptr, script_pub_key_size + 1);
        txout.script_pub_key = script_pub_key;
        ptr += script_pub_key_size + 1;
        // 释放内存
        delete [] script_pub_key;
        script_pub_key = nullptr;
        this->vout.push_back(txout);
    }
}

