#ifndef FBX_DOCUMENT_H
#define FBX_DOCUMENT_H

/**
 *   Huge thanks to: https://github.com/hamish-milne/FbxWriter/
 *   for helping me with my implementation of FBX
 */

#include "src/container/array.h"
#include "src/container/vector.h"
#include "src/config/prototype2Config.h"

#include <fstream>

namespace FBX {

class Document {
public:
    /**
     * Reads a binary FBX file from path
     * @param file : file path
     */
    Document(const char* file); 
    Document & operator=(const Document&) = delete;
    Document(const Document&) = delete;

    class Node;
    /**
     * @return the root node of the FBX document
     */
    Node const & getRoot() const { return _root; }
    /**
     * @param path : path to a node within the FBX document,
     *        eg. "parent/child1/child2"
     *        note: the method will find the first
     *        child that matches a name
     * @return pointer to node or nullptr if none was found
     */
    Node const * getNode(const char* path) const { return _root.getRelative(path); }

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
            RAW = 12,
            TOTAL_NUM_TYPES = 13
        };
        static constexpr size_t PRIMITIVE_SIZES[TOTAL_NUM_TYPES] = {1,2,4,8,4,8,1,4,8,4,8,1,4};
        static constexpr size_t PRIMITIVE_ALIGNMENTS[TOTAL_NUM_TYPES] = {alignof(char),
                                                                         alignof(int16_t),
                                                                         alignof(int32_t),
                                                                         alignof(int64_t),
                                                                         alignof(float),
                                                                         alignof(double),
                                                                         alignof(char),
                                                                         alignof(float),
                                                                         alignof(double),
                                                                         alignof(int32_t),
                                                                         alignof(int64_t),
                                                                         alignof(char),
                                                                         alignof(char)};

        TYPE type = TOTAL_NUM_TYPES;
        unsigned const char *data() const {
            if (type < ARRAY_FLOAT) {
                return _data;
            } else {
                return _data + sizeof(size_t);
            }
        }
        unsigned char *data() {
            return const_cast<unsigned char*>(const_cast<Property const *>(this)->data());
        }
        size_t bytes() const { 
            if (type < ARRAY_FLOAT) {
                return PRIMITIVE_SIZES[type];
            } else {
                return *reinterpret_cast<size_t*>(_data) * PRIMITIVE_SIZES[type];
            }
        }

        size_t length() const {
            if (type < ARRAY_FLOAT) {
                return 1;
            } else {
                return *reinterpret_cast<size_t*>(_data);
            }
        }

        void print(std::ostream & out) const;
        private:
        unsigned char *_data;
        friend class FBX::Document;
    };

    class Node {
    public:
        /**
         * @return name
         */
        inline const char* getName() const { return _name; }
        /**
         * @param name : name of child
         * @return first child with matching name
         */
        Node const * find(const char* name) const;
        /**
         * @param path : relative path to a node within the FBX document
         *               eg. "child1/child2/child3"
         *               note: the method will find the first
         *               child that matches a name
         * @return pointer to node or nullptr if none was found
         */
        Node const * getRelative(const char* path) const;

        /**
         * @param i : index of property
         * @return property with index i
         */
        Document::Property const & getProperty(size_t i) const { return properties[i]; }
        /**
         * @return const reference to a vector of all properties
         */
        prt::vector<Document::Property> const & getProperties() const { return properties; }

        /**
         * @param i : index of child
         * @return child with index i 
         */
        Node const & getChild(size_t i) const { return children[i]; };
        /**
         * @return const reference to a vector of all children
         */
        prt::vector<Node> const & getChildren() const { return children; };

        /**
         * recursively prints the node and its contents
         * @param out : output stream
         */
        void print(std::ostream & out) const;
    private:
        prt::vector<Document::Property> properties;
        prt::vector<Node> children;

        char _name[256];

        void setName(const char* name) {
            strcpy(_name, name);
        }

        void printRecursive(std::ostream & out, size_t indent) const;

        friend class FBX::Document;
    };

private:
    // Length of the header string
    static constexpr size_t HEADER_STRING_LENGTH = 23;

    // Header string, found at the top of all compliant files
    static constexpr char HEADER_STRING[HEADER_STRING_LENGTH] = "Kaydara FBX Binary  \0\x1a";//\0";
    // Size of the footer code
    static constexpr size_t FOOTER_CODE_SIZE = 16;
    static constexpr unsigned char BINARY_SEPARATOR[] = {0x00, 0x01};
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
    enum class VERSION : uint32_t {
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
    Node _root;

    prt::vector<unsigned char> _dataStack;

    void initializeStack(std::ifstream & input, VERSION version);
    bool getNodeDataSize(std::ifstream & input, VERSION version, size_t & size);
    void getPropertyDataSize(std::ifstream & input, size_t & size);
    void getArrayDataSize(std::ifstream & input, Property::TYPE type, size_t & size);

    static bool readHeader(std::ifstream & input);
    static bool readNode(std::ifstream & input, prt::vector<Node>& nodes, 
                         VERSION version, unsigned char * & stackPointer);

    static Property readArray(std::ifstream & input, Property::TYPE type, unsigned char * & stackPointer);
    static Property readProperty(std::ifstream & input, unsigned char * & stackPointer);

    static bool checkFooter(std::ifstream & input, VERSION version);
    
    void generateFooterCode(unsigned char *buffer);
    static void generateFooterCode(unsigned char *buffer,
	                               int32_t year, int32_t month, int32_t day,
	                               int32_t hour, int32_t minute, int32_t second, int32_t millisecond);

    static void encrypt(unsigned char *a, unsigned const char *b);

    static int32_t getTimestampVar(Node const & timestamp, const char *element);

    static bool allZero(const unsigned char *array, size_t sz);
    static bool checkEqual(const unsigned char *a, const unsigned char *b, size_t sz);

    static bool findBinarySeparator(char *input, size_t length);
    static prt::vector<char*> splitWithBinarySeparator(char *input, size_t length);

    static bool decompress(unsigned char *src, size_t compressedSize,
                           unsigned char *dest, size_t uncompressedSize);

    template <class T>
    static void readScalar(std::ifstream & input, T & dest) {
        readScalar<T>(input, reinterpret_cast<unsigned char*>(&dest));
    }
    template <class T>
    static void readScalar(std::ifstream & input, unsigned char *dest) {
    #if PRT_BIG_ENDIAN == 1
        for (size_t i = sizeof(T) - 1; i >= 0; --i) {
            input.get(reinterpret_cast<char &>(dest[i]));
        }
    #else
        for (size_t i = 0; i < sizeof(T); ++i) {
            input.get(reinterpret_cast<char &>(dest[i]));
        }
    #endif
    }
};

};

#endif