# make_save: AddrBoy save file generator, built from the ROM's own storage and validation code.
#   make -C tools                      build tools/build/make_save
#   make -C tools check                build, write build/example.sav, and check a bad file is refused
CXX ?= g++
CXXFLAGS := -std=c++20 -O2 -Wall -Wextra -Werror -I../include -I../tests/stubs
SOURCES := make_save.cpp ../src/storage.cpp ../src/validation.cpp

build/make_save: $(SOURCES) $(wildcard ../include/*.h) ../tests/stubs/bn_sram.h
	mkdir -p build
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $@

check: build/make_save
	./build/make_save wallets.example.json build/example.sav
	@printf '{"wallets":[{"name":"Bad","type":"hot","evm":"0x5AAeb6053F3E94C9b9A09f33669435E7Ef1BeAed"}]}' \
		> build/bad.json
	@if ./build/make_save build/bad.json build/bad.sav; then echo "bad.json was accepted"; exit 1; \
		else echo "bad.json refused as expected"; fi
	@test ! -e build/bad.sav

.PHONY: check
