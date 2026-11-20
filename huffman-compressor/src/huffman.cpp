#include "huffman.hpp"
#include <fstream>
#include <queue>
#include <iostream>
#include <cstring>
#include <algorithm>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>

using namespace std;

namespace {
struct NodeCompare {
    bool operator()(const HuffmanNode* a, const HuffmanNode* b) const {
        return a->freq > b->freq; // min-heap
    }
};

// =============================================================================
// ThreadPool using standard C++17 std::thread, std::mutex, std::condition_variable
// =============================================================================
class ThreadPool {
public:
    explicit ThreadPool(size_t numThreads) : stop(false), completedTasks(0) {
        for (size_t i = 0; i < numThreads; ++i) {
            workers.emplace_back([this] {
                workerLoop();
            });
        }
    }

    ~ThreadPool() {
        {
            unique_lock<mutex> lock(mtx);
            stop = true;
        }
        cv.notify_all();

        for (thread& w : workers) {
            if (w.joinable()) {
                w.join();
            }
        }
    }

    void submit(void (*func)(void*), void* arg) {
        {
            unique_lock<mutex> lock(mtx);
            tasks.push({func, arg});
        }
        cv.notify_one();
    }

    void waitAll(size_t totalTasks) {
        unique_lock<mutex> lock(mtx);
        cvWait.wait(lock, [this, totalTasks] {
            return completedTasks >= totalTasks;
        });
    }

    void notifyTaskDone() {
        {
            unique_lock<mutex> lock(mtx);
            completedTasks++;
        }
        cvWait.notify_one();
    }

private:
    struct Task {
        void (*func)(void*);
        void* arg;
    };

    void workerLoop() {
        while (true) {
            Task task = {nullptr, nullptr};
            {
                unique_lock<mutex> lock(mtx);
                cv.wait(lock, [this] {
                    return stop || !tasks.empty();
                });

                if (stop && tasks.empty()) {
                    return;
                }

                task = tasks.front();
                tasks.pop();
            }

            if (task.func) {
                task.func(task.arg);
            }
            notifyTaskDone();
        }
    }

    vector<thread> workers;
    queue<Task> tasks;
    mutex mtx;
    condition_variable cv;
    condition_variable cvWait;
    bool stop;
    size_t completedTasks;
};

unsigned int getHardwareConcurrency() {
    unsigned int count = thread::hardware_concurrency();
    return count > 0 ? count : 4;
}

static const size_t TARGET_CHUNK_SIZE = 512;

size_t calculateNumChunks(size_t totalSize, unsigned int numThreads) {
    if (totalSize == 0) return 0;
    size_t chunksBySize = (totalSize + TARGET_CHUNK_SIZE - 1) / TARGET_CHUNK_SIZE;
    size_t chunksByThreads = numThreads * 4;
    size_t numChunks = max(chunksBySize, chunksByThreads);
    if (numChunks > totalSize) numChunks = totalSize;
    if (numChunks > 65535) numChunks = 65535;
    if (numChunks == 0) numChunks = 1;
    return numChunks;
}

} // anonymous namespace

// =============================================================================
// BitReader / BitWriter
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
    for (bool b : bits) writeBit(b);
}

void BitWriter::flush() {
    if (bitCount > 0) {
        current <<= (8 - bitCount);
        buffer.push_back(current);
        current = 0;
        bitCount = 0;
    }
}

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

// =============================================================================
// Parallel Frequency Counting using Thread Pool & Work Queue
// =============================================================================

struct FreqTaskContext {
    const vector<uint8_t>* data;
    size_t start;
    size_t end;
    vector<uint64_t> localFreq;
};

static void runFreqTask(void* param) {
    FreqTaskContext* ctx = static_cast<FreqTaskContext*>(param);
    ctx->localFreq.assign(256, 0);
    for (size_t j = ctx->start; j < ctx->end; ++j) {
        ctx->localFreq[(*ctx->data)[j]]++;
    }
}

map<uint8_t, uint64_t> Huffman::buildFrequencyTable(const vector<uint8_t>& data) {
    if (data.empty()) return {};

    unsigned int threadnum = getHardwareConcurrency();
    size_t totalSize = data.size();
    size_t numChunks = calculateNumChunks(totalSize, threadnum);
    size_t chunkSize = totalSize / numChunks;

    ThreadPool pool(threadnum);
    vector<FreqTaskContext> contexts(numChunks);

    for (size_t i = 0; i < numChunks; ++i) {
        contexts[i].data = &data;
        contexts[i].start = i * chunkSize;
        contexts[i].end = (i == numChunks - 1) ? totalSize : contexts[i].start + chunkSize;
        pool.submit(runFreqTask, &contexts[i]);
    }

    pool.waitAll(numChunks);

    map<uint8_t, uint64_t> globalFreq;
    for (size_t i = 0; i < numChunks; ++i) {
        for (int byteVal = 0; byteVal < 256; ++byteVal) {
            if (contexts[i].localFreq[byteVal] > 0) {
                globalFreq[static_cast<uint8_t>(byteVal)] += contexts[i].localFreq[byteVal];
            }
        }
    }
    return globalFreq;
}

