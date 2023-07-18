#include <vector>
#include "third/clipp.h"
#include "blockchain.h"
#include "block.h"
#include "common.h"

using namespace std;
using namespace clipp;

// 命令
enum class Command {
    addblock,
    printchain,
    help,
};

int main(int argc, char *argv[]) {
    Command selected = Command::help;
    vector<string> input;
    
    auto addblock = (
        command("addblock").set(selected, Command::addblock),
        value("data", input)
    );
    auto printchain = (
        command("printchain").set(selected, Command::printchain),
        opt_value("hash", input)
    );
    auto help = command("help").set(selected, Command::help);
    auto cli = (
        addblock | printchain | help
    );

    Blockchain *bc = new Blockchain();
    BlockchainIterator *bi = bc->iterator();
    int seq = 0;
    string execname = argv[0];

    if (parse(argc, argv, cli)) {
        switch(selected) {
            case Command::addblock:
                bc->add_block(input[0]);
                break;
            case Command::printchain:
                while (true) {
                    Block *block = bi->next();
                    if (block == nullptr) {
                        break;
                    }
                    std::cout << ++seq << " Prev.hash: " << block->pre_block_hash << ", Hash: " << block->hash << std::endl;
                }
                break;
            case Command::help:
                cout << make_man_page(cli, execname) << endl;
                break;
        }
    } else {
        cout << usage_lines(cli, execname) << endl;
    }
}

