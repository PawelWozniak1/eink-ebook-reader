"""Rysuje rozmieszczenie płytek i przewodów w obudowie: docs/okablowanie.svg i docs/okablowanie.md.

Widok od przodu, czytnik w pionie (tak jak przy czytaniu), ekran "przezroczysty".
Wszystkie wymiary w milimetrach. Uruchomienie: python tools/okablowanie.py
"""

from pathlib import Path
from xml.sax.saxutils import escape

DOCS = Path(__file__).parent.parent / "docs"

KOLORY = {
    "tlo": "#eef0ec", "tekst": "#1d2320", "szary": "#5b6660",
    "obudowa": "#e2e6e1", "scianka": "#8f9a92", "panel": "#c9ced4", "panel_brzeg": "#a3aab2",
    "pcb_czarna": "#26292d", "pcb_niebieska": "#2c5a9a", "pcb_zielona": "#2e7148",
    "uniwersalna": "#c7a467", "bateria": "#9ea8b0", "na_pcb": "#f1f4f1", "pin": "#d8b24a",
    "akcent": "#0b6b72", "spinka": "#f2f2ee", "obrys": "#1b1f1d", "antena": "#b8452f",
}
PRZEWODY = {
    "czerwony": "#d7302a", "czarny": "#1f1f1f", "szary": "#9b9b9b", "brazowy": "#7b4a28",
    "niebieski": "#2f6fd6", "zolty": "#e7c22e", "pomaranczowy": "#ec7a1c", "zielony": "#2f9a4a",
    "bialy": "#f6f6f2", "fioletowy": "#8b4ad1", "turkusowy": "#15a7a3", "rozowy": "#d6409f",
}

# Listwy ESP32-S3-DevKitC-1 od strony anteny. Patrząc przez ekran widać płytkę od spodu,
# więc J3 jest po lewej, a J1 po prawej.
J1 = ["3V3", "3V3", "RST", "4", "5", "6", "7", "15", "16", "17", "18",
      "8", "3", "46", "9", "10", "11", "12", "13", "14", "5V", "GND"]
J3 = ["GND", "TX", "RX", "1", "2", "42", "41", "40", "39", "38", "37",
      "36", "35", "0", "45", "48", "47", "21", "20", "19", "GND", "GND"]
X_J3, X_J1 = 45.5, 68


def y_pinu(i):
    return 113.5 + 2.54 * i


class Rysunek:
    def __init__(self):
        self.warstwy = {n: [] for n in ("baza", "plytki", "przewody", "nad", "piny", "opisy", "znaczniki")}
        self.uzyte = set()
        self.przewody = []  # (grupa, kolor, punkty, opis)

    def dodaj(self, warstwa, tag, tekst=None, **atrybuty):
        attr = " ".join(f'{k.rstrip("_").replace("_", "-")}="{v}"' for k, v in atrybuty.items())
        self.warstwy[warstwa].append(f"<{tag} {attr}>{escape(tekst)}</{tag}>" if tekst is not None else f"<{tag} {attr}/>")

    def napis(self, x, y, rozmiar, tekst, kolor=KOLORY["tekst"], obrot=False, **inne):
        if obrot:
            inne["transform"] = f"rotate(-90 {x} {y})"
        self.dodaj("opisy", "text", tekst, x=x, y=y, font_size=rozmiar, fill=kolor, **inne)

    def plytka(self, x, y, w, h, kolor):
        self.dodaj("plytki", "rect", x=x, y=y, width=w, height=h, rx=1, fill=kolor,
                   stroke=KOLORY["obrys"], stroke_width=0.25)

    def pin(self, rzad, nazwa, ktory=0):
        lista = J1 if rzad == "J1" else J3
        idx = [i for i, n in enumerate(lista) if n == nazwa][ktory]
        self.uzyte.add((rzad, idx))
        return (X_J1 if rzad == "J1" else X_J3, round(y_pinu(idx), 2))

    def przewod(self, grupa, kolor, punkty, opis):
        self.przewody.append((grupa, kolor, punkty, opis))

    def svg(self):
        tresc = "\n".join(sum(self.warstwy.values(), []))
        return (f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="-10 -8 142 194" width="710" height="970" '
                f'font-family="DejaVu Sans Mono, Consolas, monospace">\n'
                f'<rect x="-10" y="-8" width="142" height="194" fill="{KOLORY["tlo"]}"/>\n{tresc}\n</svg>\n')


