MAKEFILE_DIR = $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
TESTS_BIN = $(MAKEFILE_DIR)/build/cserver_tests

CC = clang
CXX = clang++

CMAKE_CMD ?= cmake
CMAKE_BUILD_TYPE ?= Debug

ifdef CI
  CMAKE_GENERATOR ?= "Unix Makefiles"
else
  CMAKE_GENERATOR ?= Ninja
endif

CMAKE_FLAGS += -DCMAKE_CXX_COMPILER=$(CXX) \
               -DCMAKE_C_COMPILER=$(CC) \
               -DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) \
               -DCMAKE_EXPORT_COMPILE_COMMANDS=1 \
               -DENABLE_TESTS=1 \
               -DENABLE_EXAMPLES=1

# Only need to handle Ninja here.
# Make will inherit the VERBOSE variable, and the -j, -l, and -n flags.
ifeq ($(CMAKE_GENERATOR),Ninja)
	BUILD_TOOL = ninja
	ifneq ($(VERBOSE),)
		BUILD_TOOL += -v
	endif
	BUILD_TOOL += $(shell printf '%s' '$(MAKEFLAGS)' | grep -o -- ' *-[jl][0-9]\+ *')
	ifeq (n,$(findstring n,$(firstword -$(MAKEFLAGS))))
		BUILD_TOOL += -n
	endif
else
	BUILD_TOOL = $(MAKE)
endif

ifdef CI
  FORMAT_FLAGS = -n --Werror
else
  FORMAT_FLAGS = -i
endif

all: $(TESTS_BIN)

build/.ran-cmake:
	$(CMAKE_CMD) -S . -B build -G $(CMAKE_GENERATOR) $(CMAKE_FLAGS)
	touch $@

$(TESTS_BIN): build/.ran-cmake FORCE
	+$(BUILD_TOOL) -C build

cmake:
	touch CMakeLists.txt
	$(MAKE) build/.ran-cmake

format:
	clang-format $(FORMAT_FLAGS) \
		$(shell find include -name *.hpp) \
		$(shell find examples -name *.cpp) \
		$(shell find tests -name *.cpp)

clean:
	+test -d build && $(BUILD_TOOL) -C build clean || true

distclean:
	rm -rf build

test: $(TESTS_BIN)
	$(TESTS_BIN)

tidy: build/.ran-cmake
	clang-tidy -p build \
		$(shell find include -name *.hpp) \
		$(shell find examples -name *.cpp) \
		$(shell find tests -name *.cpp)

FORCE: ;

.PHONY: all cmake test format clean distclean