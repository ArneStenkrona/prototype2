#ifndef STRING_UTIL_H
#define STRING_UTIL_H

#include "src/container/vector.h"

namespace string_util {

/**
 * splits string by char c
 * @param input: input string
 * @return vector containing splits
 */ 
prt::vector<std::string> splitString(const std::string& input, char c);

};

#endif