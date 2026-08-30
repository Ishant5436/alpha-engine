CXX := clang++
CXXFLAGS := -std=c++20 -O3 -Wall -Wextra -Werror -pedantic -march=native -Iinclude
TEST_FLAGS := -std=c++20 -O3 -Wall -Wextra -Werror -pedantic -march=native -Iinclude

all: bin/alpha_engine bin/live_trader

bin/alpha_engine: src/main.cpp
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) $< -o $@

bin/live_trader: src/live_trader.cpp
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) $< -o $@

live: bin/live_trader

test: bin/test_runner
	@./bin/test_runner
	@python3 scripts/audit_safety_invariants.py

bin/test_runner: tests/test_runner.cpp
	@mkdir -p bin
	$(CXX) $(TEST_FLAGS) $< -o $@

clean:
	rm -rf bin/ *.dSYM metrics.json

.PHONY: all live test clean
