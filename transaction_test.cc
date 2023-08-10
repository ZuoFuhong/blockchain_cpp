#include <gtest/gtest.h>
#include "wallet.h"
#include "transaction.h"

TEST(TransactionTests, serialize_transaction) {
    auto wallet = Wallet::new_wallet();
    // 交易序列化字节流
    auto coinbase_tx = Transaction::new_coinbase_tx(wallet->get_address());
    // 反序列化交易
    auto tx = Transaction::deserialize_transaction(coinbase_tx->serialize_transaction());
    // 公钥哈希转地址
    string address = pub_key_hash_to_address(tx->vout[0].pub_key_hash);
    EXPECT_EQ(wallet->get_address(), address);
}