def narysuj():
    r = Rysunek()
    K, P_ = KOLORY, PRZEWODY
    na_pcb = dict(text_anchor="middle")

    # obudowa i ekran
    r.dodaj("baza", "rect", x=-3, y=-3, width=121, height=180, rx=8, fill=K["scianka"])
    r.dodaj("baza", "rect", x=0, y=0, width=115, height=174, rx=6, fill=K["obudowa"])
    r.dodaj("baza", "rect", x=2, y=2, width=111, height=170, rx=1.5, fill=K["panel"], stroke=K["panel_brzeg"], stroke_width=0.4)
    r.napis(93, 160, 2.2, 'ekran 7,5"', K["szary"], text_anchor="middle")
    r.napis(93, 163, 1.8, "patrzysz przez niego", K["szary"], text_anchor="middle")
    r.dodaj("baza", "rect", x=114.4, y=20.5, width=4, height=8, rx=1, fill=K["tlo"], stroke=K["tekst"], stroke_width=0.25)
    r.napis(122, 32, 2.1, "USB-C ładowanie", obrot=True, text_anchor="middle")
    r.dodaj("baza", "rect", x=45.5, y=173.4, width=8.5, height=4, rx=1, fill=K["tlo"], stroke=K["tekst"],
            stroke_width=0.25, stroke_dasharray="1 .6")
    r.napis(49.75, 182, 2, "otwór UART (opcja)", K["szary"], text_anchor="middle")
    r.napis(57.5, -4.5, 2.1, "GÓRA (tak trzymasz przy czytaniu)", K["szary"], text_anchor="middle")

    # joystick w tylnej klapce, piny na dolnej (krótszej) krawędzi
    r.plytka(8, 16, 24, 32, K["pcb_czarna"])
    r.dodaj("plytki", "circle", cx=20, cy=27, r=5.5, fill=K["panel_brzeg"], stroke=K["na_pcb"], stroke_width=0.25)
    r.dodaj("plytki", "circle", cx=20, cy=27, r=2.2, fill=K["pcb_czarna"])
    for x in (10.5, 25.5):
        r.dodaj("plytki", "rect", x=x, y=36, width=4, height=4, rx=0.5, fill=K["panel_brzeg"])
    r.napis(12.5, 35, 1.5, "SET", K["na_pcb"], **na_pcb)
    r.napis(27.5, 35, 1.5, "RST", K["na_pcb"], **na_pcb)
    r.napis(20, 19, 1.7, "joystick", K["na_pcb"], **na_pcb)

    # bateria, wyprowadzenia u góry
    r.plytka(38, 8, 34, 50, K["bateria"])
    r.dodaj("plytki", "rect", x=60, y=8, width=9, height=2, fill=K["pin"])
    r.napis(55, 33, 2.5, "LiPo 523450", text_anchor="middle", font_weight=600)
    r.napis(55, 36, 1.8, "1000 mAh · 3,7 V", text_anchor="middle")

    # TP4056 poziomo, gniazdo w prawą ściankę
    r.plytka(86, 16, 28, 17, K["pcb_niebieska"])
    r.dodaj("plytki", "rect", x=111, y=20.5, width=4, height=8, rx=0.6, fill=K["panel_brzeg"], stroke=K["obrys"], stroke_width=0.2)
    r.napis(100, 24.4, 2.4, "TP4056", K["na_pcb"], font_weight=600, **na_pcb)
    r.napis(100, 27, 1.7, "USB-C →", K["na_pcb"], **na_pcb)

    # przetwornica poziomo pod ładowarką, piny w stronę środka
    r.plytka(88, 38, 15.2, 11.4, K["pcb_zielona"])
    r.napis(98.4, 43.2, 1.9, "S7V8F3", K["na_pcb"], font_weight=600, **na_pcb)
    r.napis(98.4, 45.8, 1.5, "3,3 V", K["na_pcb"], **na_pcb)

    # HAT przy prawej krawędzi, środkiem na wysokości taśmy FPC
    r.plytka(76, 54.5, 30, 65, K["pcb_niebieska"])
    r.napis(97, 108, 2.4, "e-Paper Driver HAT", K["na_pcb"], obrot=True, font_weight=600, **na_pcb)
    r.dodaj("plytki", "rect", x=104, y=80, width=9, height=14, fill=P_["zolty"], opacity=0.55)
    r.dodaj("plytki", "path", d="M113 80 L113.6 80 Q114.6 80 114.6 81 L114.6 93 Q114.6 94 113.6 94 L113 94",
            fill="none", stroke=P_["zolty"], stroke_width=0.5)
    r.napis(110.5, 104, 1.8, "taśma FPC", K["szary"], obrot=True, text_anchor="middle")

    # dzielniki na płytce uniwersalnej; masa od spodu płytki (przerywana)
    r.plytka(30, 98, 12, 20, K["uniwersalna"])
    r.napis(36, 96.6, 1.6, "4×100k + 100 nF", text_anchor="middle")
    sciezka = dict(fill="none", stroke=K["tekst"], stroke_width=0.25)
    r.dodaj("plytki", "path", d="M33 100 V101.5 M33 105 V108.5 M33 112 V113 M39 100 V101.5 M39 105 V108.5 "
            "M39 112 V113 M33 107 H35.5 V118 M39 107 H37.5 V118 M37.5 111 H38.6 M40 111 H40.6 V113", **sciezka)
    r.dodaj("plytki", "path", d="M33 113 H41.5 V116", stroke_dasharray=".8 .5", **sciezka)
    r.dodaj("plytki", "path", d="M38.6 109.9 V112.1 M40 109.9 V112.1", stroke=K["tekst"], stroke_width=0.35)
    for x, y in ((33, 101.5), (33, 108.5), (39, 101.5), (39, 108.5)):
        r.dodaj("plytki", "rect", x=x - 0.9, y=y, width=1.8, height=3.5, rx=0.5, fill="#d8c7a0", stroke=K["tekst"], stroke_width=0.2)

    # ESP32 pionowo: antena u góry, gniazda USB na dole
    r.plytka(44, 108, 25.5, 63, K["pcb_czarna"])
    for x, nazwa in ((46, "UART"), (59.5, "USB")):
        r.dodaj("plytki", "rect", x=x, y=169, width=8, height=5.5, rx=1, fill=K["panel_brzeg"], stroke=K["obrys"], stroke_width=0.25)
        r.napis(x + 4, 167.8, 1.5, nazwa, K["na_pcb"], font_weight=600, **na_pcb)

    # zasilanie
    r.przewod("zasilanie", P_["czerwony"], [(62, 8), (62, 5), (81.5, 5), (81.5, 22.5), (88.5, 22.5)], "bateria + → TP4056 B+")
    r.przewod("zasilanie", P_["czarny"], [(67, 8), (67, 6.5), (80, 6.5), (80, 26.5), (88.5, 26.5)], "bateria − → TP4056 B−")
    r.przewod("zasilanie", P_["czerwony"], [(88.5, 18.5), (84, 18.5), (84, 42.1), (89.2, 42.1)], "TP4056 OUT+ → Pololu VIN")
    r.przewod("zasilanie", P_["czarny"], [(88.5, 30.5), (85.5, 30.5), (85.5, 44.7), (89.2, 44.7)], "TP4056 OUT− → Pololu GND")
    r.przewod("zasilanie", P_["czerwony"], [(89.2, 47.2), (74.8, 47.2), (74.8, 90), (69, 90), (69, 113.5), r.pin("J1", "3V3")],
              "Pololu VOUT → ESP 3V3")
    r.przewod("zasilanie", P_["czarny"], [(89.2, 45.9), (73.6, 45.9), (73.6, 88.5), (43, 88.5), (43, 113.5), r.pin("J3", "GND")],
              "Pololu GND → ESP GND")

    # pomiar baterii
    r.przewod("pomiar", P_["rozowy"], [(88.5, 18.5), (88.5, 14.5), (75.6, 14.5), (75.6, 79), (39, 79), (39, 100)],
              "TP4056 OUT+ → dzielnik baterii")
    r.przewod("pomiar", P_["rozowy"], [(110.5, 18.5), (110.5, 13.5), (72.6, 13.5), (72.6, 77.5), (33, 77.5), (33, 100)],
              "TP4056 IN+ (5 V z USB) → dzielnik ładowarki")
    r.przewod("pomiar", P_["rozowy"], [(37.5, 118), (37.5, 121.12), r.pin("J3", "1")], "dzielnik baterii → GPIO1")
    r.przewod("pomiar", P_["rozowy"], [(35.5, 118), (35.5, 123.66), r.pin("J3", "2")], "dzielnik ładowarki → GPIO2")
    r.przewod("pomiar", P_["czarny"], [(41.5, 116), (43, 116), (43, 113.5)], "masa dzielników → ESP GND")

    # ekran: 9-pinowe złącze na lewej krawędzi HAT-a, kolory jak w kabelku Waveshare
    hat = [("VCC", "3V3", 1, "szary"), ("GND", "GND", 0, "brazowy"), ("DIN", "17", 0, "niebieski"),
           ("CLK", "18", 0, "zolty"), ("CS", "8", 0, "pomaranczowy"), ("DC", "9", 0, "zielony"),
           ("RST", "10", 0, "bialy"), ("BUSY", "4", 0, "fioletowy"), ("PWR", "3V3", 1, "czerwony")]
    for i, (nazwa, pin, ktory, kolor) in enumerate(hat):
        tx, ty = r.pin("J1", pin, ktory)
        y, tor = 98 + 2.5 * i, round(69.8 + 0.7 * i, 2)
        if nazwa == "PWR":
            punkty = [(76, y), (tor, y), (tor, ty + 0.9), (tx + 1.2, ty + 0.9), (tx, ty)]
        else:
            punkty = [(76, y), (tor, y), (tor, ty), (tx, ty)]
        r.przewod("ekran", P_[kolor], punkty, f"HAT {nazwa} → ESP {'GPIO' if pin[0].isdigit() else ''}{pin}")

    # przyciski: pas wzdłuż lewej krawędzi; kierunki przechodzą pod ESP do prawej listwy
    przyciski = [("COM", "J3", "GND", 1), ("SET", "J3", "21", 0), ("RST", "J3", "47", 0), ("MID", "J1", "16", 0),
                 ("RHT", "J1", "15", 0), ("LFT", "J1", "7", 0), ("DWN", "J1", "6", 0), ("UP", "J1", "5", 0)]
    kierunki = ["UP", "DWN", "LFT", "RHT", "MID"]
    for i, (nazwa, rzad, pin, ktory) in enumerate(przyciski):
        x = round(11.1 + 2.54 * i, 2)
        p = r.pin(rzad, pin, ktory)
        if rzad == "J3":
            punkty = [(x, 48), (x, p[1]), p]
        else:
            k = kierunki.index(nazwa)
            tor, zakret = round(137.63 + 2.54 * k, 2), 62.6 + k
            punkty = [(x, 48), (x, tor), (zakret, tor), (zakret, p[1]), p]
        kolor = P_["czarny"] if nazwa == "COM" else P_["turkusowy"]
        r.przewod("przyciski", kolor, punkty, f"{nazwa} → ESP {'GND' if pin == 'GND' else 'GPIO' + pin}")

    for grupa, kolor, punkty, opis in r.przewody:
        d = " ".join(("M" if i == 0 else "L") + f"{x:g} {y:g}" for i, (x, y) in enumerate(punkty))
        wspolne = dict(d=d, fill="none", stroke_linejoin="round", stroke_linecap="round")
        r.dodaj("przewody", "path", stroke=K["obrys"], stroke_width=0.95, **wspolne)
        r.dodaj("przewody", "path", stroke=kolor, stroke_width=0.55, **wspolne)
    r.dodaj("przewody", "circle", cx=43, cy=113.5, r=0.6, fill=K["obrys"])

    # płytka ESP przykrywa przewody, które biegną pod nią
    r.dodaj("nad", "rect", x=44, y=108, width=25.5, height=63, rx=1, fill=K["pcb_czarna"], opacity=0.72)
    r.dodaj("nad", "rect", x=46.5, y=108.6, width=20.5, height=8, fill="none", stroke=K["antena"],
            stroke_width=0.35, stroke_dasharray="1 .7")
    r.napis(56.75, 105.8, 1.8, "antena", K["antena"], text_anchor="middle")
    r.napis(56.2, 142, 2.3, "ESP32-S3-DevKitC-1", K["na_pcb"], obrot=True, font_weight=600, **na_pcb)
    r.napis(59, 142, 1.5, "widok od spodu płytki", K["na_pcb"], obrot=True, opacity=0.75, **na_pcb)

    # piny
    def kropka(x, y, wlaczony=True):
        r.dodaj("piny", "rect", x=round(x - 0.55, 2), y=round(y - 0.55, 2), width=1.1, height=1.1,
                fill=K["pin"] if wlaczony else "none", stroke=K["pin"], stroke_width=0.2)

    for rzad, lista, x, dx, kotwica in (("J3", J3, X_J3, 1.4, "start"), ("J1", J1, X_J1, -1.4, "end")):
        for i, nazwa in enumerate(lista):
            uzyty = (rzad, i) in r.uzyte
            y = round(y_pinu(i), 2)
            kropka(x, y, uzyty)
            r.napis(x + dx, round(y + 0.5, 2), 1.35, nazwa, K["na_pcb"], text_anchor=kotwica,
                    opacity=1 if uzyty else 0.4, font_weight=600 if uzyty else 400)
    for i, (nazwa, *_) in enumerate(hat):
        kropka(76, 98 + 2.5 * i)
        r.napis(77.4, 98.5 + 2.5 * i, 1.4, nazwa, K["na_pcb"])
    for i, (nazwa, *_) in enumerate(przyciski):
        x = round(11.1 + 2.54 * i, 2)
        kropka(x, 48)
        r.napis(round(x + 0.5, 2), 46.6, 1.35, nazwa, K["na_pcb"], obrot=True)
    for nazwa, x, y in (("OUT+", 88.5, 18.5), ("B+", 88.5, 22.5), ("B−", 88.5, 26.5), ("OUT−", 88.5, 30.5),
                        ("IN+", 110.5, 18.5), ("IN−", 110.5, 30.5)):
        r.dodaj("piny", "circle", cx=x, cy=y, r=0.8, fill=K["pin"])
        lewy = x < 100
        r.napis(x + 1.4 if lewy else x - 1.4, y + 0.5, 1.35, nazwa, K["na_pcb"], text_anchor="start" if lewy else "end")
    for nazwa, y in (("SHDN", 39.6), ("VIN", 42.1), ("GND", 44.7), ("VOUT", 47.2)):
        kropka(89.2, y, nazwa != "SHDN")
        r.napis(90.4, y + 0.45, 1.25, nazwa, K["na_pcb"])
    for nazwa, x, y, kotwica, dx in (("5V", 33, 100, "end", -1), ("VBAT", 39, 100, "start", 1), ("2", 35.5, 118, "end", -0.9),
                                     ("1", 37.5, 118, "start", 0.9), ("GND", 41.5, 116, "start", 1)):
        r.dodaj("piny", "circle", cx=x, cy=y, r=0.7, fill=K["pin"])
        r.napis(x + dx, y + (-0.6 if y < 105 else 1.9), 1.3, nazwa, text_anchor=kotwica)

    # spinki na wiązkach
    def spinka(x, y, w, h):
        r.dodaj("znaczniki", "rect", x=x, y=y, width=w, height=h, rx=0.5, fill=K["spinka"], stroke=K["obrys"], stroke_width=0.2)

    for y, x1, x2 in ((70, 11.1, 28.9), (105, 11.1, 28.9), (130, 70.5, 74), (64, 72.6, 75.6)):
        spinka(round(x1 - 0.9, 2), y - 0.7, round(x2 - x1 + 1.8, 2), 1.4)
    spinka(49.3, 76.6, 1.4, 3.3)

    # znaczniki do listy zasad w okablowanie.md
    for n, x, y in ((1, 122, 14), (2, 38, 178), (3, 44, 13), (4, 109.5, 76.5), (5, 36, 92), (6, 5, 88), (7, 35, 30)):
        r.dodaj("znaczniki", "circle", cx=x, cy=y, r=2.3, fill=K["akcent"])
        r.dodaj("znaczniki", "text", str(n), x=x, y=y + 0.95, font_size=2.6, font_weight=600, text_anchor="middle", fill="#ffffff")

    return r