unique_ptr<HuffmanNode> Huffman::buildTree(const map<uint8_t, uint64_t>& freqTable) {
    priority_queue<HuffmanNode*, vector<HuffmanNode*>, NodeCompare> pq; 
    for (auto& entry : freqTable) {
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
        parent->freq = (a->freq) + (b->freq);
        parent->left.reset(a);
        parent->right.reset(b);
        pq.push(parent);
    }

    unique_ptr<HuffmanNode> root = nullptr;
    if (!pq.empty()) {
        root.reset(pq.top());
    }
    return root;
}

void Huffman::buildCodeTable(const HuffmanNode* node, HuffmanCode& prefix,
                             unordered_map<uint8_t, HuffmanCode>& table) {
    if (!node) return;
    if (node->isLeaf) {
        table[node->byte] = prefix;
        return;
    }
    prefix.push_back(false);
    buildCodeTable(node->left.get(), prefix, table);
    prefix.pop_back();

    prefix.push_back(true);
    buildCodeTable(node->right.get(), prefix, table);
    prefix.pop_back();
}

// =============================================================================
// File Format Helpers
// =============================================================================

namespace {
void writeU64(ofstream& out, uint64_t v) {
    for (int i = 0; i < 8; i++) out.put(static_cast<char>((v >> (8 * i)) & 0xFF));
}
uint64_t readU64(ifstream& in) {
    uint64_t v = 0;
    for (int i = 0; i < 8; i++) {
        uint8_t b = static_cast<uint8_t>(in.get());
        v |= (static_cast<uint64_t>(b) << (8 * i));
    }
    return v;
}
void writeU16(ofstream& out, uint16_t v) {
    out.put(static_cast<char>(v & 0xFF));
    out.put(static_cast<char>((v >> 8) & 0xFF));
} 
uint16_t readU16(ifstream& in) {
    uint8_t lo = static_cast<uint8_t>(in.get());
    uint8_t hi = static_cast<uint8_t>(in.get());
    return static_cast<uint16_t>(lo | (hi << 8));
}
}

// =============================================================================
// Parallel Encoding using Thread Pool & Work Queue
// =============================================================================

struct EncodeTaskContext {
    const vector<uint8_t>* data;
    size_t start;
    size_t end;
    const unordered_map<uint8_t, HuffmanCode>* codeTable;
    vector<uint8_t> compressedChunk;
};

static void runEncodeTask(void* param) {
    EncodeTaskContext* ctx = static_cast<EncodeTaskContext*>(param);
    BitWriter writer(ctx->compressedChunk);
    for (size_t i = ctx->start; i < ctx->end; ++i) {
        writer.writeBits(ctx->codeTable->at((*ctx->data)[i]));
    }
    writer.flush();
}

