CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -static
SRC = src/main.cpp src/huffman.cpp
BIN = huffman.exe

.PHONY: all clean test

all: $(BIN)

$(BIN): $(SRC) src/huffman.hpp
	$(CXX) $(CXXFLAGS) $(SRC) -o $(BIN)

clean:
	rm -f $(BIN) *.huf *.txt_out
