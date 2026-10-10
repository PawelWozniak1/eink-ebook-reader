// Czytnik książek na e-papierze 7,5" + joystick
//
// Joystick (COM do GND), podczas czytania:
//   UP     - poprzednia strona
//   DWN    - następna strona
//   LFT    - początek rozdziału / poprzedni rozdział
//   RHT    - następny rozdział
//   MID    - menu z listą książek
//   MID przytrzymany - wyczyść ekran i uśpij (potem można odłączyć zasilanie)
//   SET    - spis treści
//   RST    - wielkość czcionki (mała / średnia / duża / bardzo duża)
//
// W spisie treści:
//   UP/DWN - wybór rozdziału
//   MID/RHT - przejdź do rozdziału
//   SET/LFT - wróć do czytania
//
// W menu:
//   UP/DWN - wybór książki
//   MID/RHT - otwórz książkę (ostatnia pozycja: tryb WiFi)
//   LFT    - wróć do ostatnio czytanej
//
// Po USPIJ_PO bez naciskania przycisków czytnik sam czyści ekran i zasypia.
// MID budzi go i otwiera książkę w tym samym miejscu.
//
// Książki są plikami w pamięci flash (LittleFS, katalog /k). Wgrywa się je przez WiFi:
// w menu wybierz "Wgraj książki (WiFi)" i otwórz w przeglądarce adres pokazany na ekranie.
// Tam można też wgrać nowy program (.bin) bez kabla. Kod WiFi jest w siec.ino.
//
// Stan baterii i ładowanie widać w menu (bateria.ino).
//
// Arduino IDE: Narzędzia > Partition Scheme musi mieć OTA i SPIFFS (tam trafia LittleFS),
// np. "Default 4MB with spiffs (1.2MB APP/1.5MB SPIFFS)". Schematy z FATFS albo "No OTA" nie zadziałają.

#include <GxEPD2_BW.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <Preferences.h>
#include <LittleFS.h>
#include <vector>
#include <algorithm>
#include <driver/rtc_io.h>

// jeśli nie działa, wróć do starego sterownika: zamień oba GxEPD2_750_GDEY075T7 na GxEPD2_750_T7
GxEPD2_BW<GxEPD2_750_GDEY075T7, GxEPD2_750_GDEY075T7::HEIGHT> display(
  GxEPD2_750_GDEY075T7(/*CS=*/ 8, /*DC=*/ 9, /*RST=*/ 10, /*BUSY=*/ 4)
);
const char* WERSJA = "6 (bateria)";  // pokazywana w trybie WiFi i na stronie

U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;
Preferences pamiec;

// --- przyciski ---
enum { GORA, DOL, LEWO, PRAWO, SRODEK, SET_, RST_, ILE_PRZYCISKOW };
const int pinyPrzyciskow[ILE_PRZYCISKOW] = {5, 6, 7, 15, 16, 21, 47};
const char* nazwyPrzyciskow[ILE_PRZYCISKOW] = {"UP", "DWN", "LFT", "RHT", "MID", "SET", "RST"};
bool byloWcisniete[ILE_PRZYCISKOW];
unsigned long wcisnietyOd[ILE_PRZYCISKOW];  // 0 = wciśnięcia nie widzieliśmy (np. trzymany przy starcie)
unsigned long ostatniaAktywnosc = 0;

// --- usypianie ---
const unsigned long DLUGIE_WCISNIECIE = 1500;          // ms przytrzymania MID, żeby uśpić
const unsigned long USPIJ_PO = 30UL * 60 * 1000;       // samo zasypia po 30 min bez przycisków

// --- układ strony (ekran pionowo 480 x 800) ---
const int MARGINES = 30;           // lewy margines
const int MARGINES_PRAWY = 30;     // prawy margines
const int GORA_TEKSTU = 40;        // gdzie zaczyna się tekst
const int MIEJSCE_NA_STOPKE = 52;  // wysokość stopki na dole
const int WCIECIE_SPACJI = 8;      // ile pikseli na jedną spację wcięcia
const int WCIECIE_ZAWINIECIA = 24; // wcięcie dalszej części zbyt długiego wersu
const int WCIECIE_AKAPITU = 24;    // wcięcie pierwszej linii akapitu prozy
const int PELNE_ODSWIEZENIE_CO = 20; // co ile stron pełne odświeżenie

// znaczniki z przygotuj_ksiazke.py (i ze strony w trybie WiFi)
// T_ZWYKLY to wers wiersza, T_AKAPIT to akapit prozy (wcięty i wyjustowany)
const char T_ZWYKLY = 0, T_KSIEGA = 1, T_PODTYTUL = 2, T_STRESZCZENIE = 3, T_TYTUL = 4, T_SRODEK = 5,
           T_AKAPIT = 6;

// --- książki w pamięci flash ---
// pierwsza linia pliku: #CZYTNIK|tytuł|autor, dalej tekst ze znacznikami
const char* NAGLOWEK = "#CZYTNIK|";

struct Ksiazka {
  String plik;       // nazwa pliku w /k
  String tytul;
  String autor;
  uint32_t dlugosc;  // długość tekstu (bez nagłówka)
};
std::vector<Ksiazka> ksiazki;

// co jest na ekranie
enum Ekran { CZYTANIE, MENU, WIFI, SPIS };
Ekran ekran = MENU;

// otwarta książka
int otwarta = -1;          // numer w 'ksiazki', -1 = żadna nie jest wczytana
String otwartyPlik;        // ostatnio czytana (zostaje też po zamknięciu)
File plikKsiazki;
uint32_t poczatekTekstu = 0;  // gdzie w pliku zaczyna się tekst (za nagłówkiem)
uint32_t DLUGOSC = 0;
bool proza = false;        // true = książka prozą (akapity), false = wierszem (wersy)
int dolTekstu;

