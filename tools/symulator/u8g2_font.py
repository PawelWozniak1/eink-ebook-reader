"""Dekoder fontów bitmapowych u8g2 (tych samych, których używa czytnik).

Font u8g2 to tablica bajtów: 23-bajtowy nagłówek, potem glify skompresowane
kodowaniem długości serii (RLE) na poziomie bitów. Ten moduł odtwarza w Pythonie
zachowanie biblioteki U8g2_for_Adafruit_GFX: wyszukiwanie glifu, pomiar
szerokości tekstu (getUTF8Width) i rysowanie (drawUTF8), piksel w piksel.
"""

from __future__ import annotations

import re
from dataclasses import dataclass
from functools import lru_cache
from pathlib import Path

from PIL import Image


def _ze_znakiem(b: int) -> int:
    return b - 256 if b > 127 else b


@dataclass(frozen=True)
class Glif:
    szerokosc: int  # szerokość bitmapy w pikselach
    wysokosc: int
    x: int          # przesunięcie bitmapy względem kursora
    y: int
    dx: int         # o ile przesuwa się kursor po tym znaku
    piksele: tuple[tuple[int, int], ...]  # zapalone piksele (lx, ly) wewnątrz bitmapy


class _CzytnikBitow:
    """Czyta liczby o zadanej liczbie bitów, od najmłodszego bitu bajtu."""

    def __init__(self, dane: bytes, poz: int) -> None:
        self.dane, self.poz, self.bit = dane, poz, 0

    def bez_znaku(self, ile: int) -> int:
        wartosc = self.dane[self.poz] >> self.bit
        koniec = self.bit + ile
        if koniec >= 8:
            self.poz += 1
            wartosc |= self.dane[self.poz] << (8 - self.bit)
            koniec -= 8
        self.bit = koniec
        return wartosc & ((1 << ile) - 1)

    def ze_znakiem(self, ile: int) -> int:
        return self.bez_znaku(ile) - (1 << (ile - 1))


