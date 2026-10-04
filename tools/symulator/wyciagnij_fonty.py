"""Kopiuje fonty używane przez czytnik z biblioteki U8g2_for_Adafruit_GFX do folderu fonty/.

Uruchamia się raz (fonty są już w repozytorium), np. po dodaniu nowego fontu do firmware'u:
    python tools/symulator/wyciagnij_fonty.py "C:/Users/.../Arduino/libraries/U8g2_for_Adafruit_GFX/src/u8g2_fonts.c"
"""

import sys
from pathlib import Path

from u8g2_font import U8g2Font

FONTY = ["u8g2_font_ncenR10_te", "u8g2_font_ncenR12_te", "u8g2_font_ncenR14_te",
         "u8g2_font_ncenR18_te", "u8g2_font_ncenB18_te", "u8g2_font_ncenB24_te"]

CEL = Path(__file__).parent / "fonty"

if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    zrodlo = Path(sys.argv[1]).read_text(encoding="latin-1")
    CEL.mkdir(exist_ok=True)
    for nazwa in FONTY:
        font = U8g2Font.ze_zrodla_c(zrodlo, nazwa)
        (CEL / f"{nazwa.removeprefix('u8g2_font_')}.u8g2").write_bytes(font.dane)
        print(f"{nazwa}: {len(font.dane)} B")
