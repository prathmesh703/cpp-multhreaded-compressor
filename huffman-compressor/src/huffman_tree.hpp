#pragma once
#include "bit_stream.hpp"
#include <cstdint>
#include <memory>
#include <map>
#include <unordered_map>
#include <vector>

struct HuffmanNode {
    uint8_t byte = 0;
    uint64_t freq = 0;
    bool isLeaf = false;
    std::unique_ptr<HuffmanNode> left;
    std::unique_ptr<HuffmanNode> right;
};

// Builds a Huffman binary tree from byte frequency counts
std::unique_ptr<HuffmanNode> buildHuffmanTree(const std::map<uint8_t, uint64_t>& freqTable);

// Recursively builds the prefix code lookup table via DFS traversal
void buildHuffmanCodeTable(const HuffmanNode* node, HuffmanCode& prefix,
                           std::unordered_map<uint8_t, HuffmanCode>& table);
