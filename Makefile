.PHONY: help setup setup-debug setup-coverage setup-sanitize setup-fuzzer build debug test robustness lint lint-stale lint-stale-clean format analyze security dependency-audit clean clean-reports coverage sbom all sanitize tidy format-check run fuzzer fuzzer-smoke fuzzer-run fuzzer-corpus install-beez install-beez-completion uninstall-beez

BUILD_DIR ?= build
BUILD_TYPE ?= Release
CONAN_PROFILE ?= clang-release
REPORTS_DIR ?= report
ARGS ?=
BEEZ_BIN := $(BUILD_DIR)/build/$(BUILD_TYPE)/bin/beez
BEEZ_INSTALL_DIR ?= $(HOME)/.local/bin
FUZZER_BIN := $(BUILD_DIR)/build/Debug/fuzz/fuzz_lua_dsl
FUZZER_TIME ?= 30
MIN_LINE_COVERAGE ?= 85
FUZZER_CORPUS_DIR := $(REPORTS_DIR)/fuzz/corpus/lua_dsl
FUZZER_ARTIFACTS_DIR := $(REPORTS_DIR)/fuzz/artifacts

export CC = clang
export CXX = clang++
export REPORTS_DIR
export MIN_LINE_COVERAGE

CXX_FILES := $(shell find src include tests -name '*.cpp' -o -name '*.hpp' -o -name '*.h' 2>/dev/null)
CMAKE_FILES := CMakeLists.txt src/CMakeLists.txt src/app/CMakeLists.txt src/cli/CMakeLists.txt src/core/CMakeLists.txt src/logging/CMakeLists.txt src/plugins/CMakeLists.txt src/plugins/lua/CMakeLists.txt src/plugins/shell/CMakeLists.txt tests/CMakeLists.txt tests/unit/CMakeLists.txt tests/integration/CMakeLists.txt tests/system/CMakeLists.txt tests/fuzz/CMakeLists.txt

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
	@mkdir -p $(REPORTS_DIR)/test
	bash -o pipefail -c 'cd $(BUILD_DIR)/build/$(BUILD_TYPE) && ctest --output-on-failure --verbose 2>&1 | tee $(CURDIR)/$(REPORTS_DIR)/test/test-report.txt'

robustness: ## System-Robustness-Tests (Crash/Edge-Case E2E, schneller als make test)
	@test -d $(BUILD_DIR)/build/$(BUILD_TYPE) || (echo "Run make build first." && exit 1)
	@mkdir -p $(REPORTS_DIR)/test
	bash -o pipefail -c 'cd $(BUILD_DIR)/build/$(BUILD_TYPE) && ctest --output-on-failure -R "SystemRobustnessTest|SystemNegativeFixtureTest|SystemCacheAdversarialTest|SystemDslFieldMatrixTest" 2>&1 | tee $(CURDIR)/$(REPORTS_DIR)/test/robustness-report.txt'

run: ## Beez ausführen (z.B. make run ARGS=build BUILD_TYPE=Debug)
	BUILD_TYPE=$(BUILD_TYPE) CONAN_PROFILE=$(CONAN_PROFILE) $(BEEZ_BIN) $(ARGS)

install-beez: ## Symlink beez nach ~/.local/bin (danach: beez im Terminal)
	@test -x $(BEEZ_BIN) || (echo "Run make build first." && exit 1)
	BEEZ_INSTALL_DIR=$(BEEZ_INSTALL_DIR) BUILD_TYPE=$(BUILD_TYPE) ./scripts/install-beez.sh

install-beez-completion: ## Shell-Tab-Completion in ~/.zshrc / ~/.bashrc eintragen
	./scripts/install-beez-completion.sh

uninstall-beez: ## beez-Symlink aus ~/.local/bin entfernen
	BEEZ_INSTALL_DIR=$(BEEZ_INSTALL_DIR) ./scripts/uninstall-beez.sh

lint: ## clang-tidy + cmake-format check (full)
	@mkdir -p $(REPORTS_DIR)/lint
	bash -o pipefail -c './scripts/lint.sh $(BUILD_DIR) 2>&1 | tee $(REPORTS_DIR)/lint/lint-report.txt'

lint-stale: ## Incremental lint (only changed or previously failed files)
	@mkdir -p $(REPORTS_DIR)/lint
	bash -o pipefail -c 'LINT_BUILD_TYPE=$(BUILD_TYPE) ./scripts/lint-stale.sh $(BUILD_DIR) 2>&1 | tee $(REPORTS_DIR)/lint/lint-stale-report.txt'

lint-stale-clean: ## Clear incremental lint cache
	rm -rf $(REPORTS_DIR)/lint/stale

format: ## Alles formatieren
	clang-format -i $(CXX_FILES)
	cmake-format -i $(CMAKE_FILES)

format-check: ## Formatierung prüfen (CI)
	@mkdir -p $(REPORTS_DIR)/format
	bash -o pipefail -c '{ \
		echo "=== clang-format ==="; \
		clang-format --dry-run --Werror $(CXX_FILES); \
		echo "=== cmake-format ==="; \
		cmake-format --check $(CMAKE_FILES); \
	} 2>&1 | tee $(REPORTS_DIR)/format/format-check-report.txt'

analyze: ## Static Analysis (cppcheck + clang-tidy)
	@mkdir -p $(REPORTS_DIR)/analyze
	bash -o pipefail -c './scripts/analyze.sh $(BUILD_DIR) 2>&1 | tee $(REPORTS_DIR)/analyze/analyze-report.txt'