std::vector<uint32_t> strony;          // gdzie w tekście zaczyna się każda strona
std::vector<uint32_t> ksiegi;          // gdzie w tekście zaczyna się każdy rozdział
std::vector<int> pierwszaStronaKsiegi; // numer strony, od której zaczyna się każdy rozdział
int obecna = 0;
int odswiezenOdPelnego = 0;

// menu
int wybrana = 0;
const int NA_EKRANIE = 6;  // ile pozycji menu mieści się na ekranie

// spis treści
int wybranyRozdzial = 0;
const int NA_EKRANIE_SPISU = 15;
const int WYS_SPISU = 42;  // wysokość pozycji w spisie

// wielkość czcionki (zapamiętywana, RST zmienia)
const uint8_t* const CZCIONKI_TEKSTU[] = {u8g2_font_ncenR10_te, u8g2_font_ncenR12_te,
                                          u8g2_font_ncenR14_te, u8g2_font_ncenR18_te};
const char* NAZWY_ROZMIAROW[] = {"mała", "średnia", "duża", "bardzo duża"};
const int ILE_ROZMIAROW = 4;
int rozmiar = 1;

// --- czytanie tekstu z pliku ---
// Cała książka nie mieści się w RAM, więc tekst jest czytany kawałkami (blokami),
// a kilka ostatnio używanych bloków trzymamy w pamięci.

const int BLOK = 4096, ILE_BLOKOW = 8;
struct Blok {
  int32_t nr;
  uint32_t uzyty;
  char dane[BLOK];
};
Blok bloki[ILE_BLOKOW];
uint32_t licznikUzyc = 0;

void wyczyscBufor() {
  for (Blok& b : bloki) { b.nr = -1; b.uzyty = 0; }
}

// znak nr 'i' otwartej książki (0 za końcem tekstu)
char znak(uint32_t i) {
  if (i >= DLUGOSC) return '\0';
  int32_t nr = i / BLOK;
  Blok* b = nullptr;
  Blok* najstarszy = &bloki[0];
  for (Blok& x : bloki) {
    if (x.nr == nr) { b = &x; break; }
    if (x.uzyty < najstarszy->uzyty) najstarszy = &x;
  }
  if (!b) {
    b = najstarszy;
    plikKsiazki.seek(poczatekTekstu + (uint32_t)nr * BLOK);
    plikKsiazki.read((uint8_t*)b->dane, BLOK);
    b->nr = nr;
  }
  b->uzyty = ++licznikUzyc;
  return b->dane[i % BLOK];
}

// --- lista książek ---

// czyta pierwszą linię pliku (najwyżej 300 znaków), "" gdy jej brak
String czytajNaglowek(File& f) {
  String s;
  while (f.available() && s.length() < 300) {
    char c = f.read();
    if (c == '\n') return s;
    s += c;
  }
  return "";
}

void wczytajListe() {
  ksiazki.clear();
  File katalog = LittleFS.open("/k");
  if (katalog) {
    for (File f = katalog.openNextFile(); f; f = katalog.openNextFile()) {
      String nazwa = f.name();
      if (f.isDirectory() || nazwa.startsWith(".")) continue;
      String n = czytajNaglowek(f);
      if (!n.startsWith(NAGLOWEK)) continue;
      int kreska = n.indexOf('|', strlen(NAGLOWEK));
      Ksiazka k;
      k.plik = nazwa;
      k.tytul = n.substring(strlen(NAGLOWEK), kreska < 0 ? n.length() : kreska);
      k.autor = kreska < 0 ? "" : n.substring(kreska + 1);
      k.dlugosc = f.size() - n.length() - 1;
      ksiazki.push_back(k);
    }
  }
  std::sort(ksiazki.begin(), ksiazki.end(),
            [](const Ksiazka& a, const Ksiazka& b) { return a.tytul < b.tytul; });
  Serial.printf("Ksiazek: %d\n", (int)ksiazki.size());
}

int indeksPliku(const String& plik) {
  for (int i = 0; i < (int)ksiazki.size(); i++) {
    if (ksiazki[i].plik == plik) return i;
  }
  return -1;
}

void zamknijKsiazke() {
  plikKsiazki.close();
  otwarta = -1;
}

// --- tekst ---

// Szerokości liter (o ile przesuwa się kursor) dla każdej czcionki, liczone raz i zapamiętywane.
// Mierzenie tekstu przez bibliotekę przy każdym słowie było bardzo wolne przy długich akapitach.
const int ILE_KODOW = 384;  // do U+017F - wszystkie polskie litery
struct Szerokosci {
  const uint8_t* czcionka;
  int8_t w[ILE_KODOW];  // -1 = jeszcze nie policzone
};
Szerokosci szerokosci[10];
int ileSzerokosci = 0;
const uint8_t* biezacaCzcionka = nullptr;
Szerokosci* biezaceSzer = nullptr;

void ustawFont(const uint8_t* f) {
  u8g2Fonts.setFont(f);
  biezacaCzcionka = f;
  biezaceSzer = nullptr;
}

