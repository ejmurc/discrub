CC ?= gcc
CFLAGS = -std=c89 -Wall -Wextra
INCLUDES = -Iinclude/
OS := $(shell uname -s)

ifeq ($(OS), Darwin)
    ifeq ($(shell uname -m), arm64)
        INCLUDES += -I/opt/homebrew/include
        LIBS = -L/opt/homebrew/lib -lssl -lcrypto
    else
        INCLUDES += -I/usr/local/include
        LIBS = -L/usr/local/lib -lssl -lcrypto
    endif
else ifeq ($(OS), Linux)
    INCLUDES += -I/usr/include/openssl
    LIBS = -L/usr/lib -lssl -lcrypto
else
    INCLUDES += -I"C:/OpenSSL/include"
    LIBS = -L"C:/OpenSSL/lib" -lssl -lcrypto
endif

BUILD = build
SRCS = $(wildcard src/*.c)
HEADERS = $(wildcard include/*.h)
OBJS = $(SRCS:src/%.c=$(BUILD)/%.o)

.PHONY: release dev clean format

release: CFLAGS += -Ofast -DNDEBUG
release: $(BUILD)/discrub
	@printf "\033[1;32mProduction build completed successfully.\033[0m\n"

dev: CFLAGS = -std=c89 -O0 -Wall -Wextra -g
dev: $(BUILD)/discrub
	@printf "\033[1;32mDevelopment build completed successfully.\033[0m\n"

$(BUILD)/discrub: $(OBJS)
	@printf "\33[2K\033[1;32mLinking executable...\033[0m\r"
	@$(CC) $(CFLAGS) -o $@ $^ $(LIBS) || \
		(echo "\33[2K\033[1;31mLinking failed.\033[0m"; exit 1)
	@printf "\33[2K\033[1;32mLinked executable.\033[0m\n"

$(BUILD)/%.o: src/%.c | $(BUILD)
	@printf "\33[2K\033[1;33mCompiling $<...\033[0m\r"
	@$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@
	@printf "\33[2K\033[1;33mCompiled $<.\033[0m\n"

$(BUILD):
	@mkdir -p $(BUILD)

clean:
	@rm -rf $(BUILD)
	@printf "\33[2K\033[1;31mCleaned build directory.\033[0m\n"

format:
	@clang-format -i $(SRCS) $(HEADERS) || \
		(echo "\33[2K\033[1;31mFailed to format files.\033[0m"; exit 1)
	@printf "\33[2K\033[1;32mFormatted source and header files.\033[0m\n"
