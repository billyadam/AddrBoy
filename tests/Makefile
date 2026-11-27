CXX ?= g++
CXXFLAGS := -std=c++20 -O2 -g -Wall -Wextra -Werror -I../include -Istubs -I../third_party/qrcodegen
SOURCES := core_test.cpp ../src/controller.cpp ../src/storage.cpp ../src/validation.cpp ../src/explorers.cpp ../src/dates.cpp

run: build/core_test
	./build/core_test

build/qrcodegen.o: ../third_party/qrcodegen/qrcodegen.c ../third_party/qrcodegen/qrcodegen.h
	mkdir -p build
	$(CC) -O2 -Wall -Wextra -Werror -c $< -o $@

build/core_test: $(SOURCES) $(wildcard ../include/*.h) stubs/bn_sram.h build/qrcodegen.o
	mkdir -p build
	$(CXX) $(CXXFLAGS) $(SOURCES) build/qrcodegen.o -o $@

sanitize: build/qrcodegen.o
	mkdir -p build
	$(CXX) $(CXXFLAGS) -O1 -fsanitize=address,undefined -fno-omit-frame-pointer $(SOURCES) build/qrcodegen.o -o build/core_test_sanitize
	./build/core_test_sanitize

.PHONY: run sanitize
