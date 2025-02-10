CC ?= gcc
CFLAGS = -std=c89 -Wall -Wextra
INCLUDES = $(shell pkg-config openssl --cflags)
LIBS = $(shell pkg-config openssl --libs)
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
