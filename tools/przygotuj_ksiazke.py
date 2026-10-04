# Zamienia pliki TXT z Wolnych Lektur na pliki gotowe do wgrania do czytnika
# (folder books/prepared). Uruchomienie: python tools/przygotuj_ksiazke.py
#
# Pliki wgrywa się przez stronę czytnika (w menu czytnika: "Wgraj książki (WiFi)").
# Zwykłe pliki TXT z Wolnych Lektur też można tam wgrywać bezpośrednio - strona sama
# je przygotuje, ale ten skrypt lepiej rozpoznaje rozdziały Pana Tadeusza i Trenów.
#
# Pierwsza linia pliku: #CZYTNIK|tytuł|autor
# Znaczniki na początku linii (czytnik rysuje je inaczej):
#   \001 tytuł rozdziału (zawsze od nowej strony)
#   \002 podtytuł rozdziału
#   \003 streszczenie rozdziału (mniejsza czcionka, zawijane)
#   \004 duży tytuł (strona tytułowa)
#   \005 tekst wyśrodkowany
#   \006 akapit prozy (wcięty i wyjustowany; zwykłe linie bez znacznika to wersy)
#   linia pusta = odstęp między zwrotkami

import re
from pathlib import Path

KSIAZKI = Path(__file__).parent.parent / "books"
ZRODLA = KSIAZKI / "source"
CEL = KSIAZKI / "prepared"

# czcionki u8g2 "_te" nie mają tych znaków
ZAMIANY = {"—": "-", "–": "-", "…": "...", "„": "\"", "”": "\"", "’": "'"}


def bez_stopki(linie):
    """Wycina stopkę licencyjną Wolnych Lektur."""
    return linie[:linie.index("-----")] if "-----" in linie else linie


def strona_tytulowa(autor, tytul, *podtytuly):
    linie = ["\005", "\005" + autor, "", "\004" + tytul, ""]
    linie += ["\005" + p for p in podtytuly]
    linie += ["", "", "\005Tekst: wolnelektury.pl (domena publiczna)"]
    return linie


def pan_tadeusz():
    ksiegi = re.compile(r"^(Księga (pierwsza|druga|trzecia|czwarta|piąta|szósta|siódma|ósma|"
                        r"dziewiąta|dziesiąta|jedenasta|dwunasta)|Epilog)$")
    linie = bez_stopki((ZRODLA / "PanTadeusz_WolneLektury.txt").read_text(encoding="utf-8").splitlines())
    start = next(i for i, l in enumerate(linie) if ksiegi.match(l))

    wynik = strona_tytulowa("Adam Mickiewicz", "Pan Tadeusz",
                            "czyli ostatni zajazd na Litwie", "",
                            "Historia szlachecka z roku 1811 i 1812",
                            "we dwunastu księgach wierszem")
    i = start
    while i < len(linie):
        l = linie[i].rstrip()
        if not ksiegi.match(l):
            wynik.append(l)
            i += 1
            continue
        wynik.append("\001" + l)
        i += 1
        if l == "Epilog":  # Epilog nie ma podtytułu ani streszczenia
            continue
        # podtytuł (krótki) i streszczenie (długie, bez wcięcia) - pierwsze niepuste linie
        for znacznik, pasuje in (("\002", lambda s, r: len(s) < 40),
                                 ("\003", lambda s, r: len(s) > 60 and not r.startswith(" "))):
            j = i
            while j < len(linie) and not linie[j].strip():
                j += 1
            if j < len(linie) and pasuje(linie[j].strip(), linie[j]):
                wynik.append(znacznik + linie[j].strip())
                i = j + 1
        wynik.append("")
    return wynik


def treny():
    wynik = strona_tytulowa("Jan Kochanowski", "Treny")
    for plik in sorted((ZRODLA / "treny").glob("*.txt")):
        linie = bez_stopki(plik.read_text(encoding="utf-8").splitlines())
        # 0: autor, 2: "Treny", 3: tytuł części, potem treść
        tytul = linie[3].strip().strip("[]")
        if ". (" in tytul:
            glowny, podtytul = tytul.split(". (", 1)
            wynik += ["\001" + glowny, "\002(" + podtytul, ""]
        else:
            wynik += ["\001" + tytul, ""]
        wynik += [l.rstrip() for l in linie[4:]]
    return wynik


# nazwa pliku, tytuł i autor do menu, funkcja tworząca tekst
KSIAZKI = [
    ("pan_tadeusz", "Pan Tadeusz", "Adam Mickiewicz", pan_tadeusz),
    ("treny", "Treny", "Jan Kochanowski", treny),
]


def oczysc(linie):
    """Zamienia brakujące znaki, scala puste linie, usuwa puste na końcu."""
    czyste = []
    for l in linie:
        for a, b in ZAMIANY.items():
            l = l.replace(a, b)
        if l == "" and czyste and czyste[-1] == "":
            continue
        czyste.append(l)
    while czyste and czyste[-1] == "":
        czyste.pop()
    return czyste


CEL.mkdir(exist_ok=True)
for nazwa, tytul, autor, funkcja in KSIAZKI:
    linie = oczysc(funkcja())
    plik = CEL / f"{nazwa}.txt"
    with plik.open("w", encoding="utf-8", newline="\n") as f:
        f.write(f"#CZYTNIK|{tytul}|{autor}\n")
        for l in linie:
            f.write(l + "\n")
    print(f"{tytul}: {len(linie)} linii, {plik.stat().st_size} bajtów -> {plik}")
