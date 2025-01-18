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
	@printf "$(COLOR_GREEN)Build completed successfully.$(COLOR_RESET)\n"

dev: CFLAGS := -std=c89 -Wall -Wextra -Iinclude/ -g -fsanitize=address
dev: $(BUILD)/discrub
	@printf "$(COLOR_YELLOW)Development build completed successfully.$(COLOR_RESET)\n"

$(BUILD)/discrub: $(OBJS)
	@printf "$(CLEAR_LINE)$(COLOR_BLUE)Linking executable...$(COLOR_RESET)"
	@if $(CC) $(CFLAGS) -o $@ $^ $(LIBS); then \
		printf "$(CLEAR_LINE)$(COLOR_GREEN)Linking completed successfully.$(COLOR_RESET)\n"; \
	else \
		printf "$(CLEAR_LINE)$(COLOR_RED)Linking failed!$(COLOR_RESET)\n"; \
		exit 1; \
	fi

$(BUILD)/%.o: src/%.c | $(BUILD)
	@printf "$(CLEAR_LINE)$(COLOR_BLUE)Compiling $<...$(COLOR_RESET)"
	@if $(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@; then \
		printf "$(CLEAR_LINE)$(COLOR_GREEN)Compiled $< successfully.$(COLOR_RESET)\n"; \
	else \
		printf "$(CLEAR_LINE)$(COLOR_RED)Compilation of $< failed!$(COLOR_RESET)\n"; \
		exit 1; \
	fi

$(BUILD):
	@if [ ! -d "$(BUILD)" ]; then \
		printf "$(CLEAR_LINE)$(COLOR_YELLOW)Creating build directory...$(COLOR_RESET)"; \
		mkdir -p $(BUILD); \
	fi

format:
	@printf "$(CLEAR_LINE)$(COLOR_GREEN)Running code formatter...$(COLOR_RESET)"
	@if ./format.sh; then \
		printf "$(CLEAR_LINE)$(COLOR_GREEN)Code formatting completed successfully.$(COLOR_RESET)\n"; \
	else \
		printf "$(CLEAR_LINE)$(COLOR_RED)Code formatting failed!$(COLOR_RESET)\n"; \
		exit 1; \
	fi

clean:
	@printf "$(CLEAR_LINE)$(COLOR_RED)Cleaning build directory...$(COLOR_RESET)"
	@if rm -rf $(BUILD); then \
		printf "$(CLEAR_LINE)$(COLOR_RED)Clean completed successfully.$(COLOR_RESET)\n"; \
	else \
		printf "$(CLEAR_LINE)$(COLOR_RED)Clean failed!$(COLOR_RESET)\n"; \
		exit 1; \
	fi
	@printf "\n"
