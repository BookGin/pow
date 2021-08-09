.PHONY: clean

default: cpu_pow run
#benchmark

cpu_pow: cpu_pow.c
	gcc $< -s -Ofast -march=native -fomit-frame-pointer -fno-stack-protector -pthread -lcrypto -lssl -Wextra -Wall -o $@

run: cpu_pow
	./cpu_pow sha1 616263 00000000

benchmark: cpu_pow
	/usr/bin/time --verbose ./cpu_pow md5 616162 000000