GRUPY = {"zasilanie": "Zasilanie", "pomiar": "Pomiar baterii (opcjonalny)",
         "ekran": "Ekran (kabelek z HAT-a)", "przyciski": "Przyciski (joystick w tylnej klapce)"}
UWAGI = {
    "TP4056 OUT− → Pololu GND": "masa całego układu to OUT−, nie B−",
    "Pololu VOUT → ESP 3V3": "nie do pinu 5V",
    "Pololu GND → ESP GND": "skręć razem z przewodem 3V3",
    "TP4056 OUT+ → dzielnik baterii": "drugi przewód na ten sam pad OUT+",
    "TP4056 IN+ (5 V z USB) → dzielnik ładowarki": "pad przy gnieździe USB-C",
    "masa dzielników → ESP GND": "ten sam pin GND co przetwornica",
    "HAT VCC → ESP 3V3": "ten sam pin co PWR",
    "HAT PWR → ESP 3V3": "tylko nowsze HAT-y (złącze 9-pinowe); ten sam pin co VCC",
    "COM → ESP GND": "wspólny przewód wszystkich przycisków",
}

ZASADY = """\
1. **Gniazdo ładowania w prawej ściance, u góry.** TP4056 leży poziomo, gniazdem USB-C równo ze ścianką. Pod nim, między ładowarką a HAT-em, leży przetwornica, pinami w stronę środka. Przewody z niej schodzą szczeliną między baterią a HAT-em. Jeśli chcesz widzieć diodę ładowania, zrób przy gnieździe mały otwór.
2. **ESP na dole, antena do góry.** Gniazda USB ESP są przy dolnej ściance. Możesz zrobić otwór na gniazdo UART i wgrywać program kablem bez rozkręcania, ale zawsze najpierw odłącz baterię. Który port jest który, sprawdź po napisach na płytce. Nad anteną nie prowadź przewodów.
3. **Bateria osobno, z luzem.** Podłóż i przykryj ją taśmą kaptonową. Mocuj taśmą dwustronną, nie klejem na gorąco. Zostaw 1–2 mm luzu, bo LiPo lekko puchnie z wiekiem.
4. **Taśmę FPC zaginaj łagodnie.** Wychodzi ze środka prawej krawędzi matrycy i wraca na tył do HAT-a, którego środek jest na tej samej wysokości. Zrób łuk, nie ostre zagięcie.
5. **Dzielniki przy samym ESP.** Krótkie mają być przewody do GPIO1 i GPIO2, a długie mogą być wejścia z TP4056. Jak zbudować płytkę: [dzielnik_baterii.md](dzielnik_baterii.md).
6. **Przyciski jednym pasem wzdłuż lewej krawędzi.** SET, RST i COM dochodzą do lewej listwy ESP, a pięć kierunków przechodzi pod płytką ESP do prawej listwy (na rysunku przyciemnione). ESP stoi więc na listwach z 2–3 mm prześwitu. Białe znaczniki to miejsca na opaskę albo kroplę kleju.
7. **Joystick w tylnej klapce.** Moduł przyklej albo przykręć od środka, grzybkiem na zewnątrz, i wytnij otwór trochę większy od grzybka. Na SET i RST zrób małe otwory albo naciskaj je wykałaczką. Piny są na krótszej krawędzi modułu, ułóż ją w dół."""


