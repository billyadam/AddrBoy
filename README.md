# AddrBoy

A standalone Game Boy Advance wallet address book, built with Butano. Pick a
wallet, pick a network, and display its public receiving address and QR code for
another device to scan. Version **0.1.0**.

## Features

- Up to 64 wallets with names up to 20 characters, listed A-Z, Z-A, or in a custom order
  (AuthBoy's sort; Move rearranges the custom order).
- Cold, Hot, Exchange, and External (someone else's wallet) types; a persistent
  ALL / COLD / HOT / EXCH / EXT filter.
- Five optional address slots per wallet: BTC, EVM, SOL, SUI, and APT.
- Create, edit, and delete wallets; add, replace, or remove individual addresses.
- Receive screen with the complete case-sensitive address in groups of four.
- QR links to a mainnet explorer for each network: Blockstream, Solscan, Suiscan, Aptos Explorer,
  and Etherscan, Basescan, Arbiscan, or Robinhood Chain for EVM.
- An About section (SELECT in Settings), as in AuthBoy, with the project link as a QR Code.
- A welcome screen on boot, as in AuthBoy, with the "Book Boy" logo (a handheld drawn as an
  address book) in the theme's colors.
- Field Guide, Net Link, Pocket LCD, and Phosphor themes, based on local copies of
  AuthBoy's renderer, fonts, and palettes.
- Local SRAM saves with checksums and copy-on-write wallet updates. No RTC needed.

AddrBoy stores public receiving addresses. It does not hold private keys, sign
transactions, connect to a blockchain, or determine a balance. The EVM slot is a
shared address entry; select the intended EVM network in the sending wallet.

## Build

Requires devkitPro's GBA toolchain, Python 3, and make. The required Butano 21.10.1
core and common assets are included locally; AuthBoy is not a build dependency.

```sh
make -j4 DEVKITPRO=/opt/devkitpro DEVKITARM=/opt/devkitpro/devkitARM
```

The output is `AddrBoy.gba`. Open it in a GBA emulator or on compatible hardware
with SRAM saves. Keep the emulator's `.sav` file with the ROM to retain wallets.
The default build starts with an empty address book; test wallets are never
embedded in the ROM.

## Controls

| Screen | Controls |
| --- | --- |
| Welcome | A or START continue |
| Wallets | Up/Down select; A focus networks; L/R cycle filter; START menu; SELECT settings |
| Networks | Up/Down select; A choose QR; B wallets; L/R filter and return to wallets; START menu; SELECT settings |
| QR choices | Up/Down choose Address or a network explorer; A show QR; B networks |
| Address QR | Left/Right cycle configured networks, wrapping; B QR choices; START menu |
| Explorer QR | B QR choices; START menu |
| Menu | New wallet / Edit wallet / Networks / Move (custom order only) / Delete: Up/Down select; A choose; B or START close |
| Moving a wallet | Up/Down move it (stops at the ends; saved each step); A or B done |
| Name | D-pad select key; A type; B delete; R space; L discard; SELECT toggle case; START next |
| Type | Up/Down choose Cold / Hot / Exchange / External; A next; B name |
| Networks setup | Up/Down select; A add/edit address; B back; START save wallet |
| Address entry | D-pad select key; A type; B delete; SELECT toggle case; START validate and accept |
| Settings | Up/Down choose Sort or Theme; A or Left/Right change it; SELECT About; B or START return |
| About | Left/Right or L/R turn the 4 pages (About, Good to know, How it works, Libraries); B or SELECT back |

SELECT opens Settings from the wallets and networks panels (as in AuthBoy) and switches
between the ABC and abc tabs on the keyboards; it is unused elsewhere. The name keyboard
follows AuthBoy's: its keys end in `- . _ '`, R types a space (not as the first character),
and L opens the discard confirmation, as B does on an empty name field. The home screen's
right panel and the network setup list show each address's last four characters (`..18fd`);
the receive screen shows it in full. On an empty address field, B returns to network
setup without changing the draft address.

To remove an existing address, erase the field and press START, then confirm.
To discard changes made through the menu's Networks action, press B from network
setup and confirm. Address edits remain in the draft until START saves the wallet
from network setup. Wallets with zero addresses are allowed.

## Address validation and QR behavior

- BTC: mainnet Native SegWit (`bc1q`, bech32, a 20- or 32-byte program), Taproot (`bc1p`,
  bech32m, a 32-byte program), and Base58Check Legacy (`1`) and P2SH (`3`), each with its full
  checksum and length checks (plus witness padding for `bc1`). Other witness versions (2-16)
  are refused, as no wallet uses them. Maximum 62 characters. The receive screen labels the
  format: N. SegWit, Taproot, Legacy, or P2SH.
- EVM: `0x` plus 40 hexadecimal digits. Mixed-case addresses must pass EIP-55.
- SOL: 32–44 Base58 characters that decode to exactly 32 bytes.
- SUI: `0x` plus 64 hexadecimal digits.
- APT: `0x` plus 1–64 hexadecimal digits, left-padded to 64 on acceptance.

The counter reflects validation while typing. START shows an explanatory error
for invalid input and keeps the editor open. QR data is the exact saved address,
with no URI scheme or formatting spaces, in byte mode at ECC L. Explorer choices instead
encode an HTTPS address page URL for the selected network. Explorer QR codes are for
opening a browser, not for receiving funds. The QR matrix is cached; rendering only
happens after an input changes the screen.

The QR panel is 116×116 (the handoff's is 114×114), so the theme's full panel frame
surrounds a 112×112 white area: version 3 fits at 3 pixels per module with a full
four-module quiet zone (111 pixels); version 4 uses 2 pixels per module. The handoff's
114×114 panel leaves only 110 pixels inside a 2-pixel frame, too small for version 3 at
3 pixels per module with its quiet zone.
Explorer URLs use up to version 5, at 2 pixels per module. The QR is pure black on white
in all themes.

Validation references: [EIP-55](https://eips.ethereum.org/EIPS/eip-55),
[BIP-173](https://github.com/bitcoin/bips/blob/master/bip-0173.mediawiki),
[BIP-350](https://github.com/bitcoin/bips/blob/master/bip-0350.mediawiki), and
[DENSO WAVE's QR quiet-zone guidance](https://www.qrcode.com/en/howto/code.html).

## Save format

Public addresses are saved unencrypted in 32 KB SRAM. Two CRC32-protected
directories record wallet order, category filter, theme, sort order, and a generation number.
65 checksummed record slots provide space for 64 wallets and one spare slot. This is
save format 3 (20-character names, sort order); saves from earlier builds are not read.
A Move step swaps two directory entries in one commit, so no record is rewritten.
An edit writes and verifies the spare record before committing the new directory.
Deletion commits a new directory without moving the remaining records.

On boot, the newest valid directory is selected. An invalid directory can fall
back to the previous generation when its referenced records remain intact. A
save with no valid directory is left untouched and displays a recovery message.
Only entirely zero-filled or erased SRAM is treated as a fresh save. Back up saves
before moving them between emulators or hardware.

## Making a save from a wallet list

`tools/make_save` writes a ready-to-load `.sav` from a JSON wallet list. It is built from the
ROM's own `storage.cpp` and `validation.cpp`, so any save it writes loads in the ROM, and
anything the ROM would refuse is reported instead (with the ROM's error message). Nothing is
written if any entry is invalid.

```sh
make -C tools                                   # builds tools/build/make_save
tools/build/make_save my-wallets.json AddrBoy.sav
make -C tools check                             # example save + a refused bad file
```

See `tools/wallets.example.json`. `theme` (`field`, `net`, `lcd`, `phosphor`), `filter`
(`all`, `cold`, `hot`, `exchange`, `external`), and `sort` (`a-z`, the default, `z-a`, or
`custom` to keep the JSON's order) are optional. Each wallet has a `name` (1-20 printable
ASCII characters), a `type` (`cold`, `hot`, `exchange`, `external`), and any of `btc`,
`evm`, `sol`, `sui`, `apt`. APT addresses are padded to 64 digits, as on the device. Unknown
keys are errors, so a typo such as `eth` is not silently dropped. Put the `.sav` next to the
ROM with the same base name (`AddrBoy.sav`) for emulators, or copy it to the cartridge's
save.

## Tests

```sh
make -C tests
make -C tests sanitize
```

Tests cover EIP-55, BIP-173, and BIP-350 vectors, BTC format labels, case/checksum
failures, decoding lengths, APT normalization, all 64 wallet slots, ordering,
persistence, interrupted writes, damaged saves, and controller transitions.

`tests/emulator_runner.c` is an optional headless runner linked against mGBA.
Compile it against an mGBA build's headers, generated `mgba/flags.h`, and library:

```sh
cc -I/path/to/mgba/include -I/path/to/mgba-build/include \
  tests/emulator_runner.c -L/path/to/mgba-build \
  -Wl,-rpath,/path/to/mgba-build -lmgba -o /tmp/addrboy-emulator
```

With Pillow and zxing-cpp installed in a Python environment:

```sh
python tests/verify_rom.py /tmp/addrboy-emulator
```

This runs the actual ROM, checks exact address and explorer QR payloads for all five networks
across all four themes, verifies pure black/white QR pixels, and enters an EVM address
through GBA buttons before saving and rebooting. Screenshots and synthetic saves
are written under `tests/build/`. Real hardware phone scanning remains to be
checked on the target screen and lighting conditions.

## Project layout and credits

- `src/controller.cpp`: navigation and draft editing.
- `src/screens.cpp`: screen rendering, QR choices, settings, and confirmations.
- `src/storage.cpp`: SRAM transactions and recovery.
- `src/validation.cpp`: portable address validators.
- `src/explorers.cpp`: network-specific explorer URL choices and QR payloads.
- `tools/make_save.cpp`: save file generator from a JSON wallet list.
- `include/addr6.h`: case-sensitive 5×7 address font with a 6-pixel advance.
- `src/ui.cpp`, `src/ui_raster.bn_iwram.cpp`: locally adapted AuthBoy renderer.
- `include/logo_bookboy.h`: the "Book Boy" logo tiles and theme palettes (from the logo handoff).
- Design handoffs (`addrboy_handoff*/`) are kept locally and are not part of the repository.
- `butano/`: Butano core and common fonts, zlib license; see `butano/LICENSE`.
- `third_party/qrcodegen/`: Project Nayuki QR generator 1.8.0, MIT license; see
  `third_party/qrcodegen/LICENSE.md`.

AuthBoy's authentication, PIN, encryption, account storage, and clock logic are
not included in AddrBoy. The reference project is unchanged.
