"""Symulator czytnika: składa książkę na strony tak jak firmware i zapisuje je jako PNG.

Algorytm jest przeniesiony 1:1 z sketch_oct1a.ino (ulozStrone, podzielNaStrony,
rysujStrone), a litery rysowane są tymi samymi fontami u8g2 co na urządzeniu,
więc podział na strony i wygląd zgadzają się z prawdziwym ekranem.

Pozycje w tekście to indeksy bajtów (UTF-8), jak w firmware - zakładki zapisane
przez czytnik wskazują więc to samo miejsce.
"""

from __future__ import annotations

import argparse
from dataclasses import dataclass, field
from pathlib import Path

from PIL import Image, ImageDraw

from u8g2_font import U8g2Font

FONTY = Path(__file__).parent / "fonty"

# --- stałe z sketch_oct1a.ino ---
SZEROKOSC, WYSOKOSC = 480, 800   # ekran obrócony pionowo
MARGINES = MARGINES_PRAWY = 30
GORA_TEKSTU = 40
MIEJSCE_NA_STOPKE = 52
WCIECIE_SPACJI = 8
WCIECIE_ZAWINIECIA = 24
WCIECIE_AKAPITU = 24
DOL_TEKSTU = WYSOKOSC - MIEJSCE_NA_STOPKE

T_ZWYKLY, T_KSIEGA, T_PODTYTUL, T_STRESZCZENIE, T_TYTUL, T_SRODEK, T_AKAPIT = range(7)
NL, SPACJA = ord("\n"), ord(" ")
NAGLOWEK = b"#CZYTNIK|"

ROZMIARY = ["mała", "średnia", "duża", "bardzo duża"]


def _font(nazwa: str, _pamiec: dict[str, U8g2Font] = {}) -> U8g2Font:
    if nazwa not in _pamiec:
        _pamiec[nazwa] = U8g2Font.z_pliku(FONTY / f"{nazwa}.u8g2")
    return _pamiec[nazwa]


@dataclass
class Ksiazka:
    tytul: str
    autor: str
    tekst: bytes  # bez linii nagłówka

    @classmethod
    def z_pliku(cls, sciezka: str | Path) -> Ksiazka:
        dane = Path(sciezka).read_bytes()
        if dane.startswith(NAGLOWEK):
            naglowek, _, tekst = dane.partition(b"\n")
            _, tytul, autor = (naglowek.decode("utf-8").split("|") + ["", ""])[:3]
            return cls(tytul, autor, tekst)
        return cls(Path(sciezka).stem, "", dane)


