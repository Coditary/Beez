.PHONY: help setup setup-debug setup-coverage setup-sanitize setup-fuzzer build debug test lint format analyze security clean coverage all sanitize tidy format-check run fuzzer fuzzer-smoke fuzzer-run fuzzer-corpus

BUILD_DIR ?= build
BUILD_TYPE ?= Release
CONAN_PROFILE ?= clang-release
ARGS ?=
BEEZ_BIN := $(BUILD_DIR)/build/$(BUILD_TYPE)/bin/beez
FUZZER_BIN := $(BUILD_DIR)/build/Debug/fuzz/fuzz_lua_dsl
FUZZER_TIME ?= 30

export CC = clang
export CXX = clang++

CXX_FILES := $(shell find src include tests -name '*.cpp' -o -name '*.hpp' -o -name '*.h' 2>/dev/null)
CMAKE_FILES := CMakeLists.txt src/CMakeLists.txt src/app/CMakeLists.txt src/core/CMakeLists.txt src/plugins/CMakeLists.txt src/plugins/lua/CMakeLists.txt src/plugins/shell/CMakeLists.txt tests/CMakeLists.txt tests/unit/CMakeLists.txt tests/integration/CMakeLists.txt tests/system/CMakeLists.txt fuzz/CMakeLists.txt

help: ## Alle Targets anzeigen
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | sort | \
		awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-20s\033[0m %s\n", $$1, $$2}'

setup: ## Conan install + CMake configure (Release/Debug via BUILD_TYPE)
	conan install . --output-folder=$(BUILD_DIR) --build=missing -s build_type=$(BUILD_TYPE) -pr $(CONAN_PROFILE) -pr:b $(CONAN_PROFILE)
	cmake --preset conan-$(shell echo $(BUILD_TYPE) | tr '[:upper:]' '[:lower:]') -DBUILD_TESTING=ON -DBUILD_CACHE=ON

setup-debug: ## Conan + CMake configure (Debug)
	$(MAKE) setup BUILD_TYPE=Debug

setup-coverage: setup-debug ## CMake configure für Coverage
	cmake --preset conan-debug -DBUILD_TESTING=ON -DBUILD_COVERAGE=ON -DBUILD_FUZZER=OFF -DENABLE_ASAN=OFF -DENABLE_UBSAN=OFF

setup-sanitize: setup-debug ## CMake configure für Sanitizer-Build
	cmake --preset conan-debug -DBUILD_TESTING=ON -DBUILD_COVERAGE=OFF -DBUILD_FUZZER=OFF -DENABLE_ASAN=ON -DENABLE_UBSAN=ON

setup-fuzzer: setup-debug ## CMake configure für Fuzzer
	cmake --preset conan-debug -DBUILD_TESTING=OFF -DBUILD_COVERAGE=OFF -DBUILD_FUZZER=ON -DENABLE_ASAN=OFF -DENABLE_UBSAN=OFF

build: ## Release build
	$(MAKE) setup BUILD_TYPE=Release
	cmake --build --preset conan-release

debug: ## Debug build
	$(MAKE) setup-debug
	cmake --build --preset conan-debug

test: ## Tests ausführen (BUILD_TYPE=Release|Debug)
	cd $(BUILD_DIR)/build/$(BUILD_TYPE) && ctest --output-on-failure --verbose

run: ## Beez ausführen (z.B. make run ARGS=clean)
	$(BEEZ_BIN) $(ARGS)

lint: ## clang-tidy + cmake-format check
	./scripts/lint.sh $(BUILD_DIR)

format: ## Alles formatieren
	clang-format -i $(CXX_FILES)
	cmake-format -i $(CMAKE_FILES)

format-check: ## Formatierung prüfen (CI)
	clang-format --dry-run --Werror $(CXX_FILES)
	cmake-format --check $(CMAKE_FILES)

analyze: ## Static Analysis (cppcheck + clang-tidy)
	./scripts/analyze.sh $(BUILD_DIR)

security: ## Security Checks
	./scripts/security.sh $(BUILD_DIR)

clean: ## Build artifacts löschen
	rm -rf $(BUILD_DIR)

coverage: setup-coverage ## Code-Coverage-Report erzeugen
	cmake --build --preset conan-debug
	cd $(BUILD_DIR)/build/Debug && ctest --output-on-failure
	cd $(BUILD_DIR)/build/Debug && gcovr --gcov-executable 'llvm-cov gcov' --root $(CURDIR) --filter '$(CURDIR)/src/' --filter '$(CURDIR)/tests/' --html-details coverage.html .

sanitize: setup-sanitize ## Debug-Build mit ASan/UBSan + Tests
	cmake --build --preset conan-debug
	cd $(BUILD_DIR)/build/Debug && ctest --output-on-failure

fuzzer: setup-fuzzer ## Fuzzer bauen
	cmake --build --preset conan-debug --target fuzz_lua_dsl

fuzzer-smoke: fuzzer ## Fuzzer kurz laufen lassen (FUZZER_TIME Sekunden, default 30)
	rm -rf $(BUILD_DIR)/fuzz/corpus/lua_dsl
	mkdir -p $(BUILD_DIR)/fuzz/corpus/lua_dsl
	cp fuzz/corpus/lua_dsl/*.lua $(BUILD_DIR)/fuzz/corpus/lua_dsl/
	@echo "=== Running fuzz_lua_dsl for $(FUZZER_TIME)s (invalid Lua input is expected) ==="
	ASAN_OPTIONS=detect_leaks=0 $(FUZZER_BIN) $(BUILD_DIR)/fuzz/corpus/lua_dsl -dict=fuzz/lua_dsl.dict -detect_leaks=0 -max_total_time=$(FUZZER_TIME) -print_final_stats=1

fuzzer-run: fuzzer-smoke ## Alias für fuzzer-smoke

fuzzer-corpus: fuzzer ## Fuzzer länger laufen lassen und Corpus unter build/ sammeln
	rm -rf $(BUILD_DIR)/fuzz/corpus
	mkdir -p $(BUILD_DIR)/fuzz/corpus/lua_dsl
	cp fuzz/corpus/lua_dsl/*.lua $(BUILD_DIR)/fuzz/corpus/lua_dsl/
	ASAN_OPTIONS=detect_leaks=0 $(FUZZER_BIN) $(BUILD_DIR)/fuzz/corpus/lua_dsl -dict=fuzz/lua_dsl.dict -detect_leaks=0 -max_total_time=60 -artifact_prefix=$(BUILD_DIR)/fuzz/corpus/

all: build test format-check lint analyze security coverage sanitize fuzzer-smoke ## Komplette QS-Pipeline
	@echo "=== All quality checks passed ==="

tidy: ## Nur clang-tidy
	clang-tidy -p $(BUILD_DIR)/build/$(BUILD_TYPE) $(CXX_FILES) --use-color
