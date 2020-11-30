#ifndef IO_UTIL_H
#define IO_UTIL_H

#include "src/container/vector.h"
#include <fstream>

namespace io_util {
    prt::vector<char> readFile(const std::string& filename);

    bool is_file_exist(char const * file);
}

#endif