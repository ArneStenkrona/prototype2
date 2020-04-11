#include "fbx_document.h"

#include <cstdio>

#include "zlib.h"

#include "src/util/string_util.h"

#include "src/memory/memory_util.h"

FBX::Document::Document(const char* file) 
    :_dataStack(prt::getAlignment(alignof(std::max_align_t))) {
    std::ifstream input(file, std::ios::binary);
    assert(input && "Cannot open fbx file!");
    // Read header
    bool validHeader = readHeader(input);
    assert(validHeader && "Invalid FBX Header!");
    // read version
    readScalar<uint32_t>(input, reinterpret_cast<unsigned char*>(&version));
    // Initialize the data stack
    initializeStack(input, version);
    // Get pointer to head of stack
    unsigned char *stackPointer = _dataStack.data();
    // Read nodes
    while(readNode(input, _root.children, version, stackPointer));

     /* Since the footer of FBX is not entirely known     
    // Read footer code 
    unsigned char footerCode[FOOTER_CODE_SIZE];
    input.read(reinterpret_cast<char*>(footerCode), FOOTER_CODE_SIZE);
    unsigned char validCode[FOOTER_CODE_SIZE];
    generateFooterCode(reinterpret_cast<unsigned char*>(validCode));

   
        it remains unverified
    if (!checkEqual(footerCode, validCode, FOOTER_CODE_SIZE)) {
        //assert(false && "Incorrect footer code!");
    }

    // Read footer extension
    bool validFooterExtension = checkFooter(input, version);

    if (!validFooterExtension) {
        assert(false && "Invalid footer!");
    }
    */
}

void FBX::Document::initializeStack(std::ifstream & input, VERSION version) {
    auto pos = input.tellg();
    auto state = input.rdstate();

    size_t size = 0;
    while(getNodeDataSize(input, version, size));

    _dataStack.resize(size);

    // rewind stream
    input.setstate(state);
    input.seekg(pos);
}

bool FBX::Document::getNodeDataSize(std::ifstream & input, VERSION version, size_t & size) {
    int64_t endOffset;
    int64_t numProperties;
    int64_t propertyListLen;

    if (version >= VERSION::V7_5) {
        int64_t intbuf;
        readScalar<int64_t>(input, intbuf);
        endOffset = intbuf;

        readScalar<int64_t>(input, intbuf);
        numProperties = intbuf;

        readScalar<int64_t>(input, intbuf);
        propertyListLen = intbuf;
    } else {
        int32_t intbuf;
        readScalar<int32_t>(input, intbuf);
        endOffset = intbuf;

        readScalar<int32_t>(input, intbuf);
        numProperties = intbuf;

        readScalar<int32_t>(input, intbuf);
        propertyListLen = intbuf;
    }
    char nameLen; 
    input.get(nameLen);
    char name[256] = "";
    if (nameLen != 0) {
        input.read(name, nameLen);
    }
    
    if (endOffset == 0) {
        // The end offset should only be 0 in a null node
        if (numProperties != 0 || propertyListLen != 0 || name[0] != '\0') {
            assert(false && "Invalid node! Expected NULL record.");
        }
        return false;
    }

    auto propertyEnd = input.tellg() + propertyListLen;
    // Read properties
    for (int64_t i = 0; i < numProperties; ++i) {
        getPropertyDataSize(input, size);
    }
    if (input.tellg() != propertyEnd) {
        assert(false && "Too many bytes in property list!");
        //input.seekg(propertyEnd);
    }

    // Read nested nodes
    auto listLen = endOffset - input.tellg();
    if (endOffset < input.tellg()) {
        assert(false && "Node has invalid endpoint!");
    }

    if (listLen > 0) {
        while(getNodeDataSize(input, version, size));
        if (input.tellg() != endOffset) {
            assert(false && "Too many bytes in node!");
        }
    }
    
    return true;
}

