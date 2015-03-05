# Detect platform
OS := $(shell uname -s)

# ----

# The compile flags
CFLAGS := -MMD -std=c99 -Wall -Werror -pedantic

# ----

# GNU libc crap
ifeq ($(OS),Linux)
CFLAGS += -D_GNU_SOURCE
endif

# ----

# The linker flags
LDFLAGS = -lncurses

# ----

# Special pathes
ifeq ($(OS),FreeBSD)
INCLUDE := -I/usr/local/include
LIBPATH := -L/usr/local/lib
endif

# ----

# Debugging
ifdef NDEBUG
CFLAGS = -Os -DNDEBUG
else
CFLAGS += -O0 -g
endif

# ----

# Silent builds by default
ifdef VERBOSE
Q :=
else
Q := @
endif
# ----

# Converter rule
build/%.o: %.c
	@echo "===> CC $<"
	$(Q)mkdir -p $(@D)
	$(Q)$(CC) -c $(INCLUDE) $(CFLAGS) -o $@ $<

# ----

# All target
all: touka

# Phony targets
.PHONE: all clean

# Clean target
clean:
	@echo "===> CLEAN"
	$(Q)rm -Rf build release

# Main target
touka:
	@echo "===> Building release/touka"
	$(Q)mkdir -p release
	$(MAKE) release/touka

# ----

# Objects
OBJS_ = \
	src/data/darray.o \
	src/data/hashmap.o \
	src/data/list.o \
	src/curses.o \
	src/game.o \
	src/input.o \
	src/log.o \
	src/main.o \
	src/misc.o \
	src/parser.o \
	src/save.o \
	src/util.o

# Buildpath
OBJS = $(patsubst %,build/%,$(OBJS_))

# ----

# Generate header dependencies
DEPS = $(OBJS:.o=.d)

# Suck them in
-include $(DEPS)

# ----

# Link touka
ifndef NDEBUG
release/touka: $(OBJS)
	@echo "===> LD $@"
	$(Q)$(CC) $(OBJS) $(LIBPATH) $(LDFLAGS) -o $@
else
release/touka: $(OBJS)
	@echo "===> LD $@"
	$(Q)$(CC) $(OBJS) $(LIBPATH) $(LDFLAGS) -o $@
	$(Q)strip $@
endif
