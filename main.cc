#include <vector>
#include <map>
#include "third/clipp.h"
#include "blockchain.h"
#include "block.h"
#include "common.h"
#include "transaction.h"
#include "util.h"

using namespace std;
using namespace clipp;

// 命令行命令
enum class Command {
    createblockchain,
    getbalance,
    send,
    printchain,
    clearchain,
    help,
};

int main(int argc, char *argv[]) {
    Command selected = Command::help;
    vector<string> input;
    
    auto createblockchain = command("createblockchain").set(selected, Command::createblockchain);
    auto getbalance = (
        command("getbalance").set(selected, Command::getbalance),
        value("address", input)
    );
    auto send = (
        command("send").set(selected, Command::send),
        value("from", input),
        value("to", input),
        value("amount", input)
    );
    auto printchain = command("printchain").set(selected, Command::printchain);
    auto clearchain = command("clearchain").set(selected, Command::clearchain);
    auto help = command("help").set(selected, Command::help);
    auto cli = (
        createblockchain | 
        getbalance | 
        send | 
        printchain | 
        clearchain |
        help
    );

    string execname = argv[0];
    if (parse(argc, argv, cli)) {
        switch(selected) {
            case Command::createblockchain:
                {
                    new Blockchain();
                    std::cout << "Done!" << std::endl;
                    break;
                }
            case Command::getbalance:
                {
                    string address = input[0];
                    Blockchain *bc = new Blockchain();
                    auto utxos = bc->find_utxo(address);
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
                    Blockchain *bc = new Blockchain();
                    auto unspent_transactions = bc->find_unspent_transactions(from);
                    Transaction *tx = new_utxo_transaction(from, to, amount, unspent_transactions);
                    bc->mine_block(vector<Transaction*> {tx});
                    break;
                }
            case Command::printchain:
                {
                    Blockchain *bc = new Blockchain();
                    BlockchainIterator *iter = bc->iterator();
                    int seq = 0;
                    while (true) {
                        Block *block = iter->next();
                        if (block == nullptr) {
                            break;
                        }
                        std::cout << ++seq << "Timestamp: " << block->timestamp << ", prev_hash: " << block->pre_block_hash << ", hash: " << block->hash << std::endl;
                        for (auto tx : block->transactions) {
                            for (auto vin : tx->vin) {
                                std::cout << "Transaction input txid = " << vin.txid << ", vout = " << vin.vout << ", script_sig = " << vin.script_sig << std::endl;
                            }
                            for (auto vout : tx->vout) {
                                std::cout << "Transaction output txid = "  << tx->id << ", value = " << vout.value << ", script_pub_key = " << vout.script_pub_key << std::endl;
                            }
                        }
                    }
                    break;
                }
            case Command::clearchain:
                {
                    Blockchain *bc = new Blockchain();
                    bc->clear_data();
                    std::cout << "Done!" << std::endl;
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

