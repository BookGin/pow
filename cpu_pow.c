/*
 * gcc cpu_pow.c -s -Ofast -march=native -fomit-frame-pointer -fno-stack-protector -pthread -lcrypto -lssl -Wextra -Wall -o cpu_pow
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include <sys/sysinfo.h>

#include <openssl/sha.h>
#include <openssl/md5.h>

typedef unsigned char * (*Hash)(const unsigned char *, long unsigned int, unsigned char *);

pthread_mutex_t lock;

const char *seed = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
int MAX_COUNTER_LEN = 24; // so each thread iterates on seed.length ^ MAX_COUNTER_LEN hashes.
const int INPUT_BUF_L = 64; 
const int OUTPUT_BUF_L = 64;
uint8_t *oprefix_buf;
int oprefix_l;

uint8_t **iprefixs;
int *iprefix_ls;
int iprefix_l;
/*
 * hash: block size -> output size
 * md5: 64 -> 16
 * sha1: 64 -> 20
 * sha256: 64 -> 32
 * sha512: 128 -> 64
 */
Hash hash;


void fatal(const char *s) {
  fprintf(stderr, "%s\n", s);
  exit(1);
}

uint8_t hexCharToByte(char c) {
  if ('0' <= c && c <= '9')
    return c - '0';
  if ('a' <= c && c <= 'f')
    return c - 'a' + 10;
  if ('A' <= c && c <= 'F')
    return c - 'A' + 10;
  exit(-1);
}

void hexStringToBytes(char *s, uint8_t *output) {
  uint8_t *p = output;
  while (*s != '\0') {
    *p = hexCharToByte(*s) * 0x10 + hexCharToByte(*(s+1));
    p++;
    s += 2;
  }
}

void *work(int idx) {
    int seed_l = strlen(seed);
    int counter_l = MAX_COUNTER_LEN;
    uint8_t counter[counter_l];
    memset(counter, 0, counter_l);
    uint8_t output[INPUT_BUF_L];
    uint8_t input[INPUT_BUF_L];
    memcpy(input, iprefixs[idx], sizeof(uint8_t) * iprefix_ls[idx]);
    int input_l = iprefix_ls[idx] + 1;
    input[iprefix_ls[idx] - 1] = seed[0];


    while (1) {
        hash(input, input_l, output);
        if (0 == memcmp(output, oprefix_buf, oprefix_l)) {
          pthread_mutex_lock(&lock);
          for (int i = iprefix_l; i < input_l; i++)
            printf("%c", input[i]);
          printf("\n");
          exit(0);
        }
        uint8_t *counter_p = counter;
        uint8_t *input_p = input + iprefix_ls[idx];
        while (*counter_p == (seed_l - 1)) {
          *counter_p = 0;
          *input_p = seed[*counter_p];

          counter_p++;
          input_p++;

          if ((input_p - input + 1) > input_l)
            input_l++;
        }
        (*counter_p)++;
        *input_p = seed[*counter_p];
    }
}


Hash setHash(const char *s) {
  if (0 == strcmp(s, "md5"))
    return MD5;
  if (0 == strcmp(s, "sha1"))
    return SHA1;
  if (0 == strcmp(s, "sha256"))
    return SHA256;
  if (0 == strcmp(s, "sha512"))
    return SHA512;
  fatal("The hash algorithm is not supported :(");
}

int getThreadNum() {
  char *s = getenv("THREAD");
  if (s == NULL)
    return get_nprocs();
  return atoi(s);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
      printf("Output answer such that hash(iprefix + answer) = oprefix\n"
             "Usage: %s hash iprefix_hex oprefix_hex\n"
             "Environment varaible: THREAD=n\n"
             "Example: THREAD=8 %s sha1 616161 000000\n"
             "Support hashes: md5 sha1 sha256 sha512\n",
             argv[0], argv[0]
      );
      return 1;
    }

    hash = setHash(argv[1]);
    int thread_num = getThreadNum();

    uint8_t input_buf[INPUT_BUF_L];
    hexStringToBytes(argv[2], input_buf);
    iprefix_l = strlen(argv[2]) / 2;

    oprefix_buf = malloc(sizeof(uint8_t) * OUTPUT_BUF_L);
    hexStringToBytes(argv[3], oprefix_buf);
    oprefix_l = strlen(argv[3]) / 2;

    int seed_l = strlen(seed); 
    uint8_t *fill = input_buf + iprefix_l;
    pthread_t *threads = malloc(sizeof(pthread_t) * thread_num);
    iprefixs = malloc(sizeof(uint8_t *) * thread_num);
    iprefix_ls = malloc(sizeof(int) * thread_num);
    for (int i = 0; i < thread_num; i++) {
        if (i > 0 && i % seed_l == 0)
          fill++;
        *fill = seed[i % seed_l];

        iprefixs[i] = malloc(sizeof(uint8_t) * INPUT_BUF_L);
        memcpy(iprefixs[i], input_buf, sizeof(uint8_t) * INPUT_BUF_L);
        iprefix_ls[i] = fill - input_buf + 1;
        
        pthread_create(threads + i, NULL, work, i);

    }
    // actually it will never join any threads
    pthread_join(threads[0], NULL);
}
