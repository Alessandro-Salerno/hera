MAKEFLAGS += -rR
.SUFFIXES:

rwildcard = $(foreach d,$(wildcard $(1:=/*)),$(call rwildcard ,$d, $2) $(filter $(subst *, %, $2),$d))

TOOL_BINARY := hera
VSCODE_EXTENSION := hera.vsix
TREESITTER_PARSER := treesitter-hera.so

CC := cc
LD := cc

CFLAGS := \
	-pipe\
	-O3 \
	-I include/ \
	-Wall \
	-Wextra \
	-Wformat \
	-Wformat=2 \
	-std=gnu11 \
	-MMD -MP

CFILES := $(call rwildcard, src, *.c)
OBJ := $(addprefix obj/,$(CFILES:.c=.c.o) $(ASFILES:.S=.S.o) $(NASMFILES:.asm=.asm.o))
HEADER_DEPS := $(addprefix obj/,$(CFILES:.c=.c.d) $(ASFILES:.S=.S.d))

# Default target.
.PHONY: all
all: tool extra

.PHONY: tool
tool: bin/$(TOOL_BINARY) 

.PHONY: extra
extra: bin/$(VSCODE_EXTENSION) bin/$(TREESITTER_PARSER)

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

.PHONY: clean
clean:
	rm -rf bin obj