void FBX::Document::getPropertyDataSize(std::ifstream & input, size_t & size) {
    char dataType;
    input.get(dataType);
    switch (dataType) {
        case 'Y': {
            input.seekg(sizeof(int16_t), std::ios_base::seekdir::cur);
            size_t padding = prt::memory_util::calcPadding(uintptr_t(size), alignof(int16_t));
            size += padding + sizeof(int16_t);
            return;
        }
        case 'C': {
            input.seekg(sizeof(char), std::ios_base::seekdir::cur);
            size += sizeof(char);
            return; 
        }
        case 'I': {
            input.seekg(sizeof(int32_t), std::ios_base::seekdir::cur);
            size_t padding = prt::memory_util::calcPadding(uintptr_t(size), alignof(int32_t));
            size += padding + sizeof(int32_t);
            return;
        }
        case 'F': {
            input.seekg(sizeof(float), std::ios_base::seekdir::cur);
            size_t padding = prt::memory_util::calcPadding(uintptr_t(size), alignof(float));
            size += padding + sizeof(float);
            return;
        }
        case 'D': {
            input.seekg(sizeof(double), std::ios_base::seekdir::cur);
            size_t padding = prt::memory_util::calcPadding(uintptr_t(size), alignof(double));
            size += padding + sizeof(double);
            return;
        }
        case 'L': {
            input.seekg(sizeof(int64_t), std::ios_base::seekdir::cur);
            size_t padding = prt::memory_util::calcPadding(uintptr_t(size), alignof(int64_t));
            size += padding + sizeof(int64_t);
            return;
        }
        case 'f':
            return getArrayDataSize(input, Property::TYPE::ARRAY_FLOAT, size);
        case 'd':
            return getArrayDataSize(input, Property::TYPE::ARRAY_DOUBLE, size);
        case 'l':
            return getArrayDataSize(input, Property::TYPE::ARRAY_INT64, size);
        case 'i':
            return getArrayDataSize(input, Property::TYPE::ARRAY_INT32, size);
        case 'b':
            return getArrayDataSize(input, Property::TYPE::ARRAY_BOOL, size);
        case 'S': {
            uint32_t len;
            readScalar<uint32_t>(input, len);
            
            input.seekg(len, std::ios_base::seekdir::cur);
            size += len + 1;
            return;
        }
        case 'R': {
            uint32_t len;
            readScalar<uint32_t>(input, len);

            input.seekg(len, std::ios_base::seekdir::cur);
            size += sizeof(size_t) + len;
            return;
        }
        default: {
            assert(false && "Invalid property data type!");
            return;
        }
    }
}

void FBX::Document::getArrayDataSize(std::ifstream & input, Property::TYPE type, size_t & size) {
    int32_t len;
    readScalar<int32_t>(input, len);
    int32_t encoding;
    readScalar<int32_t>(input, encoding);
    int32_t compressedLen;
    readScalar<int32_t>(input, compressedLen);
    auto endPos = input.tellg() + std::streampos(compressedLen);
    input.seekg(endPos);

    size_t primSz = Property::PRIMITIVE_SIZES[type];
    // size_t primAlign = Property::PRIMITIVE_ALIGNMENTS[type];

    size_t padding = prt::memory_util::calcPadding(uintptr_t(size), alignof(size_t));
    size += sizeof(size_t) + padding + len*primSz;
}

const FBX::Document::Node * FBX::Document::Node::find(const char* name) const {
    for (auto const & n : children) {
        if (strcmp(n.getName(), name) == 0) {
           return &n;
        }
    }
    return nullptr;
}

const FBX::Document::Node * FBX::Document::Node::getRelative(const char* path) const {
    char buffer[256];
    strcpy(buffer, path);

    prt::vector<char*> tokens = string_util::split(buffer, "/");
    const Node *n = this;
    for (auto & t : tokens) {
        if (strcmp(t, "") == 0) {
            continue;
        }
        n = n->find(t);
        if (n == nullptr) {
            // assert(false && "TODO: figure out if this should return nullptr or crash");
            break;
        }
    }
    return n;
}


bool FBX::Document::readHeader(std::ifstream & input) {
    char buffer[HEADER_STRING_LENGTH];
    input.read(buffer, HEADER_STRING_LENGTH);
    return strcmp(buffer, HEADER_STRING) == 0;
}

