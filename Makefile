CC = gcc
CPPFLAGS = -MMD
CFLAGS = -Wall -Wextra -pedantic -Werror -std=c99 -g
LDFLAGS = -mconsole -mwindows
LDLIBS = -lcomctl32

SRC_DIR = ./src
SRC = $(wildcard $(SRC_DIR)/*.c)
RC = $(wildcard $(SRC_DIR)/*.rc)

TARGET = hltt.exe
BUILD_DIR = ./build
OBJ = $(SRC:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
OBJ += $(RC:$(SRC_DIR)/%.rc=$(BUILD_DIR)/%.o)
DEP = $(OBJ:%.o=%.d)

TEST_TARGET = test_$(TARGET)
TEST_DIR = $(BUILD_DIR)/test
TEST_OBJ = $(SRC:$(SRC_DIR)/%.c=$(TEST_DIR)/%.o)
TEST_DEP = $(TEST_OBJ:%.o=%.d)

.PHONY: all clean run test debug disasm

all: $(BUILD_DIR)/$(TARGET)
	$(info $(OBJ))

clean:
	rm -rf $(BUILD_DIR)

ARGS ?=
run: $(BUILD_DIR)/$(TARGET)
	$(BUILD_DIR)/$(TARGET) $(ARGS)

test: CPPFLAGS += -DTEST
test: $(TEST_DIR)/$(TEST_TARGET)
	$(TEST_DIR)/$(TEST_TARGET)

debug: all
	gdb $(BUILD_DIR)/$(TARGET)

disasm: $(BUILD_DIR)/disasm.out


# ==== BUILD ====

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.rc
	mkdir -p $(BUILD_DIR)
	windres -o $@ $<

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/$(TARGET): $(OBJ)
	mkdir -p $(BUILD_DIR)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

-include $(DEP)

# ==== TEST ====

$(TEST_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(TEST_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

$(TEST_DIR)/$(TEST_TARGET): $(TEST_OBJ)
	mkdir -p $(TEST_DIR)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

-include $(TEST_DEP)

$(BUILD_DIR)/disasm.out: $(BUILD_DIR)/$(TARGET)
	objdump -dS $< > $@