void ustawCzcionke(char typ) {
  switch (typ) {
    case T_KSIEGA:       ustawFont(rozmiar == 3 ? u8g2_font_ncenB24_te : u8g2_font_ncenB18_te); break;
    case T_PODTYTUL:     ustawFont(u8g2_font_ncenR14_te); break;
    case T_STRESZCZENIE: ustawFont(rozmiar >= 2 ? u8g2_font_ncenR12_te : u8g2_font_ncenR10_te); break;
    case T_TYTUL:        ustawFont(u8g2_font_ncenB24_te); break;
    case T_SRODEK:       ustawFont(u8g2_font_ncenR14_te); break;
    default:             ustawFont(CZCIONKI_TEKSTU[rozmiar]); break;
  }
}

// zapisuje znak 'kod' w UTF-8, zwraca liczbę bajtów
int doUtf8(uint16_t kod, char* s) {
  if (kod < 0x80) { s[0] = kod; return 1; }
  if (kod < 0x800) { s[0] = 0xC0 | (kod >> 6); s[1] = 0x80 | (kod & 0x3F); return 2; }
  s[0] = 0xE0 | (kod >> 12); s[1] = 0x80 | ((kod >> 6) & 0x3F); s[2] = 0x80 | (kod & 0x3F);
  return 3;
}

// o ile pikseli przesuwa się tekst po znaku 'kod' w bieżącej czcionce
int przesuniecie(uint16_t kod) {
  // szerokość "Xl" minus "l" to dokładnie przesunięcie po X (biblioteka poprawia tylko ostatnią literę)
  auto zmierz = [](uint16_t k) {
    char s[5];
    int n = doUtf8(k, s);
    s[n] = 'l';
    s[n + 1] = '\0';
    return u8g2Fonts.getUTF8Width(s) - u8g2Fonts.getUTF8Width("l");
  };
  if (kod >= ILE_KODOW) return zmierz(kod);
  if (!biezaceSzer) {
    for (int i = 0; i < ileSzerokosci; i++) {
      if (szerokosci[i].czcionka == biezacaCzcionka) biezaceSzer = &szerokosci[i];
    }
    if (!biezaceSzer) {
      biezaceSzer = &szerokosci[ileSzerokosci < 10 ? ileSzerokosci++ : 0];
      biezaceSzer->czcionka = biezacaCzcionka;
      memset(biezaceSzer->w, -1, sizeof(biezaceSzer->w));
    }
  }
  if (biezaceSzer->w[kod] < 0) biezaceSzer->w[kod] = zmierz(kod);
  return biezaceSzer->w[kod];
}

// kod znaku UTF-8 zaczynającego się na pozycji 'q', w 'dl' zwraca liczbę bajtów
uint16_t kodZnaku(uint32_t q, int& dl) {
  uint8_t b = znak(q);
  if (b < 0x80) { dl = 1; return b; }
  if ((b & 0xE0) == 0xC0) { dl = 2; return ((b & 0x1F) << 6) | (znak(q + 1) & 0x3F); }
  if ((b & 0xF0) == 0xE0) {
    dl = 3;
    return ((b & 0x0F) << 12) | ((znak(q + 1) & 0x3F) << 6) | (znak(q + 2) & 0x3F);
  }
  dl = 1;
  return '?';
}

// kopiuje kawałek tekstu do bufora, żeby można go było zmierzyć albo narysować
const char* wytnij(uint32_t od, uint32_t doo) {
  static char bufor[1024];
  uint32_t n = min<uint32_t>(doo - od, sizeof(bufor) - 1);
  for (uint32_t i = 0; i < n; i++) bufor[i] = znak(od + i);
  bufor[n] = '\0';
  return bufor;
}

int szerokosc(uint32_t od, uint32_t doo) {
  return u8g2Fonts.getUTF8Width(wytnij(od, doo));
}

// zwraca, gdzie uciąć linię od 'p' do 'koniec', żeby zmieściła się w 'szer' pikselach
uint32_t dopasuj(uint32_t p, uint32_t koniec, int szer) {
  uint32_t ostatniaSpacja = 0;
  int w = 0;  // szerokość tekstu od 'p' do 'q'
  for (uint32_t q = p; q < koniec; ) {
    int dl;
    uint16_t kod = kodZnaku(q, dl);
    if (kod == ' ' && q > p) {
      if (w > szer) break;
      ostatniaSpacja = q;
    }
    w += przesuniecie(kod);
    q += dl;
  }
  if (w <= szer) return koniec;
  return ostatniaSpacja ? ostatniaSpacja : koniec;
}

// rysuje linię od 'od' do 'doo' rozciągniętą na 'szer' pikseli (odstępy między słowami równo powiększone)
void rysujWyjustowane(int x, int y, uint32_t od, uint32_t doo, int szer) {
  int slow = 0, szerSlow = 0;
  for (uint32_t p = od; p < doo; ) {
    uint32_t k = p;
    while (k < doo && znak(k) != ' ') k++;
    if (k > p) { slow++; szerSlow += szerokosc(p, k); }
    p = k + 1;
  }
  int luz = szer - szerSlow;
  int przerw = slow - 1;
  for (uint32_t p = od; p < doo; ) {
    uint32_t k = p;
    while (k < doo && znak(k) != ' ') k++;
    if (k > p) {
      int w = szerokosc(p, k);
      u8g2Fonts.drawUTF8(x, y, wytnij(p, k));
      if (przerw > 0) {
        int odstep = luz / przerw;  // reszta pikseli rozkłada się na kolejne przerwy
        luz -= odstep;
        przerw--;
        x += w + odstep;
      }
    }
    p = k + 1;
  }
}

