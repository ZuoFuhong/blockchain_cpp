#include "json/value.h"
#include "json/writer.h"
#include <json/json.h>
#include <cstddef>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "config.h"
#include "memory_pool.h"
#include "server.h"
#include "transaction.h"
#include "utxo_set.h"
#include "config.h"
#include "util.h"

// 版本号
const uint8_t NODE_VERSION = 1;

// 内存池中的交易阈值, 触发矿工挖新区块
const uint8_t TRANSACTION_THRESHOLD = 2;

// 最大报文长度
const size_t MAXLINE = 2048;

// 报文类型
enum class PackageType: uint8_t {
    Block = 1,
    GetBlocks = 2,
    GetData = 3,
    Inv = 4,
    Tx = 5,
    Version = 6,
};

Server::Server(string addr, Blockchain* bc, UTXOSet* utxo, MemoryPool* tx_pool) {
    nodes.push_back(CENTERAL_NODE);

    this->bc = bc;
    this->utxo = utxo;
    this->addr = addr;
    this->tx_pool = tx_pool;
}

// 创建服务器
Server* Server::new_server(string addr, Blockchain* bc) {
    MemoryPool* tx_pool = MemoryPool::new_memory_pool();
    UTXOSet* utxo = UTXOSet::new_utxo_set(bc);
    return new Server(addr, bc, utxo, tx_pool);
}

void Server::run() { 
    struct sockaddr_in servaddr, cliaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    // 服务器地址
    if (!parse_address(addr, servaddr)) {
        std::cerr << "Invalid address " << addr << std::endl;
        exit(1);
    }

    // 创建 socket 文件描述符
    int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        std::cout << "socket creation failed" << std::endl;
        exit(1);
    }

    // 绑定 socket 文件描述符
    if (::bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        std::cerr << "bind socket to " << addr << " failed" << std::endl;
        exit(1);
    }

    // 发送 VERSION 消息
    if (addr != CENTERAL_NODE) {
        long height = bc->get_last_height();
        std::cout << "send version height: " << height << std::endl;
        send_version(CENTERAL_NODE, height);
    }
    std::cout << "Start node server on " << addr << std::endl;
    // 接收报文
    char buffer[MAXLINE];
    socklen_t len = sizeof(cliaddr);
    while (true) {
        int n = recvfrom(sockfd, (char *)buffer, MAXLINE, MSG_WAITALL, (struct sockaddr *)&cliaddr, &len);
        if (n < 0) {
            std::cerr << "Error in recvfrom" << std::endl;
            close(sockfd);
            exit(1);
        } else if (n == 0) {
            std::cout << "Client closed the connection" << std::endl;
            close(sockfd);
            exit(1);
        }
 
        // 处理接收到的消息
        this->serve(cliaddr, vector<unsigned char>(buffer, buffer + n));
    }
}

