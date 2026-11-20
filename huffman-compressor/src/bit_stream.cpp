#include "bit_stream.hpp"

// =============================================================================
// BitWriter
// =============================================================================

void BitWriter::writeBit(bool bit) {
    current = (current << 1) | (bit ? 1 : 0);
    bitCount++;
    if (bitCount == 8) {
        buffer.push_back(current);
        current = 0;
        bitCount = 0;
    }
}

void BitWriter::writeBits(const HuffmanCode& bits) {
    for (bool b : bits) {
        writeBit(b);
    }
}

void BitWriter::flush() {
    if (bitCount > 0) {
        current <<= (8 - bitCount);
        buffer.push_back(current);
        current = 0;
        bitCount = 0;
    }
}

// =============================================================================
// BitReader
// =============================================================================

bool BitReader::readBit(bool& bit) {
    if (bytePos >= buffer.size()) return false;
    uint8_t byte = buffer[bytePos];
    bit = (byte >> (7 - bitPos)) & 1;
    bitPos++;
    if (bitPos == 8) {
        bitPos = 0;
        bytePos++;
    }
    return true;
}
