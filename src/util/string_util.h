#ifndef STRING_UTIL_H
#define STRING_UTIL_H

#include "src/container/vector.h"

namespace string_util {

/**
 * splits string by char c
 * @param input input string
 * @return vector containing splits
 */ 
prt::vector<std::string> splitString(std::string const & input, char c);

/**
 * splits null terminated string by delimiter
 * @param input null terminated input string (will be modified by 
 *              inserting null characters into start 
 *              of delimiter occurences)
 * @param delimiter null terminated delimiter string
 * @return vector containing pointer into split 
 *         locations in original string
 */
prt::vector<char *> split(char *input, const char *delimiter);

/**
 * splits null terminated string by binary token
 * @param input null terminated input string, potentially containing
 *              bytes with value 0x00 before null termination.
 *              (will be modified by 
 *              inserting null characters into start 
 *              of delimiter occurences)
 * @param inputLength length of input string
 * @param binaryToken array of bytes
 * @param tokenLength length of byte array
 * @return vector containing pointer into split 
 *         locations in original string
 */

/**
 * Appends c string to char vector 
 * @param str null terminated vector of char
 * @param app null terminated string to be appended 
 */
void append(prt::vector<char> & str, const char* app);
void append(prt::vector<unsigned char> & str, const char* app);

};

#endif