bool FBX::Document::readNode(std::ifstream & input, prt::vector<Node>& nodes, 
                            VERSION version, unsigned char * & stackPointer) {
    int64_t endOffset;
    int64_t numProperties;
    int64_t propertyListLen;

    if (version >= VERSION::V7_5) {
        int64_t intbuf;
        readScalar<int64_t>(input, intbuf);
        endOffset = intbuf;

        readScalar<int64_t>(input, intbuf);
        numProperties = intbuf;

        readScalar<int64_t>(input, intbuf);
        propertyListLen = intbuf;
    } else {
        int32_t intbuf;
        readScalar<int32_t>(input, intbuf);
        endOffset = intbuf;

        readScalar<int32_t>(input, intbuf);
        numProperties = intbuf;

        readScalar<int32_t>(input, intbuf);
        propertyListLen = intbuf;
    }
    char nameLen; 
    input.get(nameLen);
    char name[256] = "";
    if (nameLen != 0) {
        input.read(name, nameLen);
    }
    
    if (endOffset == 0) {
        // The end offset should only be 0 in a null node
        if (numProperties != 0 || propertyListLen != 0 || name[0] != '\0') {
            assert(false && "Invalid node! Expected NULL record.");
        }
        return false;
    }

    nodes.push_back({});
    Node & node = nodes.back();
    node.setName(name);
    auto propertyEnd = input.tellg() + propertyListLen;
    // Read properties
    for (int64_t i = 0; i < numProperties; ++i) {
        node.properties.push_back(readProperty(input, stackPointer));
    }
    if (input.tellg() != propertyEnd) {
        assert(false && "Too many bytes in property list!");
        //input.seekg(propertyEnd);
    }

    // Read nested nodes
    auto listLen = endOffset - input.tellg();
    if (endOffset < input.tellg()) {
        assert(false && "Node has invalid endpoint!");
    }

    if (listLen > 0) {
        while(readNode(input, node.children, version, stackPointer));
        if (input.tellg() != endOffset) {
            assert(false && "Too many bytes in node!");
        }
    }
    
    return true;
}

FBX::Document::Property FBX::Document::readArray(std::ifstream & input, Property::TYPE type,  
                                               unsigned char * & stackPointer) {
    int32_t len;
    readScalar<int32_t>(input, len);
    int32_t encoding;
    readScalar<int32_t>(input, encoding);
    int32_t compressedLen;
    readScalar<int32_t>(input, compressedLen);
    auto endPos = input.tellg() + std::streampos(compressedLen);

    Property ret;
    ret.type = type;
    size_t primSz = Property::PRIMITIVE_SIZES[type];

    size_t padding = prt::memory_util::calcPadding(uintptr_t(stackPointer), 
                                                   alignof(size_t));//Property::PRIMITIVE_ALIGNMENTS[type]);
    stackPointer += padding;
    ret._data = stackPointer;
    *reinterpret_cast<size_t*>(ret._data) = int32_t(len);
    stackPointer += sizeof(size_t) + len*primSz;

    if (encoding != 0) {
        if (encoding != 1) {
            assert(false && "Invalid compression encoding (must be 0 or 1)");
        }
        char cmf;
        input.get(cmf);
        if ((cmf & 0xF) != 8 || (cmf >> 4) > 7) {
            assert(false && "Invalid compression format!");
        } 
        char flg;
        input.get(flg);
        if (((cmf << 8) + flg) % 31 != 0) {
            assert(false && "Invalid compression FCHECK!");
        }
        if ((flg & (1 << 5)) != 0) {
            assert(false && "Invalid compression flags; dictionary not supported!");
        }
        input.seekg(-2, std::ios_base::cur);
        // read compressed data
        prt::vector<unsigned char> compressedBuffer;
        compressedBuffer.resize(compressedLen);
        input.read(reinterpret_cast<char*>(compressedBuffer.data()), compressedLen);
        decompress(compressedBuffer.data(), compressedLen, ret.data(), len*primSz);
    } else {
        input.read(reinterpret_cast<char*>(ret.data()), len*primSz);
    }
    
    if (encoding != 0) {
        input.seekg(endPos - std::streamoff(sizeof(int32_t)));
        int32_t checkSum;
        readScalar<int32_t>(input, checkSum);
        //input.read(reinterpret_cast<char*>(&checkSum), sizeof(int32_t));
        /*TODO
        verify checksum
        */
    }    

    input.seekg(endPos);
    return ret;
}

