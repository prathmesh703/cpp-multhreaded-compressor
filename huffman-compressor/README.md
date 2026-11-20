# Huffman File Compressor (C++)

A command-line file compression tool implementing classic **Huffman coding**
from scratch — no external compression libraries. Built as a learning /
portfolio project to demonstrate binary trees, greedy algorithms, and
bit-level file I/O in C++17.

## How it works

1. **Frequency analysis** — read the input file and count how often each
   byte value (0–255) occurs.
2. **Build a Huffman tree** — repeatedly merge the two least-frequent nodes
   using a min-heap (`std::priority_queue`) until one root remains. Frequent
   bytes end up near the root (short codes); rare bytes end up deep in the
   tree (long codes).
3. **Generate codes** — walk the tree; each left branch appends a `0`, each
   right branch appends a `1`. The path to each leaf is that byte's unique
   prefix-free binary code.
4. **Encode** — replace every byte in the file with its code and pack the
   resulting bits tightly into bytes.
5. **Decode** — rebuild the same tree from a small header stored in the
   compressed file, then walk the tree bit-by-bit to recover the original
   bytes.

## Build

```bash
make          # builds ./huffman
make test     # builds and runs a quick round-trip sanity check
```

Requires a C++17 compiler (tested with g++ 13). No external dependencies.

## Usage

```bash
./huffman compress   <input>     <output.huf>
./huffman decompress <input.huf> <output>
```

Example:

```bash
./huffman compress report.txt report.huf
./huffman decompress report.huf report_restored.txt
```

## File format (`.huf`)

```
4 bytes   magic "HUF1"
8 bytes   original file size (uint64, little-endian)
2 bytes   number of distinct byte values (uint16)
N * 9 b   for each distinct byte: 1 byte value + 8 bytes frequency
...bits   packed Huffman-encoded bitstream (final byte zero-padded)
```

Storing the frequency table lets the decompressor rebuild an identical tree
without needing any external dictionary.

## Notes & limitations

- **Best on skewed, repetitive data** (plain text, logs, source code).
  Typical text files compress by roughly 25–45%.
- **Small files can grow slightly**, since the header (a fixed 14 bytes plus
  9 bytes per distinct byte value) is fixed overhead — noticeable when the
  input is smaller than the header itself.
- **Already-compressed or random data won't shrink and may grow a little**
  (e.g. `.zip`, `.jpg`, or `/dev/urandom` output), because Huffman coding
  relies on uneven symbol frequencies. This is expected/correct behavior,
  not a bug.
- For real-world use, production tools (zlib/gzip, zstd) combine Huffman
  coding with a dictionary-based pass (like LZ77) to also exploit repeated
  *sequences*, not just skewed byte frequencies — a natural next step if you
  want to extend this project.

## Possible extensions

- Add an LZ77/LZSS pass before Huffman coding (this is essentially what
  DEFLATE/zlib does) for much better ratios on repetitive data.
- Support compressing entire directories (tar-then-compress).
- Adaptive/dynamic Huffman coding (tree updates as it streams, no header
  needed).
- Canonical Huffman codes to shrink the stored header further.
