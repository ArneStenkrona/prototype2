#ifndef STRING_UTIL_H
#define STRING_UTIL_H

#include "src/container/vector.h"

namespace string_util {

/**
 * splits string by char c
 * @param input: input string
 * @return vector containing splits
 */ 
prt::vector<std::string> splitString(std::string input, char c) {
    prt::vector<std::string> vec;
    std::string buffer;

    for (unsigned int i = 0; i < input.length(); i++) {
        if (input[i] == c && !buffer.empty()) {
            vec.push_back(buffer);
            buffer = "";
        }
        else {
            buffer += input[i];
        }
    }
    if (buffer.size() > 0) vec.push_back(buffer);

    return vec;
}

};

#endif