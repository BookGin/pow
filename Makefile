.PHONY: clean

pow: pow.cpp
	g++ ./pow.cpp -s -Ofast -march=native -pthread -lcrypto -lssl --std=c++14 -Wextra -Wall -o pow

clean:
	rm -f pow
