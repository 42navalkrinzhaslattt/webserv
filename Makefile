# Authors: tischmid (timo42@proton.me)
#          sveselov (sveselov@student.42berlin.de)

# config
NAME := webserv
# change to c for C projects and cpp for C++ projects
# source files must still be specified with their extension
EXT := cpp
UNIT_TEST_DIR := test/unit_tests

# tools
# Not using CXX since it defaults to g++, thus making it impossible to use another default while also using ?= assignment
_CXX ?= c++
#_CXX := clang++ # TODO: @all: make sure it compiles with this
#_CXX := g++     # TODO: @all: make sure it compiles with this
RM := /bin/rm -f
MKDIR := /bin/mkdir -p

# flags
CPPFLAGS := -MMD -MP # dependency tracking flags
CFLAGS := -Wall -Wextra -Werror
CXXFLAGS := -std=c++98 -pedantic
LDFLAGS :=
LDLIBS :=

# debug flags
ifeq ($(DEBUG),1)
	CPPFLAGS += -DDEBUG
	CFLAGS += -g3 -O0
	CXXFLAGS += -g3 -O0
endif

# coverage flags
ifeq ($(COVERAGE),1)
	CPPFLAGS += -DCOVERAGE
	CFLAGS += -fprofile-instr-generate -fcoverage-mapping
	CXXFLAGS += -fprofile-instr-generate -fcoverage-mapping
	LDFLAGS += -fprofile-instr-generate
endif

# sanitizer flags
ifeq ($(ASAN),1)
	CPPFLAGS += -DASAN
	CFLAGS += -fsanitize=address -fno-omit-frame-pointer
	CXXFLAGS += -fsanitize=address -fno-omit-frame-pointer
	LDFLAGS += -fsanitize=address
endif

# includes
INCLUDES := -I./src

# defines
DEFINES :=

# sources
vpath %.$(EXT) src
SRC += Ansi.cpp
SRC += CgiHandler.cpp
SRC += Constants.cpp
SRC += DirectoryIndexer.cpp
SRC += Logger.cpp
SRC += Options.cpp
SRC += Repr.cpp
SRC += Utils.cpp
SRC += main.cpp # translation unit with int main(){} MUST be called main.cpp for unit tests to work, see object vars logic below

# HttpServer implementation
vpath %.$(EXT) src/HttpServer
SRC += HttpServer.cpp
SRC += AddingClientSockets.cpp
SRC += CgiHandler.cpp
# Include LocationConfig.cpp from HttpServer directory
SRC += LocationConfig.cpp
vpath LocationConfig.cpp src/HttpServer
SRC += EventMonitoring.cpp
SRC += GetRequestHandling.cpp
SRC += DeleteRequestHandling.cpp
SRC += PostRequestHandling.cpp
SRC += InitMimeTypes.cpp
SRC += InitStatusTexts.cpp
SRC += LocationMatching.cpp
SRC += RequestHandling.cpp
SRC += RequestParsing.cpp
SRC += RedirectHandling.cpp
SRC += ResponseSending.cpp
SRC += RemovingClientSockets.cpp
SRC += Setup.cpp
SRC += SocketManagement.cpp
SRC += SocketUtils.cpp
SRC += StaticContent.cpp
SRC += TimeoutHandling.cpp
SRC += UriCanonicalization.cpp
SRC += Uploads.cpp
SRC += ConfigParsing.cpp

# object vars
OBJ := $(SRC:.$(EXT)=.o)
OBJDIR := obj
OBJ := $(addprefix $(OBJDIR)/,$(OBJ))

# deps (relink also when header files change)
DEPS := $(OBJ:.o=.d)
-include $(DEPS)

# rules
.DEFAULT_GOAL := all

## Build project
all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(sort $(OBJ)) $(LDFLAGS) $(LDLIBS) -o $@

$(OBJDIR)/%.o: %.$(EXT) | $(OBJDIR)
	$(CXX) $< $(CPPFLAGS) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) -c -o $@

