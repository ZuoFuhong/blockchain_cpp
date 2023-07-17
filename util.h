#ifndef UTIL_H
#define UTIL_H

#include <iostream>

using namespace std;

// 获取当前时间戳
long current_timestamp();

// 计算字符串的sha256
string sha256_digest(const string str);

// 转换为 16 进制
string to_hex(long num);

#endif // UTIL_H