FBX::Document::Property FBX::Document::readProperty(std::ifstream & input, unsigned char * & stackPointer) {
    char dataType;
    input.get(dataType);
    Property property{};
    switch (dataType) {
        case 'Y': {
            property.type = Property::INT16;
            size_t padding = prt::memory_util::calcPadding(uintptr_t(stackPointer), alignof(int16_t));
            stackPointer += padding;
            property._data = stackPointer;
            readScalar<int16_t>(input, property._data);
            stackPointer += sizeof(int16_t);
            return property;
        }
        case 'C': {
            property.type = Property::CHAR;
            property._data = stackPointer;
            input.read(reinterpret_cast<char*>(property._data), sizeof(char));
            stackPointer += sizeof(char);
            return property;
        }
        case 'I': {
            property.type = Property::INT32;
            size_t padding = prt::memory_util::calcPadding(uintptr_t(stackPointer), alignof(int32_t));
            stackPointer += padding;
            property._data = stackPointer;
            readScalar<int32_t>(input, property._data);
            stackPointer += sizeof(int32_t);
            return property;
        }
        case 'F': {
            property.type = Property::FLOAT;
            size_t padding = prt::memory_util::calcPadding(uintptr_t(stackPointer), alignof(float));
            stackPointer += padding;
            property._data = stackPointer;
            readScalar<float>(input, property._data);
            stackPointer += sizeof(float);
            return property;
        }
        case 'D': {
            property.type = Property::DOUBLE;
            size_t padding = prt::memory_util::calcPadding(uintptr_t(stackPointer), alignof(double));
            stackPointer += padding;
            property._data = stackPointer;
            readScalar<double>(input, property._data);
            stackPointer += sizeof(double);
            return property;
        }
        case 'L': {
            property.type = Property::INT64;
            size_t padding = prt::memory_util::calcPadding(uintptr_t(stackPointer), alignof(int64_t));
            stackPointer += padding;
            property._data = stackPointer;
            readScalar<int64_t>(input, property._data);
            stackPointer += sizeof(int64_t);
            return property;
        }
        case 'f':
            return readArray(input, Property::TYPE::ARRAY_FLOAT, stackPointer);
        case 'd':
            return readArray(input, Property::TYPE::ARRAY_DOUBLE, stackPointer);
        case 'l':
            return readArray(input, Property::TYPE::ARRAY_INT64, stackPointer);
        case 'i':
            return readArray(input, Property::TYPE::ARRAY_INT32, stackPointer);
        case 'b':
            return readArray(input, Property::TYPE::ARRAY_BOOL, stackPointer);
        case 'S': {
            property.type = Property::STRING;
            property._data = stackPointer;
            uint32_t len;
            readScalar<uint32_t>(input, len);
            std::fill(property._data, &property._data[len], '\0');
            prt::vector<char> data;
            data.resize(len + 1);
            if (len > 0) {
                input.read(data.data(), len);
            }
            // Convert \0\1 to '::' and reverse the tokens
            if (findBinarySeparator(data.data(), len)) {
                prt::vector<char*> tokens = splitWithBinarySeparator(data.data(), len);

                bool first = true;
                for (size_t i = tokens.size(); i > 0; --i) {
                    if (!first) {
                        strcat(reinterpret_cast<char*>(property._data), ASCII_SEPARATOR);
                    }
                    strcat(reinterpret_cast<char*>(property._data), tokens[i-1]);
                    first = false;
                }
            } else {
                std::memcpy(property._data, data.data(), data.size());
                stackPointer += len + 1;
            }
            return property;
        }
        case 'R': {
            uint32_t len;
            readScalar<uint32_t>(input, len);
            property._data = stackPointer;
            *reinterpret_cast<size_t*>(property._data) = size_t(len);
            input.read(reinterpret_cast<char*>(property.data()), len);
            stackPointer += len;
            return property;
        }
        default: {
            assert(false && "Invalid property data type!");
            return property;
        }
    }
}

bool FBX::Document::checkFooter(std::ifstream & input, VERSION version) {
    unsigned char buffer[footerZeroes2];
    input.read(reinterpret_cast<char*>(buffer), footerZeroes1);
    bool correct = allZero(buffer, footerZeroes1);

    int32_t readVersion;
    readScalar<int32_t>(input, readVersion);
    correct &= (readVersion == int32_t(version));

    input.read(reinterpret_cast<char*>(buffer), footerZeroes2);
    correct &= allZero(buffer, footerZeroes2);

    input.read(reinterpret_cast<char*>(buffer), FOOTER_CODE_SIZE);
    correct &= checkEqual(buffer, EXTENSION, FOOTER_CODE_SIZE);

    return correct;
}

