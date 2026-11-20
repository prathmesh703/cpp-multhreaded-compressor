#include "huffman.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <chrono> 
namespace {

void printUsage(const char* prog) {
    std::cerr << "Huffman file compressor\n\n"
              << "Usage:\n"
              << "  " << prog << " compress   <input> <output.huf>\n"
              << "  " << prog << " decompress <input.huf> <output>\n\n"
              << "Example:\n"
              << "  " << prog << " compress notes.txt notes.huf\n"
              << "  " << prog << " decompress notes.huf notes_restored.txt\n";
}

long fileSize(const std::string& path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) return -1;
    return static_cast<long>(f.tellg());
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 4) {
        printUsage(argv[0]);
        return 1;
    }

    std::string mode = argv[1];
    std::string inputPath = argv[2];
    std::string outputPath = argv[3];

    if (mode == "compress") {
        auto start = std::chrono::high_resolution_clock::now();

        bool success = Huffman::compressFile(inputPath, outputPath);

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> durationMs = end - start;

        if (!success) return 1;

        long before = fileSize(inputPath);
        long after = fileSize(outputPath);
        std::cout << "Compressed: " << inputPath << " -> " << outputPath << "\n";
        std::cout << "  Execution time:  " << durationMs.count() << " ms\n";
        if (before >= 0 && after >= 0) {
            std::cout << "  Original size:   " << before << " bytes\n";
            std::cout << "  Compressed size: " << after << " bytes\n";
            if (before > 0) {
                double ratio = 100.0 * (1.0 - static_cast<double>(after) / static_cast<double>(before));
                std::cout << "  Space saved:     " << ratio << "%\n";
            }
        }
    } else if (mode == "decompress") {
        auto start = std::chrono::high_resolution_clock::now();

        bool success = Huffman::decompressFile(inputPath, outputPath);

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> durationMs = end - start;

        if (!success) return 1;

        std::cout << "Decompressed: " << inputPath << " -> " << outputPath << "\n";
        std::cout << "  Execution time:  " << durationMs.count() << " ms\n";
    } else {
        printUsage(argv[0]);
        return 1;
    }

    return 0;
}