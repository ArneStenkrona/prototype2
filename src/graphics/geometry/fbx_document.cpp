#include "fbx_document.h"

#include <cstdio>

#include "zlib.h"

#include "src/util/string_util.h"

FBX_Document::FBX_Document(const char* file) {
    std::ifstream input(file, std::ios::binary);
    assert(input && "Cannot open fbx file!");

    // Read header
    bool validHeader = readHeader(input);
    assert(validHeader && "Invalid FBX Header!");
    // read version
    readScalar<uint32_t>(input, reinterpret_cast<unsigned char*>(&version));
    // Read nodes
    while(readNode(input, _root.children, version));

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

const FBX_Document::FBX_Node * FBX_Document::FBX_Node::find(const char* name) const {
    for (auto const & n : children) {
        if (strcmp(n.getName(), name) == 0) {
           return &n;
        }
    }
    return nullptr;
}

const FBX_Document::FBX_Node * FBX_Document::FBX_Node::getRelative(const char* path) const {
    char buffer[256];
    strcpy(buffer, path);

    prt::vector<char*> tokens = string_util::split(buffer, "/");
    const FBX_Node *n = this;
    for (auto & t : tokens) {
        if (strcmp(t, "") == 0) {
            continue;
        }
        n = n->find(t);
        if (n == nullptr) {
            assert(false && "TODO: figure out if this should return nullptr or just break");
            return nullptr;
            //break;
        }
    }
    return n;
}


bool FBX_Document::readHeader(std::ifstream & input) {
    char buffer[HEADER_STRING_LENGTH];
    input.read(buffer, HEADER_STRING_LENGTH);
    return strcmp(buffer, HEADER_STRING) == 0;
}

bool FBX_Document::readNode(std::ifstream & input, prt::vector<FBX_Node>& nodes, VERSION version) {
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
    FBX_Node & node = nodes.back();
    node.setName(name);
    auto propertyEnd = input.tellg() + propertyListLen;
    // Read properties
    for (int64_t i = 0; i < numProperties; ++i) {
        node.properties.push_back(readProperty(input));
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
        while(readNode(input, node.children, version));
        if (input.tellg() != endOffset) {
            assert(false && "Too many bytes in node!");
        }
    }
    
    return true;
}

FBX_Document::Property FBX_Document::readArray(std::ifstream & input, Property::TYPE type) {
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
        // read compressed data
        prt::vector<unsigned char> compressedBuffer;
        compressedBuffer.resize(compressedLen);
        input.read(reinterpret_cast<char*>(compressedBuffer.data()), compressedLen);
        prt::vector<unsigned char> uncompressedBuffer(Property::PRIMITIVE_ALIGNMENTS[type]);
        decompress(compressedBuffer, uncompressedBuffer, len*primSz);
        
        ret.data = uncompressedBuffer;

    } else {
        ret.data.resize(len*primSz);
        input.read(reinterpret_cast<char*>(ret.data.data()), len*primSz);
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

FBX_Document::Property FBX_Document::readProperty(std::ifstream & input) {
    char dataType;
    input.get(dataType);
    Property property{};
    switch (dataType) {
        case 'Y': {
            property.type = Property::INT16;
            property.data = prt::vector<unsigned char>(prt::getAlignment(alignof(int16_t)));
            property.data.resize(sizeof(int16_t));
            readScalar<int16_t>(input, property.data.data());
            return property;
        }
        case 'C': {
            property.type = Property::CHAR;
            property.data.resize(sizeof(char));
            input.read(reinterpret_cast<char*>(property.data.data()), sizeof(char));
            return property;
        }
        case 'I': {
            property.type = Property::INT32;
            property.data = prt::vector<unsigned char>(prt::getAlignment(alignof(int32_t)));
            property.data.resize(sizeof(int32_t));
            readScalar<int32_t>(input, property.data.data());
            return property;
        }
        case 'F': {
            property.type = Property::FLOAT;
            property.data = prt::vector<unsigned char>(prt::getAlignment(alignof(float)));
            property.data.resize(sizeof(float));
            readScalar<float>(input, property.data.data());
            return property;
        }
        case 'D': {
            property.type = Property::DOUBLE;
            property.data = prt::vector<unsigned char>(prt::getAlignment(alignof(double)));
            property.data.resize(sizeof(double));
            readScalar<double>(input, property.data.data());
            return property;
        }
        case 'L': {
            property.type = Property::INT64;
            property.data = prt::vector<unsigned char>(prt::getAlignment(alignof(int64_t)));
            property.data.resize(sizeof(int64_t));
            readScalar<int64_t>(input, property.data.data());
            return property;
        }
        case 'f':
            return readArray(input, Property::TYPE::ARRAY_FLOAT);
        case 'd':
            return readArray(input, Property::TYPE::ARRAY_DOUBLE);
        case 'l':
            return readArray(input, Property::TYPE::ARRAY_INT64);
        case 'i':
            return readArray(input, Property::TYPE::ARRAY_INT32);
        case 'b':
            return readArray(input, Property::TYPE::ARRAY_BOOL);
        case 'S': {
            property.type = Property::STRING;
            property.data.resize(1,'\0');
            uint32_t len;
            readScalar<uint32_t>(input, len);
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
                        string_util::append(property.data, ASCII_SEPARATOR);
                    }
                    string_util::append(property.data, tokens[i-1]);
                    first = false;
                }
            } else {
                property.data.resize(data.size());
                for (size_t i = 0; i < data.size(); i++) {
                    property.data[i] = reinterpret_cast<char>(data[i]);
                }
            }
            return property;
        }
        case 'R': {
            property.type = Property::RAW;
            property.data = prt::vector<unsigned char>(prt::getAlignment(alignof(uint32_t)));
            uint32_t len;
            readScalar<uint32_t>(input, len);
            property.data.resize(len);
            input.read(reinterpret_cast<char*>(property.data.data()), len);
            return property;
        }
        default: {
            assert(false && "Invalid property data type!");
            return property;
        }
    }
}

