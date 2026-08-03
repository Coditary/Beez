.PHONY: help setup build debug test lint format analyze security clean coverage all sanitize tidy format-check

BUILD_DIR ?= build
BUILD_TYPE ?= Release
CONAN_PROFILE ?= clang-release

export CC = clang
export CXX = clang++

CXX_FILES := $(shell find src include tests -name '*.cpp' -o -name '*.hpp' 2>/dev/null)
CMAKE_FILES := CMakeLists.txt src/CMakeLists.txt tests/CMakeLists.txt libs/CMakeLists.txt plugins/CMakeLists.txt

help: ## Alle Targets anzeigen
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | sort | \
		awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-20s\033[0m %s\n", $$1, $$2}'

setup: ## Conan install + CMake configure
	conan install . --output-folder=$(BUILD_DIR) --build=missing -s build_type=$(BUILD_TYPE) -pr $(CONAN_PROFILE) -pr:b $(CONAN_PROFILE)
	cmake --preset conan-$(shell echo $(BUILD_TYPE) | tr '[:upper:]' '[:lower:]') -DBUILD_TESTING=ON -DBUILD_CACHE=ON

build: ## Release build
	$(MAKE) setup BUILD_TYPE=Release
	cmake --build --preset conan-release

debug: ## Debug build mit Sanitizern
	$(MAKE) setup BUILD_TYPE=Debug
	cmake --build --preset conan-debug

test: ## Tests ausführen
	cd $(BUILD_DIR)/build/$(BUILD_TYPE) && ctest --output-on-failure --verbose

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

coverage: ## Code Coverage Report
	$(MAKE) clean
	conan install . --output-folder=$(BUILD_DIR) --build=missing -s build_type=Debug -pr $(CONAN_PROFILE) -pr:b $(CONAN_PROFILE)
	cmake --preset conan-debug -DBUILD_TESTING=ON -DBUILD_COVERAGE=ON
	cmake --build --preset conan-debug --target all
	cd $(BUILD_DIR)/build/Debug && ctest --output-on-failure
	cd $(BUILD_DIR)/build/Debug && gcovr --gcov-executable 'llvm-cov gcov' --root $(CURDIR) --filter '$(CURDIR)/src/' --filter '$(CURDIR)/tests/' --html-details coverage.html .

sanitize: ## Debug Build mit allen Sanitizern
	$(MAKE) setup BUILD_TYPE=Debug
	cd $(BUILD_DIR)/build/Debug && cmake .. -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON -DENABLE_UBSAN=ON
	cmake --build --preset conan-debug
	cd $(BUILD_DIR)/build/Debug && ctest --output-on-failure

all: build test lint analyze ## build + test + lint + analyze

tidy: ## Nur clang-tidy
	clang-tidy -p $(BUILD_DIR)/build/$(BUILD_TYPE) $(CXX_FILES) --use-color

fuzzer: ## Fuzzer bauen
	$(MAKE) clean
	conan install . --output-folder=$(BUILD_DIR) --build=missing -s build_type=Debug -pr $(CONAN_PROFILE) -pr:b $(CONAN_PROFILE)
	cmake --preset conan-debug -DBUILD_TESTING=OFF -DBUILD_FUZZER=ON
	cmake --build --preset conan-debug --target fuzz_version

fuzzer-run: fuzzer ## Fuzzer ausführen (30 Sekunden)
	$(BUILD_DIR)/build/Debug/fuzz/fuzz_version -max_total_time=30

fuzzer-corpus: fuzzer ## Fuzzer Corpus erstellen
	mkdir -p $(BUILD_DIR)/fuzz/corpus
	$(BUILD_DIR)/build/Debug/fuzz/fuzz_version -max_total_time=60 -artifact_prefix=$(BUILD_DIR)/fuzz/corpus/
