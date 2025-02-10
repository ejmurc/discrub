CC ?= gcc
CFLAGS = -std=c89 -Ofast
INCLUDES = -Iinclude/ $(shell pkg-config openssl --cflags)
LIBS = $(shell pkg-config openssl --libs)
BUILD = build
SRCS = $(wildcard src/*.c)
HEADERS = $(wildcard include/*.h)
OBJS = $(SRCS:src/%.c=$(BUILD)/%.o)

.PHONY: release clean format

release: $(BUILD)/discrub
	@printf "\033[1;32mProduction build completed successfully.\033[0m\n"

$(BUILD)/discrub: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

$(BUILD)/%.o: src/%.c | $(BUILD)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD):
	@mkdir -p $(BUILD)

clean:
	@rm -rf $(BUILD)
	@printf "\33[2K\033[1;31mCleaned build directory.\033[0m\n"

format:
	@clang-format -i $(SRCS) $(HEADERS) || \
		(echo "\33[2K\033[1;31mFailed to format files.\033[0m"; exit 1)
	@printf "\33[2K\033[1;32mFormatted source and header files.\033[0m\n"