// skraca tekst (UTF-8) w buforze, aż zmieści się w 'szer' pikselach, i dopisuje "..."
// bufor musi mieć 4 bajty zapasu
void skroc(char* s, int szer) {
  if (u8g2Fonts.getUTF8Width(s) <= szer) return;
  int n = strlen(s);
  while (n > 0) {
    do n--; while (n > 0 && (s[n] & 0xC0) == 0x80);  // nie tnij w środku polskiej litery
    while (n > 0 && s[n - 1] == ' ') n--;
    strcpy(s + n, "...");
    if (u8g2Fonts.getUTF8Width(s) <= szer) return;
  }
}

// Układa jedną stronę zaczynając od miejsca 'start' w tekście.
// Gdy rysuj == true, także ją rysuje. Zwraca miejsce, od którego zaczyna się następna strona.
uint32_t ulozStrone(uint32_t start, bool rysuj) {
  uint32_t p = start;
  int y = GORA_TEKSTU;
  bool pierwszaNaStronie = true;

  while (p < DLUGOSC) {
    uint32_t poczatekLinii = p;
    while (poczatekLinii > 0 && znak(poczatekLinii - 1) != '\n') poczatekLinii--;
    uint32_t koniecLinii = p;
    while (koniecLinii < DLUGOSC && znak(koniecLinii) != '\n') koniecLinii++;
    uint32_t nastepnaLinia = koniecLinii < DLUGOSC ? koniecLinii + 1 : DLUGOSC;

    char z = znak(poczatekLinii);
    char typ = (z >= T_KSIEGA && z <= T_AKAPIT) ? z : T_ZWYKLY;
    uint32_t tresc = poczatekLinii + (typ ? 1 : 0);
    // w prozie każda linia bez znacznika to akapit (plik przygotowany starszą wersją strony)
    if (typ == T_ZWYKLY && proza) typ = T_AKAPIT;
    bool kontynuacja = p > tresc;  // dalsza część zawiniętej linii
    uint32_t powrot = kontynuacja ? p : poczatekLinii;
    if (!kontynuacja) p = tresc;

    // każdy rozdział od nowej strony
    if (typ == T_KSIEGA && !pierwszaNaStronie) return powrot;

    // pusta linia = odstęp (na górze strony pomijany)
    if (koniecLinii == tresc) {
      if (typ == T_SRODEK) y += 150;  // odstęp na stronie tytułowej
      else if (!pierwszaNaStronie && !proza) y += 10;  // w prozie akapity bez odstępów
      p = nastepnaLinia;
      continue;
    }

    ustawCzcionke(typ);
    int wznios = u8g2Fonts.getFontAscent();
    int zejscie = u8g2Fonts.getFontDescent();  // ujemne
    int przed = (typ == T_KSIEGA && !kontynuacja) ? 60 : 0;
    bool wysrodkuj = typ != T_ZWYKLY && typ != T_AKAPIT;

    int wciecie = 0;
    if (typ == T_ZWYKLY) {
      if (kontynuacja) wciecie = WCIECIE_ZAWINIECIA;
      else while (znak(p) == ' ') { p++; wciecie += WCIECIE_SPACJI; }
    } else if (typ == T_AKAPIT && !kontynuacja) {
      wciecie = WCIECIE_AKAPITU;
      while (znak(p) == ' ') p++;
    }

    int szer = display.width() - MARGINES - MARGINES_PRAWY - wciecie;
    uint32_t koniecKawalka = dopasuj(p, koniecLinii, szer);

    int linia = y + przed + wznios;  // linia bazowa tekstu
    if (linia - zejscie > dolTekstu && !pierwszaNaStronie) return powrot;

    if (rysuj) {
      int x = MARGINES + wciecie;
      if (wysrodkuj) x = MARGINES + (display.width() - MARGINES - MARGINES_PRAWY - szerokosc(p, koniecKawalka)) / 2;
      // w prozie wszystkie linie poza ostatnią w akapicie są wyjustowane
      if (typ == T_AKAPIT && koniecKawalka < koniecLinii) rysujWyjustowane(x, linia, p, koniecKawalka, szer);
      else u8g2Fonts.drawUTF8(x, linia, wytnij(p, koniecKawalka));
    }

    y = linia - zejscie + (typ == T_STRESZCZENIE ? 3 : 5);
    pierwszaNaStronie = false;

    if (koniecKawalka >= koniecLinii) {
      if (typ == T_KSIEGA) y += 8;
      else if (typ == T_PODTYTUL) y += 12;
      else if (typ == T_STRESZCZENIE) y += 14;
      p = nastepnaLinia;
    } else {
      p = koniecKawalka;
      while (p < koniecLinii && znak(p) == ' ') p++;
    }
  }
  return DLUGOSC;
}

void podzielNaStrony() {
  unsigned long t = millis();
  strony.clear();
  ksiegi.clear();
  pierwszaStronaKsiegi.clear();

  // Proza czy wiersz? W prozie większość tekstu jest w długich liniach (akapitach),
  // w wierszu linie to krótkie wersy.
  uint32_t wszystkie = 0, wDlugich = 0, poczatekLinii = 0;
  for (uint32_t i = 0; i < DLUGOSC; i++) {
    char z = znak(i);
    if (z == T_KSIEGA && i == poczatekLinii) ksiegi.push_back(i);
    if (z == '\n' || i == DLUGOSC - 1) {
      uint32_t dl = i - poczatekLinii;
      wszystkie += dl;
      if (dl > 120) wDlugich += dl;
      poczatekLinii = i + 1;
    }
  }
  proza = wDlugich > wszystkie / 2;

  uint32_t p = 0;
  while (p < DLUGOSC) {
    strony.push_back(p);
    uint32_t nastepna = ulozStrone(p, false);
    if (nastepna <= p) break;  // zabezpieczenie przed zapętleniem
    p = nastepna;
  }
  if (strony.empty()) strony.push_back(0);  // pusta książka

  // rozdziały zawsze zaczynają się od nowej strony, więc wystarczy znaleźć stronę z tym samym początkiem
  for (uint32_t k : ksiegi) {
    int s = 0;
    while (s < (int)strony.size() - 1 && strony[s + 1] <= k) s++;
    pierwszaStronaKsiegi.push_back(s);
  }

  Serial.printf("Stron: %d, rozdzialow: %d, %s, liczenie trwalo %lu ms\n",
                (int)strony.size(), (int)ksiegi.size(), proza ? "proza" : "wiersz", millis() - t);
}

