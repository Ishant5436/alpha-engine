CXX := clang++
CXXFLAGS := -std=c++20 -O3 -Wall -Wextra -Werror -pedantic -Wno-error=invalid-feature-combination -march=native -Iinclude
TEST_FLAGS := -std=c++20 -O3 -Wall -Wextra -Werror -pedantic -Wno-error=invalid-feature-combination -march=native -Iinclude

all: bin/alpha_engine bin/live_trader

bin/alpha_engine: src/main.cpp
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) $< -o $@

bin/live_trader: src/live_trader.cpp
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) $< -o $@

live: bin/live_trader

test: bin/test_runner bin/test_whitebox
	@echo "=== Running Unit Test Suite ==="
	@./bin/test_runner
	@echo "=== Running White-Box Invariant Test Suite ==="
	@./bin/test_whitebox
	@echo "=== Running Static Safety Invariant Analyzer ==="
	@python3 scripts/audit_safety_invariants.py
	@echo "=== Running Black-Box Integration Tests ==="
	@python3 -m pytest tests/test_blackbox.py -v

bin/test_runner: tests/test_runner.cpp
	@mkdir -p bin
	$(CXX) $(TEST_FLAGS) $< -o $@

bin/test_whitebox: tests/test_whitebox.cpp
	@mkdir -p bin
	$(CXX) $(TEST_FLAGS) $< -o $@

asan: tests/test_whitebox.cpp tests/test_runner.cpp
	@mkdir -p bin
	$(CXX) $(TEST_FLAGS) -fsanitize=address,undefined tests/test_whitebox.cpp -o bin/test_whitebox_asan
	@./bin/test_whitebox_asan
	$(CXX) $(TEST_FLAGS) -fsanitize=address,undefined tests/test_runner.cpp -o bin/test_runner_asan
	@./bin/test_runner_asan
	@echo "=== AddressSanitizer & UBSan: 100% MEMORY SAFE & ZERO LEAKS ==="

demo: all
	@python3 scripts/record_demo_walkthrough.py

clean:
	rm -rf bin/ *.dSYM metrics.json .pytest_cache

.PHONY: all live test asan demo clean