class U8g2Font:
    def __init__(self, dane: bytes, nazwa: str = "") -> None:
        self.dane = dane
        self.nazwa = nazwa
        n = dane
        (self.bity_0, self.bity_1, self.bity_szer, self.bity_wys,
         self.bity_x, self.bity_y, self.bity_dx) = n[2:9]
        self.wznios = _ze_znakiem(n[13])   # getFontAscent()
        self.zejscie = _ze_znakiem(n[14])  # getFontDescent(), zwykle ujemne
        self._start_A = int.from_bytes(n[17:19], "big")
        self._start_a = int.from_bytes(n[19:21], "big")
        self._start_unicode = int.from_bytes(n[21:23], "big")

    # --- wczytywanie ---

    @classmethod
    def z_pliku(cls, sciezka: str | Path) -> U8g2Font:
        sciezka = Path(sciezka)
        return cls(sciezka.read_bytes(), sciezka.stem)

    @classmethod
    def ze_zrodla_c(cls, zrodlo: str, nazwa: str) -> U8g2Font:
        """Wyciąga font z pliku u8g2_fonts.c (tablica zapisana jako literał napisu C)."""
        m = re.search(rf'\b{re.escape(nazwa)}\[(\d+)\][^=]*=\s*((?:"(?:[^"\\]|\\.)*"\s*)+);',
                      zrodlo, re.S)
        if not m:
            raise KeyError(f"nie ma fontu {nazwa}")
        dane = _literal_c(m.group(2))
        if len(dane) not in (int(m.group(1)), int(m.group(1)) - 1):
            raise ValueError(f"{nazwa}: {len(dane)} bajtów zamiast {m.group(1)}")
        return cls(dane, nazwa)

    # --- glify ---

    def _pozycja_glifu(self, kod: int) -> int | None:
        d = self.dane
        p = 23
        if kod <= 255:
            if kod >= ord("a"):
                p += self._start_a
            elif kod >= ord("A"):
                p += self._start_A
            while d[p + 1]:
                if d[p] == kod:
                    return p + 2
                p += d[p + 1]
            return None
        p += self._start_unicode
        tablica = p
        while True:  # tablica skoków przyspieszająca szukanie
            p += int.from_bytes(d[tablica:tablica + 2], "big")
            koniec_zakresu = int.from_bytes(d[tablica + 2:tablica + 4], "big")
            tablica += 4
            if koniec_zakresu >= kod:
                break
        while (e := int.from_bytes(d[p:p + 2], "big")) != 0:
            if e == kod:
                return p + 3
            p += d[p + 2]
        return None

    @lru_cache(maxsize=1024)
    def glif(self, kod: int) -> Glif | None:
        poz = self._pozycja_glifu(kod)
        if poz is None:
            return None
        b = _CzytnikBitow(self.dane, poz)
        szer, wys = b.bez_znaku(self.bity_szer), b.bez_znaku(self.bity_wys)
        x, y, dx = b.ze_znakiem(self.bity_x), b.ze_znakiem(self.bity_y), b.ze_znakiem(self.bity_dx)
        piksele = []
        if szer > 0:
            lx = ly = 0
            while ly < wys:
                zera, jedynki = b.bez_znaku(self.bity_0), b.bez_znaku(self.bity_1)
                while True:
                    for kolor, dlugosc in ((0, zera), (1, jedynki)):
                        for _ in range(dlugosc):
                            if kolor:
                                piksele.append((lx, ly))
                            lx += 1
                            if lx == szer:
                                lx, ly = 0, ly + 1
                    if not b.bez_znaku(1):
                        break
        return Glif(szer, wys, x, y, dx, tuple(piksele))

    # --- tekst ---

    def szerokosc(self, tekst: str) -> int:
        """Odpowiednik getUTF8Width(): suma przesunięć, ale ostatni znak liczony
        szerokością jego bitmapy (tak liczy biblioteka)."""
        w = dx = 0
        ostatni = None  # ostatni znaleziony glif (biblioteka pamięta go jako efekt uboczny)
        for znak in tekst:
            g = self.glif(ord(znak))
            dx = g.dx if g else 0
            ostatni = g or ostatni
            w += dx
        if ostatni and ostatni.szerokosc:
            w += ostatni.szerokosc + ostatni.x - dx
        return w

    def rysuj(self, obraz: Image.Image, x: int, y: int, tekst: str, kolor: int = 0) -> int:
        """Odpowiednik drawUTF8(): rysuje tekst z linią bazową na wysokości y."""
        piks = obraz.load()
        W, H = obraz.size
        start = x
        for znak in tekst:
            g = self.glif(ord(znak))
            if not g:
                continue
            gx, gy = x + g.x, y - (g.wysokosc + g.y)
            for lx, ly in g.piksele:
                px, py = gx + lx, gy + ly
                if 0 <= px < W and 0 <= py < H:
                    piks[px, py] = kolor
            x += g.dx
        return x - start


def _literal_c(tekst: str) -> bytes:
    """Zamienia sklejone literały napisów C ("..." "...") na bajty."""
    wynik = bytearray()
    for kawalek in re.findall(r'"((?:[^"\\]|\\.)*)"', tekst, re.S):
        i = 0
        while i < len(kawalek):
            c = kawalek[i]
            if c != "\\":
                wynik.append(ord(c))
                i += 1
                continue
            nast = kawalek[i + 1]
            if nast in "01234567":
                m = re.match(r"[0-7]{1,3}", kawalek[i + 1:])
                wynik.append(int(m.group(), 8))
                i += 1 + len(m.group())
            elif nast == "x":
                m = re.match(r"[0-9a-fA-F]+", kawalek[i + 2:])
                wynik.append(int(m.group(), 16) & 0xFF)
                i += 2 + len(m.group())
            else:
                wynik.append(ord({"n": "\n", "t": "\t", "r": "\r", "0": "\0"}.get(nast, nast)))
                i += 2
    return bytes(wynik)