// numer rozdziału, w którym jest strona (-1 = strona tytułowa)
int ksiegaStrony(int strona) {
  int k = -1;
  for (int i = 0; i < (int)pierwszaStronaKsiegi.size(); i++) {
    if (pierwszaStronaKsiegi[i] <= strona) k = i;
  }
  return k;
}

// --- zakładki ---

// klucz w Preferences (najwyżej 15 znaków), więc zamiast nazwy pliku jej skrót
String kluczZakladki(const String& plik) {
  uint32_t h = 2166136261u;
  for (const char* c = plik.c_str(); *c; c++) {
    h ^= (uint8_t)*c;
    h *= 16777619u;
  }
  char k[12];
  snprintf(k, sizeof(k), "z%08lx", (unsigned long)h);
  return k;
}

uint32_t zakladka(int ksiazka) {
  return pamiec.getUInt(kluczZakladki(ksiazki[ksiazka].plik).c_str(), 0);
}

// zakładki ze starej wersji, w której książki były wkompilowane w program
void przeniesStareZakladki() {
  const char* stare[] = {"pan_tadeusz.txt", "treny.txt"};
  for (int i = 0; i < 2; i++) {
    String k = "k" + String(i);
    if (!pamiec.isKey(k.c_str())) continue;
    pamiec.putUInt(kluczZakladki(stare[i]).c_str(), pamiec.getUInt(k.c_str(), 0));
    pamiec.remove(k.c_str());
  }
  if (pamiec.isKey("ostatnia")) {
    int o = pamiec.getInt("ostatnia", -1);
    if (o >= 0 && o < 2) pamiec.putString("ostatniPlik", stare[o]);
    pamiec.remove("ostatnia");
  }
}

// --- rysowanie ---

void rysujStrone(int strona) {
  ulozStrone(strony[strona], true);

  if (strona == 0) return;  // strona tytułowa bez stopki

  int W = display.width();
  int H = display.height();
  display.drawLine(MARGINES, H - 42, W - MARGINES_PRAWY, H - 42, GxEPD_BLACK);

  u8g2Fonts.setFont(u8g2_font_ncenR10_te);
  char numer[32];
  snprintf(numer, sizeof(numer), "%d / %d", strona, (int)strony.size() - 1);
  int szerNumeru = u8g2Fonts.getUTF8Width(numer);
  u8g2Fonts.drawUTF8(W - MARGINES_PRAWY - szerNumeru, H - 20, numer);

  int k = ksiegaStrony(strona);
  if (k >= 0) {
    uint32_t od = ksiegi[k] + 1;
    uint32_t doo = od;
    while (doo < DLUGOSC && znak(doo) != '\n') doo++;
    // długi tytuł rozdziału (np. w Lalce) skracamy, żeby nie wszedł na numer strony
    char rozdzial[200];
    strncpy(rozdzial, wytnij(od, min<uint32_t>(doo, od + 190)), sizeof(rozdzial));
    skroc(rozdzial, W - MARGINES - MARGINES_PRAWY - szerNumeru - 20);
    u8g2Fonts.drawUTF8(MARGINES, H - 20, rozdzial);
  }
}

void naSrodku(const char* tekst, int y) {
  u8g2Fonts.drawUTF8((display.width() - u8g2Fonts.getUTF8Width(tekst)) / 2, y, tekst);
}

void rysujStopke(const char* tekst) {
  int H = display.height();
  display.drawLine(MARGINES, H - 42, display.width() - MARGINES_PRAWY, H - 42, GxEPD_BLACK);
  u8g2Fonts.setFont(u8g2_font_ncenR10_te);
  naSrodku(tekst, H - 20);
}

// tytuł rozdziału 'k' razem z podtytułem, np. "Księga pierwsza - Gospodarstwo"
// (bufor musi mieć 4 bajty zapasu na skroc())
void tytulRozdzialu(int k, char* bufor, int rozmiarBufora) {
  uint32_t od = ksiegi[k] + 1;
  uint32_t doo = od;
  while (doo < DLUGOSC && znak(doo) != '\n') doo++;
  String t = wytnij(od, min<uint32_t>(doo, od + 150));

  uint32_t p = doo + 1;
  while (p < DLUGOSC && znak(p) == '\n') p++;
  if (p < DLUGOSC && znak(p) == T_PODTYTUL) {
    uint32_t koniec = p + 1;
    while (koniec < DLUGOSC && znak(koniec) != '\n') koniec++;
    t += " - ";
    t += wytnij(p + 1, min<uint32_t>(koniec, p + 1 + 100));
  }
  strlcpy(bufor, t.c_str(), rozmiarBufora - 4);
}

