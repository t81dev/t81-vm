.PHONY: check docs-check build-check test-check harness-check clean tree

CXX ?= c++
CXXFLAGS ?= -std=c++23 -O2 -Wall -Wextra -Wpedantic -Iinclude

VM_SRC := src/vm/vm.cpp src/vm/loader.cpp src/vm/validator.cpp src/vm/summary.cpp
VM_CLI_SRC := src/vm/main.cpp
VM_BIN := build/t81vm
TEST_SRCS := $(wildcard tests/cpp/*_test.cpp)
TEST_BINS := $(patsubst tests/cpp/%.cpp,build/%,$(TEST_SRCS))

check: docs-check build-check test-check harness-check

docs-check:
	@test -f README.md
	@test -f SPEC.md
	@test -f CONTRIBUTING.md
	@test -f docs/architecture.md
	@test -f docs/ecosystem-map.md
	@test -f docs/foundation-migration.md
	@test -f docs/roadmap.md
	@echo "docs-check: ok"

build-check: $(VM_BIN)
	@echo "build-check: ok"

$(VM_BIN): $(VM_SRC) $(VM_CLI_SRC) include/t81/tisc/opcodes.hpp include/t81/tisc/program.hpp include/t81/vm/loader.hpp include/t81/vm/state.hpp include/t81/vm/summary.hpp include/t81/vm/traps.hpp include/t81/vm/validator.hpp include/t81/vm/vm.hpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -o $@ $(VM_SRC) $(VM_CLI_SRC)

build/%: tests/cpp/%.cpp $(VM_SRC) include/t81/tisc/opcodes.hpp include/t81/tisc/program.hpp include/t81/vm/loader.hpp include/t81/vm/state.hpp include/t81/vm/summary.hpp include/t81/vm/traps.hpp include/t81/vm/validator.hpp include/t81/vm/vm.hpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -o $@ $< $(VM_SRC)

test-check: $(TEST_BINS)
	@set -e; for t in $(TEST_BINS); do echo "running $$t"; "$$t"; done
	@echo "test-check: ok"

harness-check: $(VM_BIN)
	@python3 tests/harness/harness.py run
	@echo "harness-check: ok"

clean:
	@rm -rf build

tree:
	@find . -maxdepth 4 -type f | sort