security: ## Security Checks (code + dependency audit)
	@mkdir -p $(REPORTS_DIR)/security
	bash -o pipefail -c './scripts/security.sh $(BUILD_DIR) 2>&1 | tee $(REPORTS_DIR)/security/security-report.txt'
	bash -o pipefail -c './scripts/dependency-audit.sh $(BUILD_DIR) $(REPORTS_DIR) 2>&1 | tee $(REPORTS_DIR)/security/dependency-audit.txt'

dependency-audit: ## Conan-Abhängigkeiten gegen OSV prüfen
	@mkdir -p $(REPORTS_DIR)/security
	bash -o pipefail -c './scripts/dependency-audit.sh $(BUILD_DIR) $(REPORTS_DIR) 2>&1 | tee $(REPORTS_DIR)/security/dependency-audit.txt'

clean: ## Build artifacts löschen
	rm -rf $(BUILD_DIR)

clean-reports: ## QA-Reports löschen
	rm -rf $(REPORTS_DIR)

coverage: setup-coverage ## Code-Coverage-Report (bricht ab unter $(MIN_LINE_COVERAGE)% auf src/)
	cmake --build --preset conan-debug
	./scripts/coverage-test.sh $(BUILD_DIR) $(REPORTS_DIR)
	./scripts/coverage-report.sh $(BUILD_DIR) $(REPORTS_DIR)

sbom: ## CycloneDX-SBOM aus Conan-Abhängigkeiten erzeugen
	./scripts/sbom-generate.sh $(BUILD_DIR) $(REPORTS_DIR)

sanitize: setup-sanitize ## Debug-Build mit ASan/UBSan + Tests
	cmake --build --preset conan-debug
	@mkdir -p $(REPORTS_DIR)/sanitize
	bash -o pipefail -c 'cd $(BUILD_DIR)/build/Debug && ctest --output-on-failure 2>&1 | tee $(CURDIR)/$(REPORTS_DIR)/sanitize/sanitize-report.txt'

fuzzer: setup-fuzzer ## Fuzzer bauen
	cmake --build --preset conan-debug --target fuzz_lua_dsl

fuzzer-smoke: fuzzer ## Fuzzer kurz laufen lassen (FUZZER_TIME Sekunden, default 30)
	@test -n "$$(ls tests/fuzz/corpus/lua_dsl/*.lua 2>/dev/null)" || \
		(echo "ERROR: No fuzz seeds in tests/fuzz/corpus/lua_dsl/*.lua" && exit 1)
	rm -rf $(FUZZER_CORPUS_DIR) $(FUZZER_ARTIFACTS_DIR)
	mkdir -p $(FUZZER_CORPUS_DIR) $(FUZZER_ARTIFACTS_DIR)
	cp tests/fuzz/corpus/lua_dsl/*.lua $(FUZZER_CORPUS_DIR)/
	@echo "=== Running fuzz_lua_dsl for $(FUZZER_TIME)s (invalid Lua input is expected) ==="
	@mkdir -p $(REPORTS_DIR)/fuzz
	bash -o pipefail -c 'ASAN_OPTIONS=detect_leaks=0 $(FUZZER_BIN) $(FUZZER_CORPUS_DIR) -dict=tests/fuzz/lua_dsl.dict -detect_leaks=0 -max_total_time=$(FUZZER_TIME) -print_final_stats=1 -rss_limit_mb=0 -artifact_prefix=$(FUZZER_ARTIFACTS_DIR)/ 2>&1 | tee $(REPORTS_DIR)/fuzz/fuzz-smoke-report.txt'

fuzzer-run: fuzzer-smoke ## Alias für fuzzer-smoke

fuzzer-corpus: fuzzer ## Fuzzer länger laufen lassen und Corpus sammeln
	@test -n "$$(ls tests/fuzz/corpus/lua_dsl/*.lua 2>/dev/null)" || \
		(echo "ERROR: No fuzz seeds in tests/fuzz/corpus/lua_dsl/*.lua" && exit 1)
	rm -rf $(REPORTS_DIR)/fuzz/corpus $(FUZZER_ARTIFACTS_DIR)
	mkdir -p $(FUZZER_CORPUS_DIR) $(FUZZER_ARTIFACTS_DIR)
	cp tests/fuzz/corpus/lua_dsl/*.lua $(FUZZER_CORPUS_DIR)/
	@mkdir -p $(REPORTS_DIR)/fuzz
	bash -o pipefail -c 'ASAN_OPTIONS=detect_leaks=0 $(FUZZER_BIN) $(FUZZER_CORPUS_DIR) -dict=tests/fuzz/lua_dsl.dict -detect_leaks=0 -max_total_time=60 -rss_limit_mb=0 -artifact_prefix=$(FUZZER_ARTIFACTS_DIR)/ 2>&1 | tee $(REPORTS_DIR)/fuzz/fuzz-corpus-report.txt'

all: build test format-check lint analyze security coverage sanitize fuzzer-smoke ## Komplette QS-Pipeline
	@echo "=== All quality checks passed ==="
	@echo "=== Reports written to $(REPORTS_DIR)/ ==="

tidy: ## Nur clang-tidy
	clang-tidy -p $(BUILD_DIR)/build/$(BUILD_TYPE) $(CXX_FILES) \
		--header-filter='(src|include|tests)/.*' --use-color
