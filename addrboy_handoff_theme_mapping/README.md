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
| 08 | Address entry | Keyboard 9×4, key caps 23×13 on a 25×15 pitch. ABC/abc tabs, with SELECT switching case (same as AuthBoy). Show the format hint and an n/max counter. The header shows the network coin and name. |

Edit wallet reuses screens 05–07, pre-filled.

## Layout (home)
- Left panel: 4,26 106×114. Rows are 15 px, and the selected row is 23 px: name plus "N networks" in the dim role, with a 14 px type badge. 4 px scrollbar.
- Right panel: 114,26 122×114. The "NETWORKS n/5" label is at y+4 with a 1 px rule at y+14. Rows start at y+18. All rows are single-line and 15 px, including the selected one: coin, ticker and short address. For EVM, the chain list ("ETH + L2s") appears only on screen 03.
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

On the home and receive screens SELECT is unused. On keyboards (05, 08) it switches case, as in AuthBoy. Settings (including Theme) is in the START menu.

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

### Per theme
See "Theme mapping" below. Only Field Guide uses colored C/H/E boxes and coins.

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

## Theme mapping (read this before coding any screen)
The themes are **not** color swaps. Each theme has its own components, and AuthBoy's theme switches (`badges`, `list_panel`, `zebra_rows`, `row_separators`, `dot_leaders`, `menu_cursor`, `danger_bang`, `selected_shift_x`, `modal_backdrop`) apply here too. The mockup shows Field Guide only. For the other themes, build each AddrBoy part from the matching AuthBoy component of that theme, as mapped below.

Fonts: the AuthBoy theme READMEs say not to add fonts. If adding addr6 is costly, use body8 for addresses, since it has lowercase. The groups of 4 keep it readable.

### States every theme must support
AddrBoy has a state that AuthBoy doesn't: **two panels, only one focused**. In the mockup, the panel that isn't focused is drawn with opacity. **Don't use alpha.** Use the theme's dim text role and the "dimmed row" style below.
- **Focused panel**: contains the selected row.
- **Unfocused panel**: its rows are drawn "dimmed".
- **Selected row**: in the focused panel.
- **Locked row**: the chosen wallet while the network panel has focus. It shows which wallet you're in, without a cursor.

### Element map
**Panel frame**
- FIELD: 2 px #404858, #f8f8f8 fill.
- NET: #001830 fill, radius 3. 1 px #38d8f8 border when focused, #2878d0 when not.
- LCD: no fill, radius 2. 2 px #183020 border when focused, 1 px #507850 when not.
- AMBER: no fill. 1 px #f8b030 frame when focused, #806020 when not.

