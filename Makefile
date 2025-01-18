CC ?= gcc
CFLAGS = -std=c89 -Ofast -Wall -Wextra -Iinclude/
BUILD = build
SRCS = $(wildcard src/*.c)
OBJS = $(SRCS:src/%.c=$(BUILD)/%.o)
LIBS = $(shell pkg-config sdl3 sdl3-ttf openssl --libs)
INCLUDES = $(shell pkg-config openssl sdl3 sdl3-ttf --cflags)

COLOR_RESET = \033[0m
COLOR_GREEN = \033[1;32m
COLOR_YELLOW = \033[1;33m
COLOR_BLUE = \033[1;34m
COLOR_RED = \033[1;31m
CLEAR_LINE = \033[2K\r

.PHONY: all dev format clean

all: $(BUILD)/discrub
	@printf "$(CLEAR_LINE)$(COLOR_GREEN)Build completed.$(COLOR_RESET)\n"

dev: CFLAGS := -std=c89 -Wall -Wextra -Iinclude/ -g -fsanitize=address
dev: $(BUILD)/discrub
	@printf "$(CLEAR_LINE)$(COLOR_YELLOW)Development build completed.$(COLOR_RESET)\n"

$(BUILD)/discrub: $(OBJS)
	@printf "$(CLEAR_LINE)$(COLOR_BLUE)Linking executable...$(COLOR_RESET)"
	@$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

$(BUILD)/%.o: src/%.c | $(BUILD)
	@printf "$(CLEAR_LINE)$(COLOR_BLUE)Compiling $<...$(COLOR_RESET)"
	@$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD):
	@if [ ! -d "$(BUILD)" ]; then \
		printf "$(CLEAR_LINE)$(COLOR_YELLOW)Creating build directory...$(COLOR_RESET)"; \
		mkdir -p $(BUILD); \
	fi

format:
	@printf "$(CLEAR_LINE)$(COLOR_GREEN)Running code formatter...$(COLOR_RESET)"
	./format.sh

clean:
	@printf "$(CLEAR_LINE)$(COLOR_RED)Cleaning build directory...$(COLOR_RESET)"
	@rm -rf $(BUILD)
