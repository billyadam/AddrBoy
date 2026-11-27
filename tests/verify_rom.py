"""Emulator smoke test. Requires Pillow, zxing-cpp, and the mGBA runner.

Run after `make -C tests`:
  python tests/verify_rom.py /path/to/emulator_runner
All wallet data here is synthetic and written under tests/build.
"""
from pathlib import Path
import subprocess
import sys
from PIL import Image, ImageDraw
import zxingcpp

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "tests/build/screens"
OUT.mkdir(parents=True, exist_ok=True)
SAVE = ROOT / "tests/build/demo.sav"
RUNNER = Path(sys.argv[1]).resolve()
subprocess.run([ROOT / "tests/build/core_test", SAVE], check=True)
commands = ["wait 300", f"shot {OUT / 'welcome.ppm'}", "key 1"]  # welcome screen, A continues
keys = {"A": 1, "B": 2, "SELECT": 4, "START": 8, "RIGHT": 16, "LEFT": 32, "UP": 64, "DOWN": 128, "R": 256, "L": 512}


def key(button):
    commands.append(f"key {keys[button]}")


def shot(name):
    commands.append(f"shot {OUT / (name + '.ppm')}")


def type_text(value, columns, alphabet, lower):
    """Types value starting from the keyboard's current case (lower=True on the abc tab)."""
    row = column = 0
    for character in value:
        want_lower = character.islower()
        if character.isalpha() and want_lower != lower:
            key("SELECT")
            lower = want_lower
        index = alphabet.index(character.upper())
        target_row, target_column = divmod(index, columns)
        while row != target_row:
            key("DOWN")
            row = (row + 1) % 4
        while column != target_column:
            key("RIGHT")
            column = (column + 1) % columns
        key("A")


themes = ["field", "net", "lcd", "amber"]
expected = [
    "bc1qw508d6qejxtdg4y5r3zarvary0c5xw7kv8f3t4",
    "0x5aAeb6053F3E94C9b9A09f33669435E7Ef1BeAed",
    "So11111111111111111111111111111111111111112",
    "0x" + "a" * 64,
    "0x" + "b" * 64,
]
explorers = [
    ["https://blockstream.info/address/"],
    ["https://etherscan.io/address/", "https://basescan.org/address/",
     "https://arbiscan.io/address/", "https://robinhoodchain.blockscout.com/address/"],
    ["https://solscan.io/account/"],
    ["https://suiscan.xyz/mainnet/account/"],
    ["https://explorer.aptoslabs.com/account/"],
]
for theme in themes:
    shot(f"{theme}-home")
    key("A")
    shot(f"{theme}-networks")
    key("A")
    shot(f"{theme}-qr-choices")
    key("A")  # Address choice
    for network in range(5):
        shot(f"{theme}-qr-{network}")
        key("RIGHT")  # Left/Right switch networks on the receive screen
    key("START")
    shot(f"{theme}-menu")
    key("B")  # Close the menu
    key("B")  # Back to QR choices
    key("B")  # Back to the networks panel
    for network, links in enumerate(explorers):
        if network:
            key("DOWN")
        key("A")  # QR choices for selected network
        for choice, _ in enumerate(links):
            key("DOWN")
            key("A")
            shot(f"{theme}-explorer-{network}-{choice}")
            key("B")
        key("B")  # Back to networks
    key("SELECT")  # Settings
    shot(f"{theme}-settings")
    key("SELECT")  # About, page 1 (with the project link's QR Code)
    shot(f"{theme}-about")
    key("B")  # Back to Settings
    key("DOWN")  # Theme (Sort is the first item)
    key("A")  # Next theme, committed to SRAM
    key("B")
    key("B")

# Drive the entire editor using actual GBA button presses.
key("START")
key("A")
shot("name")
type_text("Demo", 10, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-._'", lower=False)
key("START")
shot("type")
key("DOWN")
key("A")
shot("setup")
key("DOWN")  # EVM
key("A")
shot("address")
type_text(expected[1], 9, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789", lower=True)  # Address entry opens on abc
shot("address-filled")
key("START")
shot("setup-filled")
key("START")
shot("saved-wallet")
key("A")
key("A")
key("A")
shot("saved-qr")
commands.append(f"save {ROOT / 'tests/build/edited.sav'}")

with (OUT / "emulator.log").open("w") as log:
    subprocess.run([RUNNER, ROOT / "AddrBoy.gba", SAVE], input="\n".join(commands), text=True, stdout=log, stderr=log, check=True, timeout=120)

for theme in themes:
    for network, address in enumerate(expected):
        image = Image.open(OUT / f"{theme}-qr-{network}.ppm")
        decoded = zxingcpp.read_barcodes(image)
        assert len(decoded) == 1 and decoded[0].text == address, (theme, network, decoded)
        colors = {color for _, color in image.crop((122, 27, 234, 139)).getcolors()}
        assert colors == {(0, 0, 0), (255, 255, 255)}, (theme, network, colors)
    for network, links in enumerate(explorers):
        for choice, prefix in enumerate(links):
            image = Image.open(OUT / f"{theme}-explorer-{network}-{choice}.ppm")
            decoded = zxingcpp.read_barcodes(image)
            assert len(decoded) == 1 and decoded[0].text == prefix + expected[network], (theme, network, choice, decoded)
            colors = {color for _, color in image.crop((122, 27, 234, 139)).getcolors()}
            assert colors == {(0, 0, 0), (255, 255, 255)}, (theme, network, choice, colors)
for theme in themes:
    decoded = zxingcpp.read_barcodes(Image.open(OUT / f"{theme}-about.ppm"))
    assert len(decoded) == 1 and decoded[0].text == "https://github.com/billyadam/AddrBoy", (theme, decoded)
decoded = zxingcpp.read_barcodes(Image.open(OUT / "saved-qr.ppm"))
assert len(decoded) == 1 and decoded[0].text == expected[1], decoded

# Reboot from the save written by the ARM code, select the last wallet, and decode it.
commands = ["wait 300", "key 1", "key 64", "key 1", "key 1", "key 1", f"shot {OUT / 'reloaded-qr.ppm'}"]
with (OUT / "reboot.log").open("w") as log:
    subprocess.run([RUNNER, ROOT / "AddrBoy.gba", ROOT / "tests/build/edited.sav"], input="\n".join(commands), text=True, stdout=log, stderr=log, check=True, timeout=30)
decoded = zxingcpp.read_barcodes(Image.open(OUT / "reloaded-qr.ppm"))
assert len(decoded) == 1 and decoded[0].text == expected[1], decoded

for path in OUT.glob("*.ppm"):
    Image.open(path).resize((720, 480), Image.Resampling.NEAREST).save(path.with_suffix(".png"))
sheet = Image.new("RGB", (960, 1360), "#202020")
labels = ImageDraw.Draw(sheet)
for row, theme in enumerate(themes):
    for col, screen in enumerate(["home", "qr-1"]):
        x, y = col * 480, row * 340
        labels.text((x+4, y+2), theme + " / " + screen, fill="white")
        sheet.paste(Image.open(OUT / f"{theme}-{screen}.ppm").resize((480, 320), Image.Resampling.NEAREST), (x, y+18))
sheet.save(OUT / "theme-overview.png")
print("ROM passed: 20 exact QR decodes, the About link QR in every theme, pure black/white in every theme, button-driven creation and save/reboot.")
