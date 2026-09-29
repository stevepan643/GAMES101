CC := clang
PKG_CONFIG := pkg-config
PACKAGES := SDL3

BUILD ?= debug
TARGET := Rasterization
SRCS := src/main.c src/window.c src/pipeline.c
LIB_SRC := src/stb_image_impl.c src/tiny_obj_c.c src/tobj_tess.c

.DEFAULT_GOAL := $(TARGET)

CFLAGS := -Wall -Wextra -Wpedantic -Wshadow -Wconversion \
          -Werror=implicit-function-declaration \
          -Werror=return-type \
          -Werror=incompatible-pointer-types \
          -MMD -MP -fno-common -Isrc \
          $(shell $(PKG_CONFIG) --cflags $(PACKAGES))
LIBS := $(shell $(PKG_CONFIG) --libs $(PACKAGES))
LDFLAGS :=

LIB_CFLAGS := -O2 -g -Isrc -MMD -MP -DTOBJ_ENABLE_FILE_IO

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
LIB_OBJ := $(patsubst %.c,$(BUILD_DIR)/%.o,$(LIB_SRC))
ALL_OBJS := $(OBJS) $(LIB_OBJ)

$(TARGET): $(ALL_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)

$(LIB_OBJ): $(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(LIB_CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

-include $(ALL_OBJS:.o=.d)

clean:
	rm -rf build $(TARGET)

.PHONY: clean