# 📖 Czytnik e-booków DIY na e-papierze

🇬🇧 [English](README.md) · 🇵🇱 **Polski**

[![tests](https://github.com/PawelWozniak1/eink-ebook-reader/actions/workflows/tests.yml/badge.svg)](https://github.com/PawelWozniak1/eink-ebook-reader/actions/workflows/tests.yml)

Własny czytnik książek zbudowany od zera — od przetwarzania tekstu w Pythonie, przez firmware z silnikiem składu stron, po obudowę zaprojektowaną w OpenSCAD i wydrukowaną w 3D.

![Strony wygenerowane przez symulator](docs/strony_pan_tadeusz.png)

<sub>Strony narysowane przez symulator w Pythonie fontami z czytnika: strona tytułowa, początek księgi z podtytułem i streszczeniem oraz zwykła strona ze stopką.</sub>

---

## ✨ Co potrafi

- **Czytanie z prawdziwym składem tekstu** — justowanie akapitów prozy, wcięcia, łamanie zbyt długich wersów, zachowane zwrotki w poezji
- **Automatyczne rozpoznanie prozy i wiersza** — na podstawie statystyki długości linii
- **Rozdziały i spis treści** — każdy rozdział od nowej strony, nawigacja joystickiem
- **4 rozmiary czcionki** z pełnymi polskimi znakami; po zmianie czytnik zostaje w tym samym miejscu tekstu
- **Zakładki** — czytnik pamięta miejsce w każdej książce, także po wyłączeniu zasilania
- **Oszczędzanie energii** — deep sleep po 30 min bezczynności lub na przytrzymanie przycisku, wybudzenie przyciskiem prosto na ostatnią stronę
- **Tryb Wi-Fi** — strona w przeglądarce do wgrywania i usuwania książek oraz aktualizacji firmware'u (OTA) bez kabla
- **Captive portal** — gdy nie ma domowej sieci, czytnik tworzy własną, a telefon sam otwiera jego stronę
- **Bonus: kółko i krzyżyk** na dwa telefony, z planszą rysowaną na e-papierze 🎮

---

## 🐍 Python: przygotowanie książek

[`przygotuj_ksiazke.py`](przygotuj_ksiazke.py) zamienia surowe pliki TXT z [Wolnych Lektur](https://wolnelektury.pl) na lekki format zrozumiały dla czytnika.

```bash
python przygotuj_ksiazke.py
```
```
Pan Tadeusz: 10742 linii, 479913 bajtów -> ksiazki_do_wgrania/pan_tadeusz.txt
Treny: 699 linii, 28279 bajtów -> ksiazki_do_wgrania/treny.txt
```

Co robi skrypt:

| Krok | Jak |
|---|---|
| Wycina stopkę licencyjną | wyszukanie separatora `-----` |
| Rozpoznaje księgi *Pana Tadeusza* | wyrażenie regularne dla „Księga pierwsza … dwunasta” i „Epilog” |
| Odróżnia podtytuł od streszczenia księgi | heurystyka: długość linii i brak wcięcia |
| Składa *Treny* z 20 osobnych plików | `pathlib.glob` + rozbicie tytułu na tytuł i podtytuł |
| Generuje stronę tytułową | funkcja z `*args` na dowolną liczbę podtytułów |
| Dopasowuje tekst do czcionek mikrokontrolera | tablica zamian typograficznych znaków (`—`, `„`, `…`) |
| Normalizuje białe znaki | scalanie pustych linii, LF zamiast CRLF, UTF-8 |

Dodanie nowej książki to jedna linia w liście `KSIAZKI` i funkcja zwracająca listę linii.

Ta sama logika jest też przepisana na JavaScript w stronie czytnika ([`strona.h`](sketch_oct1a/strona.h)), więc zwykły plik TXT można wgrać prosto z telefonu — skrypt w Pythonie lepiej radzi sobie jednak z nietypową strukturą *Pana Tadeusza* i *Trenów*.

### Format pliku książki

Pierwsza linia to nagłówek, dalej tekst, w którym pierwszy bajt linii może być znacznikiem:

```
#CZYTNIK|Pan Tadeusz|Adam Mickiewicz
\004Pan Tadeusz
\001Księga pierwsza
\002Gospodarstwo
\003Powrót panicza - Spotkanie się pierwsze w pokoiku, drugie u stołu - ...

    Litwo! Ojczyzno moja! ty jesteś jak zdrowie:
```

| Znacznik | Znaczenie |
|---|---|
| `\001` | tytuł rozdziału (zawsze od nowej strony) |
| `\002` | podtytuł rozdziału |
| `\003` | streszczenie (mniejsza czcionka, zawijane) |
| `\004` | duży tytuł (strona tytułowa) |
| `\005` | tekst wyśrodkowany |
| `\006` | akapit prozy (wcięty i wyjustowany) |
| *(brak)* | wers wiersza |
| pusta linia | odstęp między zwrotkami |

Format jest celowo prosty: czytnik nie musi parsować HTML ani EPUB-a, a jedno przejście po pliku wystarcza, żeby znaleźć rozdziały i podzielić tekst na strony.

---

## 🖥️ Python: symulator czytnika

[`symulator/`](symulator/) składa książkę dokładnie tak, jak pokaże ją czytnik, i zapisuje strony jako PNG. To przeniesiony do Pythona silnik składu stron z firmware'u, więc wygląd książki można sprawdzić bez wgrywania czegokolwiek.

```bash
pip install -r requirements.txt
python symulator/czytnik.py ksiazki_do_wgrania/pan_tadeusz.txt -s 0 1 2 -m podglad.png
```
```
Pan Tadeusz: 308 stron, 13 rozdziałów, wiersz, czcionka średnia
-> podglad.png
```

![Ta sama strona w trzech rozmiarach czcionki](docs/rozmiary_czcionki.png)

<sub>Tren I w czcionce małej, średniej i bardzo dużej. Zbyt długie wersy zawijają się z wcięciem, a liczba stron w stopce zmienia się razem z rozmiarem czcionki.</sub>

Jak to działa:

- **Dekoder fontów u8g2** ([`u8g2_font.py`](symulator/u8g2_font.py)). Fonty z firmware'u to fonty bitmapowe zapisane w pliku C jako tablice bajtów skompresowane kodowaniem długości serii (RLE) na poziomie bitów. Dekoder parsuje literały napisów C (escape'y ósemkowe i szesnastkowe), czyta 23-bajtowy nagłówek, wyszukuje glify w listach ASCII i tablicy skoków Unicode, a potem rozpakowuje bitmapy. Szerokość tekstu liczy dokładnie tak jak biblioteka, łącznie z jej osobliwościami, np. ostatni znak liczony jest szerokością bitmapy, a nie przesunięciem kursora.
- **Port silnika składu** ([`czytnik.py`](symulator/czytnik.py)). Kod odwzorowuje `ulozStrone`, `podzielNaStrony` i `rysujStrone` z firmware'u i tak jak C++ operuje na pozycjach bajtów w UTF-8. Zakładka zapisana przez czytnik wskazuje więc to samo miejsce w symulatorze.
- **CLI** w `argparse`: wybór stron, rozmiaru czcionki, folderu wyjściowego albo jednego obrazka ze stronami obok siebie.
- **Testy** ([`tests/`](tests/)) uruchamiane w GitHub Actions. Sprawdzają czytnik bitów, parser literałów C, polskie litery we wszystkich fontach, pomiar tekstu i niezmienniki podziału na strony: strony pokrywają cały tekst, każdy rozdział zaczyna się od nowej strony, a większa czcionka daje więcej stron.

Obrazki w README generuje `python symulator/zrzuty_do_readme.py`.

---

## 🔌 Firmware

Kod Arduino jest w folderze [`sketch_oct1a/`](sketch_oct1a/). To ok. 1900 linii C++, HTML i JavaScriptu:

| Plik | Co zawiera |
|---|---|
| [`sketch_oct1a.ino`](sketch_oct1a/sketch_oct1a.ino) | pętla główna, przyciski, cache bloków, silnik składu, justowanie, menu, spis treści, zakładki, deep sleep |
| [`siec.ino`](sketch_oct1a/siec.ino) | Wi-Fi (domowa sieć albo własna), serwer WWW, wgrywanie i usuwanie książek, aktualizacja programu, ArduinoOTA, captive portal |
| [`strona.h`](sketch_oct1a/strona.h) | strona do wgrywania (HTML/CSS/JS w `PROGMEM`), razem z przygotowywaniem książek w JS |
| [`gra.ino`](sketch_oct1a/gra.ino) / [`gra.h`](sketch_oct1a/gra.h) | kółko i krzyżyk na dwa telefony: stan gry, miejsca graczy z wygasaniem, plansza na e-papierze |

---

## 🏗️ Architektura

```mermaid
flowchart LR
    WL["TXT z Wolnych Lektur"] --> PY["przygotuj_ksiazke.py"]
    WL --> JS["strona WWW czytnika<br/>przygotowanie w JS"]
    PY --> TXT["format CZYTNIK"]
    TXT -->|"upload przez Wi-Fi"| FS[("LittleFS<br/>pamięć flash")]
    JS --> FS
    FS --> CACHE["cache bloków 8×4 KB"]
    CACHE --> LAYOUT["silnik składu stron"]
    LAYOUT --> EPD["e-papier 7,5 cala"]
    JOY["joystick + SET/RST"] --> UI["menu / czytanie / spis treści"]
    UI --> LAYOUT
    NVS[("Preferences<br/>zakładki")] <--> UI
```

### Ciekawsze rozwiązania w firmware

- **Książka większa niż RAM** — *Pan Tadeusz* ma ~480 KB, więc tekst jest czytany z flasha blokami po 4 KB, a 8 ostatnio używanych bloków trzymanych jest w cache'u LRU.
- **Własny pomiar szerokości tekstu** — mierzenie przez bibliotekę przy każdym słowie było zbyt wolne, więc szerokości znaków (do U+017F, czyli z polskimi literami) są liczone raz i zapamiętywane dla każdej czcionki.
- **Paginacja z przeliczaniem** — przy zmianie czcionki cała książka jest dzielona na strony na nowo, a czytnik wraca do tego samego miejsca w tekście, nie do tego samego numeru strony.
- **Zakładki w NVS** — klucze w `Preferences` mogą mieć maks. 15 znaków, więc nazwa pliku jest zamieniana na hash FNV-1a; jest też migracja zakładek ze starszej wersji firmware'u.
- **Bezpieczny upload** — plik zapisuje się pod tymczasową nazwą i dopiero po skończeniu podmienia, więc przerwane wgrywanie nie psuje istniejącej książki. Nazwy plików są czyszczone ze znaków spoza `a-z0-9_-`.
- **Odświeżanie e-papieru** — szybkie częściowe odświeżanie przy przewracaniu stron i pełne co 20 stron, żeby nie zostawały powidoki.

---

## 🔧 Sprzęt

| Element | Model |
|---|---|
| Mikrokontroler | ESP32-S3-DevKitC-1 |
| Wyświetlacz | Waveshare e-Paper 7,5" 800×480 (GDEY075T7) + HAT |
| Sterowanie | joystick 5-kierunkowy + SET + RST |
| Zasilanie | LiPo 523450 + ładowarka TP4056 (USB-C) |

<details>
<summary>Podłączenie</summary>

| Sygnał | GPIO |
|---|---|
| e-papier CS / DC / RST / BUSY | 8 / 9 / 10 / 4 |
| e-papier SCK / MOSI | 18 / 17 |
| UP / DWN / LFT / RHT / MID | 5 / 6 / 7 / 15 / 16 |
| SET / RST | 21 / 47 |

Przyciski zwierają do GND (wewnętrzne pull-upy).
</details>

### Obsługa

| Przycisk | Czytanie | Menu | Spis treści |
|---|---|---|---|
| UP / DWN | poprzednia / następna strona | wybór książki | wybór rozdziału |
| LFT | początek / poprzedni rozdział | ostatnio czytana | powrót |
| RHT | następny rozdział | otwórz | przejdź |
| MID | menu (przytrzymanie: uśpij) | otwórz | przejdź |
| SET | spis treści | — | powrót |
| RST | rozmiar czcionki | — | — |

---

## 🖨️ Obudowa

![Podgląd obudowy (widok rozłożony)](projekt%20obudowy/files/podglad.png)

Projekt parametryczny w OpenSCAD: [`czytnik_eink.scad`](projekt%20obudowy/files/czytnik_eink.scad). Wszystkie wymiary (panel, luzy, położenie baterii, ESP32 i ładowarki, gniazda USB-C, przycisk z tyłu) są zmiennymi, więc obudowę łatwo dopasować do innego egzemplarza wyświetlacza.

Trzy części do druku: przednia ramka, płytka podporowa pod panel i tylna klapka. Gotowe pliki są w formatach STL i OBJ.

---

## 🚀 Uruchomienie

1. **Arduino IDE** z obsługą ESP32 oraz bibliotekami `GxEPD2` i `U8g2_for_Adafruit_GFX`.
2. Płytka: *ESP32S3 Dev Module*, **Partition Scheme** z OTA i SPIFFS, np. *Default 4MB with spiffs (1.2MB APP/1.5MB SPIFFS)*.
3. Wgraj szkic z folderu [`sketch_oct1a/`](sketch_oct1a/).
4. Przygotuj książki: `python przygotuj_ksiazke.py`. Możesz je obejrzeć w symulatorze.
5. Na czytniku wybierz **Wgraj książki (WiFi)**, otwórz w przeglądarce adres z ekranu (albo `http://czytnik.local`) i wgraj pliki z `ksiazki_do_wgrania/`.

Kolejne wersje firmware'u można wgrywać przez tę samą stronę (plik `.bin`) albo z Arduino IDE przez port sieciowy.

---

## 📁 Struktura

```
├── przygotuj_ksiazke.py      # Python: TXT z Wolnych Lektur -> format czytnika
├── PanTadeusz_WolneLektury.txt
├── treny_czesci/             # 20 plików z Trenami (źródło)
├── ksiazki_do_wgrania/       # wynik skryptu, gotowe do wgrania
├── symulator/
│   ├── czytnik.py            # port silnika składu + CLI
│   ├── u8g2_font.py          # dekoder fontów bitmapowych u8g2
│   ├── fonty/                # 6 fontów używanych przez firmware
│   └── zrzuty_do_readme.py   # generuje docs/*.png
├── tests/                    # testy unittest, uruchamiane w GitHub Actions
├── docs/                     # obrazki do README
├── sketch_oct1a/
│   ├── sketch_oct1a.ino      # główny program: skład stron, menu, zakładki, usypianie
│   ├── siec.ino              # Wi-Fi, serwer WWW, upload, OTA, captive portal
│   ├── strona.h              # strona WWW (HTML/CSS/JS w PROGMEM)
│   ├── gra.ino / gra.h       # kółko i krzyżyk
└── projekt obudowy/
    ├── files/                # OpenSCAD + STL + podgląd
    └── files obj/            # te same części w OBJ
```

---

## 🛠️ Technologie

**Python** (pathlib, re, Pillow, argparse, dataclasses, unittest) · **GitHub Actions** · **C++ / Arduino** (ESP32-S3, LittleFS, NVS, deep sleep) · **GxEPD2, U8g2** · **HTML / CSS / JavaScript** · **WebServer, DNSServer, mDNS, ArduinoOTA** · **OpenSCAD** · druk 3D

## 📜 Licencja i źródła

Teksty *Pana Tadeusza* i *Trenów* pochodzą z [Wolnych Lektur](https://wolnelektury.pl) i należą do domeny publicznej.

Fonty bitmapowe w `symulator/fonty/` pochodzą z projektu [u8g2](https://github.com/olikraus/u8g2), który rozpowszechnia je na ich oryginalnych licencjach X11/Adobe.
