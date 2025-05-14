# Makefile for HttpServer fixes

# Compiler and flags
CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++98 -I..

# Source files
SRCS = PathResolutionFix.cpp \
       SocketHandlingFix.cpp \
       ServerSocketFix.cpp \
       ClientDataFix.cpp

# Object files
OBJS = $(SRCS:.cpp=.o)

# Target library
TARGET = libhttpserverfixes.a

# Default target
all: $(TARGET)

# Build the library
$(TARGET): $(OBJS)
	ar rcs $@ $^

# Compile source files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up
clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
