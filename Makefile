# Compiler
CC = gcc

# Compiler flags with debug and address sanitizer
CFLAGS = -I./include/ \
         -I/usr/local/include -DSQLITE_THREADSAFE=1 -DSQLITE_OMIT_LOAD_EXTENSION

# Linker flags
LDFLAGS = -L/usr/local/lib -Wl,-rpath,/usr/local/lib

# Debug flags
DEBUG_FLAGS = -g -O0

# Libraries to link against
LIBS = -lhiredis -lpthread -lcsv -lcjson -lm

# Target executable name
TARGET = bin/main

# Source files: find all .c files in src and its subdirectories
SRCS = $(shell find src -name '*.c')

# Build rules
all: bin $(TARGET)

# Debug build rules
debug: CFLAGS += $(DEBUG_FLAGS)
debug: bin $(TARGET)

# Ensure required directories exist
bin:
	mkdir -p bin

# Compile and link all source files in a single step
$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET) $(LDFLAGS) $(LIBS)

# Clean up the target executable
clean:
	rm -rf $(TARGET)

# Run the program
run: $(TARGET)
	./$(TARGET) kmeans
