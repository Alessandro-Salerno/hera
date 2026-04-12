MAKEFLAGS += -rR
.SUFFIXES:

rwildcard = $(foreach d,$(wildcard $(1:=/*)),$(call rwildcard ,$d, $2) $(filter $(subst *, %, $2),$d))

TOOL_BINARY := hera
FUZZ_BINARY := hera-fuzzer
VSCODE_EXTENSION := hera.vsix
TREESITTER_PARSER := treesitter-hera.so

CC := cc
LD := cc
FUZZ_CC := clang

CFLAGS := \
	-pipe\
	-O3 -g3 \
	-I include/ \
	-Wall \
	-Wextra \
	-Wformat \
	-Wformat=2 \
	-std=gnu11 \
	-MMD -MP

FUZZ_CFLAGS := $(filter-out -O3, $(CFLAGS)) \
			   -O0 -g3 -fsanitize=fuzzer,address,undefined \
			   -fno-omit-frame-pointer

CFILES := $(call rwildcard, src, *.c)
FUZZ_CFILES := $(filter-out src/main.c, $(CFILES)) fuzz/libfuzzer.c
OBJ := $(addprefix obj/,$(CFILES:.c=.c.o) $(ASFILES:.S=.S.o) $(NASMFILES:.asm=.asm.o))
HEADER_DEPS := $(addprefix obj/,$(CFILES:.c=.c.d) $(ASFILES:.S=.S.d))

# Default target.
.PHONY: all
all: tool extra

.PHONY: tool
tool: bin/$(TOOL_BINARY) 

.PHONY: extra
extra: bin/$(VSCODE_EXTENSION) bin/$(TREESITTER_PARSER)

.PHONY: fuzzer
fuzzer: bin/$(FUZZ_BINARY)

bin/$(FUZZ_BINARY): $(FUZZ_CFILES)
	@echo "    CC " $(FUZZ_CFILES)
	@mkdir -p "$$(dirname $@)"
	$(FUZZ_CC) $(FUZZ_CFLAGS) $(FUZZ_CFILES) -o $@

bin/$(TOOL_BINARY): GNUmakefile $(OBJ)
	@echo "    LD " $(OBJ)
	@mkdir -p "$$(dirname $@)"
	@$(LD) $(OBJ) -o $@

bin/$(VSCODE_EXTENSION): GNUmakefile plugins/vscode
	@echo "    VS " plugins/vscode
	@cd plugins/vscode && vsce package --out ../../bin/$(VSCODE_EXTENSION) \
		> /dev/null

bin/$(TREESITTER_PARSER): plugins/treesitter/src/parser.c GNUmakefile
	@echo "    CC " $<
	@$(CC) -shared -fPIC plugins/treesitter/src/parser.c -o bin/$(TREESITTER_PARSER)

plugins/treesitter/src/parser.c: GNUmakefile plugins/treesitter
	@echo "    TS " plugins/treesitter
	@cd plugins/treesitter && tree-sitter generate

-include $(HEADER_DEPS)

obj/src/%.c.o: src/%.c GNUmakefile
	@echo "    CC " $<
	@mkdir -p "$$(dirname $@)"
	@$(CC) $(CFLAGS) -c $< -o $@

.PHONY: install
install: tool
	cp bin/$(TOOL_BINARY) /usr/bin

.PHONY: uninstall
uninstall:
	rm /usr/bin/$(TOOL_BINARY)

.PHONY: fuzz
fuzz: bin/$(FUZZ_BINARY)
	@echo "[*] Running fuzzer with corpus: $(CORPUS)"
	mkdir -p fuzz_outputs/ && mkdir -p fuzz_corpus && cp examples/* fuzz_corpus/ && \
		cd fuzz_outputs && \
			ASAN_SYMBOLIZER_PATH=$(shell which llvm-symbolizer) \
			ASAN_OPTIONS=detect_leaks=0:symbolize=1 \
		../bin/$(FUZZ_BINARY) \
			-max_len=8192 \
			-use_value_profile=1 \
			-entropic=1 \
			../fuzz_corpus/

.PHONY: clean
clean:
	rm -rf bin obj

.PHONY: purge
purge: clean
	rm -rf fuzz_corpus fuzz_outputs
