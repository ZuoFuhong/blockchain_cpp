#include <chrono>
#include <openssl/sha.h>
#include <sstream>
#include "util.h"

// 获取当前时间戳
long current_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    return timestamp;
}

// 计算字符串的sha256
string sha256_digest(const string str) {
    char buf[3];
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, str.c_str(), str.size());
    SHA256_Final(hash, &sha256);
    string output = "";
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        snprintf(buf, sizeof(buf), "%02x", hash[i]);
        output += buf;
    }
    return output;
}

// 转换为 16 进制
string to_hex(long num) {
    stringstream ss;
    ss << hex << num;
    return ss.str();
}

