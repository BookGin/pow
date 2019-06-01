.PHONY: clean

pow: pow.cpp
	g++ $< -s -Ofast -march=native -pthread -lcrypto -lssl --std=c++14 -Wextra -Wall -o $@

pow_output_prefix: pow_output_prefix.cpp
	g++ $< -s -Ofast -march=native -pthread -lcrypto -lssl --std=c++14 -Wextra -Wall -o $@

clean:
	rm -f pow pow_output_prefix