@dataclass
class Czytnik:
    ksiazka: Ksiazka
    rozmiar: int = 1  # 0..3, jak przycisk RST
    strony: list[int] = field(default_factory=list)       # początek każdej strony
    ksiegi: list[int] = field(default_factory=list)       # początek każdego rozdziału
    pierwsza_strona_ksiegi: list[int] = field(default_factory=list)
    proza: bool = False

    def __post_init__(self) -> None:
        self.t = self.ksiazka.tekst
        self.dlugosc = len(self.t)
        self._przesuniecia: dict[tuple[str, int], int] = {}
        self.podziel_na_strony()

    # --- czcionki (ustawCzcionke) ---

    def czcionka(self, typ: int) -> U8g2Font:
        r = self.rozmiar
        return _font({
            T_KSIEGA: "ncenB24_te" if r == 3 else "ncenB18_te",
            T_PODTYTUL: "ncenR14_te",
            T_STRESZCZENIE: "ncenR12_te" if r >= 2 else "ncenR10_te",
            T_TYTUL: "ncenB24_te",
            T_SRODEK: "ncenR14_te",
        }.get(typ, ["ncenR10_te", "ncenR12_te", "ncenR14_te", "ncenR18_te"][r]))

    # --- pomiar tekstu ---

    def _kod_znaku(self, q: int) -> tuple[int, int]:
        """Kod znaku UTF-8 na pozycji q i jego długość w bajtach (kodZnaku)."""
        b = self.t[q]
        if b < 0x80:
            return b, 1
        if b & 0xE0 == 0xC0:
            return ((b & 0x1F) << 6) | (self._bajt(q + 1) & 0x3F), 2
        if b & 0xF0 == 0xE0:
            return ((b & 0x0F) << 12) | ((self._bajt(q + 1) & 0x3F) << 6) | (self._bajt(q + 2) & 0x3F), 3
        return ord("?"), 1

    def _bajt(self, i: int) -> int:
        return self.t[i] if i < self.dlugosc else 0

    def _przesuniecie(self, font: U8g2Font, kod: int) -> int:
        # szerokość "Xl" minus "l" = przesunięcie kursora po X (jak w firmware)
        klucz = (font.nazwa, kod)
        if klucz not in self._przesuniecia:
            self._przesuniecia[klucz] = font.szerokosc(chr(kod) + "l") - font.szerokosc("l")
        return self._przesuniecia[klucz]

    def _wytnij(self, od: int, do: int) -> str:
        return self.t[od:min(do, od + 1023)].decode("utf-8", errors="ignore")

    def _szerokosc(self, font: U8g2Font, od: int, do: int) -> int:
        return font.szerokosc(self._wytnij(od, do))

    def _dopasuj(self, font: U8g2Font, p: int, koniec: int, szer: int) -> int:
        """Gdzie uciąć linię od p do koniec, żeby zmieściła się w szer pikselach."""
        ostatnia_spacja = 0
        w = 0
        q = p
        while q < koniec:
            kod, dl = self._kod_znaku(q)
            if kod == SPACJA and q > p:
                if w > szer:
                    break
                ostatnia_spacja = q
            w += self._przesuniecie(font, kod)
            q += dl
        if w <= szer:
            return koniec
        return ostatnia_spacja or koniec

    # --- skład strony (ulozStrone) ---

    def uloz_strone(self, start: int, obraz: Image.Image | None = None) -> int:
        """Układa stronę od miejsca start; gdy podano obraz, także ją rysuje.
        Zwraca miejsce, od którego zaczyna się następna strona."""
        t, dlugosc = self.t, self.dlugosc
        p, y = start, GORA_TEKSTU
        pierwsza_na_stronie = True

        while p < dlugosc:
            poczatek_linii = t.rfind(b"\n", 0, p) + 1
            koniec_linii = t.find(b"\n", p)
            if koniec_linii < 0:
                koniec_linii = dlugosc
            nastepna_linia = min(koniec_linii + 1, dlugosc)

            z = t[poczatek_linii]
            typ = z if T_KSIEGA <= z <= T_AKAPIT else T_ZWYKLY
            tresc = poczatek_linii + (1 if typ else 0)
            if typ == T_ZWYKLY and self.proza:
                typ = T_AKAPIT
            kontynuacja = p > tresc
            powrot = p if kontynuacja else poczatek_linii
            if not kontynuacja:
                p = tresc

            if typ == T_KSIEGA and not pierwsza_na_stronie:
                return powrot

            if koniec_linii == tresc:  # pusta linia = odstęp
                if typ == T_SRODEK:
                    y += 150
                elif not pierwsza_na_stronie and not self.proza:
                    y += 10
                p = nastepna_linia
                continue

            font = self.czcionka(typ)
            przed = 60 if typ == T_KSIEGA and not kontynuacja else 0
            wysrodkuj = typ not in (T_ZWYKLY, T_AKAPIT)

            wciecie = 0
            if typ == T_ZWYKLY:
                if kontynuacja:
                    wciecie = WCIECIE_ZAWINIECIA
                else:
                    while self._bajt(p) == SPACJA:
                        p += 1
                        wciecie += WCIECIE_SPACJI
            elif typ == T_AKAPIT and not kontynuacja:
                wciecie = WCIECIE_AKAPITU
                while self._bajt(p) == SPACJA:
                    p += 1

            szer = SZEROKOSC - MARGINES - MARGINES_PRAWY - wciecie
            koniec_kawalka = self._dopasuj(font, p, koniec_linii, szer)

            linia = y + przed + font.wznios  # linia bazowa tekstu
            if linia - font.zejscie > DOL_TEKSTU and not pierwsza_na_stronie:
                return powrot

            if obraz is not None:
                x = MARGINES + wciecie
                if wysrodkuj:
                    x = MARGINES + (SZEROKOSC - MARGINES - MARGINES_PRAWY
                                    - self._szerokosc(font, p, koniec_kawalka)) // 2
                if typ == T_AKAPIT and koniec_kawalka < koniec_linii:
                    self._rysuj_wyjustowane(obraz, font, x, linia, p, koniec_kawalka, szer)
                else:
                    font.rysuj(obraz, x, linia, self._wytnij(p, koniec_kawalka))

            y = linia - font.zejscie + (3 if typ == T_STRESZCZENIE else 5)
            pierwsza_na_stronie = False

            if koniec_kawalka >= koniec_linii:
                y += {T_KSIEGA: 8, T_PODTYTUL: 12, T_STRESZCZENIE: 14}.get(typ, 0)
                p = nastepna_linia
            else:
                p = koniec_kawalka
                while p < koniec_linii and t[p] == SPACJA:
                    p += 1
        return dlugosc

    def _rysuj_wyjustowane(self, obraz, font, x, y, od, do, szer) -> None:
        """Linia rozciągnięta na szer pikseli - odstępy między słowami równo powiększone."""
        slowa = [s.decode("utf-8", errors="ignore") for s in self.t[od:do].split(b" ") if s]
        luz = szer - sum(font.szerokosc(s) for s in slowa)
        przerw = len(slowa) - 1
        for s in slowa:
            font.rysuj(obraz, x, y, s)
            if przerw > 0:
                odstep = int(luz / przerw)  # jak dzielenie całkowite w C (obcina do zera)
                luz -= odstep
                przerw -= 1
                x += font.szerokosc(s) + odstep

    # --- podział na strony (podzielNaStrony) ---

    def podziel_na_strony(self) -> None:
        t = self.t
        self.strony, self.ksiegi, self.pierwsza_strona_ksiegi = [], [], []

        # proza czy wiersz: w prozie większość tekstu jest w długich liniach
        wszystkie = w_dlugich = poczatek = 0
        for linia in t.split(b"\n"):
            if linia[:1] == bytes([T_KSIEGA]):
                self.ksiegi.append(poczatek)
            wszystkie += len(linia)
            if len(linia) > 120:
                w_dlugich += len(linia)
            poczatek += len(linia) + 1
        self.proza = w_dlugich > wszystkie // 2

        p = 0
        while p < self.dlugosc:
            self.strony.append(p)
            nastepna = self.uloz_strone(p)
            if nastepna <= p:
                break
            p = nastepna
        if not self.strony:
            self.strony.append(0)

        for k in self.ksiegi:
            s = 0
            while s < len(self.strony) - 1 and self.strony[s + 1] <= k:
                s += 1
            self.pierwsza_strona_ksiegi.append(s)

    def ksiega_strony(self, strona: int) -> int:
        k = -1
        for i, s in enumerate(self.pierwsza_strona_ksiegi):
            if s <= strona:
                k = i
        return k

    def tytul_rozdzialu(self, k: int) -> str:
        od = self.ksiegi[k] + 1
        return self._wytnij(od, self.t.find(b"\n", od) % (self.dlugosc + 1))

    # --- rysowanie (rysujStrone) ---

    def rysuj_strone(self, strona: int) -> Image.Image:
        obraz = Image.new("1", (SZEROKOSC, WYSOKOSC), 1)
        self.uloz_strone(self.strony[strona], obraz)
        if strona == 0:  # strona tytułowa bez stopki
            return obraz

        W, H = SZEROKOSC, WYSOKOSC
        ImageDraw.Draw(obraz).line([(MARGINES, H - 42), (W - MARGINES_PRAWY, H - 42)], fill=0)
        font = _font("ncenR10_te")
        numer = f"{strona} / {len(self.strony) - 1}"
        szer_numeru = font.szerokosc(numer)
        font.rysuj(obraz, W - MARGINES_PRAWY - szer_numeru, H - 20, numer)

        k = self.ksiega_strony(strona)
        if k >= 0:
            rozdzial = _skroc(font, self.tytul_rozdzialu(k)[:190],
                              W - MARGINES - MARGINES_PRAWY - szer_numeru - 20)
            font.rysuj(obraz, MARGINES, H - 20, rozdzial)
        return obraz