void rysujSpis() {
  int W = display.width();
  int ile = ksiegi.size();

  u8g2Fonts.setFont(u8g2_font_ncenB24_te);
  naSrodku("Spis treści", 80);
  display.drawLine(MARGINES, 100, W - MARGINES_PRAWY, 100, GxEPD_BLACK);

  int od = wybranyRozdzial / NA_EKRANIE_SPISU * NA_EKRANIE_SPISU;
  if (ile > NA_EKRANIE_SPISU) {
    char str[16];
    snprintf(str, sizeof(str), "%d/%d", od / NA_EKRANIE_SPISU + 1, (ile + NA_EKRANIE_SPISU - 1) / NA_EKRANIE_SPISU);
    u8g2Fonts.setFont(u8g2_font_ncenR10_te);
    u8g2Fonts.drawUTF8(W - MARGINES_PRAWY - u8g2Fonts.getUTF8Width(str), 80, str);
  }

  u8g2Fonts.setFont(u8g2_font_ncenR12_te);
  for (int i = od; i < min(od + NA_EKRANIE_SPISU, ile); i++) {
    int y = 112 + (i - od) * WYS_SPISU;
    if (i == wybranyRozdzial) {
      display.fillRoundRect(MARGINES - 10, y, W - MARGINES - MARGINES_PRAWY + 20, WYS_SPISU - 6, 6, GxEPD_BLACK);
      u8g2Fonts.setForegroundColor(GxEPD_WHITE);
      u8g2Fonts.setBackgroundColor(GxEPD_BLACK);
    }

    char numer[12];
    snprintf(numer, sizeof(numer), "%d", pierwszaStronaKsiegi[i]);
    int szerNumeru = u8g2Fonts.getUTF8Width(numer);
    u8g2Fonts.drawUTF8(W - MARGINES_PRAWY - szerNumeru, y + 26, numer);

    char tytul[260];
    tytulRozdzialu(i, tytul, sizeof(tytul));
    skroc(tytul, W - MARGINES - MARGINES_PRAWY - szerNumeru - 20);
    u8g2Fonts.drawUTF8(MARGINES, y + 26, tytul);

    u8g2Fonts.setForegroundColor(GxEPD_BLACK);
    u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
  }

  rysujStopke("UP/DWN - wybierz    MID - przejdź    SET - wróć");
}

// napis na środku ekranu, np. na czas układania stron
void pokazKomunikat(const char* linia1, const char* linia2) {
  display.setPartialWindow(0, 0, display.width(), display.height());
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    u8g2Fonts.setFont(u8g2_font_ncenB18_te);
    naSrodku(linia1, 370);
    u8g2Fonts.setFont(u8g2_font_ncenR14_te);
    naSrodku(linia2, 420);
  } while (display.nextPage());
  odswiezenOdPelnego++;
}

void rysujMenu() {
  int W = display.width();
  int ile = ksiazki.size() + 1;  // ostatnia pozycja to tryb WiFi

  u8g2Fonts.setFont(u8g2_font_ncenB24_te);
  naSrodku("Biblioteka", 80);
  rysujBaterie(MARGINES, 80);
  display.drawLine(MARGINES, 100, W - MARGINES_PRAWY, 100, GxEPD_BLACK);

  int od = wybrana / NA_EKRANIE * NA_EKRANIE;
  if (ile > NA_EKRANIE) {
    char str[16];
    snprintf(str, sizeof(str), "%d/%d", od / NA_EKRANIE + 1, (ile + NA_EKRANIE - 1) / NA_EKRANIE);
    u8g2Fonts.setFont(u8g2_font_ncenR10_te);
    u8g2Fonts.drawUTF8(W - MARGINES_PRAWY - u8g2Fonts.getUTF8Width(str), 80, str);
  }

  const int WYS = 100;  // wysokość pozycji na liście
  for (int i = od; i < min(od + NA_EKRANIE, ile); i++) {
    int y = 125 + (i - od) * WYS;
    if (i == wybrana) {
      display.fillRoundRect(MARGINES - 10, y, W - MARGINES - MARGINES_PRAWY + 20, WYS - 12, 8, GxEPD_BLACK);
      // setFont() wyłącza przezroczystość, więc tło liter też musi być czarne
      u8g2Fonts.setForegroundColor(GxEPD_WHITE);
      u8g2Fonts.setBackgroundColor(GxEPD_BLACK);
    }

    if (i < (int)ksiazki.size()) {
      int szer = W - MARGINES - MARGINES_PRAWY - 16;
      char tekst[200];
      u8g2Fonts.setFont(u8g2_font_ncenB18_te);
      strlcpy(tekst, ksiazki[i].tytul.c_str(), sizeof(tekst) - 4);
      skroc(tekst, szer);
      u8g2Fonts.drawUTF8(MARGINES + 8, y + 34, tekst);
      u8g2Fonts.setFont(u8g2_font_ncenR12_te);
      strlcpy(tekst, ksiazki[i].autor.c_str(), sizeof(tekst) - 4);
      skroc(tekst, szer - 60);  // miejsce na procent
      u8g2Fonts.drawUTF8(MARGINES + 8, y + 62, tekst);

      char postep[32];
      uint32_t dl = max<uint32_t>(ksiazki[i].dlugosc, 1);
      snprintf(postep, sizeof(postep), "%d%%", (int)(100ULL * zakladka(i) / dl));
      u8g2Fonts.drawUTF8(W - MARGINES_PRAWY - 8 - u8g2Fonts.getUTF8Width(postep), y + 62, postep);
    } else {
      u8g2Fonts.setFont(u8g2_font_ncenB18_te);
      u8g2Fonts.drawUTF8(MARGINES + 8, y + 34, "Wgraj książki (WiFi)");
      u8g2Fonts.setFont(u8g2_font_ncenR12_te);
      u8g2Fonts.drawUTF8(MARGINES + 8, y + 62, "dodaj, usuń, zaktualizuj program");
    }

    u8g2Fonts.setForegroundColor(GxEPD_BLACK);
    u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
  }

  rysujStopke("UP/DWN - wybierz    MID - otwórz    trzymaj MID - uśpij");
}

