CXX := clang++
CXXFLAGS := -std=c++20 -O3 -Wall -Wextra -Werror -pedantic -march=native -Iinclude
TEST_FLAGS := -std=c++20 -O3 -Wall -Wextra -Werror -pedantic -march=native -Iinclude

all: bin/alpha_engine

bin/alpha_engine: src/main.cpp
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) $< -o $@

test: bin/test_runner
	@./bin/test_runner

bin/test_runner: tests/test_runner.cpp
	@mkdir -p bin
	$(CXX) $(TEST_FLAGS) $< -o $@

clean:
	rm -rf bin/ *.dSYM

.PHONY: all test clean
