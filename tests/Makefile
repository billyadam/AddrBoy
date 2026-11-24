CXX ?= g++
CXXFLAGS := -std=c++20 -O2 -g -Wall -Wextra -Werror -I../include -Istubs
SOURCES := core_test.cpp ../src/controller.cpp ../src/storage.cpp ../src/validation.cpp

run: build/core_test
	./build/core_test

build/core_test: $(SOURCES) $(wildcard ../include/*.h) stubs/bn_sram.h
	mkdir -p build
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $@

sanitize:
	mkdir -p build
	$(CXX) $(CXXFLAGS) -O1 -fsanitize=address,undefined -fno-omit-frame-pointer $(SOURCES) -o build/core_test_sanitize
	./build/core_test_sanitize

.PHONY: run sanitize
