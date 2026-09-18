mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
PWD := $(dir $(mkfile_path))
VCPKG_ROOT_PATH ?= $(if $(VCPKG_ROOT),$(VCPKG_ROOT),~/vcpkg)
VCPKG_EXEC := $(VCPKG_ROOT_PATH)/vcpkg
VCPKG_TOOLCHAIN := $(VCPKG_ROOT_PATH)/scripts/buildsystems/vcpkg.cmake
TOOLCHAIN_ARG := $(if $(VCPKG_ROOT_PATH),-DCMAKE_TOOLCHAIN_FILE=$(VCPKG_TOOLCHAIN),)
BUILD_TYPE ?= Release
DEFAULT_APP_EXECUTABLE := build/bin/malasim
# Capture the second word in MAKECMDGOALS (if it exists)
APP_EXECUTABLE ?= $(or $(word 2,$(MAKECMDGOALS)),$(DEFAULT_APP_EXECUTABLE))
ENABLE_TRAVEL_TRACKING ?= OFF
BUILD_TESTS ?= OFF
ENABLE_PCH ?= ON
DOCS_OUTPUT_DIR := docs/Doxygen
CLANG_TIDY ?= clang-tidy
LINT_SOURCES := $(shell find src tests -type f -name '*.cpp')
CLANG_TIDY_HEADER_FILTER ?= .*
CCACHE := $(shell command -v ccache 2>/dev/null)
CCACHE_ARGS := $(if $(CCACHE),-DCMAKE_C_COMPILER_LAUNCHER=$(CCACHE) -DCMAKE_CXX_COMPILER_LAUNCHER=$(CCACHE),)

ifeq ($(ENABLE_PCH),ON)
CCACHE_SLOPPINESS ?= pch_defines,time_macros
export CCACHE_SLOPPINESS
endif

ifeq ($(shell uname -s),Darwin)
MACOS_SDK ?= $(shell xcrun --show-sdk-path 2>/dev/null)
CLANG_TIDY_PLATFORM_ARGS := $(if $(MACOS_SDK),--extra-arg=-isysroot --extra-arg=$(MACOS_SDK) --extra-arg=-stdlib=libc++,)
endif

.PHONY: all build b clean setup-vcpkg install-deps generate g generate-no-test help test t run r coverage coverage-baseline lint

all: build

build b:
	@echo "Building the project..."
	cmake --build build --config $(BUILD_TYPE) -j 6

test t: build
	cd build && GTEST_COLOR=1 ctest -V $(ARGS)

gtest: build
	cd build/bin && ./malasim_test --gtest_filter=$(filter)

run r: build 
	./$(APP_EXECUTABLE)

clean:
	rm -rf build

setup-vcpkg:
	if [ -n "$(VCPKG_ROOT_PATH)" ] && [ ! -x "$(VCPKG_EXEC)" ]; then \
		git submodule update --init; \
		$(VCPKG_ROOT_PATH)/bootstrap-vcpkg.sh; \
	fi

install-deps: setup-vcpkg
	[ -z "$(VCPKG_ROOT_PATH)" ] || $(VCPKG_EXEC) install

generate-test gt:
	@$(MAKE) generate BUILD_TESTS=ON

generate-coverage gc:
	@$(MAKE) generate BUILD_TESTS=ON ENABLE_COVERAGE=ON

generate g:
	cmake -Bbuild -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DENABLE_TRAVEL_TRACKING=$(ENABLE_TRAVEL_TRACKING) -DBUILD_TESTS=$(BUILD_TESTS) -DENABLE_COVERAGE=$(ENABLE_COVERAGE) -DENABLE_PCH=$(ENABLE_PCH) $(CCACHE_ARGS) $(TOOLCHAIN_ARG) .
	cp $(PWD)build/compile_commands.json $(PWD)

generate-ninja gn:
	cmake -Bbuild -GNinja -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DENABLE_TRAVEL_TRACKING=$(ENABLE_TRAVEL_TRACKING) -DBUILD_TESTS=$(BUILD_TESTS) -DENABLE_PCH=$(ENABLE_PCH) $(CCACHE_ARGS) $(TOOLCHAIN_ARG) .
	cp $(PWD)build/compile_commands.json $(PWD)

generate-gcc-12-owlsnest gog12:
	cmake -DCMAKE_CXX_COMPILER=/gpfs/opt/tools/gcc-12.2.0/bin/g++ -DCMAKE_C_COMPILER=/gpfs/opt/tools/gcc-12.2.0/bin/gcc  -DCMAKE_EXE_LINKER_FLAGS="-L/path/to/gcc-12.2.0/lib64" \
	-Bbuild -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DENABLE_TRAVEL_TRACKING=$(ENABLE_TRAVEL_TRACKING) -DBUILD_TESTS=$(BUILD_TESTS) -DENABLE_COVERAGE=$(ENABLE_COVERAGE) -DENABLE_PCH=$(ENABLE_PCH) $(CCACHE_ARGS) $(TOOLCHAIN_ARG) .
	cp $(PWD)build/compile_commands.json $(PWD)

coverage:
	./scripts/run_coverage.sh $(COVERAGE_ARGS)

coverage-baseline:
	./scripts/run_coverage.sh --update-baseline $(COVERAGE_ARGS)


format:
	find src tests -name '*.cpp' -o -name '*.h' | xargs clang-format -i --style=file

lint: build
	$(CLANG_TIDY) -p build --quiet $(CLANG_TIDY_PLATFORM_ARGS) \
		--header-filter='$(CLANG_TIDY_HEADER_FILTER)' $(LINT_SOURCES)

docs:
	@echo "Generating documentation..."
	mkdir -p $(DOCS_OUTPUT_DIR)  # Ensure the docs output directory exists
	doxygen Doxyfile  # Run Doxygen to generate the docs
	@echo "Documentation generated in $(DOCS_OUTPUT_DIR)"

.PHONY: docs  # Prevent make from interpreting 'docs' as a file

help h:
	@echo "Available targets:"
	@echo "  all                  : Default target, builds the project."
	@echo "  build (b)            : Build the project with specified BUILD_TYPE (default: Release)."
	@echo "  format (f)           : Format the source code using clang-format."
	@echo "  lint                 : Run clang-tidy on the source code."
	@echo "  test                 : Rebuild and run tests."
	@echo "  run [path]           : Rebuild and run the executable. Provide path to override default."
	@echo "  clean                : Remove the build directory."
	@echo "  setup-vcpkg          : Setup vcpkg if specified by VCPKG_ROOT."
	@echo "  install-deps         : Install dependencies using vcpkg."
	@echo "  generate (g)         : Generate the build system. Can specify BUILD_CLUSTER, ENABLE_TRAVEL_TRACKING, BUILD_TEST (e.g., make generate BUILD_CLUSTER=ON ENABLE_TRAVEL_TRACKING=ON)."
	@echo "  generate-ninja (gn)  : Generate the build system using Ninja."
	@echo "  generate-test (gt)   : Generate the build system with tests."
	@echo "  ENABLE_PCH=OFF        : Disable precompiled headers (enabled by default)."
	@echo "  coverage             : Run instrumented tests and save text, JSON, HTML, and history reports."
	@echo "  coverage-baseline    : Run full coverage and refresh the tracked baseline."
	@echo "  docs                 : Generate Doxygen documentation into $(DOCS_OUTPUT_DIR)."
	@echo "  help                 : Show this help message."
