# AddrBoy — UI handoff (delta from AuthBoy)

A standalone app. It shares no code paths with AuthBoy. It reuses AuthBoy's **visual system**: palettes, the 4 theme headers (Field Guide, `theme_net.h`, `theme_lcd.h`, `theme_amber.h`), fonts, header, footer, panel 9-slice, selection bar, cursor, keyboard and modal styles. Copy those into this project instead of linking to AuthBoy.

The visual reference is `AddrBoy Screens.dc.html`. Every element in it is absolutely positioned in native 240×160 pixels, so read its source for coordinates. This doc covers only what's new.

## Screens
| # | Screen | Notes |
|---|---|---|
| 01 | Wallets | Left panel active. The right panel shows the highlighted wallet's networks as a preview, drawn at about 55% (use the dim text role). |
| 02 | Networks | Right panel active. The chosen wallet stays marked on the left with a locked row: zebra fill, 1 px secondary border, no cursor. The other wallet rows are dimmed. |
| 03 | Receive | Left (4,26 114×114): wallet, network and the address in groups of 4. Right (122,26 114×114): QR. Dots at the bottom show the network position. |
| 04 | START menu | Command box (134,66 102×92): New wallet / Edit wallet / Networks / Delete (danger) / Settings. Dialogue box (4,108 128×50): wallet name and "Type · N networks". |
| 05 | New wallet 1/3 · Name | Same as AuthBoy's account-name screen. Max 16 characters. The header shows 3 step pips instead of the counter. |
| 06 | New wallet 2/3 · Type | 3 cards, each 232×31 at y 40/74/108: Cold / Hot / Exchange, each with a one-line description. |
| 07 | New wallet 3/3 · Networks | All 5 networks are listed. A set network shows its short address and a dot in the "ok" color. An unset one shows "empty" in the dim role, and the selected row shows "+ add" instead. START saves. Allow saving with 0 networks set. |
| 08 | Address entry | Keyboard 9×4, key caps 23×13 on a 25×15 pitch. ABC/abc tabs, with L/R switching case. Show the format hint and an n/max counter. The header shows the network coin and name. |

Edit wallet reuses screens 05–07, pre-filled.

## Layout (home)
- Left panel: 4,26 106×114. Rows are 15 px, and the selected row is 23 px: name plus "N networks" in the dim role, with a 14 px type badge. 4 px scrollbar.
- Right panel: 114,26 122×114. The "NETWORKS n/5" label is at y+4 with a 1 px rule at y+14. Rows start at y+18. The selected row is 23 px: ticker, short address, and a second line ("ETH, Base, Arbitrum" for EVM).
- Only list networks that have an address set. A wallet with 0 networks shows "No addresses yet" plus a "START › Networks" hint in the right panel.

## Filter: header pill (chosen option FB)
- The pill replaces the header's right-side text: `L ◂ [FILTER n] ▸ R`. Pill bg = header shadow, inner chip = panel fill, text = ink.
- L/R cycles ALL → COLD → HOT → EXCH and wraps. `n` is the number of wallets shown. The COLD/HOT/EXCH chips also show a 6×6 swatch of the type color.
- Store the filter in save data. If a filter has no wallets, the left panel shows "No cold wallets" (etc.) in the dim role, and the right panel is empty.
- There's no L/R hint in the footer, because the pill shows it.

## Buttons
| State | A | B | L/R | ↑↓ | START |
|---|---|---|---|---|---|
| Wallets | focus networks | — | filter | move | menu |
| Networks | show QR | back to wallets | filter (returns focus to wallets) | move | menu |
| Receive | — | back to networks | — | next/prev network of this wallet (wraps) | menu |

SELECT is unused. Settings (including Theme) is in the START menu.

## New colors
These are the only colors not in the AuthBoy palettes. The wallet type uses a **square** badge and the network uses a **round** coin, so a coin and a badge can share a color without being confused.

| Role | Hex | bn::color |
|---|---|---|
| Type: Cold (C) | #3070e0 | (6,14,28) |
| Type: Hot (H) | #f07830 | (30,15,6) |
| Type: Exchange (E) | #8060e8 | (16,12,29) |
| Coin: BTC | #f07830 | (30,15,6) |
| Coin: EVM | #8060e8 | (16,12,29) |
| Coin: SOL | #38b0a0 | (7,22,20) |
| Coin: SUI | #2898f8 | (5,19,31) |
| Coin: APT | #404858 | (8,9,11) |

All of these are already in AuthBoy's badge list except APT, which is the ink color, so they fit in the existing badge sprite palettes.

### Per theme (follow each theme's existing `badges` switch)
- **Field Guide:** as in the table.
- **Net Link** (`badges = true`, shown as a 4–5 px left strip on chips): the wallet chip's left strip uses the type color. Draw network coins as in the table, except APT: use the text color (#f8f8f8) with a deep-color (#001830) letter.
- **Pocket LCD and Phosphor** (`badges = false`): no colors.
  - Wallet type becomes a letter box (C/H/E): a 1 px frame in the ink color with the letter in ink, inverted on the selected row.
  - Coins are hidden. The ticker text (BTC, EVM, …) already identifies the network.
  - The filter pill drops the swatch and keeps the label.

## QR panel
- Draw the QR as pure black #000 on white #fff in **every theme**, including Phosphor and LCD. Theme palettes don't give a scanner enough contrast. The panel frame still uses the theme.
- 3 px per module, with a quiet zone of at least 2 modules.
  - BTC bech32 (42 characters): version 3, 29×29.
  - EVM (42): version 3, 29×29.
  - SOL (up to 44): version 3, 29×29.
  - SUI and APT (66): version 4, 33×33. 99 px plus the quiet zone fits the 110 px inner panel.
- Use byte mode, ECC level L. Encode the bare address with no URI scheme (`bitcoin:` / `ethereum:`), because wallets disagree on how to parse schemes.
- Generate the QR on entering the screen and when ↑↓ changes the network, then cache it in VRAM. Don't regenerate it every frame.

## Address font
- AuthBoy's digits8 font is uppercase only. Base58 (BTC legacy, SOL) and EIP-55 (EVM) are case sensitive.
- Add an **addr6** font: 5×7 glyphs on a 6 px advance, covering 0-9, A-Z and a-z. The mockup uses Press Start 2P at 6 px as a stand-in.
- On screen 03, show the address in groups of 4 with a space between groups, 3 groups per line.
- On the home screen and screen 07, show a short form: the first 4 characters, `..`, then the last 4 (e.g. `0x71..976F`).

## Validation (on START in screen 08)
- **BTC:** bech32/bech32m (`bc1…`) or Base58Check (`1…` / `3…`).
- **EVM:** `0x` + 40 hex. If mixed case, verify the EIP-55 checksum.
- **SOL:** Base58, 32–44 characters, decodes to 32 bytes.
- **SUI and APT:** `0x` + 64 hex. Also accept shorter APT addresses and left-pad them to 64.
- If an address is invalid, turn the counter red (danger role) and show a one-line reason in the dialogue box. Don't save it.

## Save data
- Per wallet: name[17], type u8, and 5 address slots of up to 67 bytes each. That's about 352 B per wallet, so cap it at 64 wallets.
- Also save the global filter and the theme.
- Sort order: the order the wallets were added.
