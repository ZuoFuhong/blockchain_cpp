#ifndef TRANSACTION_H
#define TRANSACTION_H

#include "common.h"

// 创建 coinbase 交易, 该交易没有输入, 只有一个输出
Transaction* new_coinbase_tx(string to, string data);

// 创建一笔 UTXO 交易 
Transaction* new_utxo_transaction(string from, string to, int amount, vector<Transaction*> &unspent_txs);

#endif // TRANSACTION_H