bool FBX_Document::checkFooter(std::ifstream & input, VERSION version) {
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

void FBX_Document::generateFooterCode(unsigned char *buffer,
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

void FBX_Document::generateFooterCode(unsigned char *buffer) {
    static constexpr char timePath[] = "FBXHeaderExtension/CreationTimeStamp";
    const FBX_Node *timestamp =  _root.getRelative(timePath);
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

void FBX_Document::encrypt(unsigned char *a, const unsigned char *b) {
    char c = 64;
    for (size_t i = 0; i < FOOTER_CODE_SIZE; ++i) {
        a[i] = (a[i] ^ (c ^ b[i]));
        c = a[i];
    }
}

int32_t FBX_Document::getTimestampVar(FBX_Node const & timestamp, const char *element) {
    const FBX_Node *elementNode = timestamp.find(element);

    if (elementNode != nullptr && elementNode->properties.size() > 0) {
        Property const & prop = elementNode->properties[0];
        if (prop.type == Property::TYPE::INT32 || prop.type == Property::TYPE::INT64) {
            int32_t ret;
            std::memcpy(&ret, prop.data.data(), sizeof(int32_t));
            return ret;
        }
    }
    
    assert(false && "Timestamp does not contain element!");
    return 0;
}

bool FBX_Document::allZero(const unsigned char  *array, size_t sz) {
    for (size_t i = 0; i < sz; i++) {
        if (array[i] != 0) return false;
    }
    return true;
}

bool FBX_Document::checkEqual(const unsigned char *a, const unsigned char *b, size_t sz) {
    for (size_t i = 0; i < sz; i++) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

bool FBX_Document::findBinarySeparator(char *input, size_t length) {
    for (size_t i = 0; i < length; i++) {
        if (input[i] == '\0') {
            assert((input[i+1] == reinterpret_cast<const char&>(BINARY_SEPARATOR[1])) && "Invalid binary separator");
            return true;
        }
    }
    return false;
}

prt::vector<char*> FBX_Document::splitWithBinarySeparator(char *input, size_t length) {
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

bool FBX_Document::decompress(prt::vector<unsigned char> const & compressed, 
                              prt::vector<unsigned char> & uncompressed, size_t uncompressedSize) {
    // Thanks: https://gist.github.com/arq5x/5315739
    // zlib struct
    z_stream infstream;
    infstream.zalloc = Z_NULL;
    infstream.zfree = Z_NULL;
    infstream.opaque = Z_NULL;
    // setup "compress" as the input and "uncompressed" as the compressed output
    infstream.avail_in = uInt(compressed.size()); // size of input
    infstream.next_in = reinterpret_cast<Bytef*>(compressed.data()); // input char array

    uncompressed.resize(uncompressedSize);
    infstream.avail_out = uInt(uncompressed.size()); // size of output
    infstream.next_out = reinterpret_cast<Bytef*>(uncompressed.data()); // output char array
     
    // the actual DE-compression work.
    inflateInit(&infstream);
    inflate(&infstream, Z_NO_FLUSH);
    auto res = inflateEnd(&infstream);
    assert((res == Z_OK) && "Could not decompress!");
    return res == Z_OK;
}

void FBX_Document::FBX_Node::print() const {
    printRecursive(0);
}

void FBX_Document::FBX_Node::printRecursive(size_t indent) const {
    for (size_t i = 0; i < indent; ++i) {
        std::cout << "    ";
    }
    std::cout << _name << " {";
    for (auto const & property : properties) {
        property.print();
    }
    std::cout << " }" << std::endl;
    for (auto const & child : children) {
        child.printRecursive(indent + 1);
    }
}

void FBX_Document::Property::print() const {
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
            std::cout << " (Char: \'"
                      << *reinterpret_cast<const char*>(&data[0])
                      << "\')";
            break;
        case TYPE::INT16:
            std::cout << " (int16: "
                      << *reinterpret_cast<const int16_t*>(data.data())
                      << ")";
            break;
        case TYPE::INT32:
            std::cout << " (int32: "
                      << *reinterpret_cast<const int32_t*>(data.data())
                      << ")";
            break;
        case TYPE::INT64:
            std::cout << " (int64: "
                      << *reinterpret_cast<const int64_t*>(data.data())
                      << ")";
            break;
        case TYPE::FLOAT:
            std::cout << " (float: "
                      << *reinterpret_cast<const float*>(data.data())
                      << ")";
            break;
        case TYPE::DOUBLE:
            std::cout << " (double: "
                      << *reinterpret_cast<const double*>(data.data())
                      << ")";
            break;
        case TYPE::STRING:
            std::cout << " (string: \""
                      << reinterpret_cast<const char*>(data.data())
                      << "\")";
            break;
        case TYPE::ARRAY_FLOAT:
            std::cout << " (float["
                      << data.size() / sizeof(float)
                      << "])";
            break;
        case TYPE::ARRAY_DOUBLE:
            std::cout << " (double["
                      << data.size() / sizeof(double)
                      << "])";
            break;
        case TYPE::ARRAY_INT32:
            std::cout << " (int32["
                    << data.size() / sizeof(int32_t)
                    << "])";
            break;
        case TYPE::ARRAY_INT64:
            std::cout << " (int64["
                      << data.size() / sizeof(int64_t)
                      << "])";
            break;
        case TYPE::ARRAY_BOOL:
            std::cout << " (bool["
                      << data.size()
                      << "])";
            break;
        case TYPE::RAW:
            std::cout << " (byte["
                      << data.size()
                      << "])";
            break;
        default:
            std::cout << "(INVALID TYPE)";
            break;

    }
}
