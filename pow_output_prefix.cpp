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

void pow(
  const unsigned char *input_prefix, const uint32_t input_prefix_len,
  unsigned char *output_prefix, const uint32_t output_prefix_len,
  const uint64_t start, const uint64_t end
) {
    unsigned char input[MAX_PREFIX_SIZE + sizeof(uint64_t)];
    const uint32_t input_len = input_prefix_len + sizeof(uint64_t);
    memcpy(input, input_prefix, input_prefix_len);
    memcpy(input + input_prefix_len, &start, sizeof(start));

    unsigned char output[OUTPUT_BUFFER_SIZE];
    for (uint64_t *p = (uint64_t *)(input + input_prefix_len); *p < end; (*p)++) {
      SHA1(input, input_len, output);
      if (0 == memcmp(output, output_prefix, output_prefix_len))
        printAnswerAndExit(input, input_len);
    }
}

uint32_t hexToString(const char *s, const uint32_t len, unsigned char *output_prefix) {
  assert(len % 2 == 0);
  uint32_t index = 0;
  for (uint32_t i = 0; i < len; i += 2, index++) {
    unsigned char n = 0;
    for (int j = 0; j < 2; j++) {
      int base = 0;
      if ('0' <= s[i+j] and s[i+j] <= '9')
        base = '0';
      else if ('a' <= s[i+j] and s[i+j] <= 'f')
        base = 'a' - 10;
      else
        assert(false);
      n += (j == 0 ? 0x10: 0x01) * (s[i+j] - base);
    }
    assert(index < OUTPUT_BUFFER_SIZE);
    output_prefix[index] = n;
  }
  return index;
}

int main() {
    cout << "hash(input_prefix + ...) = output_prefix_hex ..." << endl;
    cout << "e.g. hash(foo + ...) = 00aabb ..." << endl;

    string input_prefix;
    cout << "input_prefix (e.g. foobar) = ";
    cin >> input_prefix;
    assert(input_prefix.size() <= MAX_PREFIX_SIZE);

    string output_prefix_hex;
    cout << "out_prefix_hex (e.g. 0abbcc) = ";
    cin >> output_prefix_hex;
    unsigned char output_prefix[OUTPUT_BUFFER_SIZE];
    assert(output_prefix_hex.size() / 2 <= OUTPUT_BUFFER_SIZE);
    const uint32_t output_prefix_len = hexToString(output_prefix_hex.c_str(), output_prefix_hex.size(), output_prefix);
    
    thread *threads[THREAD_NUM];
    uint64_t start = 0ULL, step = STEP;
    for (uint64_t i = 0ULL; i < THREAD_NUM; i++)
      threads[i] = new thread(
          pow,
          (unsigned char *)input_prefix.c_str(),
          input_prefix.size(),
          output_prefix,
          output_prefix_len,
          start + step * i,
          start + step * (i+1ULL)
      );

    // Acctually, it will never join all threads. The proces will print the answer and exit.
    for (uint32_t i = 0; i < THREAD_NUM; i++)
      threads[i]->join();
}

