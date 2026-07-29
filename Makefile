# Cardigan — one entry point for every workflow.
#
#   make test        full host suite (no Pebble SDK required)
#   make build       build the .pbw for all target platforms (needs SDK)
#   make run         build + launch the Pebble Time 2 emulator
#   make release     test, build, tag and publish to GitHub
#   make assets      regenerate store banner + icons
#   make screens     guided emulator screenshot capture
#   make clean

SHELL     := /bin/bash
OUT       := test/out
CC        ?= gcc
CFLAGS    := -std=c11 -Wall -Wextra -Werror -Wno-unused-parameter -Itest/stub -Isrc/c
SRC       := test/stub/pebble_stub.c test/harness.c \
             src/c/storage.c src/c/comm.c src/c/card_window.c src/c/menu_window.c
APP_SRC   := $(wildcard src/c/*.c) $(wildcard src/c/*.h)
JS_SRC    := $(wildcard src/pkjs/*.js)

.DEFAULT_GOAL := help
.PHONY: help test unit lint render scan build run install release assets screens clean deps

help:
	@grep -E '^[a-z-]+:.*?## .*$$' $(MAKEFILE_LIST) \
	  | awk 'BEGIN{FS=":.*?## "}{printf "  \033[1m%-10s\033[0m %s\n", $$1, $$2}'

deps: ## install host test dependencies
	pip3 install -q zxing-cpp pillow 2>/dev/null \
	  || pip3 install -q --break-system-packages zxing-cpp pillow

# ---------------------------------------------------------------- testing

test: unit lint render scan ## run the complete host test suite
	@echo "── all checks passed ──"

unit: ## encoder unit tests (Code 128 round trip, QR packing)
	@echo "▸ unit tests"
	@node test/test_encoders.js

lint: ## syntax-check every JS file and both C configurations
	@echo "▸ lint"
	@for f in $(JS_SRC) test/*.js test/shims/*.js; do node --check $$f || exit 1; done
	@$(CC) -fsyntax-only $(CFLAGS) $(APP_SRC:%.h=)          # colour + rectangular
	@$(CC) -fsyntax-only $(CFLAGS) -DPBL_COLOR $(filter %.c,$(APP_SRC))
	@$(CC) -fsyntax-only $(CFLAGS) -DPBL_COLOR -DPBL_ROUND $(filter %.c,$(APP_SRC))
	@bash -n release.sh build_mac.sh docs/store/capture_screenshots.sh
	@echo "  ok   all sources parse"

$(OUT)/harness: $(SRC) $(APP_SRC) test/stub/pebble.h test/stub/stub.h
	@mkdir -p $(OUT)
	@$(CC) $(CFLAGS) -DPBL_COLOR $(SRC) -o $@

$(OUT)/harness_round: $(SRC) $(APP_SRC) test/stub/pebble.h test/stub/stub.h
	@mkdir -p $(OUT)
	@$(CC) $(CFLAGS) -DPBL_COLOR -DPBL_ROUND $(SRC) -o $@

$(OUT)/cards.txt: $(JS_SRC) test/run_pkjs.js
	@mkdir -p $(OUT)
	@NODE_PATH=test/shims node test/run_pkjs.js

render: $(OUT)/harness $(OUT)/harness_round $(OUT)/cards.txt ## render every display geometry
	@echo "▸ render + app logic"
	@./$(OUT)/harness       200 228 $(OUT)/emery_  --logic | tail -1
	@./$(OUT)/harness       144 168 $(OUT)/basalt_ | tail -1
	@./$(OUT)/harness_round 180 180 $(OUT)/chalk_  | tail -1
	@./$(OUT)/harness_round 260 260 $(OUT)/gabbro_ | tail -1

scan: render ## decode the rendered output with a real scanner library
	@echo "▸ scanner verification"
	@python3 test/scan_check.py $(OUT)/emery_ $(OUT)/basalt_ $(OUT)/chalk_ $(OUT)/gabbro_ | tail -1

# ---------------------------------------------------------------- shipping

build: ## build the .pbw (requires the Pebble SDK)
	pebble build
	@echo "platforms in the bundle:"
	@unzip -l build/*.pbw | awk '/\/app.bin/ {split($$4,a,"/"); print "  " a[1]}'

run: build ## build and launch the Pebble Time 2 emulator
	pebble install --emulator emery --logs

install: build ## sideload to a phone: make install PHONE=192.168.1.42
	@test -n "$(PHONE)" || { echo "usage: make install PHONE=<ip>"; exit 1; }
	pebble install --phone $(PHONE) --logs

release: test ## test, build, tag and publish (make release V=1.1.0)
	@bash release.sh $(V)

assets: ## regenerate store banner and icons
	python3 docs/make_assets.py

screens: ## guided emulator screenshot capture
	bash docs/store/capture_screenshots.sh

clean:
	rm -rf $(OUT) build
	@echo "cleaned"
