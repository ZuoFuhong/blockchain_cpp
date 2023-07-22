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
string sha256_digest(vector<char> bytes) {
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

// 转换为 16 进制
string to_hex(long num) {
    stringstream ss;
    ss << hex << num;
    return ss.str();
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