bool Huffman::compressFile(const string& inputPath, const string& outputPath) {
    ifstream in(inputPath, ios::binary);
    if (!in) {
        cerr << "Error: file input error " << inputPath << endl;
        return false;
    }
    vector<uint8_t> data((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
    in.close();

    ofstream out(outputPath, ios::binary);
    if (!out) {
        cerr << "Error: output file error " << outputPath << endl;
        return false;
    }

    out.write("HUF1", 4);
    writeU64(out, data.size());

    if (data.empty()) {
        writeU16(out, 0);
        return true;
    }

    auto freqTable = buildFrequencyTable(data);
    writeU16(out, static_cast<uint16_t>(freqTable.size()));
    for (const auto& entry : freqTable) {
        out.put(static_cast<char>(entry.first));
        writeU64(out, entry.second);
    }

    if (freqTable.size() == 1) {
        return true;
    }

    auto root = buildTree(freqTable);
    unordered_map<uint8_t, HuffmanCode> codeTable;
    HuffmanCode prefix;
    buildCodeTable(root.get(), prefix, codeTable);

    unsigned int numThreads = getHardwareConcurrency();
    size_t totalSize = data.size();
    size_t numChunks = calculateNumChunks(totalSize, numThreads);
    size_t chunkSize = totalSize / numChunks;

    ThreadPool pool(numThreads);
    vector<EncodeTaskContext> contexts(numChunks);

    for (size_t i = 0; i < numChunks; ++i) {
        contexts[i].data = &data;
        contexts[i].start = i * chunkSize;
        contexts[i].end = (i == numChunks - 1) ? totalSize : contexts[i].start + chunkSize;
        contexts[i].codeTable = &codeTable;
        pool.submit(runEncodeTask, &contexts[i]);
    }

    pool.waitAll(numChunks);

    // Write Chunk Header Information
    writeU16(out, static_cast<uint16_t>(numChunks));

    for (size_t i = 0; i < numChunks; ++i) {
        writeU64(out, contexts[i].compressedChunk.size());
    }

    for (size_t i = 0; i < numChunks; ++i) {
        out.write(reinterpret_cast<const char*>(contexts[i].compressedChunk.data()),
                  static_cast<streamsize>(contexts[i].compressedChunk.size()));
    }
    
    return true;
}

// =============================================================================
// Parallel Decoding using Thread Pool & Work Queue
// =============================================================================

struct DecodeTaskContext {
    const vector<uint8_t>* compressedChunk;
    const HuffmanNode* root;
    uint64_t targetByteCount;
    vector<uint8_t> decodedChunk;
};

static void runDecodeTask(void* param) {
    DecodeTaskContext* ctx = static_cast<DecodeTaskContext*>(param);
    BitReader reader(*ctx->compressedChunk);
    ctx->decodedChunk.reserve(ctx->targetByteCount);

    const HuffmanNode* node = ctx->root;
    bool bit;

    while (ctx->decodedChunk.size() < ctx->targetByteCount && reader.readBit(bit)) {
        node = bit ? node->right.get() : node->left.get();
        if (node->isLeaf) {
            ctx->decodedChunk.push_back(node->byte);
            node = ctx->root;
        }
    }
}

bool Huffman::decompressFile(const string& inputPath, const string& outputPath) {
    ifstream in(inputPath, ios::binary);
    if (!in) {
        cerr << "Error: file input error decompress " << inputPath << endl;
        return false;
    }
    char magic[4];
    in.read(magic, 4);
    if (in.gcount() != 4 || memcmp(magic, "HUF1", 4) != 0) {
        cerr << "Error: not a valid .huf file (bad magic header)" << endl;
        return false;
    }

    uint64_t originalSize = readU64(in);
    uint16_t compsiz = readU16(in);

    ofstream out(outputPath, ios::binary);
    if (!out) {
        cerr << "Error: could not open output file " << outputPath << endl;
        return false;
    }

    if (originalSize == 0 || compsiz == 0) {
        return true;
    }

    map<uint8_t, uint64_t> freqTable;
    for (uint16_t i = 0; i < compsiz; i++) {
        uint8_t byte = static_cast<uint8_t>(in.get());
        uint64_t freq = readU64(in);
        freqTable[byte] = freq;
    }

    if (compsiz == 1) {
        uint8_t onlyByte = freqTable.begin()->first;
        vector<char> chunk(min<uint64_t>(originalSize, 1 << 20), static_cast<char>(onlyByte));
        uint64_t remaining = originalSize;
        while (remaining > 0) {
            uint64_t n = min<uint64_t>(remaining, chunk.size());
            out.write(chunk.data(), static_cast<streamsize>(n));
            remaining -= n;
        }
        return true;
    }

    auto root = buildTree(freqTable);

    uint16_t numChunks = readU16(in);
    vector<uint64_t> chunkSizes(numChunks);
    for (uint16_t i = 0; i < numChunks; ++i) {
        chunkSizes[i] = readU64(in);
    }

    vector<vector<uint8_t>> compressedChunks(numChunks);
    for (uint16_t i = 0; i < numChunks; ++i) {
        compressedChunks[i].resize(chunkSizes[i]);
        in.read(reinterpret_cast<char*>(compressedChunks[i].data()), static_cast<streamsize>(chunkSizes[i]));
    }

    uint64_t baseChunkSize = originalSize / numChunks;
    vector<uint64_t> targetByteCounts(numChunks);
    for (uint16_t i = 0; i < numChunks; ++i) {
        targetByteCounts[i] = (i == numChunks - 1) ? (originalSize - i * baseChunkSize) : baseChunkSize;
    }

    unsigned int numThreads = getHardwareConcurrency();
    ThreadPool pool(numThreads);
    vector<DecodeTaskContext> contexts(numChunks);

    for (uint16_t i = 0; i < numChunks; ++i) {
        contexts[i].compressedChunk = &compressedChunks[i];
        contexts[i].root = root.get();
        contexts[i].targetByteCount = targetByteCounts[i];
        pool.submit(runDecodeTask, &contexts[i]);
    }

    pool.waitAll(numChunks);

    uint64_t totalDecodedBytes = 0;
    for (uint16_t i = 0; i < numChunks; ++i) {
        totalDecodedBytes += contexts[i].decodedChunk.size();
        out.write(reinterpret_cast<const char*>(contexts[i].decodedChunk.data()),
                  static_cast<streamsize>(contexts[i].decodedChunk.size()));
    }

    if (totalDecodedBytes != originalSize) {
        cerr << "Warning: decoded size (" << totalDecodedBytes
             << ") does not match expected size (" << originalSize << "). File may be corrupt." << endl;
    }

    return true;
}
