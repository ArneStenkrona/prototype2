#include "test/src/prt_test.h"
#include <catch2/catch.hpp>
#include "src/container/hash_map.h"

#include <string>

TEST_CASE( "hash_table: Test insert", "[hash_table]") {
    prt::hash_map<uint32_t, uint32_t> table;

    for (uint32_t i = 0; i < 1000; i++) {
        table.insert(i, i * i - i);
    }
    for (uint32_t i = 0; i < 1000; i++) {
        REQUIRE(table.find(i)->value() == i * i - i);
    }
}

TEST_CASE( "hash_table: Test erase", "[hash_table]") {
    prt::hash_map<uint32_t, uint32_t> table;

    size_t s = 0;
    for (uint32_t i = 0; i < 1000; i++) {
        table.insert(i, i * i - i);
        s++;
    }

    for (uint32_t i = 0; i < 1000; i++) {
        if (i % 3 == 0) {
            table.erase(i);
            s--;
        }
    }

    for (uint32_t i = 0; i < 1000; i++) {
        if (i % 3 == 0) {
            REQUIRE((table.find(i) == table.end()));
        } else {
            REQUIRE((table.find(i) != table.end()));
        }
    }
    REQUIRE(table.size() == s);
}

TEST_CASE( "hash_table: Test subscript", "[hash_table]") {
    prt::hash_map<uint32_t, uint32_t> table;

    for (uint32_t i = 0; i < 1000; i++) {
        table[i] = i * i - i;
    }
    for (uint32_t i = 0; i < 1000; i++) {
        REQUIRE(table.find(i)->value() == i * i - i);
    }
}

TEST_CASE( "hash_table: Test find", "[hash_table]") {
    prt::hash_map<std::string, uint32_t> table;

    for (uint32_t i = 0; i < 1000; i++) {
        std::string str = std::to_string(i);
        table[str] = i;
    }

    for (uint32_t i = 0; i < 1000; i++) {
        std::string str = std::to_string(i);
        REQUIRE((table.find(str) != table.end()));
    }
    for (uint32_t i = 1000; i < 2000; i++) {
        std::string str = std::to_string(i);
        REQUIRE((table.find(str) == table.end()));
    }
}

TEST_CASE( "hash_table: Test iterate", "[hash_table]") {
    prt::hash_map<std::string, uint32_t> table;

    for (uint32_t i = 0; i < 1000; i++) {
        std::string str = std::to_string(i);
        table[str] = i;
    }

    for (auto it = table.begin(); it != table.end(); it++) {
        std::string& key = it->key();
        uint32_t val = it->value();
        REQUIRE(key == std::to_string(val));
    }
}

TEST_CASE( "hash_table: Test copy constructor", "[hash_table]") {
    prt::hash_map<uint32_t, uint32_t> table1;

    for (uint32_t i = 0; i < 1000; i++) {
        table1.insert(i, i * i - i);
    }

    prt::hash_map<uint32_t, uint32_t> table2(table1);

    for (uint32_t i = 0; i < 1000; i++) {
        REQUIRE(table2.find(i)->value() == i * i - i);
    }
}

TEST_CASE( "hash_table: Test copy assignment operator", "[hash_table]") {
    prt::hash_map<uint32_t, uint32_t> table1, table2;

    for (uint32_t i = 0; i < 1000; i++) {
        table1.insert(i, i * i - i);
    }
    
    table2 = table1;

    for (uint32_t i = 0; i < 1000; i++) {
        REQUIRE(table2.find(i)->value() == i * i - i);
    }
}

TEST_CASE( "hash_table: Test move constructor", "[hash_table]") {
    prt::hash_map<uint32_t, uint32_t> table1;

    for (uint32_t i = 0; i < 1000; i++) {
        table1.insert(i, i * i - i);
    }

    size_t size = table1.size();
    
    prt::hash_map<uint32_t, uint32_t> table2{std::move(table1)};

    REQUIRE(table1.size() == 0);
    REQUIRE(table2.size() == size);

    for (uint32_t i = 0; i < 1000; i++) {
        REQUIRE(table2.find(i)->value() == i * i - i);
    }
}

TEST_CASE( "hash_table: Test move assignment operator", "[hash_table]") {
    prt::hash_map<uint32_t, uint32_t> table1;

    for (uint32_t i = 0; i < 1000; i++) {
        table1.insert(i, i * i - i);
    }

    size_t size = table1.size();
    
    prt::hash_map<uint32_t, uint32_t> table2 = std::move(table1);

    REQUIRE(table1.size() == 0);
    REQUIRE(table2.size() == size);

    for (uint32_t i = 0; i < 1000; i++) {
        REQUIRE(table2.find(i)->value() == i * i - i);
    }
}
