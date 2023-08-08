#include "json/value.h"
#include "json/writer.h"
#include <netinet/in.h>
#include <openssl/sha.h>
#include <openssl/ecdsa.h>
#include <openssl/evp.h>
#include <openssl/ripemd.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <json/json.h>
#include <algorithm>
#include <sys/stat.h>
#include <chrono>
#include <sstream>
#include <random>
#include <arpa/inet.h>
#include <unistd.h>
#include "util.h"

// 获取当前时间戳
long current_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    return timestamp;
}

// 比较两个 vector 是否相等
bool are_vectors_equal(const std::vector<unsigned char>& v1, const std::vector<unsigned char>& v2) {
    if (v1.size() != v2.size()) {
        return false;
    }
    return std::equal(v1.begin(), v1.end(), v2.begin());
}

// 计算 sha256 摘要
vector<unsigned char> sha256_digest(vector<unsigned char> bytes) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, bytes.data(), bytes.size());
    SHA256_Final(hash, &sha256);
    return vector<unsigned char>(hash, hash + SHA256_DIGEST_LENGTH);
}

// 计算字符串的sha256
string sha256_digest_hex(vector<unsigned char> bytes) {
    char buf[3];
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, bytes.data(), bytes.size());
    SHA256_Final(hash, &sha256);
    string output = "";
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        snprintf(buf, sizeof(buf), "%02x", hash[i]);
        output += buf;
    }
    return output;
}

// 计算 ripemd160 摘要
vector<unsigned char> ripemd160_digest(vector<unsigned char> bytes) {
    vector<unsigned char> hash(RIPEMD160_DIGEST_LENGTH);
    RIPEMD160(bytes.data(), bytes.size(), hash.data());
    return hash;
}

