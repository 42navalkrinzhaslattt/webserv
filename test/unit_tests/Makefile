CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -pedantic -I../../src -I../..
LDFLAGS =

# All test sources
SRCS = main.cpp \
       HttpStatusTests.cpp \
       CGITests.cpp \
       HttpMethodsTests.cpp \
       FileUploadTests.cpp \
       StatusCodeTests.cpp \
       UnknownRequestTests.cpp \
       PortConfigTests.cpp \
       StressTests.cpp

OBJS = $(SRCS:.cpp=.o)
TARGET = webserv_tests

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
