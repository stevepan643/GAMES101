CC := clang
PKG_CONFIG := pkg-config
PACKAGES := SDL3

BUILD ?= debug
TARGET := Rasterization
SRCS := src/main.c src/window.c

CFLAGS := -Wall -Wextra -Wpedantic -Wshadow -Wconversion \
          -Werror=implicit-function-declaration \
          -Werror=return-type \
          -Werror=incompatible-pointer-types \
          -MMD -MP -fno-common -Isrc \
          $(shell $(PKG_CONFIG) --cflags $(PACKAGES))
LIBS := $(shell $(PKG_CONFIG) --libs $(PACKAGES))
LDFLAGS :=

ifeq ($(BUILD),debug)
    CFLAGS  += -O0 -g3 -DDEBUG -fsanitize=address,undefined -fno-omit-frame-pointer
    LDFLAGS += -fsanitize=address,undefined
    BUILD_DIR := build/debug
else ifeq ($(BUILD),release)
    CFLAGS  += -O2 -DNDEBUG -D_FORTIFY_SOURCE=2 -fstack-protector-strong
    BUILD_DIR := build/release
else
    $(error BUILD must be either 'debug' or 'release')
endif

OBJS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRCS))

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

-include $(OBJS:.o=.d)

clean:
	rm -rf build $(TARGET)

.PHONY: clean