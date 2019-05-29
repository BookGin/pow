/*
 * Pow example: sha1(prefix + input) == 000000...
 * Compile: g++ ./pow.cpp -s -Ofast -march=native -pthread -lcrypto -lssl --std=c++14 -Wextra -Wall -o pow
 * Run as server: ncat -klv 1337 -e pow
 */
#include <iostream>
#include <string>
#include <cstring>
#include <thread>
#include <mutex>
#include <cassert>
#include <openssl/sha.h>

#define THREAD_NUM (4) // Optimal: CPU cores
#define MAX_PREFIX_SIZE (6)
#define OUTPUT_BUFFER_SIZE (20) // SHA1: 160 bits, 20 bytes
#define STEP (0x100000000ULL) // 8 bytes unsigned integer

using std::mutex;
using std::thread;
using std::cin;
using std::cout;
using std::endl;
using std::string;

mutex cout_lock;

inline void printAnswerAndExit(unsigned char *input, const uint32_t input_len) {
  cout_lock.lock();
  for (uint32_t i = 0; i < input_len; i++)
    printf("%02x", input[i]);
  exit(0);
}

void pow(const char *prefix, const uint32_t prefix_len, const uint64_t start, const uint64_t end) {
    unsigned char input[MAX_PREFIX_SIZE + sizeof(uint64_t)];
    unsigned char output[OUTPUT_BUFFER_SIZE];
    const uint32_t input_len = prefix_len + sizeof(start);
    memcpy(input, prefix, prefix_len);
    memcpy(input + prefix_len, &start, sizeof(start));
    for (uint64_t *p = (uint64_t *)(input + prefix_len); *p < end; (*p)++) {
      SHA1(input, input_len, output);
      if (0 == memcmp(output, "\x00\x00", 2))
        printAnswerAndExit(input, input_len);
    }
}

int main() {
    string prefix;
    cin >> prefix;
    assert(prefix.size() <= MAX_PREFIX_SIZE);
    thread *threads[THREAD_NUM];
    uint64_t start = 0ULL, step = STEP;
    for (uint64_t i = 0ULL; i < THREAD_NUM; i++)
      threads[i] = new thread(
          pow,
          prefix.c_str(),
          prefix.size(),
          start + step * i,
          start + step * (i+1ULL)
      );

    // Acctually, it will never join all threads. The proces will print the answer and exit.
    for (int i = 0; i < THREAD_NUM; i++)
      threads[i]->join();
}

