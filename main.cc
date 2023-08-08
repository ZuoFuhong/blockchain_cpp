#include "third/clipp.h"
#include "blockchain.h"
#include "block.h"
#include "transaction.h"
#include "util.h"
#include "wallet.h"
#include "utxo_set.h"
#include "server.h"
#include "config.h"

using namespace std;
using namespace clipp;

// mine 标志指的是块会立刻被同一节点挖出来。必须要有这个标志，因为初始状态时，网络中没有矿工节点。
bool MINE_TRUE = false;

// 命令行命令
enum class Command {
    createblockchain,
    createwallet,
    listaddresses,
    getbalance,
    send,
    printchain,
    clearchain,
    reindexutxo,
    startnode,
    help,
};

int main(int argc, char *argv[]) {
    Command selected = Command::help;
    vector<string> input;
    
    auto createblockchain = command("createblockchain").set(selected, Command::createblockchain);
    auto createwallet = command("createwallet").set(selected, Command::createwallet);
    auto listaddresses = command("listaddresses").set(selected, Command::listaddresses);
    auto getbalance = (
        command("getbalance").set(selected, Command::getbalance),
        value("address", input)
    );
    auto send = (
        command("send").set(selected, Command::send),
        value("from", input),
        value("to", input),
        value("amount", input),
        option("-mine").set(MINE_TRUE)
    );
    auto printchain = command("printchain").set(selected, Command::printchain);
    auto clearchain = command("clearchain").set(selected, Command::clearchain);
    auto reindexutxo = command("reindexutxo").set(selected, Command::reindexutxo);
    auto startnode = (
        command("startnode").set(selected, Command::startnode),
        option("miner") & value("address", input)
    );
    auto help = command("help").set(selected, Command::help);
    auto cli = (
        createblockchain | 
        getbalance | 
        send | 
        createwallet |
        listaddresses |
        printchain | 
        clearchain |
        reindexutxo |
        startnode |
        help
    );

    string execname = argv[0];
    if (parse(argc, argv, cli)) {
        switch(selected) {
            case Command::createblockchain:
                {
                    auto bc = Blockchain::new_blockchain();
                    auto utxo_set = UTXOSet::new_utxo_set(bc);
                    utxo_set->reindex();
                    std::cout << "Done!" << std::endl;
                    break;
                }
            case Command::createwallet:
                {
                    Wallet *wallet = Wallet::new_wallet();
                    string address = wallet->get_address();
                    std::cout << "Your new address: " << address << std::endl;
                    break;
                }
            case Command::listaddresses:
                { vector<string> address = get_addresses();
                    for (auto addr : address) {
                        std::cout << addr << std::endl;
                    }
                    break;
                }
            case Command::getbalance:
                {
                    string address = input[0];
                    if (!validate_address(address)) {
                        std::cout << "ERROR: Address is not valid" << std::endl;
                        break;
                    }
                    vector<unsigned char> payload;
                    decode_base58(address, payload);
                    auto pub_key_hash = vector<unsigned char>(payload.begin() + 1, payload.end() - ADDRESS_CHECK_SUM_LEN);
                    // 统计余额
                    auto bc = Blockchain::new_blockchain();
                    auto utxo_set = UTXOSet::new_utxo_set(bc);
                    auto utxos = utxo_set->find_utxo(pub_key_hash);
                    int balance = 0;
                    for (auto utxo : utxos) {
                        balance += utxo.value;
                    }
                    std::cout << "Balance of " << address << ": " << balance << std::endl;
                    break;
                }
            case Command::send:
                {
                    string from = input[0];
                    string to = input[1];
                    int amount = stoi(input[2]);
                    if (!validate_address(from)) {
                        std::cout << "ERROR: Sender address is not valid" << std::endl;
                        break;
                    }
                    if (!validate_address(to)) {
                        std::cout << "ERROR: Recipient address is not valid" << std::endl;
                        break;
                    }
                    if (amount <= 0) {
                        std::cout << "ERROR: Amount must be greater than 0" << std::endl;
                        break;
                    }
                    Blockchain *bc = Blockchain::new_blockchain();
                    UTXOSet* utxo_set = UTXOSet::new_utxo_set(bc);
                    // 创建 UTXO 交易
                    auto tx = Transaction::new_utxo_transaction(from, to, amount, utxo_set);
                    if (MINE_TRUE) {
                        // 挖矿奖励
                        auto coinbase_tx = Transaction::new_coinbase_tx(from);
                        // 挖新区块
                        auto block = bc->mine_block(vector<Transaction*> {tx, coinbase_tx});
                        // 更新 UTXO 集
                        utxo_set->update(block);
                    } else {
                        // 发送交易到中心节点
                        send_tx(CENTERAL_NODE, tx);
                    }
                    cout << "Success!" << endl;
                    break;
                }
            case Command::printchain:
                {
                    Blockchain *bc = Blockchain::new_blockchain();
                    BlockchainIterator *iter = bc->iterator();
                    while (true) {
                        Block *block = iter->next();
                        if (block == nullptr) {
                            break;
                        }
                        std::cout << "Prev_hash: " << block->pre_block_hash << ", hash: " << block->hash << ", height: " << block->height << std::endl;
                        for (auto tx : block->transactions) {
                            for (auto vin : tx->vin) {
                                string address = pub_key_hash_to_address(hash_pub_key(vin.pub_key));
                                cout << "Transaction input txid = " << vin.txid << ", vout = " << vin.vout << ", from = " << address << endl;  
                            }
                            for (auto vout : tx->vout) {
                                string address = pub_key_hash_to_address(vout.pub_key_hash);
                                cout << "Transaction output txid = " << tx->id << ", value = " << vout.value << ", to = " << address << endl;
                            }
                        }
                        std::cout << "Timestamp: " << block->timestamp << std::endl;
                    }
                    break;
                }
            case Command::clearchain:
                {
                    Blockchain::clear_data();
                    std::cout << "Done!" << std::endl;
                    break;
                }
            case Command::reindexutxo:
                {
                    Blockchain *bc = Blockchain::new_blockchain();
                    UTXOSet *utxo_set = UTXOSet::new_utxo_set(bc);
                    utxo_set->reindex();
                    cout << "Done! There are " << utxo_set->count_transactions() << " transactions in the UTXO set." << endl;
                    break;
                }
            case Command::startnode:
                {
                    if (input.size() == 1) {
                        string miner_address = input[0];
                        if (!validate_address(miner_address)) {
                            std::cout << "ERROR: Miner address is not valid" << std::endl;
                            break;
                        }
                        std::cout << "Mining is on. Address to receive rewards: " << miner_address << std::endl;
                        auto config = Config::get_instance(); 
                        config->set_mining_address(miner_address);
                    }
                    Blockchain *bc = Blockchain::new_blockchain();
                    string node_addr = Config::get_instance()->get_node_address();
                    Server::new_server(node_addr, bc)->run();
                    break;
                }
            case Command::help:
                cout << make_man_page(cli, execname) << endl;
                break;
        }
    } else {
        cout << "Usage:\n" << usage_lines(cli, execname) << endl;
    }
    return 0;
}