void odswiez(bool pelne) {
  if (pelne || odswiezenOdPelnego >= PELNE_ODSWIEZENIE_CO) {
    display.setFullWindow();
    odswiezenOdPelnego = 0;
  } else {
    display.setPartialWindow(0, 0, display.width(), display.height());
    odswiezenOdPelnego++;
  }
  zmierzBaterie();
  Serial.println("  rysuje");
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    switch (ekran) {
      case MENU:     rysujMenu(); break;
      case CZYTANIE: rysujStrone(obecna); break;
      case WIFI:     rysujEkranWifi(); break;
      case SPIS:     rysujSpis(); break;
    }
  } while (display.nextPage());
  Serial.println("  przed powerOff");
  display.powerOff();

  if (ekran == CZYTANIE) {
    Serial.println("  zapisuje miejsce");
    pamiec.putUInt(kluczZakladki(otwartyPlik).c_str(), strony[obecna]);
    Serial.printf("Strona %d\n", obecna);
  }
}

// --- nawigacja ---

void pokazMenu() {
  ekran = MENU;
  int o = indeksPliku(otwartyPlik);
  if (o >= 0) wybrana = o;
  wybrana = constrain(wybrana, 0, (int)ksiazki.size());
  odswiez(true);
}

void otworz(int ksiazka) {
  if (ksiazka != otwarta) {
    plikKsiazki.close();
    plikKsiazki = LittleFS.open("/k/" + ksiazki[ksiazka].plik, "r");
    if (!plikKsiazki) {
      Serial.printf("Nie moge otworzyc %s\n", ksiazki[ksiazka].plik.c_str());
      otwarta = -1;
      pokazMenu();
      return;
    }
    otwarta = ksiazka;
    otwartyPlik = ksiazki[ksiazka].plik;
    DLUGOSC = ksiazki[ksiazka].dlugosc;
    poczatekTekstu = plikKsiazki.size() - DLUGOSC;
    wyczyscBufor();
    Serial.printf("Otwieram: %s\n", ksiazki[ksiazka].tytul.c_str());
    pokazKomunikat(ksiazki[ksiazka].tytul.c_str(), "Układam strony...");
    podzielNaStrony();
  }
  // wróć tam, gdzie skończyło się czytanie
  obecna = stronaZMiejscem(zakladka(ksiazka));

  pamiec.putString("ostatniPlik", otwartyPlik);
  ekran = CZYTANIE;
  odswiez(true);
}

// numer strony, na której jest miejsce 'miejsce' w tekście
int stronaZMiejscem(uint32_t miejsce) {
  int s = 0;
  while (s < (int)strony.size() - 1 && strony[s + 1] <= miejsce) s++;
  return s;
}

void zmienCzcionke() {
  rozmiar = (rozmiar + 1) % ILE_ROZMIAROW;
  pamiec.putInt("czcionka", rozmiar);
  uint32_t miejsce = strony[obecna];  // po zmianie zostajemy w tym samym miejscu tekstu

  char napis[48];
  snprintf(napis, sizeof(napis), "Czcionka: %s", NAZWY_ROZMIAROW[rozmiar]);
  pokazKomunikat(napis, "Układam strony...");
  podzielNaStrony();
  obecna = stronaZMiejscem(miejsce);
  odswiez(true);
}

void pokazSpis() {
  if (ksiegi.empty()) {
    pokazKomunikat("Brak rozdziałów", "Ta książka nie ma spisu treści");
    delay(1500);
    odswiez(false);
    return;
  }
  ekran = SPIS;
  wybranyRozdzial = max(0, ksiegaStrony(obecna));
  odswiez(false);
}

void wcisnietoWSpisie(int przycisk) {
  int ile = ksiegi.size();
  switch (przycisk) {
    case GORA:
      wybranyRozdzial = (wybranyRozdzial + ile - 1) % ile;
      odswiez(false);
      break;
    case DOL:
      wybranyRozdzial = (wybranyRozdzial + 1) % ile;
      odswiez(false);
      break;
    case SRODEK:
    case PRAWO:
      obecna = pierwszaStronaKsiegi[wybranyRozdzial];
      ekran = CZYTANIE;
      odswiez(false);
      break;
    case SET_:
    case LEWO:
      ekran = CZYTANIE;
      odswiez(false);
      break;
  }
}

void idzDo(int strona) {
  strona = constrain(strona, 0, (int)strony.size() - 1);
  if (strona == obecna) return;
  obecna = strona;
  odswiez(false);
}

void wcisnietoWMenu(int przycisk) {
  int ile = ksiazki.size() + 1;
  switch (przycisk) {
    case GORA:
      wybrana = (wybrana + ile - 1) % ile;
      odswiez(false);
      break;
    case DOL:
      wybrana = (wybrana + 1) % ile;
      odswiez(false);
      break;
    case SRODEK:
    case PRAWO:
      if (wybrana < (int)ksiazki.size()) otworz(wybrana);
      else wlaczWifi();
      break;
    case LEWO: {
      int o = indeksPliku(otwartyPlik);
      if (o >= 0) otworz(o);
      break;
    }
  }
}

