#include <gtest/gtest.h>
#include "wallet.h"

TEST(WalletTests, create_wallet) {
    auto wallet = Wallet::new_wallet();
    string address = wallet->get_address(); 
    EXPECT_TRUE(validate_address(address));
}

TEST(WalletTests, verify_address) {
    string address = "1BvBMSEYstWetqTFn5Au4m4GFg7xJaNVN2";
    EXPECT_TRUE(validate_address(address));
}
