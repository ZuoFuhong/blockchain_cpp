#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include <vector>
#include "block.h"

class Blockchain {
public:
    // 创建新的区块链
    Blockchain();
    
    // 析构函数
    ~Blockchain();

    // 添加区块
    void add_block(string data);

    // 获取区块链中的所有区块
    vector<Block*> get_blocks();
private:
    vector<Block*> blocks;
};

#endif // BLOCKCHAIN_H
