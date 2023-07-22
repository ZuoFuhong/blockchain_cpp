#ifndef UTIL_H
#define UTIL_H

#include <iostream>
#include <dirent.h>
#include <unistd.h>

using namespace std;

// 获取当前时间戳
long current_timestamp();

// 计算字符串的sha256
string sha256_digest(vector<char> bytes);

// 转换为 16 进制
string to_hex(long num);

// 删除文件夹
void delete_directory(const string& dirname);

#endif // UTIL_H
