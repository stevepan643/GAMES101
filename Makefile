CC := clang
PKG_CONFIG := pkg-config
PACKAGES := SDL3

BUILD ?= debug
TARGET := Rasterization
SRCS := src/main.c src/window.c

CFLAGS := -Wall $(shell $(PKG_CONFIG) --cflags $(PACKAGES)) -Isrc/
LIBS := $(shell $(PKG_CONFIG) --libs $(PACKAGES))

ifeq ($(BUILD),debug)
    CFLAGS += -O0 -g3 -DDEBUG
    BUILD_DIR := build/debug
else ifeq ($(BUILD),release)
    CFLAGS += -O2 -DNDEBUG
    BUILD_DIR := build/release
else
    $(error BUILD must be either 'debug' or 'release')
endif

OBJS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRCS))

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LIBS)

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf build $(TARGET)

.PHONY: clean