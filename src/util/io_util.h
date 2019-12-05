#ifdef IO_UTIL_H
#define IO_UTIL_H

namespace IO_util {
    static prt::vector<char> readFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("failed to open file: " + filename);
        }
        
        size_t fileSize = (size_t) file.tellg();
        prt::vector<char> buffer(fileSize);
        
        file.seekg(0);
        file.read(buffer.data(), fileSize);
        
        file.close();
        
        return buffer;
    }

}

#endif