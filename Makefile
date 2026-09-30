# Developer conveniences. CMake remains the build system of record.
#
#   make sources    fetch the pinned assembly listing (evidence, not a dependency)
#   make fixtures   re-extract every fixture and regenerate the lexicon header
#   make build      configure and build
#   make test       run the conformance tests
#   make traces     regenerate every trace from its script
#   make verify     check fixture hashes against MANIFEST.json
#   make all        fixtures + build + test + traces + verify
#   make web        WebAssembly build into web/dist (needs emsdk; ADR-0011)
#   make web-test   browser storage test against web/dist (Chrome, Node 22+)
#   make android    debug APK, arm64 + x86_64 (Android SDK/NDK, Gradle; ADR-0012)
#   make android-install  install that APK on the attached device (adb)

ASM_COMMIT := a94326f00ebb16a106b540c58bc2ccf5f7b66dac
ASM_REPO   := https://github.com/MichaelSpencerJr/DungeonsOfDaggorath.git
ASM_DIR    ?= third_party/dod-asm

PACK       := docs/archaeology/phase-0b
FIXTURES   := $(PACK)/fixtures
PHASE6     := docs/archaeology/phase-6/fixtures
TEXT_FIX   := docs/archaeology/phase-6/fixtures/text
TEXT_HDR   := src/presentation/include/daggorath/text_tables.hpp
TRACES     := $(PACK)/traces
BUILD      ?= build
# An active emsdk puts directories named cmake/ and node/ on PATH, and make's
# exec would pick those directories; resolve the binaries the way the shell does.
CMAKE      ?= $(shell command -v cmake)
CTEST      ?= $(shell command -v ctest)
NODE       ?= $(shell command -v node)
SCRIPTS    := $(wildcard $(TRACES)/*.script)
TRACEFILES := $(SCRIPTS:.script=.trace)
WEB_BUILD  ?= build-web
WEB_DIST   := web/dist

.PHONY: all sources check-pin fixtures build test traces verify web web-test android android-install format clean distclean

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
	python3 tools/extract_vectors.py "$(ASM_DIR)" "$(PHASE6)"
	python3 tools/gen_vector_header.py "$(PHASE6)/vectors.json" \
	    src/presentation/include/daggorath/vector_tables.hpp
	python3 tools/viewer_ref.py --vectors "$(PHASE6)/vectors.json" \
	    --maze-dir "$(FIXTURES)" --write "$(PHASE6)"
	python3 tools/extract_sounds.py "$(ASM_DIR)" "$(FIXTURES)" \
	    src/core/include/daggorath/sound_tables.hpp
	python3 tools/extract_text.py "$(ASM_DIR)" "$(TEXT_FIX)" "$(TEXT_HDR)" "$(FIXTURES)"

build:
	$(CMAKE) -S . -B $(BUILD) -DDAGGORATH_FIXTURE_DIR=$(CURDIR)/$(FIXTURES)
	$(CMAKE) --build $(BUILD) -j

test: build
	$(CTEST) --test-dir $(BUILD) --output-on-failure

traces: build $(TRACEFILES)

%.trace: %.script $(BUILD)/src/app/dcli
	./$(BUILD)/src/app/dcli --script $< --jiffies 200 --trace $@

verify:
	python3 tools/verify_manifest.py "$(FIXTURES)"
	python3 tools/verify_manifest.py "$(PHASE6)"
	python3 tools/verify_manifest.py "$(TEXT_FIX)"

web:
	@command -v emcmake >/dev/null || { echo "emsdk not active; source <emsdk>/emsdk_env.sh"; exit 1; }
	$(CMAKE) --preset web
	$(CMAKE) --build $(WEB_BUILD) -j --target dod
	mkdir -p $(WEB_DIST) && rm -f $(WEB_DIST)/*   # keep the directory: wrangler dev watches it
	cp web/public/* $(WEB_DIST)/
	cp $(WEB_BUILD)/src/platform/dod.js $(WEB_BUILD)/src/platform/dod.wasm $(WEB_DIST)/
	python3 tools/web/make_icons.py $(WEB_DIST)
	@# Licence notices for the runtime code dod.js/dod.wasm carry (ledger §6),
	@# taken from emsdk's own copies so no licence text lives in this tree.
	@{ for f in "$$EMSDK/upstream/emscripten/LICENSE" \
	            "$$EMSDK/upstream/emscripten/cache/ports/sdl3/SDL-release-3.4.2/LICENSE.txt" \
	            "$$EMSDK/upstream/emscripten/system/lib/libc/musl/COPYRIGHT" \
	            "$$EMSDK/upstream/emscripten/system/lib/libcxx/LICENSE.TXT"; do \
	    test -f "$$f" || { echo "missing notice: $$f" >&2; exit 1; }; \
	    printf '==== %s ====\n\n' "$${f#$$EMSDK/upstream/emscripten/}"; cat "$$f"; printf '\n\n'; \
	  done; } > $(WEB_DIST)/THIRD_PARTY_NOTICES.txt
	@hash=$$(cat $$(ls -d $(WEB_DIST)/* | sort) | sha256sum | cut -c1-12); \
	sed -i "s/__BUILD__/$$hash/" $(WEB_DIST)/sw.js; \
	echo "web build $$hash in $(WEB_DIST)"

web-test:
	$(NODE) tools/web/storage-test.mjs $(WEB_DIST)

GRADLE     ?= $(shell command -v gradle)
APK        := android/app/build/outputs/apk/debug/app-debug.apk

android:
	tools/android/fetch-sdl.sh
	cd android && $(GRADLE) --no-daemon assembleDebug
	@echo "private build (docs/licensing/README.md D5): $(APK)"

android-install: android
	adb install -r $(APK)

format:
	@command -v clang-format >/dev/null || { echo "clang-format not installed"; exit 1; }
	clang-format -i $$(git ls-files '*.cpp' '*.hpp')

clean:
	rm -rf $(BUILD) $(WEB_BUILD) $(WEB_DIST)

distclean: clean
	rm -rf third_party
