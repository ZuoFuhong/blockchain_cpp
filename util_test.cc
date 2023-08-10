#include <gtest/gtest.h>
#include "util.h"

TEST(UtilTests, encode_base64) {
    string str = "hello world";
    vector<unsigned char> bytes(str.begin(), str.end());
    string base64_str = encode_base64(bytes);
   
    vector<unsigned char> vchRet;
    bool ans = decode_base64(base64_str, vchRet);
    EXPECT_TRUE(ans);
    EXPECT_EQ(str, string(vchRet.begin(), vchRet.end()));
}
