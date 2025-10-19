# Compiler
CC = gcc

# Warning flags
WARN_FLAGS = -Wall -Wpointer-arith \
	-Werror=vla -Wendif-labels \
	-Wmissing-format-attribute -Wimplicit-fallthrough=3 \
	-Wcast-function-type -Wshadow=compatible-local \
	-Wformat-security -Wno-format-truncation \
	-Wno-stringop-truncation -Wdate-time -Wformat \
	-Werror=format-security -Werror=implicit-function-declaration

# Optimization and debugging flags
OPT_FLAGS = -std=gnu99 -O3 -g -flto=auto -ffat-lto-objects

# Security and hardening flags
SEC_FLAGS = -fno-strict-aliasing -fwrapv -fexcess-precision=standard \
	-fno-omit-frame-pointer -mno-omit-leaf-frame-pointer \
	-fstack-protector-strong -fstack-clash-protection \
	-fcf-protection -D_FORTIFY_SOURCE=3

# Position independent code flags
PIC_FLAGS = -fPIC -fvisibility=hidden

# PostgreSQL
PG_VERSION = 17
PG_INCLUDE = /usr/include/postgresql/$(PG_VERSION)/server
PG_INTERNAL = /usr/include/postgresql/internal

# TA-Lib paths
TALIB_DIR = ../..
TALIB_INCLUDE = $(TALIB_DIR)/include
TALIB_COMMON = $(TALIB_DIR)/src/ta_common
TALIB_FUNC = $(TALIB_DIR)/src/ta_func

# Include paths
INCLUDES = -I. -I./ -I$(PG_INCLUDE) -I$(PG_INTERNAL) \
	-I/usr/include/libxml2 -I$(TALIB_INCLUDE) -I$(TALIB_COMMON)

# Preprocessor flags
CPPFLAGS = -D_GNU_SOURCE

# All compiler flags
CFLAGS = $(WARN_FLAGS) $(OPT_FLAGS) $(SEC_FLAGS) $(PIC_FLAGS) \
	$(INCLUDES) $(CPPFLAGS)

# Source files
SOURCES = ta_pg.c $(TALIB_FUNC)/*.c $(TALIB_COMMON)/*.c

# Output
TARGET = ta_pg.so

# Build rule
$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -shared -o $@ $^

.PHONY: clean
clean:
	rm -f $(TARGET)
