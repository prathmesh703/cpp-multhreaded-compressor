#pragma once
#include <cstdint>
#include <vector>
#include <cstddef>

using HuffmanCode = std::vector<bool>;

// BitWriter packs individual bits into full 8-bit bytes
class BitWriter {
public:
    explicit BitWriter(std::vector<uint8_t>& out) : buffer(out) {}
    void writeBit(bool bit);
    void writeBits(const HuffmanCode& bits);
    void flush(); // pads the final byte with zero bits

private:
    std::vector<uint8_t>& buffer;
    uint8_t current = 0;
    int bitCount = 0;
};

// BitReader unpacks full bytes into individual bits
class BitReader {
public:
    explicit BitReader(const std::vector<uint8_t>& in) : buffer(in) {}
    bool readBit(bool& bit);

private:
    const std::vector<uint8_t>& buffer;
    std::size_t bytePos = 0;
    int bitPos = 0;
};
