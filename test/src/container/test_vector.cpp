#include "test/src/prt_test.h"
#include <catch2/catch.hpp>
#include "src/container/vector.h"

#include <string>

TEST_CASE( "vector: Test vector", "[vector]") {
    prt::vector<uint32_t> vec1;
    for (size_t i = 0; i < 100; i++) {
        vec1.push_back(i*i + i);
    }
    for (size_t i = 0; i < 100; i++) {
        REQUIRE(vec1[i] == i*i + i);
    }

    prt::vector<uint32_t> vec2;
    vec2.resize(10000);
    for (size_t i = 0; i < 10000; i++) {
        vec2[i] = i*i + i;
    }
    for (size_t i = 0; i < 10000; i++) {
        REQUIRE(vec2[i] == i*i + i);
    }
}

TEST_CASE( "vector: Test insert string", "[vector]") {
    prt::vector<std::string> vec;
    for (size_t i = 0; i < 1000; i++) {
        std::string str = std::to_string(i);
        vec.push_back(str);
    }
    for (size_t i = 0; i < 1000; i++) {
        REQUIRE(vec[i] == std::to_string(i));
    }
}

TEST_CASE( "vector: Test copy constructor", "[vector]") {
    prt::vector<std::string> vec1;
    for (size_t i = 0; i < 1000; i++) {
        std::string str = std::to_string(i);
        vec1.push_back(str);
    }
    prt::vector<std::string> vec2 = vec1;
    REQUIRE(vec2.size() == vec1.size());

    for (size_t i = 0; i < 1000; i++) {
        REQUIRE(vec2[i] == std::to_string(i));
    }
}

TEST_CASE( "vector: Test copy assignment operator", "[vector]") {
    prt::vector<std::string> vec1, vec2;
    for (size_t i = 0; i < 1000; i++) {
        std::string str = std::to_string(i);
        vec1.push_back(str);
    }
    vec2 = vec1;
    REQUIRE(vec2.size() == vec1.size());
    REQUIRE(vec2.capacity() == 1000);

    for (size_t i = 0; i < 1000; i++) {
        REQUIRE(vec2[i] == std::to_string(i));
    }
}

TEST_CASE( "vector: Test nested", "[vector]") {
    prt::vector<prt::vector<uint32_t> > test;
    test.resize(100);
    for (uint32_t i = 0; i <  test.size(); i++) {
        test[i].resize(100);
        for  (uint32_t j = 0; j < test[i].size(); j++) {
            test[i][j] = i * j + i + j;
        }
    }
    for (uint32_t i = 0; i <  test.size(); i++) {
        for  (uint32_t j = 0; j < test[i].size(); j++) {
            REQUIRE(test[i][j] == i * j + i + j);
        }
    }
}

struct TestEmplace {
    TestEmplace(int32_t x, int32_t y, int32_t z) : _x(x+8), _y(y*y), _z(z-5) {}
    int32_t _x, _y, _z;
};

TEST_CASE( "vector: Test vector emplace", "[vector]") {
    prt::vector<TestEmplace> vec1;
    for (size_t i = 0; i < 100; i++) {
        vec1.emplace_back(i, i+1, i+2);
    }
    for (size_t i = 0; i < 100; i++) {
        REQUIRE(vec1[i]._x == i + 8);
        REQUIRE(vec1[i]._y == (i+1)*(i+1));
        REQUIRE(vec1[i]._z == i + 2 - 5);
    }
    REQUIRE(vec1.size() == 100);
}

TEST_CASE( "vector: Test vector remove", "[vector]") {
    prt::vector<uint32_t> vec1 = {0,1,2,3,4,5,6,7,8,9};

    vec1.remove(4);   
    prt::vector<uint32_t> vec2 = {0,1,2,3,5,6,7,8,9};

    REQUIRE(vec1.size() == vec2.size());

    for (size_t i = 0; i < vec1.size(); ++i) {
        REQUIRE(vec1[i] == vec2[i]);
    }

    vec1.remove(2);   
    vec2 = {0,1,3,5,6,7,8,9};

    REQUIRE(vec1.size() == vec2.size());
    
    for (size_t i = 0; i < vec1.size(); ++i) {
        REQUIRE(vec1[i] == vec2[i]);
    }

    vec1.remove(5);   
    vec2 = {0,1,3,5,6,8,9};

    REQUIRE(vec1.size() == vec2.size());
    
    for (size_t i = 0; i < vec1.size(); ++i) {
        REQUIRE(vec1[i] == vec2[i]);
    }
}

TEST_CASE( "vector: Test vector remove many", "[vector]") {
    prt::vector<uint32_t> vec1 = {0,1,2,3,4,5,6,7,8,9};

    vec1.remove(4, 3);   
    prt::vector<uint32_t> vec2 = {0,1,2,3,7,8,9};

    REQUIRE(vec1.size() == vec2.size());

    for (size_t i = 0; i < vec1.size(); ++i) {
        REQUIRE(vec1[i] == vec2[i]);
    }
}

TEST_CASE( "vector: Test move constructor", "[vector]") {
    prt::vector<std::string> vec1;
    for (size_t i = 0; i < 1000; i++) {
        std::string str = std::to_string(i);
        vec1.push_back(str);
    }

    std::string * data = vec1.data();
    size_t size = vec1.size();

    prt::vector<std::string> vec2{std::move(vec1)};
    REQUIRE(vec2.size() == size);
    REQUIRE(vec2.data() == data);

    REQUIRE(vec1.size() == 0);
    REQUIRE(vec1.data() == nullptr);

    for (size_t i = 0; i < 1000; i++) {
        std::string str = std::to_string(i);
        REQUIRE(vec2[i].compare(str) == 0);
    }
}

TEST_CASE( "vector: Test move assignment operator", "[vector]") {
    prt::vector<std::string> vec1, vec2;
    for (size_t i = 0; i < 1000; i++) {
        std::string str = std::to_string(i);
        vec1.push_back(str);
    }

    std::string * data = vec1.data();
    size_t size = vec1.size();

    vec2 = std::move(vec1);
    REQUIRE(vec2.size() == size);
    REQUIRE(vec2.data() == data);

    REQUIRE(vec1.size() == 0);
    REQUIRE(vec1.data() == nullptr);

    for (size_t i = 0; i < 1000; i++) {
        std::string str = std::to_string(i);
        REQUIRE(vec2[i].compare(str) == 0);
    }
}
