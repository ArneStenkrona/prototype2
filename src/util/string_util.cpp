#include "string_util.h"

prt::vector<std::string> string_util::splitString(std::string const & input, char c) {
    prt::vector<std::string> vec;
    std::string buffer;

    for (unsigned int i = 0; i < input.length(); ++i) {
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
    assert((delimiter[0] != '\0') && "Delimiter may not be empty!");

    prt::vector<char *> split;
    if (input[0] == '\0') return split;

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
    assert((str.size() > 0) && "Empty string must be null terminated!");
    if (app == nullptr) return;
    str[str.size() - 1] = *app;
    do {
        ++app;
        str.push_back(*app);
    } while(*app != '\0');
}
void string_util::append(prt::vector<unsigned char> & str, const char* app) {
    assert((str.size() > 0) && "Empty string must be null terminated!");
    if (app == nullptr) return;
    str[str.size() - 1] = *app;
    do {
        ++app;
        str.push_back(*app);
    } while(*app != '\0');
}
