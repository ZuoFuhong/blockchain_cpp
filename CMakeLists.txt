cmake_minimum_required(VERSION 3.18.2)
project(blockchain_cpp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -std=c++17")
set(INC_DIR /usr/local/include)
set(LINK_DIR /usr/local/lib)

include_directories(${INC_DIR})
link_directories(${LINK_DIR})

add_executable(blockchain 
    main.cc block.cc blockchain.cc proofofwork.cc transaction.cc wallet.cc utxo_set.cc server.cc memory_pool.cc config.cc util.cc
)
target_link_libraries(blockchain crypto gmp rocksdb jsoncpp)

# blockchain_test --gtest_main --gtest_filter=WalletTests.create_wallet
add_executable(blockchain_test 
    wallet_test.cc util_test.cc transaction_test.cc 
    block.cc block.cc blockchain.cc proofofwork.cc transaction.cc wallet.cc utxo_set.cc server.cc memory_pool.cc config.cc util.cc
)
target_link_libraries(blockchain_test crypto gmp rocksdb jsoncpp gtest gtest_main pthread)

# make test ARGS="-R WalletTests.create_wallet"
enable_testing()
add_test(NAME WalletTests.create_wallet COMMAND blockchain_test --gtest_filter=WalletTests.create_wallet)
add_test(NAME WalletTests.verify_address COMMAND blockchain_test --gtest_filter=WalletTests.verify_address)
add_test(NAME UtilTests.encode_base64 COMMAND blockchain_test --gtest_filter=UtilTests.encode_base64)
add_test(NAME TransactionTests.serialize_transaction COMMAND blockchain_test --gtest_filter=TransactionTests.serialize_transaction)

