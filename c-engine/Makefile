CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c99 -O2 -fPIC
LDFLAGS = -lsqlite3 -lssl -lcrypto

SRC_DIR = src
INCLUDE_DIR = include
OBJ_DIR = build
BIN_DIR = bin

SOURCES = $(SRC_DIR)/main.c $(SRC_DIR)/file_scanner.c $(SRC_DIR)/database.c $(SRC_DIR)/json_exporter.c
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
TARGET = $(BIN_DIR)/automation-engine

all: directories $(TARGET)

directories:
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(BIN_DIR)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "✓ Build complete: $(TARGET)"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -c $< -o $@

clean:
	@rm -rf $(OBJ_DIR) $(BIN_DIR)
	@echo "✓ Cleaned"

install: all
	@cp $(TARGET) /usr/local/bin/automation-engine
	@echo "✓ Installed to /usr/local/bin/"

uninstall:
	@rm -f /usr/local/bin/automation-engine
	@echo "✓ Uninstalled"

.PHONY: all clean install uninstall directories
