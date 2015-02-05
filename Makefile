# The compile flags
CFLAGS := -MMD -std=c99 -Wall -Werror -pedantic -Os

# ----

# Debugging
ifdef NDEBUG
CFLAGS = -DNDEBUG
else
CFLAGS += -g
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
	$(Q)$(CC) -c $(CFLAGS) -o $@ $<

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
	src/log.o \
	src/main.o

# Buildpath
OBJS = $(patsubst %,build/%,$(OBJS_))

# ----

# Generate header dependencies
DEPS = $(OBJS:.o=.d)

# Suck them in
-include $(DEPS)

# ----

# Link touka
release/touka: $(OBJS)
	@echo "===> LD $@"
	$(Q)$(CC) $(OBJS) -o $@