$(OBJDIR):
	$(MKDIR) $@

## Build unit tests (using local catch.hpp)
test:
	$(MAKE) -C $(UNIT_TEST_DIR)

# Cleanup
## Remove intermediate files
clean:
	$(RM) $(OBJ)
	$(RM) $(DEPS)
	$(RM) -r $(OBJDIR)
	$(RM) -r llvm_profdata
	$(RM) -r lcov_report
	$(RM) -r gcovr_report
	$(MAKE) -C $(UNIT_TEST_DIR) clean

## Remove intermediate files as well as well as build artefacts
fclean: clean
	$(RM) $(NAME)

## Remove intermediate files, build artefacts, and untracked files (interactively)
pristine: fclean
	git clean -dfi

## Rebuild project
re: fclean
	$(MAKE) all

## Build project, then run
run: all
	@printf '\n'
	@# This allows $(NAME) to be run using either an absolute, relative or no path.
	@# You can pass arguments like this: make run ARGS="hello ' to this world ! ' ."
	@PATH=".$${PATH:+:$${PATH}}" && $(NAME) $(ARGS)

## Build project, then run with debug loglevel
debug: all
	@printf '\n'
	@PATH=".$${PATH:+:$${PATH}}" && $(NAME) -l DEBUG $(ARGS)

## Build project, then run with trace loglevel
trace: all
	@printf '\n'
	@PATH=".$${PATH:+:$${PATH}}" && $(NAME) -l TRACE $(ARGS)

## Build project, then run with trace2 loglevel
trace2: all
	@printf '\n'
	@PATH=".$${PATH:+:$${PATH}}" && $(NAME) -l TRACE2 $(ARGS)

## Build project, then run with trace3 loglevel
trace3: all
	@printf '\n'
	@PATH=".$${PATH:+:$${PATH}}" && $(NAME) -l TRACE3 $(ARGS)

## Build project, then run with trace4 loglevel
trace4: all
	@printf '\n'
	@PATH=".$${PATH:+:$${PATH}}" && $(NAME) -l TRACE4 $(ARGS)

## Build project, then run with trace5 loglevel
trace5: all
	@printf '\n'
	@PATH=".$${PATH:+:$${PATH}}" && $(NAME) -l TRACE5 $(ARGS)

## Build project, then run with trace6 loglevel
trace6: all
	@printf '\n'
	@PATH=".$${PATH:+:$${PATH}}" && $(NAME) -l TRACE6 $(ARGS)

## Build project, then run with trace7 loglevel
trace7: all
	@printf '\n'
	@PATH=".$${PATH:+:$${PATH}}" && $(NAME) -l TRACE7 $(ARGS)

## Build project, then run with trace8 loglevel
trace8: all
	@printf '\n'
	@PATH=".$${PATH:+:$${PATH}}" && $(NAME) -l TRACE8 $(ARGS)

## Build project, then run with trace9 loglevel
trace9: all
	@printf '\n'
	@PATH=".$${PATH:+:$${PATH}}" && $(NAME) -l TRACE9 $(ARGS)

## Build project, then run with alternative config file
run_alt: all
	@printf '\n'
	@PATH=".$${PATH:+:$${PATH}}" && $(NAME) -c $(ARGS)

## Build project, then test config file
test_conf: all
	@printf '\n'
	@PATH=".$${PATH:+:$${PATH}}" && $(NAME) -t $(ARGS)

## Build project, then test config file and dump it
test_conf_dump: all
	@printf '\n'
	@PATH=".$${PATH:+:$${PATH}}" && $(NAME) -T $(ARGS)

## Build project, then run unit tests
unit_tests: all_c2
	@printf '\n'
	@PATH=".$${PATH:+:$${PATH}}" && $(TEST) $(ARGS)

## Build project, then run unit tests with debug loglevel
unit_tests_debug: all_c2
	@printf '\n'
	@PATH=".$${PATH:+:$${PATH}}" && $(TEST) -l DEBUG $(ARGS)

