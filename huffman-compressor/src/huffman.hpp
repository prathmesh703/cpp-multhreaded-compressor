#pragma once
#include <string>

class Huffman {
public:
    static bool compressFile(const std::string& inputPath, const std::string& outputPath);
    static bool decompressFile(const std::string& inputPath, const std::string& outputPath);
};