void FBX::Document::generateFooterCode(unsigned char *buffer,
	                                  int32_t year, int32_t month, int32_t day,
	                                  int32_t hour, int32_t minute, int32_t second, int32_t millisecond) {
    if(year < 0 || year > 9999)
        assert(false && "Invalid year!");
    if(month < 0 || month > 12)
        assert(false && "Invalid month!");
    if(day < 0 || day > 31)
        assert(false && "Invalid day!");
    if(hour < 0 || hour >= 24)
        assert(false && "Invalid hour!");
    if(minute < 0 || minute >= 60)
        assert(false && "Invalid minute!");
    if(second < 0 || second >= 60)
        assert(false && "Invalid second!");
    if(millisecond < 0 || millisecond >= 1000)
        assert(false && "Invalid millisecond!");
    unsigned char mangledBytes[FOOTER_CODE_SIZE];
    for (size_t i = 0; i < FOOTER_CODE_SIZE; ++i) {
        buffer[i] = SOURCE_ID[i];
        mangledBytes[i] = '0';
    }
    sprintf(reinterpret_cast<char*>(&mangledBytes[0]),"%02d", second);
    sprintf(reinterpret_cast<char*>(&mangledBytes[2]),"%02d", month);
    sprintf(reinterpret_cast<char*>(&mangledBytes[4]),"%02d", hour);
    sprintf(reinterpret_cast<char*>(&mangledBytes[6]),"%02d", day);
    sprintf(reinterpret_cast<char*>(&mangledBytes[8]),"%02d", millisecond/10);
    sprintf(reinterpret_cast<char*>(&mangledBytes[10]),"%04d", year);
    sprintf(reinterpret_cast<char*>(&mangledBytes[14]),"%02d", minute);

    encrypt(buffer, mangledBytes);
    encrypt(buffer, KEY);
    encrypt(buffer, mangledBytes);                                          
}

void FBX::Document::generateFooterCode(unsigned char *buffer) {
    static constexpr char timePath[] = "FBXHeaderExtension/CreationTimeStamp";
    const Node *timestamp =  _root.getRelative(timePath);
    if (timestamp == nullptr) {
        assert(false && "No creation timestamp");
    }

    return generateFooterCode(buffer,
                              getTimestampVar(*timestamp, "Year"),
                              getTimestampVar(*timestamp, "Month"),
                              getTimestampVar(*timestamp, "Day"),
                              getTimestampVar(*timestamp, "Hour"),
                              getTimestampVar(*timestamp, "Minute"),
                              getTimestampVar(*timestamp, "Second"),
                              getTimestampVar(*timestamp, "Millisecond"));
}

void FBX::Document::encrypt(unsigned char *a, const unsigned char *b) {
    char c = 64;
    for (size_t i = 0; i < FOOTER_CODE_SIZE; ++i) {
        a[i] = (a[i] ^ (c ^ b[i]));
        c = a[i];
    }
}

int32_t FBX::Document::getTimestampVar(Node const & timestamp, const char *element) {
    const Node *elementNode = timestamp.find(element);

    if (elementNode != nullptr && elementNode->properties.size() > 0) {
        Property const & prop = elementNode->properties[0];
        if (prop.type == Property::TYPE::INT32 || prop.type == Property::TYPE::INT64) {
            int32_t ret;
            std::memcpy(&ret, prop._data, sizeof(int32_t));
            return ret;
        }
    }
    
    assert(false && "Timestamp does not contain element!");
    return 0;
}

bool FBX::Document::allZero(const unsigned char  *array, size_t sz) {
    for (size_t i = 0; i < sz; i++) {
        if (array[i] != 0) return false;
    }
    return true;
}

