#ifndef STRING_UTIL_H
#define STRING_UTIL_H

#include "src/container/vector.h"

namespace string_util {

/**
 * splits string by char c
 * @param input input string
 * @return vector containing splits
 */ 
prt::vector<std::string> splitString(const std::string& input, char c);

/**
 * splits c string by delimiter
 * @param input input c string (will be modified by 
 *              inserting null characters into start 
 *              of delimiter occurences)
 * @param delimiter delimiter c string
 * @return vector containing pointer into split 
 *         locations in original string
 */
prt::vector<char *> split(char *input, const char *delimiter);

/**
 * Appends c string to char vector 
 * @param str char vector
 * @param app string to be appended 
 */
void append(prt::vector<char> & str, const char* app);

};

#endif