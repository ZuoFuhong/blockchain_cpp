#pragma once

#include <map>
#include "util.h"

// 中心节点地址
const string CENTERAL_NODE = "127.0.0.1:2001";

class Config {
public:

    // 获取配置
    static Config* get_instance();

    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    // 获取节点地址
    string get_node_address();

    // 设置矿工钱包地址
    void set_mining_address(string& mining_address);

    // 获取矿工钱包地址
    string get_mining_address();

    // 是否是矿工
    bool is_miner(); 

private:
    Config() = default;
    map<string, string> inner;
};