void wcisnietoWKsiazce(int przycisk) {
  int k = ksiegaStrony(obecna);
  switch (przycisk) {
    case GORA:  idzDo(obecna - 1); break;
    case DOL:   idzDo(obecna + 1); break;
    case SET_:  pokazSpis(); break;
    case RST_:  zmienCzcionke(); break;
    case PRAWO:
      if (k + 1 < (int)pierwszaStronaKsiegi.size()) idzDo(pierwszaStronaKsiegi[k + 1]);
      break;
    case LEWO:
      if (k >= 0 && obecna > pierwszaStronaKsiegi[k]) idzDo(pierwszaStronaKsiegi[k]);
      else if (k > 0) idzDo(pierwszaStronaKsiegi[k - 1]);
      else idzDo(0);
      break;
    case SRODEK: pokazMenu(); break;
  }
}

void wcisnieto(int przycisk) {
  Serial.printf("Przycisk %s\n", nazwyPrzyciskow[przycisk]);
  switch (ekran) {
    case MENU:     wcisnietoWMenu(przycisk); break;
    case CZYTANIE: wcisnietoWKsiazce(przycisk); break;
    case WIFI:     wylaczWifi(); break;  // dowolny przycisk kończy tryb WiFi
    case SPIS:     wcisnietoWSpisie(przycisk); break;
  }
}

// Czyści ekran na biało (żeby obraz nie zostawał na nim długo bez odświeżania)
// i usypia czytnik. Miejsce w książce jest już zapisane. MID budzi.
void uspij() {
  Serial.println("Usypiam");
  if (ekran == WIFI) wylaczRadio();
  display.setFullWindow();
  display.clearScreen();
  display.hibernate();

  // gdyby MID był jeszcze trzymany, czytnik od razu by się obudził
  while (digitalRead(pinyPrzyciskow[SRODEK]) == LOW) delay(10);
  delay(50);
  zasnij();
}

// głęboki sen, z którego budzi tylko MID (ekran zostaje taki, jaki jest)
void zasnij() {
  gpio_num_t pin = (gpio_num_t)pinyPrzyciskow[SRODEK];
  rtc_gpio_pullup_en(pin);
  rtc_gpio_pulldown_dis(pin);
  esp_sleep_enable_ext0_wakeup(pin, 0);
  esp_deep_sleep_start();
}

void setup() {
  Serial.begin(115200);
  // poczekaj chwilę na Serial Monitor, żeby nic nie zginęło po resecie
  // (po obudzeniu przyciskiem nie czekamy - książka ma się pokazać od razu)
  unsigned long czekaj = millis();
  bool obudzony = esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0;
  while (!obudzony && !Serial && millis() - czekaj < 3000) delay(10);
  Serial.printf("START, wersja %s\n", WERSJA);
  // 1 = włączenie zasilania, 3 = przycisk RST/wgranie, 4 = błąd programu,
  // 5/6/7 = watchdog, 9 = za niskie napięcie (brownout)
  Serial.printf("Powod ostatniego resetu: %d\n", (int)esp_reset_reason());

  for (int i = 0; i < ILE_PRZYCISKOW; i++) {
    pinMode(pinyPrzyciskow[i], INPUT_PULLUP);
    byloWcisniete[i] = digitalRead(pinyPrzyciskow[i]) == LOW;  // np. MID, którym obudzono czytnik
    wcisnietyOd[i] = 0;
  }

  SPI.begin(18, -1, 17, 8);
  display.init(115200, true, 2, false);
  display.setRotation(1);  // pionowo: 480 x 800
  dolTekstu = display.height() - MIEJSCE_NA_STOPKE;

  u8g2Fonts.begin(display);
  u8g2Fonts.setFontMode(1);
  u8g2Fonts.setFontDirection(0);
  u8g2Fonts.setForegroundColor(GxEPD_BLACK);
  u8g2Fonts.setBackgroundColor(GxEPD_WHITE);

  // true = sformatuj, jeśli pamięć jest pusta (pierwsze uruchomienie)
  if (!LittleFS.begin(true)) Serial.println("LittleFS nie dziala - sprawdz Partition Scheme!");
  LittleFS.mkdir("/k");
  Serial.printf("Pamiec na ksiazki: %u / %u B\n", (unsigned)LittleFS.usedBytes(), (unsigned)LittleFS.totalBytes());

  pamiec.begin("czytnik", false);
  przeniesStareZakladki();
  rozmiar = constrain(pamiec.getInt("czcionka", 1), 0, ILE_ROZMIAROW - 1);
  otwartyPlik = pamiec.getString("ostatniPlik", "");
  wczytajListe();

  int o = indeksPliku(otwartyPlik);
  if (o >= 0) otworz(o);
  else pokazMenu();
  ostatniaAktywnosc = millis();
}

void loop() {
  if (ekran == WIFI) obsluzWifi();
  for (int i = 0; i < ILE_PRZYCISKOW; i++) {
    bool wcisniete = digitalRead(pinyPrzyciskow[i]) == LOW;
    if (wcisniete && !byloWcisniete[i]) {
      wcisnietyOd[i] = millis();
      ostatniaAktywnosc = millis();
      if (i != SRODEK) wcisnieto(i);  // MID działa dopiero po puszczeniu (krótko/długo)
    }
    if (i == SRODEK && wcisnietyOd[i]) {
      unsigned long trzymany = millis() - wcisnietyOd[i];
      if (wcisniete && trzymany >= DLUGIE_WCISNIECIE) uspij();
      if (!wcisniete) {
        wcisnietyOd[i] = 0;
        wcisnieto(i);
      }
    }
    byloWcisniete[i] = wcisniete;
  }
  // w trybie WiFi nie usypiamy, żeby nie przerwać wgrywania
  if (ekran != WIFI && millis() - ostatniaAktywnosc > USPIJ_PO) uspij();
  pilnujBaterii();
  delay(ekran == WIFI ? 2 : 20);
}