// 处理接收到的消息
void Server::serve(sockaddr_in cliaddr, std::vector<unsigned char> data) {
    // 第一个字节为报文类型
    PackageType ptype = static_cast<PackageType>(data.front());
    switch (ptype) {
        case PackageType::Block:
            {
                Json::Value root;
                Json::Reader reader;
                if (!reader.parse(string(data.begin() + 1, data.end()), root)) {
                    std::cout << "Error in parsing json data." << std::endl;
                    return;
                }
                string addr_from = root["addr_from"].asString(); 
                unique_ptr<Block> block(Block::from_json(root["block"].asString()));
                if (block == nullptr) {
                    std::cout << "Error in parsing block." << std::endl;
                    return;
                }
                bc->add_block(block.get());
                std::cout << "Added block " << block->hash << std::endl;

                // 继续区块下载
                if (blocks_in_transit.size() > 0) {
                    string block_hash = blocks_in_transit.front();
                    send_get_data(addr_from, OpType::Block, block_hash);
                    // 从下载队列中移除
                    blocks_in_transit.erase(blocks_in_transit.begin()); 
                } else {
                    // 区块全部下载后, 再重建索引
                    utxo->reindex();
                }
                break;
            }
        case PackageType::GetBlocks:
            {
                string addr_from = string(data.begin() + 1, data.end());
                vector<string> block_hashes = bc->get_block_hashes();
                send_inv(addr_from, OpType::Block, block_hashes);
                break;
            }
        case PackageType::GetData:
            {
                Json::Value root;
                Json::Reader reader;
                if (!reader.parse(string(data.begin() + 1, data.end()), root)) {
                    std::cout << "Invalid get data." << std::endl;
                    return;
                }
                string addr_from = root["addr_from"].asString();
                OpType otype = static_cast<OpType>(root["op_type"].asInt());
                string id = root["id"].asString();
                switch (otype) {
                    case OpType::Block:
                        {
                            unique_ptr<Block> block(bc->get_block(id));
                            if (block == nullptr) {
                                std::cout << "Block not found." << std::endl;
                                return;
                            }
                            send_block(addr_from, block.get());
                            break;
                        }
                    case OpType::Tx:
                        {
                            auto tx = tx_pool->get(id);
                            if (tx == nullptr) {
                                std::cout << "Transaction not found." << std::endl;
                                return;
                            }
                            send_tx(addr_from, tx);
                            break;
                        }
                    default:
                        std::cout << "Invalid operation type." << std::endl;
                        return;
                }
                break;
            }
        case PackageType::Inv:
            {
                Json::Value root;
                Json::Reader reader;
                if (!reader.parse(string(data.begin() + 1, data.end()), root)) {
                    std::cout << "Invalid package." << std::endl;
                    return;
                }
                string addr_from = root["addr_from"].asString(); 
                OpType otype = static_cast<OpType>(root["op_type"].asInt());
                vector<string> items;
                if (root["items"].isArray()) {
                    for (auto item : root["items"]) {
                        items.push_back(item.asString());
                    }
                }
                if (items.size() == 0) {
                    return;
                }
                switch (otype) {
                    // 两种触发情况: 
                    // 1. 当 version 消息检查到区块高度落后时, 会收到全量的 block hash 列表.
                    // 2. 矿工挖出新的区块后, 会将新区块的 hash 广播给其他节点.
                    case OpType::Block:
                        {
                            blocks_in_transit.insert(blocks_in_transit.end(), items.begin(), items.end());
                            string block_hash = items.front();
                            // 下载一个区块
                            send_get_data(addr_from, OpType::Block, block_hash);
                            // 从下载队列中移除
                            blocks_in_transit.erase(blocks_in_transit.begin());
                            break;
                        }
                    case OpType::Tx:
                        {
                            string txid = items.front();
                            // 检查交易池, 不包含则下载
                            if (!tx_pool->containes(txid)) {
                                send_get_data(addr_from, OpType::Tx, txid);
                            }
                            break;
                        }
                    default:
                        std::cout << "Invalid operation type." << std::endl;
                        return;
                }
                break;
            }
        case PackageType::Tx:
            {
                Json::Value root;
                Json::Reader reader;
                if (!reader.parse(string(data.begin() + 1, data.end()), root)) {
                    std::cout << "Invalid transaction." << std::endl;
                    return;
                }
                string addr_from = root["addr_from"].asString();
                Transaction* tx = Transaction::from_json(root["tx"].asString());
                if (tx == nullptr) {
                    std::cout << "Invalid transaction." << std::endl;
                    return;
                } 
                // 将交易添加到内存池
                tx_pool->add(tx);

                // 中心节点广播交易
                string node_addr = Config::get_instance()->get_node_address();
                if (node_addr == CENTERAL_NODE) {
                    for (auto node : nodes) {
                        // 过滤当前节点
                        if (node == node_addr) {
                            continue;
                        }
                        // 过滤来源节点
                        if (node == sockaddr_tostring(cliaddr)) {
                            continue;
                        }
                        // 发送交易
                        send_inv(node, OpType::Tx, vector<string>{tx->id});
                    }
                }
                // 矿工节点(内存池中的交易数量达到阈值, 挖新区块)
                if (tx_pool->len() >= TRANSACTION_THRESHOLD && Config::get_instance()->is_miner()) {
                    // 挖矿奖励
                    string mining_address = Config::get_instance()->get_mining_address();  
                    Transaction* coinbase_tx = Transaction::new_coinbase_tx(mining_address);
                    // 内存池中的交易
                    auto txs = tx_pool->get_all();
                    txs.push_back(coinbase_tx);
                    // 挖区块
                    unique_ptr<Block> new_block(bc->mine_block(txs));
                    // 重建 UTXO
                    utxo->reindex();
                    std::cout << "New block mined: " << new_block->hash << std::endl;

                    // 从内存池中移除交易
                    for (auto tx : txs) {
                        tx_pool->remove(tx->id);
                    }
                    // 广播区块
                    for (auto node : nodes) {
                        // 过滤当前节点
                        if (node == node_addr) {
                            continue;
                        }
                        // 发送区块
                        send_inv(node, OpType::Block, vector<string>{new_block->hash});
                    }
                }
                break;
            }
        case PackageType::Version:
            {
                Json::Value root;
                Json::Reader reader;
                if (!reader.parse(string(data.begin() + 1, data.end()), root)) {
                    std::cout << "Invalid version message." << std::endl;
                    return;
                }
                string addr_from = root["addr_from"].asString();
                int version = root["version"].asInt();
                long height = std::stol(root["height"].asString());
                std::cout << "Version: " << version << ", height: " << height << std::endl;

                long local_height = this->bc->get_last_height();
                if (height > local_height) {
                    // 向目标节点请求区块
                    send_get_blocks(addr_from);
                }
                if (height < local_height) {
                    // 提醒目标节点拉取区块
                    send_version(addr_from, local_height);
                }
                // 记录节点地址
                this->add_node(addr_from);
                break;
            }
        default:
            std::cout << "Invalid package type." << std::endl;
            return;
    }
}

