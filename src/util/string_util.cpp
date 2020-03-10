#include "string_util.h"

prt::vector<std::string> string_util::splitString(const std::string& input, char c) {
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

prt::vector<char *> string_util::split(char *input, const char *delimiter) {
    assert(input != nullptr);
    assert(delimiter != nullptr);
    assert((strcmp(delimiter, "") == 0) && "Delimiter may not be empty!");
    //assert((strcmp(input, "") == 0));

    prt::vector<char *> split;

    char *str = input;
    if (strlen(str) > 0) {
            split.push_back(str);
    }
    char *end = strstr(str, delimiter);
    while (end != nullptr) {
        *end = '\0';
        str = end + strlen(delimiter);
        if (strlen(str) > 0) {
            split.push_back(str);
        }
        end = strstr(str, delimiter);
    }
    
    return split;
}

void string_util::append(prt::vector<char> & str, const char* app) {
    if (app == nullptr) return;
    while (*app != '\0') {
        str.push_back(*app++);
    }
}

