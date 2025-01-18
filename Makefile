CC ?= gcc
CFLAGS = -std=c89 -Ofast -Wall -Wextra -Iinclude/
BUILD = build
SRCS = $(wildcard src/*.c)
HEADERS = $(wildcard include/*.h)
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

prod: $(BUILD)/discrub
	@printf "$(COLOR_GREEN)Build completed successfully.$(COLOR_RESET)\n"

dev: CFLAGS := -std=c89 -Wall -Wextra -Iinclude/ -g -fsanitize=address
dev: $(BUILD)/discrub
	@printf "$(COLOR_YELLOW)Development build completed successfully.$(COLOR_RESET)\n"

$(BUILD)/discrub: $(OBJS)
	@printf "$(CLEAR_LINE)$(COLOR_BLUE)Linking executable...$(COLOR_RESET)\r"
	@if $(CC) $(CFLAGS) -o $@ $^ $(LIBS); then \
		printf "$(CLEAR_LINE)$(COLOR_GREEN)Linked executable$(COLOR_RESET)\r"; \
	else \
		printf "$(CLEAR_LINE)$(COLOR_RED)Linking failed$(COLOR_RESET)\n"; \
		exit 1; \
	fi

$(BUILD)/%.o: src/%.c | $(BUILD)
	@printf "$(CLEAR_LINE)$(COLOR_BLUE)Compiling $<...$(COLOR_RESET)\r"
	@if $(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@; then \
		printf "$(CLEAR_LINE)$(COLOR_GREEN)Compiled $<$(COLOR_RESET)\r"; \
	else \
		printf "$(CLEAR_LINE)$(COLOR_RED)Compilation failed$<$(COLOR_RESET)\n"; \
		exit 1; \
	fi

$(BUILD):
	@if [ ! -d "$(BUILD)" ]; then \
		printf "$(CLEAR_LINE)$(COLOR_YELLOW)Creating build directory...$(COLOR_RESET)"; \
		mkdir -p $(BUILD); \
	fi

format:
	@printf "$(COLOR_BLUE)Checking for clang-format...$(COLOR_RESET)\r"
	@if ! command -v clang-format &> /dev/null; then \
		printf "$(CLEAR_LINE)$(COLOR_RED)clang-format could not be found. Exiting...$(COLOR_RESET)\n"; \
		exit 1; \
	fi
	@printf "$(CLEAR_LINE)$(COLOR_YELLOW)Formatting source and header files...$(COLOR_RESET)\n"
	@for file in $(SRCS) $(HEADERS); do \
		printf "$(CLEAR_LINE)$(COLOR_BLUE)Formatting $$file...$(COLOR_RESET)\r"; \
		if ! clang-format -i $$file; then \
			printf "$(CLEAR_LINE)$(COLOR_RED)Failed to format: $$file. Exiting...$(COLOR_RESET)\n"; \
			exit 1; \
		fi; \
		printf "$(CLEAR_LINE)$(COLOR_GREEN)Formatted $$file$(COLOR_RESET)\r"; \
	done
	@printf "$(CLEAR_LINE)$(COLOR_GREEN)Formatted source and header files$(COLOR_RESET)\n"

clean:
	@printf "$(CLEAR_LINE)$(COLOR_RED)Cleaning build directory...$(COLOR_RESET)\r"
	@if rm -rf $(BUILD); then \
		printf "$(CLEAR_LINE)$(COLOR_RED)Cleaned build directory$(COLOR_RESET)\r"; \
	else \
		printf "$(CLEAR_LINE)$(COLOR_RED)Failed to clean build directory$(COLOR_RESET)\n"; \
		exit 1; \
	fi
	@printf "\n"