// 转换为 16 进制
string to_hex(vector<unsigned char> bytes) {
    char buf[3];
    string output = "";
    for(int i = 0; i < bytes.size(); i++) {
        snprintf(buf, sizeof(buf), "%02x", bytes[i]);
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

// 创建密钥对( ECDSA 椭圆曲线)
EC_KEY* new_ecdsa_key_pair() {
    EC_KEY* eckey = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
    EC_KEY_generate_key(eckey);
    return eckey;
}

// 获取 DER 格式私钥
vector<unsigned char> get_private_key(EC_KEY* ec_key) {
    vector<unsigned char> private_key_vec;
    unsigned char* private_key_ptr = private_key_vec.data();
    i2d_ECPrivateKey(ec_key, &private_key_ptr);
    return private_key_vec;
}

// ECDSA 签名
vector<unsigned char> ecdsa_p256_sha256_sign_digest(EC_KEY* eckey, const vector<unsigned char>& digest) {
    // 计算签名
    // https://www.openssl.org/docs/man1.1.1/man3/ECDSA_sign.html
    ECDSA_SIG* signature = ECDSA_do_sign(digest.data(), digest.size(), eckey);
    // 将签名编码成 DER 格式
    vector<unsigned char> signature_vec;
    int signature_len = i2d_ECDSA_SIG(signature, nullptr);
    signature_vec.resize(signature_len);
    unsigned char* signature_ptr = signature_vec.data();
    i2d_ECDSA_SIG(signature, &signature_ptr);
    // 释放资源
    ECDSA_SIG_free(signature);
    return signature_vec;
}

// ECDSA 验证签名
bool ecdsa_p256_sha256_sign_verify(const vector<unsigned char>& public_key, const vector<unsigned char>& signature, const vector<unsigned char>& digest) {
    // 解码 DER 格式的公钥
    // https://www.openssl.org/docs/man1.1.1/man3/d2i_EC_PUBKEY.html 
    EC_KEY* eckey = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
    const unsigned char* public_key_ptr = public_key.data();
    o2i_ECPublicKey(&eckey, &public_key_ptr, public_key.size()); 
    // 解码 DER 格式的签名
    // https://www.openssl.org/docs/man1.1.1/man3/d2i_ECDSA_SIG.html
    ECDSA_SIG* ecdsa_sig = ECDSA_SIG_new();
    const unsigned char* signature_ptr = signature.data();
    d2i_ECDSA_SIG(&ecdsa_sig, &signature_ptr, signature.size());
    // 验证签名
    int result = ECDSA_do_verify(digest.data(), digest.size(), ecdsa_sig, eckey);
    // 释放资源
    ECDSA_SIG_free(ecdsa_sig);
    EC_KEY_free(eckey);
    return result;
}

// All alphanumeric characters except for "0", "I", "O", and "l"
static const char* pszBase58 = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

// 编码 base58
std::string encode_base58(const unsigned char* pbegin, const unsigned char* pend)
{
    // Skip & count leading zeroes.
    int zeroes = 0;
    int length = 0;
    while (pbegin != pend && *pbegin == 0) {
        pbegin++;
        zeroes++;
    }
    // Allocate enough space in big-endian base58 representation.
    int size = (pend - pbegin) * 138 / 100 + 1; // log(256) / log(58), rounded up.
    std::vector<unsigned char> b58(size);
    // Process the bytes.
    while (pbegin != pend) {
        int carry = *pbegin;
        int i = 0;
        // Apply "b58 = b58 * 256 + ch".
        for (std::vector<unsigned char>::reverse_iterator it = b58.rbegin(); (carry != 0 || i < length) && (it != b58.rend()); it++, i++) {
            carry += 256 * (*it);
            *it = carry % 58;
            carry /= 58;
        }

        assert(carry == 0);
        length = i;
        pbegin++;
    }
    // Skip leading zeroes in base58 result.
    std::vector<unsigned char>::iterator it = b58.begin() + (size - length);
    while (it != b58.end() && *it == 0)
        it++;
    // Translate the result into a string.
    std::string str;
    str.reserve(zeroes + (b58.end() - it));
    str.assign(zeroes, '1');
    while (it != b58.end())
        str += pszBase58[*(it++)];
    return str;
}

// 解码 base58
bool decode_base58(const char* psz, std::vector<unsigned char>& vch) {
    // Skip leading spaces.
    while (*psz && isspace(*psz))
        psz++;
    // Skip and count leading '1's.
    int zeroes = 0;
    int length = 0;
    while (*psz == '1') {
        zeroes++;
        psz++;
    }
    // Allocate enough space in big-endian base256 representation.
    int size = strlen(psz) * 733 /1000 + 1; // log(58) / log(256), rounded up.
    std::vector<unsigned char> b256(size);
    // Process the characters.
    while (*psz && !isspace(*psz)) {
        // Decode base58 character
        const char* ch = strchr(pszBase58, *psz);
        if (ch == nullptr)
            return false;
        // Apply "b256 = b256 * 58 + ch".
        int carry = ch - pszBase58;
        int i = 0;
        for (std::vector<unsigned char>::reverse_iterator it = b256.rbegin(); (carry != 0 || i < length) && (it != b256.rend()); ++it, ++i) {
            carry += 58 * (*it);
            *it = carry % 256;
            carry /= 256;
        }
        assert(carry == 0);
        length = i;
        psz++;
    }
    // Skip trailing spaces.
    while (isspace(*psz))
        psz++;
    if (*psz != 0)
        return false;
    // Skip leading zeroes in b256.
    std::vector<unsigned char>::iterator it = b256.begin() + (size - length);
    while (it != b256.end() && *it == 0)
        it++;
    // Copy result into output vector.
    vch.reserve(zeroes + (b256.end() - it));
    vch.assign(zeroes, 0x00);
    while (it != b256.end())
        vch.push_back(*(it++));
    return true;
}

// 编码 base58
std::string encode_base58(const std::vector<unsigned char>& vch) {
    return encode_base58(vch.data(), vch.data() + vch.size());
}

// 解码 base58
bool decode_base58(const std::string& str, std::vector<unsigned char>& vchRet) {
    return decode_base58(str.c_str(), vchRet);
}

// 编码 base64
string encode_base64(const vector<unsigned char>& vch) {
    BIO* b64_bio = BIO_new(BIO_f_base64());
    BIO_set_flags(b64_bio, BIO_FLAGS_BASE64_NO_NL);

    BIO* mem_bio = BIO_new(BIO_s_mem());
    BIO_push(b64_bio, mem_bio);

    BIO_write(b64_bio, vch.data(), vch.size());
    BIO_flush(b64_bio);

    BUF_MEM* mem_ptr;
    BIO_get_mem_ptr(b64_bio, &mem_ptr);

    string encoded_data = string(mem_ptr->data, mem_ptr->length);

    BIO_free_all(b64_bio);

    return encoded_data;
}

// 解码 base64
bool decode_base64(const string& str, vector<unsigned char>& vchRet) {
    BIO* b64_bio = BIO_new(BIO_f_base64());
    BIO_set_flags(b64_bio, BIO_FLAGS_BASE64_NO_NL);

    BIO* mem_bio = BIO_new_mem_buf(str.c_str(), str.size());
    BIO_push(b64_bio, mem_bio);

    vector<unsigned char> buffer(str.size());
    int decoded_size = BIO_read(b64_bio, buffer.data(), buffer.size());

    BIO_free_all(b64_bio);

    if (decoded_size > 0) {
        vchRet.assign(buffer.begin(), buffer.begin() + decoded_size);
        return true;
    }
    return false;
}

// 创建目录
bool create_directory(const string& path) {
    struct stat st;
    if (stat(path.c_str(), &st) == 0) {
        // 目录已经存在
        if (S_ISDIR(st.st_mode)) {
            return true;
        }
        // 不是目录，返回错误
        return false;
    }
    // 目录不存在，尝试创建
    if (mkdir(path.c_str(), 0777) == 0) {
        return true;
    }
    // 创建失败，尝试递归创建父目录
    char parent[1024];
    strcpy(parent, path.c_str());
    char* p = strrchr(parent, '/');
    if (p != NULL) {
        *p = '\0';
        if (create_directory(parent)) {
            if (mkdir(path.c_str(), 0777) == 0) {
                return true;
            }
        }
    }
    return false;
}

// 删除文件夹
void delete_directory(const std::string& dirname) {
    DIR* dir = opendir(dirname.c_str());
    if (dir == nullptr) {
        std::cout << "Cannot open directory " << dirname << std::endl;
        return;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        std::string path = dirname + "/" + entry->d_name;
        if (entry->d_type == DT_DIR) {
            // 如果是目录，递归删除
            delete_directory(path);
        } else {
            // 如果是文件，直接删除
            remove(path.c_str());
        }
    }

    closedir(dir);
    rmdir(dirname.c_str());
}

// 生成 UUID
std::string generateUUID() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);

    std::stringstream ss;
    ss << std::hex;

    for (int i = 0; i < 32; ++i) {
        int random_value = dis(gen);
        char random_digit = static_cast<char>(random_value < 10 ? '0' + random_value : 'a' + (random_value - 10));
        ss << random_digit;
        if (i == 7 || i == 11 || i == 15 || i == 19) {
            ss << '-';
        }
    }
    return ss.str();
}