def markdown(r):
    wiersze = []
    for grupa, tytul in GRUPY.items():
        wiersze.append(f"| **{tytul}** | | |")
        for g, _, _, opis in r.przewody:
            if g == grupa:
                skad, dokad = opis.split(" → ")
                wiersze.append(f"| {skad} | {dokad} | {UWAGI.get(opis, '')} |")
    tabela = "\n".join(wiersze)
    return f"""# Okablowanie i rozmieszczenie płytek

Czytnik w pionie, widok od przodu, tak jak patrzysz na tekst. Ekran jest narysowany jak przezroczysty, więc widać przez niego płytki leżące za nim. Przy składaniu od tyłu lewa i prawa strona się zamieniają. Skala: 1 jednostka = 1 mm.

![Rozmieszczenie płytek i przewodów](okablowanie.svg)

Wymiary płytek są rzeczywiste: ekran 111×170 mm, ESP32-S3-DevKitC-1 25,5×63 mm, HAT 30×65 mm, LiPo 523450 34×50 mm, TP4056 17×28 mm, Pololu S7V8F3 11,4×15,2 mm. Wnętrze obudowy: ok. 115×174 mm. Kolejność pinów na module joysticka i padów na TP4056 zależy od wersji płytki, więc sprawdź napisy na swoich.

## Zasady prowadzenia przewodów

Numery odpowiadają znacznikom na rysunku.

{ZASADY}

## Przewody

- **Zasilanie** (bateria → TP4056 → Pololu → ESP): silikonowy 24 AWG. Przy nadawaniu Wi-Fi ESP32 pobiera chwilowo ok. 0,5 A.
- **Sygnały** (przyciski, dzielniki): silikonowy 28–30 AWG albo kynar.
- **Ekran:** oryginalny kabelek PH2.0 z HAT-a, od strony ESP obcięty i przylutowany. Wtyczki Dupont dodają ok. 10 mm wysokości.

## Tabela połączeń

| Skąd | Dokąd | Uwagi |
|---|---|---|
{tabela}

---

Rysunek i ta strona są generowane przez [`tools/okablowanie.py`](../tools/okablowanie.py). Po zmianie układu uruchom `python tools/okablowanie.py`.
"""


if __name__ == "__main__":
    rysunek = narysuj()
    (DOCS / "okablowanie.svg").write_text(rysunek.svg(), encoding="utf-8", newline="\n")
    (DOCS / "okablowanie.md").write_text(markdown(rysunek), encoding="utf-8", newline="\n")
    print("docs/okablowanie.svg, docs/okablowanie.md")
