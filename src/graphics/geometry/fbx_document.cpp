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
    input.read(reinterpret_cast<char*>(&version), sizeof(int32_t));
    // Read nodes
    // auto dataPos = input.tellg();
    while(readNode(input, _root.nodes, version));

    // Read footer code
    char footerCode[FOOTER_CODE_SIZE];
    input.read(footerCode, FOOTER_CODE_SIZE);
    char validCode[FOOTER_CODE_SIZE];
    generateFooterCode(validCode);
    if (strcmp(footerCode, validCode) != 0) {
        assert(false && "Incorrect footer code!");
    }

    // Read footer extension
    bool validFooterExtension = checkFooter(input, version);

    if (!validFooterExtension) {
        assert(false && "Invalid footer!");
    }
}

const FBX_Document::FBX_Node * FBX_Document::FBX_Node::find(const char* name) const {
    for (auto const & n : nodes) {
        if (strcmp(n.getName(), name) == 0) {
           return &n;
        }
    }
    return nullptr;
}

const FBX_Document::FBX_Node * FBX_Document::FBX_Node::getRelative(const char* path) const {
    char buffer[256];
    size_t ind = 0;
    while (path[ind] != '\0') {
        buffer[ind] = path[ind];
    }
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
        //input.read(reinterpret_cast<char *>(&intbuf), sizeof(int64_t));
        endOffset = intbuf;
        readScalar<int64_t>(input, intbuf);
        //input.read(reinterpret_cast<char *>(&intbuf), sizeof(int64_t));
        numProperties = intbuf;
        readScalar<int64_t>(input, intbuf);
        //input.read(reinterpret_cast<char *>(&intbuf), sizeof(int64_t));
        propertyListLen = intbuf;
    } else {
        int32_t intbuf;
        readScalar<int32_t>(input, intbuf);
        //input.read(reinterpret_cast<char *>(&intbuf), sizeof(int32_t));
        endOffset = intbuf;
        readScalar<int32_t>(input, intbuf);
        //input.read(reinterpret_cast<char *>(&intbuf), sizeof(int32_t));
        numProperties = intbuf;
        readScalar<int32_t>(input, intbuf);
        //input.read(reinterpret_cast<char *>(&intbuf), sizeof(int32_t));
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
    std::cout << numProperties << " | " << name << std::endl;
    // Read properties
    for (int64_t i = 0; i < numProperties; i++) {
        node.properties.push_back(readProperty(input));
    }

    if (input.tellg() != propertyEnd) {
        assert(false && "Too many bytes in property list!");
    }

    // Read nested nodes
    auto listLen = endOffset - input.tellg();
    if (endOffset < input.tellg()) {
        assert(false && "Node has invalid endpoint!");
    }

    if (listLen > 0) {
        while(readNode(input, node.nodes, version));
        if (input.tellg() != endOffset) {
            assert(false && "Too many bytes in node!");
        }
    }
    
    return true;
}

