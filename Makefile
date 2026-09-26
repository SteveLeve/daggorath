# Developer conveniences. CMake remains the build system of record.
#
#   make sources    fetch the pinned assembly listing (evidence, not a dependency)
#   make fixtures   re-extract every fixture and regenerate the lexicon header
#   make build      configure and build
#   make test       run the conformance tests
#   make traces     regenerate every trace from its script
#   make verify     check fixture hashes against MANIFEST.json
#   make all        fixtures + build + test + traces + verify

ASM_COMMIT := a94326f00ebb16a106b540c58bc2ccf5f7b66dac
ASM_REPO   := https://github.com/MichaelSpencerJr/DungeonsOfDaggorath.git
ASM_DIR    ?= third_party/dod-asm

PACK       := docs/archaeology/phase-0b
FIXTURES   := $(PACK)/fixtures
TRACES     := $(PACK)/traces
BUILD      ?= build
SCRIPTS    := $(wildcard $(TRACES)/*.script)
TRACEFILES := $(SCRIPTS:.script=.trace)

.PHONY: all sources check-pin fixtures build test traces verify format clean distclean

all: fixtures build test traces verify

sources:
	@if [ ! -d "$(ASM_DIR)/.git" ]; then \
	  mkdir -p $(dir $(ASM_DIR)); \
	  git clone --quiet $(ASM_REPO) $(ASM_DIR); \
	fi
	@git -C $(ASM_DIR) fetch --quiet origin
	@git -C $(ASM_DIR) checkout --quiet $(ASM_COMMIT)
	@echo "listing checked out at $(ASM_COMMIT)"

check-pin:
	@test -d "$(ASM_DIR)" || { echo "no listing at $(ASM_DIR); run 'make sources'"; exit 1; }
	@got=$$(git -C "$(ASM_DIR)" rev-parse HEAD); \
	if [ "$$got" != "$(ASM_COMMIT)" ]; then \
	  echo "listing is at $$got, expected $(ASM_COMMIT)"; exit 1; fi; \
	echo "listing pinned at $(ASM_COMMIT)"

fixtures: check-pin
	python3 tools/extract_fixtures.py "$(ASM_DIR)" "$(FIXTURES)"
	python3 tools/gen_lexicon_header.py "$(FIXTURES)/tokens.json" \
	    src/core/include/daggorath/lexicon_tables.hpp

build:
	cmake -S . -B $(BUILD) -DDAGGORATH_FIXTURE_DIR=$(CURDIR)/$(FIXTURES)
	cmake --build $(BUILD) -j

test: build
	ctest --test-dir $(BUILD) --output-on-failure

traces: build $(TRACEFILES)

%.trace: %.script $(BUILD)/src/app/dcli
	./$(BUILD)/src/app/dcli --script $< --jiffies 200 --trace $@

verify:
	python3 tools/verify_manifest.py "$(FIXTURES)"

format:
	@command -v clang-format >/dev/null || { echo "clang-format not installed"; exit 1; }
	clang-format -i $$(git ls-files '*.cpp' '*.hpp')

clean:
	rm -rf $(BUILD)

distclean: clean
	rm -rf third_party