// socket 地址转字符串
std::string sockaddr_tostring(sockaddr_in addr) {
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(addr.sin_addr), ip, INET_ADDRSTRLEN);
    return string(ip) + ":" + to_string(ntohs(addr.sin_port));
}


bool isValidIPAddress(const char* ipAddress) {
    struct sockaddr_in sa;
    return inet_pton(AF_INET, ipAddress, &(sa.sin_addr)) != 0;
}

bool isValidPort(int port) {
    return port > 0 && port <= 65535;
}

// 解析 socket 地址
bool parse_address(const string& addr, sockaddr_in& sockaddr) {
    // 解析地址
    char ip[16];
    int port;
    std::sscanf(addr.data(), "%[^:]:%d", ip, &port);
    // 检查地址的有效性
    if (!isValidIPAddress(ip)) {
        return false;
    }
    if (!isValidPort(port)) {
        return false;
    }
    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET; // IPv4
    sockaddr.sin_addr.s_addr = inet_addr(ip);
    sockaddr.sin_port = htons(port);
    return true;
}

// 序列化为 JSON
std::string serialize_to_json(vector<string> data) {
    Json::Value root;
    for (int i = 0; i < data.size(); i++) {
        root.append(data[i]);
    }
    Json::FastWriter writer;
    return writer.write(root);
}

// 反序列化为 JSON
vector<string> deserialize_from_json(const string& json) {
    vector<string> data;
    Json::Reader reader;
    Json::Value root;
    if (reader.parse(json, root)) {
        if (!root.isArray()) {
            return data;
        }
        for (int i = 0; i < root.size(); i++) {
            data.push_back(root[i].asString());
        }
    }
    return data;
}

