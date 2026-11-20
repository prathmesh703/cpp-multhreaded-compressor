#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <unordered_map>

using namespace std;
using HuffmanCode = vector<bool>;

struct HuffmanNode {
    uint8_t byte = 0;
    uint64_t freq = 0;
    bool isLeaf = false;
    unique_ptr<HuffmanNode> left;
    unique_ptr<HuffmanNode> right;
};

class Huffman {
public:
    static bool compressFile(const string& inputPath, const string& outputPath);
    static bool decompressFile(const string& inputPath, const string& outputPath);
private:
    static map<uint8_t, uint64_t> buildFrequencyTable(const vector<uint8_t>& data);
    static unique_ptr<HuffmanNode> buildTree(const map<uint8_t, uint64_t>& freqTable);
    static void buildCodeTable(const HuffmanNode* node, HuffmanCode& prefix,
                               unordered_map<uint8_t, HuffmanCode>& table);
};

class BitWriter {
public:
    explicit BitWriter(vector<uint8_t>& out) : buffer(out) {}
    void writeBit(bool bit);
    void writeBits(const HuffmanCode& bits);
    void flush(); // pads the final byte with zero bits

private:
    vector<uint8_t>& buffer;
    uint8_t current = 0;
    int bitCount = 0;
};

class BitReader {
public:
    BitReader(const vector<uint8_t>& in) : buffer(in) {}
    bool readBit(bool& bit);  

private:
    const vector<uint8_t>& buffer;
    size_t bytePos = 0;
    int bitPos = 0;
};
