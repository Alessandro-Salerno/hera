MAKEFLAGS += -rR
.SUFFIXES:

rwildcard = $(foreach d,$(wildcard $(1:=/*)),$(call rwildcard ,$d, $2) $(filter $(subst *, %, $2),$d))

OUTPUT := ergen

CC := cc
LD := cc
CFLAGS := -g3 -pipe -O0

CFLAGS := \
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
all: bin/$(OUTPUT)

bin/$(OUTPUT): GNUmakefile $(OBJ)
	@echo "    LD " $(OBJ)
	@mkdir -p "$$(dirname $@)"
	@$(LD) $(OBJ) -o $@

-include $(HEADER_DEPS)

obj/src/%.c.o: src/%.c GNUmakefile
	@echo "    CC " $<
	@mkdir -p "$$(dirname $@)"
	@$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -rf bin obj

