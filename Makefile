.PHONY: check docs-check build-check test-check harness-check examples-check canary-check clean tree

CXX ?= c++
CXXFLAGS ?= -std=c++23 -O2 -Wall -Wextra -Wpedantic -Iinclude
UNAME_S := $(shell uname -s)

VM_SRC := src/vm/vm.cpp src/vm/loader.cpp src/vm/validator.cpp src/vm/summary.cpp src/vm/program_io.cpp
VM_C_API_SRC := src/vm/c_api.cpp
VM_CLI_SRC := src/vm/main.cpp
VM_BIN := build/t81vm
VM_C_API_LIB := build/libt81vm_capi.a
ifeq ($(UNAME_S),Darwin)
VM_C_API_SHARED := build/libt81vm_capi.dylib
SHARED_LDFLAGS := -dynamiclib
else
VM_C_API_SHARED := build/libt81vm_capi.so
SHARED_LDFLAGS := -shared
endif
TEST_SRCS := $(wildcard tests/cpp/*_test.cpp)
TEST_BINS := $(patsubst tests/cpp/%.cpp,build/%,$(TEST_SRCS))

check: docs-check build-check test-check harness-check examples-check

canary-check:
	@bash scripts/ecosystem-canary.sh

docs-check:
	@test -f README.md
	@test -f SPEC.md
	@test -f CONTRIBUTING.md
	@test -f docs/architecture.md
	@test -f docs/ecosystem-map.md
	@test -f docs/runtime-ownership.md
	@test -f docs/contracts/vm-compatibility.json
	@test -f docs/foundation-migration.md
	@test -f docs/roadmap.md
	@echo "docs-check: ok"

build-check: $(VM_BIN) $(VM_C_API_LIB) $(VM_C_API_SHARED)
	@echo "build-check: ok"

$(VM_BIN): $(VM_SRC) $(VM_CLI_SRC) include/t81/tisc/opcodes.hpp include/t81/tisc/program.hpp include/t81/vm/loader.hpp include/t81/vm/program_io.hpp include/t81/vm/state.hpp include/t81/vm/summary.hpp include/t81/vm/traps.hpp include/t81/vm/validator.hpp include/t81/vm/vm.hpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -o $@ $(VM_SRC) $(VM_CLI_SRC)

$(VM_C_API_LIB): $(VM_SRC) $(VM_C_API_SRC) include/t81/vm/c_api.h include/t81/tisc/opcodes.hpp include/t81/tisc/program.hpp include/t81/vm/loader.hpp include/t81/vm/program_io.hpp include/t81/vm/state.hpp include/t81/vm/summary.hpp include/t81/vm/traps.hpp include/t81/vm/validator.hpp include/t81/vm/vm.hpp
	@mkdir -p build
	@rm -f build/c_api.o build/vm_core_objs/*.o
	@mkdir -p build/vm_core_objs
	@set -e; for s in $(VM_SRC); do \
		o="build/vm_core_objs/$$(basename $$s .cpp).o"; \
		$(CXX) $(CXXFLAGS) -c $$s -o $$o; \
	done
	$(CXX) $(CXXFLAGS) -c $(VM_C_API_SRC) -o build/c_api.o
	$(AR) rcs $@ build/vm_core_objs/*.o build/c_api.o

$(VM_C_API_SHARED): $(VM_SRC) $(VM_C_API_SRC) include/t81/vm/c_api.h include/t81/tisc/opcodes.hpp include/t81/tisc/program.hpp include/t81/vm/loader.hpp include/t81/vm/program_io.hpp include/t81/vm/state.hpp include/t81/vm/summary.hpp include/t81/vm/traps.hpp include/t81/vm/validator.hpp include/t81/vm/vm.hpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(SHARED_LDFLAGS) -o $@ $(VM_SRC) $(VM_C_API_SRC)

build/%: tests/cpp/%.cpp $(VM_SRC) include/t81/tisc/opcodes.hpp include/t81/tisc/program.hpp include/t81/vm/loader.hpp include/t81/vm/program_io.hpp include/t81/vm/state.hpp include/t81/vm/summary.hpp include/t81/vm/traps.hpp include/t81/vm/validator.hpp include/t81/vm/vm.hpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -o $@ $< $(VM_SRC)

test-check: $(TEST_BINS)
	@set -e; for t in $(TEST_BINS); do echo "running $$t"; "$$t"; done
	@echo "test-check: ok"

harness-check: $(VM_BIN)
	@python3 tests/harness/harness.py run
	@echo "harness-check: ok"

examples-check: $(VM_BIN)
	@tests/examples/check_examples.sh

clean:
	@rm -rf build

tree:
	@find . -maxdepth 4 -type f | sort
