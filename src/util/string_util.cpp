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
