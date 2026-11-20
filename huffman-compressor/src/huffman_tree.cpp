#include "huffman_tree.hpp"
#include <queue>

namespace {
struct NodeCompare {
    bool operator()(const HuffmanNode* a, const HuffmanNode* b) const {
        return a->freq > b->freq; // min-heap: lowest frequency at top
    }
};
}

std::unique_ptr<HuffmanNode> buildHuffmanTree(const std::map<uint8_t, uint64_t>& freqTable) {
    std::priority_queue<HuffmanNode*, std::vector<HuffmanNode*>, NodeCompare> pq;

    for (const auto& entry : freqTable) {
        auto* leaf = new HuffmanNode();
        leaf->byte = entry.first;
        leaf->freq = entry.second;
        leaf->isLeaf = true;
        pq.push(leaf);
    }

    while (pq.size() > 1) {
        HuffmanNode* a = pq.top(); pq.pop();
        HuffmanNode* b = pq.top(); pq.pop();
        auto* parent = new HuffmanNode();
        parent->freq = a->freq + b->freq;
        parent->left.reset(a);
        parent->right.reset(b);
        pq.push(parent);
    }

    std::unique_ptr<HuffmanNode> root = nullptr;
    if (!pq.empty()) {
        root.reset(pq.top());
    }
    return root;
}

void buildHuffmanCodeTable(const HuffmanNode* node, HuffmanCode& prefix,
                           std::unordered_map<uint8_t, HuffmanCode>& table) {
    if (!node) return;
    if (node->isLeaf) {
        table[node->byte] = prefix;
        return;
    }
    prefix.push_back(false); // 0 = left
    buildHuffmanCodeTable(node->left.get(), prefix, table);
    prefix.pop_back();

    prefix.push_back(true);  // 1 = right
    buildHuffmanCodeTable(node->right.get(), prefix, table);
    prefix.pop_back();
}
