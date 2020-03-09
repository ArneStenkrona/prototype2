#ifndef FBX_DOCUMENT_H
#define FBX_DOCUMENT_H

/**
 *   Huge thanks to: https://github.com/hamish-milne/FbxWriter/
 *   for helping me with my implementation of FBX
 */

#include "src/container/array.h"
#include "src/container/vector.h"

#include <string>

class FBX_Document {
public:
    /**
     * Reads a binary FBX file from path
     * @param: file file path
     */
    FBX_Document(const char* file); 

    struct Property {
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
            INVALID_TYPE,
            TOTAL_NUM_TYPES = 12
        };
        static constexpr size_t PRIMITIVE_SIZES[TOTAL_NUM_TYPES] = {1,2,4,8,4,8,1,4,8,4,8,1};

        TYPE type = INVALID_TYPE;
        prt::vector<char> data;
    };

    class FBX_Node {
    public:
        prt::vector<FBX_Document::Property> properties;
        prt::vector<FBX_Node> nodes;

        inline const char* getName() const { return _name; }
        void setName(const char* name) {
            strcpy(_name, name);
        }
        const FBX_Node * find(const char* name) const;
        const FBX_Node * getRelative(const char* path) const;
    private:
        char _name[256];
    };

private:
    // Length of the header string
    static constexpr size_t HEADER_STRING_LENGTH = 24;

    // Header string, found at the top of all compliant files
    static constexpr char HEADER_STRING[HEADER_STRING_LENGTH] = "Kaydara FBX Binary  \0\x1a\0";
    // Size of the footer code
    static constexpr size_t FOOTER_CODE_SIZE = 16;
    static constexpr char BINARY_SEPARATOR[] = "\0\x1";
    static constexpr char ASCII_SEPARATOR[] = "::";

    // Data used in encryption
    static constexpr unsigned char SOURCE_ID[] =
			{ 0x58, 0xAB, 0xA9, 0xF0, 0x6C, 0xA2, 0xD8, 0x3F, 0x4D, 0x47, 0x49, 0xA3, 0xB4, 0xB2, 0xE7, 0x3D };
	static constexpr unsigned char KEY[] =
			{ 0xE2, 0x4F, 0x7B, 0x5F, 0xCD, 0xE4, 0xC8, 0x6D, 0xDB, 0xD8, 0xFB, 0xD7, 0x40, 0x58, 0xC6, 0x78 };
	static constexpr unsigned char EXTENSION[] =
			{ 0xF8, 0x5A, 0x8C, 0x6A, 0xDE, 0xF5, 0xD9, 0x7E, 0xEC, 0xE9, 0x0C, 0xE3, 0x75, 0x8F, 0x29, 0x0B };
    // Number of null bytes between the footer code and the version
    static constexpr size_t footerZeroes1 = 20;
    // Number of null bytes between the footer version and extension code
    static constexpr size_t footerZeroes2 = 120;

    // Version numbers
    enum class VERSION : int32_t {
        V6_0 = 6000,
        V6_1 = 6100,
        V7_0 = 7000,
        V7_1 = 7100,
        V7_2 = 7200,
        V7_3 = 7300,
        V7_4 = 7400,
        V7_5 = 7500
    };

    VERSION version;
    // Root node
    FBX_Node _root;

    static bool readHeader(std::ifstream & input);
    static bool readNode(std::ifstream & input, prt::vector<FBX_Node>& nodes, VERSION version);

    static Property readArray(std::ifstream & input, Property::TYPE type);
    static Property readProperty(std::ifstream & input);

    static bool checkFooter(std::ifstream & input, VERSION version);
    
    void generateFooterCode(char *buffer);
    static void generateFooterCode(char *buffer,
	                               int year, int month, int day,
	                               int hour, int minute, int second, int millisecond);

    static void encrypt(char *a, const char *b);

    static int32_t getTimestampVar(FBX_Node const & timestamp, const char *element);

    static bool allZero(const char *array, size_t sz);
    static bool checkEqual(const char *a, const char *b, size_t sz);
};

#endif