bool FBX::Document::checkEqual(const unsigned char *a, const unsigned char *b, size_t sz) {
    for (size_t i = 0; i < sz; i++) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

bool FBX::Document::findBinarySeparator(char *input, size_t length) {
    for (size_t i = 0; i < length; i++) {
        if (input[i] == '\0') {
            assert((input[i+1] == reinterpret_cast<const char&>(BINARY_SEPARATOR[1])) && "Invalid binary separator");
            return true;
        }
    }
    return false;
}

prt::vector<char*> FBX::Document::splitWithBinarySeparator(char *input, size_t length) {
    prt::vector<char*> token;
    if (length == 0) return token;
    token.push_back(input);
    for (size_t i = 0; i < length; i++) {
        if (input[i] == '\0') {
            assert((input[i+1] == reinterpret_cast<const char&>(BINARY_SEPARATOR[1])) && "Invalid binary separator");
            i += 2;
            token.push_back(&input[i]);
        }
    }
    return token;
}

bool FBX::Document::decompress(unsigned char *src, size_t compressedSize,
                              unsigned char *dest, size_t uncompressedSize) {
    // Thanks: https://gist.github.com/arq5x/5315739
    // zlib struct
    z_stream infstream;
    infstream.zalloc = Z_NULL;
    infstream.zfree = Z_NULL;
    infstream.opaque = Z_NULL;
    // setup "compress" as the input and "uncompressed" as the compressed output
    infstream.avail_in = uInt(compressedSize); // size of input
    infstream.next_in = reinterpret_cast<Bytef*>(src); // input char array

    infstream.avail_out = uInt(uncompressedSize); // size of output
    infstream.next_out = reinterpret_cast<Bytef*>(dest); // output char array
     
    // the actual DE-compression work.
    auto res1 = inflateInit(&infstream);
    auto res2 = inflate(&infstream, Z_NO_FLUSH);
    auto res3 = inflateEnd(&infstream);

    bool success = res1 == Z_OK && res2 == Z_STREAM_END && res3 == Z_OK;
    assert(success && "Could not decompress!");
    return success;
}

void FBX::Document::Node::print(std::ostream & out) const {
    printRecursive(out, 0);
}

void FBX::Document::Node::printRecursive(std::ostream & out, size_t indent) const {
    for (size_t i = 0; i < indent; ++i) {
        out << "    ";
    }
    out << _name << " {";
    for (auto const & property : properties) {
        property.print(out);
    }
    out << " }" << std::endl;
    for (auto const & child : children) {
        child.printRecursive(out, indent + 1);
    }
}

void FBX::Document::Property::print(std::ostream & out) const {
    enum TYPE {
        CHAR = 0,
        INT16 = 1,
        INT32 = 2,
        INT64 = 3,
        FLOAT = 4,
        DOUBLE = 5,
        STRING = 6,
        ARRAY_FLOAT = 7,
        ARRAY_DOUBLE = 8,
        ARRAY_INT32 = 9,
        ARRAY_INT64 = 10,
        ARRAY_BOOL = 11,
        RAW = 12,
        TOTAL_NUM_TYPES = 13
    };
    switch (type) {
        case TYPE::CHAR:
            out << " (Char: \'"
                      << *reinterpret_cast<const char*>(data())
                      << "\')";
            break;
        case TYPE::INT16:
            out << " (int16: "
                      << *reinterpret_cast<const int16_t*>(data())
                      << ")";
            break;
        case TYPE::INT32:
            out << " (int32: "
                      << *reinterpret_cast<const int32_t*>(data())
                      << ")";
            break;
        case TYPE::INT64:
            out << " (int64: "
                      << *reinterpret_cast<const int64_t*>(data())
                      << ")";
            break;
        case TYPE::FLOAT:
            out << " (float: "
                      << *reinterpret_cast<const float*>(data())
                      << ")";
            break;
        case TYPE::DOUBLE:
            out << " (double: "
                      << *reinterpret_cast<const double*>(data())
                      << ")";
            break;
        case TYPE::STRING:
            out << " (string: \""
                      << reinterpret_cast<const char*>(data())
                      << "\")";
            break;
        case TYPE::ARRAY_FLOAT:
            out << " (float["
                      << length()
                      << "])";
            break;
        case TYPE::ARRAY_DOUBLE:
            out << " (double["
                      << length()
                      << "])";
            break;
        case TYPE::ARRAY_INT32:
            out << " (int32["
                    << length()
                    << "])";
            break;
        case TYPE::ARRAY_INT64:
            out << " (int64["
                      << length()
                      << "])";
            break;
        case TYPE::ARRAY_BOOL:
            out << " (bool["
                      << length()
                      << "])";
            break;
        case TYPE::RAW:
            out << " (byte["
                      << length()
                      << "])";
            break;
        default:
            out << "(INVALID TYPE)";
            break;

    }
}
