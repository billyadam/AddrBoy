# make_save: AddrBoy save file generator, built from the ROM's own storage and validation code.
#   make -C tools                      build tools/build/make_save
#   make -C tools check                build, write build/example.sav, and check a bad file is refused
CXX ?= g++
CXXFLAGS := -std=c++20 -O2 -Wall -Wextra -Werror -I../include -I../tests/stubs
SOURCES := make_save.cpp ../src/storage.cpp ../src/validation.cpp ../src/dates.cpp

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
	@printf '{"wallets":[{"name":"Deposit","type":"exchange","btc":"bc1qw508d6qejxtdg4y5r3zarvary0c5xw7kv8f3t4","checked_on":{"btc":"2026-02-29"}}]}' \
		> build/bad-date.json
	@if ./build/make_save build/bad-date.json build/bad-date.sav; then echo "bad-date.json was accepted"; exit 1; \
		else echo "bad-date.json refused as expected"; fi
	@test ! -e build/bad-date.sav

.PHONY: check
