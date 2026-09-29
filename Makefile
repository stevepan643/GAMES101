CC := clang
PKG_CONFIG := pkg-config
PACKAGES := SDL3

BUILD ?= debug
TARGET := Rasterization
SRCS := src/main.c src/window.c src/pipeline.c
STB_SRC := src/stb_image_impl.c

.DEFAULT_GOAL := $(TARGET)

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
    CFLAGS  += -O3 -march=native -funroll-loops \
               -fno-math-errno -fno-trapping-math \
               -DNDEBUG -D_FORTIFY_SOURCE=2 -fstack-protector-strong
    BUILD_DIR := build/release
else
    $(error BUILD must be either 'debug' or 'release')
endif

OBJS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRCS))
STB_OBJ := $(patsubst %.c,$(BUILD_DIR)/%.o,$(STB_SRC))
ALL_OBJS := $(OBJS) $(STB_OBJ)

$(TARGET): $(ALL_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)

$(STB_OBJ): $(STB_SRC)
	@mkdir -p $(dir $@)
	$(CC) -O2 -g -Isrc -MMD -MP -c $< -o $@

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

-include $(ALL_OBJS:.o=.d)

clean:
	rm -rf build $(TARGET)

.PHONY: clean