## Build project, then run unit tests with trace loglevel
unit_tests_trace: all_c2
	@printf '\n'
	@PATH=".$${PATH:+:$${PATH}}" && $(TEST) -l TRACE $(ARGS)

## Build project, then run unit tests with trace2 loglevel
unit_tests_trace2: all_c2
	@printf '\n'
	@PATH=".$${PATH:+:$${PATH}}" && $(TEST) -l TRACE2 $(ARGS)

## Build project, then run unit tests with trace3 loglevel
unit_tests_trace3: all_c2
	@printf '\n'
	@PATH=".$${PATH:+:$${PATH}}" && $(TEST) -l TRACE3 $(ARGS)

## Build project, then run unit tests with trace4 loglevel
unit_tests_trace4: all_c2
	@printf '\n'
	@PATH=".$${PATH:+:$${PATH}}" && $(TEST) -l TRACE4 $(ARGS)

## Build project, then run unit tests with trace5 loglevel
unit_tests_trace5: all_c2
	@printf '\n'
	@PATH=".$${PATH:+:$${PATH}}" && $(TEST) -l TRACE5 $(ARGS)

## Build project, then run unit tests with trace6 loglevel
unit_tests_trace6: all_c2
	@printf '\n'
	@PATH=".$${PATH:+:$${PATH}}" && $(TEST) -l TRACE6 $(ARGS)

## Build project, then run unit tests with trace7 loglevel
unit_tests_trace7: all_c2
	@printf '\n'
	@PATH=".$${PATH:+:$${PATH}}" && $(TEST) -l TRACE7 $(ARGS)

## Build project, then run unit tests with trace8 loglevel
unit_tests_trace8: all_c2
	@printf '\n'
	@PATH=".$${PATH:+:$${PATH}}" && $(TEST) -l TRACE8 $(ARGS)

## Build project, then run unit tests with trace9 loglevel
unit_tests_trace9: all_c2
	@printf '\n'
	@PATH=".$${PATH:+:$${PATH}}" && $(TEST) -l TRACE9 $(ARGS)

## Build project, then generate llvm coverage report
llvmcov:
	$(TOOLS)/llvmcov.sh

## Rebuild project, then generate llvm coverage report
rellvmcov: fclean
	$(MAKE) llvmcov

## Build project, then generate lcov coverage report
lcov:
	$(TOOLS)/lcov.sh

## Rebuild project, then generate lcov coverage report
relcov: fclean
	$(MAKE) lcov

## Build project, then generate gcovr coverage report
gcovr:
	$(TOOLS)/gcovr.sh

## Rebuild project, then generate gcovr coverage report
regcovr: fclean
	$(MAKE) gcovr

### Display this helpful message
h help:
	@printf '\033[31m%b\033[m\n\nTARGETs:\n' "USAGE:\n\tmake <TARGET> [ARGS=\"\"]"
	@grep -B1 -E "^[a-zA-Z0-9_-]+:.*## .*$$" $(MAKEFILE_LIST) | \
		awk 'BEGIN {FS = ":.*?## "}; { \
			if ($$0 ~ /^--/) { \
				printf "\033[90m%s\033[0m\n", $$1; \
			} else if ($$0 ~ /^[a-zA-Z0-9_-]+:.*?## /) { \
				printf "\033[34m%-30s\033[0m %s\n", $$1, $$2; \
			} else if ($$0 ~ /^##/) { \
				printf "\n\033[33m%s\033[0m\n", substr($$0, 3); \
			} \
		}'

.PHONY: all all_c2 clean fclean pristine re run debug trace trace2 trace3 trace4 trace5 trace6 trace7 trace8 trace9 run_alt test_conf test_conf_dump unit_tests unit_tests_debug unit_tests_trace unit_tests_trace2 unit_tests_trace3 unit_tests_trace4 unit_tests_trace5 unit_tests_trace6 unit_tests_trace7 unit_tests_trace8 unit_tests_trace9 llvmcov rellvmcov lcov relcov gcovr regcovr h help