**Normal row**
- FIELD: zebra rows as in the mockup.
- NET: chip (#0850a0, 1 px #2878d0 top highlight), x +3 inside the panel, 1 px gaps.
- LCD: text #183020, with a dotted #a8c890 separator between rows.
- AMBER: text #f8b030. Network rows have a dot leader between the ticker and the short address.

**Selected row**
- FIELD: yellow bar + red cursor.
- NET: #1878e0 fill, 2 px #78f858 border, green cursor. Use `selected_shift_x` = 3 inside panels, not 6 (the panels are narrow).
- LCD: #183020 fill, text #e8f0d0, no cursor.
- AMBER: #f8b030 fill, `>` + text in #081008.

**Locked row**
- FIELD: #e0ecf8 fill, 1 px #6878a0 border, no cursor.
- NET: #1878e0 fill, 1 px #38d8f8 border, no cursor.
- LCD: no fill, 1 px #183020 outline, text #183020.
- AMBER: no fill, 1 px #f8b030 outline, text #f8b030.

**Dimmed row**
- FIELD: text #6878a0, badges kept.
- NET: chip #083878, text #78b8e8, no top highlight.
- LCD: text #507850.
- AMBER: text #a07828.

**Wallet type mark** (C / H / E)
- FIELD: colored square badge.
- NET: the type color as the chip's 4 px **left strip**, with no letter box. On the selected chip the strip is 5 px.
- LCD: a 9×9 letter box: 1 px frame + letter, both in the row's text color (inverted on the selected row).
- AMBER: text `[C]` before the name, in the row's text color.

**Network mark**
- FIELD: round coin.
- NET: round 10×10 coin, Field Guide colors. APT is a #f8f8f8 coin with a #001830 letter.
- LCD and AMBER: none; the ticker identifies the network.

**Short address**
- FIELD: Silkscreen-style text, #404858.
- NET: AuthBoy code slot (#001830, cyan text), 54×11, right-aligned.
- LCD: right-aligned, #507850 (#e8f0d0 on the selected row).
- AMBER: right-aligned after the dot leader, #f8b030.

**Section label** ("NETWORKS 3/5", "42 CHARS", format hint)
- FIELD: #6878a0, 1 px #c8d0d8 rule.
- NET: #78b8e8, 1 px #103878 rule.
- LCD: #507850, dotted #a8c890 rule.
- AMBER: #a07828, dashed #806020 rule.

**Tag** ("ADDRESS", "NAME", "TYPE")
- FIELD: #404858 box, yellow text.
- NET: cyan parallelogram, #001830 text.
- LCD: #183020 box (radius 2), #e8f0d0 text.
- AMBER: #f8b030 box (square), #081008 text.

**Header filter pill**
- FIELD: as in the mockup.
- NET: chip = cyan parallelogram with #001830 text, plus a 6×6 type swatch. The arrows and L/R are #78b8e8, on the band.
- LCD: chip #e8f0d0 with #183020 text, no swatch. The arrows and L/R are #a8c890.
- AMBER: no band. Text `L < [COLD 3] > R`: the chip is inverted (#f8b030 fill, #081008 text), and the rest is #a07828.

**Header title**
- FIELD: "AddrBoy".
- NET: cyan title tab, as in AuthBoy NET.
- LCD: "ADDRBOY" in upper case.
- AMBER: `> ADDRBOY`, then a dashed rule at y 23.

**Step pips** (new wallet)
- FIELD: 14×6 bars, white when done or current, #982010 when not.
- NET: 14×6 parallelograms, cyan when done or current, #103878 when not.
- LCD: 14×6 bars on the band, #e8f0d0 when done or current, #507850 when not.
- AMBER: 14×6 blocks, #f8b030 when done or current, #302008 when not.

**Type cards (06)**
- FIELD: as in the mockup.
- NET: chip 232×31 with a 5 px type strip. The description is #78b8e8. When selected: selected-chip style + cursor.
- LCD: 2 px #183020 border, no fill, letter box + name, description #507850. When selected: inverted (#183020 fill, #e8f0d0 text, #a8c890 description).
- AMBER: 1 px #806020 frame, `[C] Cold wallet`, description #a07828. When selected: inverted.

**Status in 07** (set / empty / + add)
- FIELD: green dot / "empty" / "+ add".
- NET: #78f858 dot / "empty" in #78b8e8 / "+ add" in #78f858.
- LCD: 6×6 #183020 square / "empty" in #507850 / "+ add" in #e8f0d0 (on the selected row).
- AMBER: 6×6 #f8b030 block / "--" in #806020 / "+ add" in #081008 (on the selected row).

**Network position dots (03)**
- FIELD: red for current, #c8d0d8 for the others.
- NET: #38d8f8 / #103878.
- LCD: #183020 / #a8c890.
- AMBER: #f8b030 / #302008.

**Menu, modals, keyboards, footer**
- Reuse AuthBoy's component for that theme as-is: menu bar, cursor or inverted bar, the `!` on danger items, the backdrop, key caps (AMBER has no caps), case tabs, footer pills.

**QR panel**
- The frame follows the theme's panel frame. Inside it, always #000 on #fff.
- LCD and AMBER have no black or white in their palettes. Add #ffffff and #000000 at free palette indexes 6 and 7, used **only** by the QR BG tiles.

### Status bar sanity check
In every theme, screens 01–03 must still read as "left panel active → right panel active → QR". If a screenshot of LCD or AMBER doesn't make it obvious which panel has focus, the panel-frame rule above has not been applied.