def _skroc(font: U8g2Font, tekst: str, szer: int) -> str:
    """Skraca tekst, aż zmieści się w szer pikselach, i dopisuje '...'."""
    if font.szerokosc(tekst) <= szer:
        return tekst
    while tekst:
        tekst = tekst[:-1].rstrip(" ")
        if font.szerokosc(tekst + "...") <= szer:
            return tekst + "..."
    return "..."


# --- uruchamianie z wiersza poleceń ---

def montaz(strony: list[Image.Image], odstep: int = 40) -> Image.Image:
    """Kilka stron obok siebie na tle, z cienką ramką - do README."""
    W, H = SZEROKOSC, WYSOKOSC
    tlo = Image.new("RGB", (len(strony) * (W + odstep) + odstep, H + 2 * odstep), (236, 233, 225))
    for i, s in enumerate(strony):
        x = odstep + i * (W + odstep)
        ImageDraw.Draw(tlo).rectangle([x - 1, odstep - 1, x + W, odstep + H], outline=(160, 156, 148))
        tlo.paste(s.convert("RGB"), (x, odstep))
    return tlo


def main() -> None:
    parser = argparse.ArgumentParser(description="Podgląd stron książki tak, jak pokaże je czytnik.")
    parser.add_argument("ksiazka", type=Path, help="plik z ksiazki_do_wgrania/")
    parser.add_argument("-s", "--strony", type=int, nargs="+", help="numery stron (domyślnie wszystkie)")
    parser.add_argument("-r", "--rozmiar", type=int, choices=range(4), default=1,
                        help="rozmiar czcionki: 0 mała, 1 średnia, 2 duża, 3 bardzo duża")
    parser.add_argument("-o", "--wyjscie", type=Path, default=Path("podglad"), help="folder na PNG")
    parser.add_argument("-m", "--montaz", type=Path, help="zapisz wybrane strony obok siebie w jednym pliku")
    args = parser.parse_args()

    czytnik = Czytnik(Ksiazka.z_pliku(args.ksiazka), args.rozmiar)
    print(f"{czytnik.ksiazka.tytul}: {len(czytnik.strony) - 1} stron, {len(czytnik.ksiegi)} rozdziałów, "
          f"{'proza' if czytnik.proza else 'wiersz'}, czcionka {ROZMIARY[args.rozmiar]}")

    numery = args.strony if args.strony is not None else range(len(czytnik.strony))
    obrazy = [czytnik.rysuj_strone(n) for n in numery]
    if args.montaz:
        montaz(obrazy).save(args.montaz)
        print(f"-> {args.montaz}")
    else:
        args.wyjscie.mkdir(parents=True, exist_ok=True)
        for n, obraz in zip(numery, obrazy):
            obraz.save(args.wyjscie / f"strona_{n:04d}.png")
        print(f"{len(obrazy)} stron -> {args.wyjscie}/")


if __name__ == "__main__":
    main()
