CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -static
SRC = src/main.cpp src/huffman.cpp src/huffman_tree.cpp src/bit_stream.cpp src/thread_pool.cpp
HEADERS = src/huffman.hpp src/huffman_tree.hpp src/bit_stream.hpp src/thread_pool.hpp
BIN = huffman.exe

.PHONY: all clean test

all: $(BIN)

$(BIN): $(SRC) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(BIN)

clean:
	rm -f $(BIN) *.huf *.txt_out
