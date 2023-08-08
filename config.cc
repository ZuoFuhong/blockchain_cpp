#include <cstdlib>
#include "config.h"

const string NODE_ADDRESS_KEY = "NODE_ADDRESS";
const string MINING_ADDRESS_KEY = "MINING_ADDRESS"; 

// 获取配置
Config* Config::get_instance() {
    static Config instance;
    return &instance;
}

// 获取节点地址
string Config::get_node_address() {
    string node_addr = CENTERAL_NODE; 
    if (char* env_val = getenv(NODE_ADDRESS_KEY.c_str()); env_val != nullptr) {
        node_addr = env_val;
    }
    return node_addr;
}

// 设置矿工钱包地址
void Config::set_mining_address(string& mining_address) {
    inner[MINING_ADDRESS_KEY] = mining_address;
}

// 获取矿工钱包地址
string Config::get_mining_address() {
    return inner[MINING_ADDRESS_KEY];
}

// 是否是矿工
bool Config::is_miner() {
    return inner.find(MINING_ADDRESS_KEY) != inner.end();
}