FBX_Document::Property FBX_Document::readArray(std::ifstream & input, Property::TYPE type) {
    int32_t len;
    readScalar<int32_t>(input, len);
    //input.read(reinterpret_cast<char*>(&len), sizeof(int32_t));
    int32_t encoding;
    readScalar<int32_t>(input, encoding);
    //input.read(reinterpret_cast<char*>(&encoding), sizeof(int32_t));
    int32_t compressedLen;
    readScalar<int32_t>(input, compressedLen);
    //input.read(reinterpret_cast<char*>(&compressedLen), sizeof(int32_t));
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
        prt::vector<char> compressedBuffer;
        compressedBuffer.resize(compressedLen);
        input.read(compressedBuffer.data(), compressedLen);

        // decompress
        prt::vector<char> uncompressedBuffer(Property::PRIMITIVE_ALIGNMENTS[type]);
        uLongf uncompressedLen; 
        auto res = Z_ERRNO;
        size_t bufferFactor = 2;
        while (res != Z_OK) {
            uncompressedBuffer.resize(bufferFactor*compressedLen);

            res = uncompress(reinterpret_cast<Bytef*>(uncompressedBuffer.data()), &uncompressedLen, 
                             reinterpret_cast<Bytef*>(compressedBuffer.data()), compressedLen);
            bufferFactor *= 2;
            assert(bufferFactor < 16 && "Is buffer really supposed to be this big?");
        }
        uncompressedBuffer.resize(uncompressedLen);
        
        ret.data = uncompressedBuffer;

    } else {
        ret.data.resize(len*primSz);
        input.read(ret.data.data(), len*primSz);
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
            property.data = prt::vector<char>(prt::getAlignment(alignof(int16_t)));
            property.data.resize(sizeof(int16_t));
            readScalar<int16_t>(input, property.data.data());
            //input.read(property.data.data(), sizeof(int16_t));
            return property;
        }
        case 'C': {
            property.type = Property::CHAR;
            property.data.resize(sizeof(char));
            input.read(property.data.data(), sizeof(char));
            return property;
        }
        case 'I': {
            property.type = Property::INT32;
            property.data = prt::vector<char>(prt::getAlignment(alignof(int32_t)));
            property.data.resize(sizeof(int32_t));
            readScalar<int32_t>(input, property.data.data());
            //input.read(property.data.data(), sizeof(int32_t));
            return property;
        }
        case 'F': {
            property.type = Property::FLOAT;
            property.data = prt::vector<char>(prt::getAlignment(alignof(float)));
            property.data.resize(sizeof(float));
            readScalar<float>(input, property.data.data());
            //input.read(property.data.data(), sizeof(float));
            return property;
        }
        case 'D': {
            property.type = Property::DOUBLE;
            property.data = prt::vector<char>(prt::getAlignment(alignof(double)));
            property.data.resize(sizeof(double));
            readScalar<double>(input, property.data.data());
            //input.read(property.data.data(), sizeof(double));
            return property;
        }
        case 'L': {
            property.type = Property::INT64;
            property.data = prt::vector<char>(prt::getAlignment(alignof(int64_t)));
            property.data.resize(sizeof(int64_t));
            readScalar<int64_t>(input, property.data.data());
            //input.read(property.data.data(), sizeof(int64_t));
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
            int32_t len;
            readScalar<int32_t>(input, len);
            //input.read(reinterpret_cast<char*>(&len), sizeof(int32_t));
            prt::vector<char> data;
            data.resize(len);
            if (len > 0) {
                input.read(data.data(), len);
            }
            // Convert \0\1 to '::' and reverse the tokens
            if (strstr(data.data(), BINARY_SEPARATOR) != nullptr) {
                prt::vector<char*> tokens = string_util::split(data.data(), BINARY_SEPARATOR);

                bool first = true;
                for (size_t i = tokens.size(); i != 0; i--) {
                    if (!first) {
                        string_util::append(property.data, ASCII_SEPARATOR);
                    }
                    string_util::append(property.data, tokens[i]);
                    first = false;
                }
            }
            return property;
        }
        case 'R': {
            property.type = Property::INT32;
            property.data = prt::vector<char>(prt::getAlignment(alignof(int32_t)));
            property.data.resize(sizeof(int32_t));
            readScalar<int32_t>(input, property.data.data());
            //input.read(property.data.data(), sizeof(int32_t));
            return property;
        }
        default: {
            assert(false && "Invalid property data type!");
            return property;
        }
    }
}

bool FBX_Document::checkFooter(std::ifstream & input, VERSION version) {
    char buffer[footerZeroes2];
    input.read(buffer, footerZeroes1);
    bool correct = allZero(buffer, footerZeroes1);

    int32_t readVersion;
    readScalar<int32_t>(input, readVersion);
    //input.read(reinterpret_cast<char*>(&readVersion), sizeof(int32_t));
    correct &= (readVersion == int32_t(version));

    input.read(buffer, footerZeroes2);
    correct &= allZero(buffer, footerZeroes2);

    input.read(buffer, FOOTER_CODE_SIZE);
    correct &= checkEqual(buffer, reinterpret_cast<const char*>(EXTENSION), FOOTER_CODE_SIZE);

    return correct;
}

void FBX_Document::generateFooterCode(char *buffer,
	                                  int year, int month, int day,
	                                  int hour, int minute, int second, int millisecond) {
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
    char mangledBytes[FOOTER_CODE_SIZE];
    for (size_t i = 0; i < FOOTER_CODE_SIZE; i++) {
        buffer[i] = SOURCE_ID[i];
        mangledBytes[i] = '0';
    }
    sprintf(&mangledBytes[0],"%02d", second);
    sprintf(&mangledBytes[2],"%02d", month);
    sprintf(&mangledBytes[4],"%02d", hour);
    sprintf(&mangledBytes[6],"%02d", day);
    sprintf(&mangledBytes[8],"%02d", millisecond/10);
    sprintf(&mangledBytes[10],"%04d", year);
    sprintf(&mangledBytes[14],"%02d", minute);
    encrypt(buffer, mangledBytes);
    encrypt(buffer, reinterpret_cast<const char *>(KEY));
    encrypt(buffer, mangledBytes);
                                          
}

void FBX_Document::generateFooterCode(char *buffer) {
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

void FBX_Document::encrypt(char *a, const char *b) {
    char c = 64;
    for (size_t i = 0; i < FOOTER_CODE_SIZE; i++)
    {
        a[i] = (char)(a[i] ^ (char)(c ^ b[i]));
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

bool FBX_Document::allZero(const char  *array, size_t sz) {
    for (size_t i = 0; i < sz; i++) {
        if (array[i] != 0) return false;
    }
    return true;
}

bool FBX_Document::checkEqual(const char *a, const char *b, size_t sz) {
    for (size_t i = 0; i < sz; i++) {
        if (a[i] != b[i]) return false;
    }
    return true;
}