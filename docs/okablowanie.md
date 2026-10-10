# Okablowanie i rozmieszczenie płytek

Czytnik w pionie, widok od przodu, tak jak patrzysz na tekst. Ekran jest narysowany jak przezroczysty, więc widać przez niego płytki leżące za nim. Przy składaniu od tyłu lewa i prawa strona się zamieniają. Skala: 1 jednostka = 1 mm.

![Rozmieszczenie płytek i przewodów](okablowanie.svg)

Wymiary płytek są rzeczywiste: ekran 111×170 mm, ESP32-S3-DevKitC-1 25,5×63 mm, HAT 30×65 mm, LiPo 523450 34×50 mm, TP4056 17×28 mm, Pololu S7V8F3 11,4×15,2 mm. Wnętrze obudowy: ok. 115×174 mm. Kolejność pinów na module joysticka i padów na TP4056 zależy od wersji płytki, więc sprawdź napisy na swoich.

## Zasady prowadzenia przewodów

Numery odpowiadają znacznikom na rysunku.

1. **Gniazdo ładowania w prawej ściance, u góry.** TP4056 leży poziomo, gniazdem USB-C równo ze ścianką. Pod nim, między ładowarką a HAT-em, leży przetwornica, pinami w stronę środka. Przewody z niej schodzą szczeliną między baterią a HAT-em. Jeśli chcesz widzieć diodę ładowania, zrób przy gnieździe mały otwór.
2. **ESP na dole, antena do góry.** Gniazda USB ESP są przy dolnej ściance. Możesz zrobić otwór na gniazdo UART i wgrywać program kablem bez rozkręcania, ale zawsze najpierw odłącz baterię. Który port jest który, sprawdź po napisach na płytce. Nad anteną nie prowadź przewodów.
3. **Bateria osobno, z luzem.** Podłóż i przykryj ją taśmą kaptonową. Mocuj taśmą dwustronną, nie klejem na gorąco. Zostaw 1–2 mm luzu, bo LiPo lekko puchnie z wiekiem.
4. **Taśmę FPC zaginaj łagodnie.** Wychodzi ze środka prawej krawędzi matrycy i wraca na tył do HAT-a, którego środek jest na tej samej wysokości. Zrób łuk, nie ostre zagięcie.
5. **Dzielniki przy samym ESP.** Krótkie mają być przewody do GPIO1 i GPIO2, a długie mogą być wejścia z TP4056. Jak zbudować płytkę: [dzielnik_baterii.md](dzielnik_baterii.md).
6. **Przyciski jednym pasem wzdłuż lewej krawędzi.** SET, RST i COM dochodzą do lewej listwy ESP, a pięć kierunków przechodzi pod płytką ESP do prawej listwy (na rysunku przyciemnione). ESP stoi więc na listwach z 2–3 mm prześwitu. Białe znaczniki to miejsca na opaskę albo kroplę kleju.
7. **Joystick w tylnej klapce.** Moduł przyklej albo przykręć od środka, grzybkiem na zewnątrz, i wytnij otwór trochę większy od grzybka. Na SET i RST zrób małe otwory albo naciskaj je wykałaczką. Piny są na krótszej krawędzi modułu, ułóż ją w dół.

## Przewody

- **Zasilanie** (bateria → TP4056 → Pololu → ESP): silikonowy 24 AWG. Przy nadawaniu Wi-Fi ESP32 pobiera chwilowo ok. 0,5 A.
- **Sygnały** (przyciski, dzielniki): silikonowy 28–30 AWG albo kynar.
- **Ekran:** oryginalny kabelek PH2.0 z HAT-a, od strony ESP obcięty i przylutowany. Wtyczki Dupont dodają ok. 10 mm wysokości.

## Tabela połączeń

| Skąd | Dokąd | Uwagi |
|---|---|---|
| **Zasilanie** | | |
| bateria + | TP4056 B+ |  |
| bateria − | TP4056 B− |  |
| TP4056 OUT+ | Pololu VIN |  |
| TP4056 OUT− | Pololu GND | masa całego układu to OUT−, nie B− |
| Pololu VOUT | ESP 3V3 | nie do pinu 5V |
| Pololu GND | ESP GND | skręć razem z przewodem 3V3 |
| **Pomiar baterii (opcjonalny)** | | |
| TP4056 OUT+ | dzielnik baterii | drugi przewód na ten sam pad OUT+ |
| TP4056 IN+ (5 V z USB) | dzielnik ładowarki | pad przy gnieździe USB-C |
| dzielnik baterii | GPIO1 |  |
| dzielnik ładowarki | GPIO2 |  |
| masa dzielników | ESP GND | ten sam pin GND co przetwornica |
| **Ekran (kabelek z HAT-a)** | | |
| HAT VCC | ESP GPIO3V3 |  |
| HAT GND | ESP GND |  |
| HAT DIN | ESP GPIO17 |  |
| HAT CLK | ESP GPIO18 |  |
| HAT CS | ESP GPIO8 |  |
| HAT DC | ESP GPIO9 |  |
| HAT RST | ESP GPIO10 |  |
| HAT BUSY | ESP GPIO4 |  |
| HAT PWR | ESP GPIO3V3 |  |
| **Przyciski (joystick w tylnej klapce)** | | |
| COM | ESP GND | wspólny przewód wszystkich przycisków |
| SET | ESP GPIO21 |  |
| RST | ESP GPIO47 |  |
| MID | ESP GPIO16 |  |
| RHT | ESP GPIO15 |  |
| LFT | ESP GPIO7 |  |
| DWN | ESP GPIO6 |  |
| UP | ESP GPIO5 |  |

---

Rysunek i ta strona są generowane przez [`tools/okablowanie.py`](../tools/okablowanie.py). Po zmianie układu uruchom `python tools/okablowanie.py`.
