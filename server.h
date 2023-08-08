#ifndef SERVER_H
#define SERVER_H

#include <netinet/in.h>
#include <string>
#include "blockchain.h"
#include "memory_pool.h"
#include "transaction.h"

// 操作类型
enum class OpType: uint8_t {
    Block = 1,
    Tx = 2,
};

class Server {
public:
    // 构造函数
    Server(string addr, Blockchain* bc, UTXOSet* utxo, MemoryPool* tx_pool);

    // 析构函数
    ~Server();

    static Server* new_server(string addr, Blockchain* bc);

    // 启动服务器
    void run();

private:
    Blockchain* bc;
    UTXOSet* utxo;
    string addr;
    vector<string> nodes;
    vector<string> blocks_in_transit;
    MemoryPool* tx_pool;

    // 处理接收到的消息
    void serve(struct sockaddr_in addr, std::vector<unsigned char> data);

    // 注册节点
    void add_node(string addr);
};

// 下载数据
void send_get_data(string addr, OpType otype, string id);

// 发送区块
void send_block(string addr, Block* block);

// 发送交易
void send_tx(string addr, Transaction* tx);

// 发送 INV 消息
void send_inv(string addr, OpType otype, vector<string> block_hashes);

// 发送 VERSION 消息
void send_version(string addr, long height);

// 发送 GET_BLOCKS 消息
void send_get_blocks(string addr);

#endif // SERVER_H
