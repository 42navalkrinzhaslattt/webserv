CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -I../../src

CONFIG_TEST = config_test
CONFIG_TEST_SRCS = config_tests/ConfigParserTest.cpp ../../src/Config.cpp ../../src/Utils.cpp ../../src/Errors.cpp
CONFIG_TEST_OBJS = $(CONFIG_TEST_SRCS:.cpp=.o)

HTTP_TEST = http_test
HTTP_TEST_SRCS = http_tests/HeaderParserTest.cpp ../../src/Server.cpp ../../src/Utils.cpp ../../src/ParseHeader.cpp
HTTP_TEST_OBJS = $(HTTP_TEST_SRCS:.cpp=.o)

all: $(CONFIG_TEST) $(HTTP_TEST)

$(CONFIG_TEST): $(CONFIG_TEST_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(HTTP_TEST): $(HTTP_TEST_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -f $(CONFIG_TEST_OBJS) $(HTTP_TEST_OBJS)

fclean: clean
	rm -f $(CONFIG_TEST) $(HTTP_TEST)

re: fclean all

test: all
	@echo "\nRunning all tests..."
	@echo "\nRunning Config Parser Tests..."
	@./$(CONFIG_TEST)
	@echo "\nRunning HTTP Parser Tests..."
	@./$(HTTP_TEST)

.PHONY: all clean fclean re test
