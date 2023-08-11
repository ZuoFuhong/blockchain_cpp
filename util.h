#pragma once

#include <openssl/ecdsa.h>
#include <iostream>
#include <dirent.h>
#include <unistd.h>
#include <netinet/in.h>

using namespace std;

// 获取当前时间戳
long current_timestamp();

// 比较两个 vector 是否相等
bool are_vectors_equal(const std::vector<unsigned char>& v1, const std::vector<unsigned char>& v2);

// 计算 sha256 摘要
vector<unsigned char> sha256_digest(vector<unsigned char> bytes);

// 计算 sha256 16 进制摘要
string sha256_digest_hex(vector<unsigned char> bytes);

// 计算 ripemd160 摘要
vector<unsigned char> ripemd160_digest(vector<unsigned char> bytes);

// 转换为 16 进制
string to_hex(vector<unsigned char> bytes);

// 转换为 16 进制
string to_hex(long num);

// 创建密钥对( ECDSA 椭圆曲线)
EC_KEY* new_ecdsa_key_pair();

// 获取 DER 格式私钥
vector<unsigned char> get_private_key(EC_KEY* ec_key);

// ECDSA 签名
vector<unsigned char> ecdsa_p256_sha256_sign_digest(EC_KEY* eckey, const vector<unsigned char>& digest);

// ECDSA 验证签名
bool ecdsa_p256_sha256_sign_verify(const vector<unsigned char>& public_key, const vector<unsigned char>& signature, const vector<unsigned char>& digest);

// 编码 base58
std::string encode_base58(const std::vector<unsigned char>& vch);

// 解码 base58
bool decode_base58(const std::string& str, std::vector<unsigned char>& vchRet);

// 编码 base64
string encode_base64(const vector<unsigned char>& vch);

// 解码 base64
bool decode_base64(const string& str, vector<unsigned char>& vchRet);

// 创建目录
bool create_directory(const string& path);

// 删除文件夹
void delete_directory(const string& dirname);

// 生成 UUID
std::string generateUUID();

// socket 地址转字符串
std::string sockaddr_tostring(sockaddr_in addr);

// 解析 socket 地址
bool parse_address(const string& addr, sockaddr_in& sockaddr);

// 序列化为 JSON
std::string serialize_to_json(vector<string> data);

// 反序列化为 JSON
vector<string> deserialize_from_json(const string& json);