// 注册节点
void Server::add_node(string addr) {
    if (std::find(nodes.begin(), nodes.end(), addr) == nodes.end()) {
        nodes.push_back(addr);
    }
}

// 析构函数~Server();
Server::~Server() {
    delete bc;
}

// 发送 UDP 数据包
void send_udp(string addr, vector<unsigned char> data) {
    sockaddr_in sockaddr;
    if (!parse_address(addr, sockaddr)) {
        std::cerr << "Invalid address " << addr << std::endl; 
        exit(1);
    }
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        std::cerr << "Failed to create socket." << std::endl;
        exit(1);
    }
    ssize_t n = sendto(sockfd, data.data(), data.size(), 0, (const struct sockaddr *)&sockaddr, sizeof(sockaddr));
    if (n == -1) {
        std::cerr << "Failed to send data." << std::endl;
        close(sockfd);
        exit(1);
    }
    close(sockfd);
}

// 下载数据
void send_get_data(string addr, OpType otype, string id) {
    Json::Value root;
    root["addr_from"] = Config::get_instance()->get_node_address();
    root["op_type"] = static_cast<int>(otype);
    root["id"] = id;
    Json::FastWriter writer;
    string body = writer.write(root);
    // 转换为字节流
    vector<unsigned char> data;
    data.push_back(static_cast<unsigned char>(PackageType::GetData));
    data.insert(data.end(), body.begin(), body.end());
    // 发送数据
    send_udp(addr, data); 
}

// 发送区块
void send_block(string addr, Block* block) {
    Json::Value root;
    root["addr_from"] = Config::get_instance()->get_node_address();
    root["block"] = block->to_json();
    Json::FastWriter writer;
    string body = writer.write(root);
    // 转换为字节流
    vector<unsigned char> data;
    data.push_back(static_cast<unsigned char>(PackageType::Block));
    data.insert(data.end(), body.begin(), body.end());
    // 发送数据
    send_udp(addr, data);
}

// 发送交易
void send_tx(string addr, Transaction* tx) {
    Json::Value root;
    root["addr_from"] = Config::get_instance()->get_node_address();
    root["tx"] = tx->to_json();
    Json::FastWriter writer;
    string body = writer.write(root);
    // 转换为字节流
    vector<unsigned char> data;
    data.push_back(static_cast<unsigned char>(PackageType::Tx));
    data.insert(data.end(), body.begin(), body.end());
    // 发送数据
    send_udp(addr, data);
}

// 发送 INV 消息
void send_inv(string addr, OpType otype, vector<string> block_hashes) {
    Json::Value root;
    root["addr_from"] = Config::get_instance()->get_node_address();
    root["op_type"] = static_cast<int>(otype);
    Json::Value items;
    for (auto s : block_hashes) {
        items.append(s);
    }
    root["items"] = items;
    Json::FastWriter writer;
    string body = writer.write(root);
    // 转换为字节流
    vector<unsigned char> data;
    data.push_back(static_cast<unsigned char>(PackageType::Inv));
    data.insert(data.end(), body.begin(), body.end());
    // 发送数据
    send_udp(addr, data);
}

// 发送 VERSION 消息
void send_version(string addr, long height) {
    Json::Value root;
    root["addr_from"] = Config::get_instance()->get_node_address();
    root["version"] = NODE_VERSION;
    root["height"] = std::to_string(height);
    Json::FastWriter writer;
    string body = writer.write(root);
    // 转换为字节流
    vector<unsigned char> data{static_cast<unsigned char>(PackageType::Version)};
    data.insert(data.end(), body.begin(), body.end());
    // 发送数据
    send_udp(addr, data);
}

// 发送 GET_BLOCKS 消息
void send_get_blocks(string addr) {
    string add_from = Config::get_instance()->get_node_address();
    vector<unsigned char> data{static_cast<unsigned char>(PackageType::GetBlocks)};
    data.insert(data.end(), add_from.begin(), add_from.end());
    // 发送数据
    send_udp(addr, data);
}

