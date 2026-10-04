"""Generuje obrazki do README (folder docs/). Uruchomienie: python symulator/zrzuty_do_readme.py"""

from pathlib import Path

from czytnik import Czytnik, Ksiazka, montaz

KORZEN = Path(__file__).parent.parent
KSIAZKI = KORZEN / "ksiazki_do_wgrania"
DOCS = KORZEN / "docs"

if __name__ == "__main__":
    DOCS.mkdir(exist_ok=True)

    pan_tadeusz = Czytnik(Ksiazka.z_pliku(KSIAZKI / "pan_tadeusz.txt"))
    montaz([pan_tadeusz.rysuj_strone(n) for n in (0, 1, 2)]).save(DOCS / "strony_pan_tadeusz.png")

    treny = Ksiazka.z_pliku(KSIAZKI / "treny.txt")
    strony = []
    for rozmiar in (0, 1, 3):
        czytnik = Czytnik(treny, rozmiar)
        strony.append(czytnik.rysuj_strone(czytnik.pierwsza_strona_ksiegi[1]))  # Tren I
    montaz(strony).save(DOCS / "rozmiary_czcionki.png")

    print("docs/strony_pan_tadeusz.png, docs/rozmiary_czcionki.png")
