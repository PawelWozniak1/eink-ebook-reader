// Kółko i krzyżyk na dwa telefony w trybie WiFi (strona /gra, wygląd w gra.h).
//
// Każdy telefon losuje sobie identyfikator i co chwilę pyta czytnik o stan gry (/gra/stan).
// Pierwszy telefon gra jako X, drugi jako O, kolejne tylko oglądają.
// Telefon, który nie pytał od GRACZ_ZNIKA, zwalnia miejsce (np. ktoś zamknął stronę).
// Gdy ktoś gra, plansza jest też na ekranie czytnika (rysujGre, wołane z rysujEkranWifi).

#include <WebServer.h>
#include "gra.h"

extern WebServer serwer;  // w siec.ino (Arduino dokleja gra.ino przed siec.ino)

const unsigned long GRACZ_ZNIKA = 15000;

char plansza[10] = "         ";  // 9 pól: ' ', 'X' albo 'O'
char tura = 'X';                  // czyj ruch
char zaczyna = 'X';               // kto zaczynał tę partię (następną zaczyna drugi)
char wynik = 0;                   // 0 = gra trwa, 'X'/'O' = wygrana, 'R' = remis
int wygraneX = 0, wygraneO = 0, remisy = 0;
String graczX, graczO;
unsigned long widzianyX = 0, widzianyO = 0;
bool graZmieniona = false;        // trzeba przerysować ekran czytnika
unsigned long graZmienionaO = 0;

const uint8_t LINIE[8][3] = {{0,1,2},{3,4,5},{6,7,8},{0,3,6},{1,4,7},{2,5,8},{0,4,8},{2,4,6}};

void ustawTrasyGry() {
  serwer.on("/gra", HTTP_GET, []() {
    serwer.sendHeader("Cache-Control", "no-store");
    serwer.send_P(200, "text/html; charset=utf-8", GRA);
  });
  serwer.on("/gra/stan", HTTP_GET, wyslijStanGry);
  serwer.on("/gra/ruch", HTTP_POST, ruchWGrze);
  serwer.on("/gra/nowa", HTTP_POST, nowaGra);
}

void zmianaGry() {
  graZmieniona = true;
  graZmienionaO = millis();
}

// zwalnia miejsca telefonów, które przestały pytać (wołane też z obsluzWifi)
void wygasGraczy() {
  unsigned long teraz = millis();
  if (graczX.length() && teraz - widzianyX > GRACZ_ZNIKA) { graczX = ""; zmianaGry(); }
  if (graczO.length() && teraz - widzianyO > GRACZ_ZNIKA) { graczO = ""; zmianaGry(); }
}

// 'X', 'O' albo 0 (widz); przy okazji zajmuje wolne miejsce
char ktoryGracz(const String& id) {
  unsigned long teraz = millis();
  if (id.length() == 0) return 0;
  wygasGraczy();
  if (id == graczX) { widzianyX = teraz; return 'X'; }
  if (id == graczO) { widzianyO = teraz; return 'O'; }
  if (!graczX.length()) { graczX = id; widzianyX = teraz; zmianaGry(); return 'X'; }
  if (!graczO.length()) { graczO = id; widzianyO = teraz; zmianaGry(); return 'O'; }
  return 0;
}

// Ekran przerysowuje się dopiero po wysłaniu odpowiedzi do telefonu, bo trwa to chwilę
// i w tym czasie czytnik nie odpowiada (telefony poczekają i dostaną odpowiedź później).
void obsluzGre() {
  wygasGraczy();
  if (graZmieniona && millis() - graZmienionaO > 200) {
    graZmieniona = false;
    odswiez(false);
  }
}

// plansza na ekranie, gdy ktoś jest na stronie gry albo gra nie jest pusta
bool graNaEkranie() {
  return graczX.length() || graczO.length() || strcmp(plansza, "         ") != 0;
}

// numer wygrywającej linii z LINIE albo -1
int wygranaLinia() {
  for (int i = 0; i < 8; i++) {
    char c = plansza[LINIE[i][0]];
    if (c != ' ' && c == plansza[LINIE[i][1]] && c == plansza[LINIE[i][2]]) return i;
  }
  return -1;
}

char sprawdzWynik() {
  int l = wygranaLinia();
  if (l >= 0) return plansza[LINIE[l][0]];
  return strchr(plansza, ' ') ? 0 : 'R';
}

void wyslijStanGry() {
  char ja = ktoryGracz(serwer.arg("id"));
  String j = "{\"plansza\":\"";
  j += plansza;
  j += "\",\"tura\":\"";
  j += tura;
  j += "\",\"ja\":\"";
  if (ja) j += ja;
  j += "\",\"wynik\":\"";
  if (wynik) j += wynik;
  j += "\",\"graczy\":";
  j += String((graczX.length() ? 1 : 0) + (graczO.length() ? 1 : 0));
  j += ",\"wygraneX\":" + String(wygraneX) + ",\"wygraneO\":" + String(wygraneO) + ",\"remisy\":" + String(remisy);
  j += "}";
  serwer.sendHeader("Cache-Control", "no-store");
  serwer.send(200, "application/json", j);
}

void ruchWGrze() {
  char ja = ktoryGracz(serwer.arg("id"));
  int pole = serwer.arg("pole").toInt();
  const char* blad = nullptr;
  if (!ja) blad = "Oglądasz grę - grają już dwie osoby";
  else if (wynik) blad = "Gra skończona - zacznij nową";
  else if (!graczX.length() || !graczO.length()) blad = "Czekamy na drugiego gracza";
  else if (ja != tura) blad = "Teraz ruch przeciwnika";
  else if (pole < 0 || pole > 8 || plansza[pole] != ' ') blad = "To pole jest zajęte";
  if (blad) {
    serwer.send(400, "text/plain; charset=utf-8", blad);
    return;
  }
  plansza[pole] = ja;
  tura = ja == 'X' ? 'O' : 'X';
  wynik = sprawdzWynik();
  if (wynik == 'X') wygraneX++;
  else if (wynik == 'O') wygraneO++;
  else if (wynik == 'R') remisy++;
  serwer.send(200, "text/plain", "OK");
  zmianaGry();
}

void nowaGra() {
  if (!ktoryGracz(serwer.arg("id"))) {
    serwer.send(400, "text/plain; charset=utf-8", "Nową grę zaczynają gracze");
    return;
  }
  strcpy(plansza, "         ");
  zaczyna = zaczyna == 'X' ? 'O' : 'X';
  tura = zaczyna;
  wynik = 0;
  serwer.send(200, "text/plain", "OK");
  zmianaGry();
}

// --- ekran czytnika ---

// gruba linia (e-papier: cienka jest słabo widoczna)
void grubaLinia(int x1, int y1, int x2, int y2, int grubosc) {
  for (int d = -grubosc / 2; d <= grubosc / 2; d++) {
    if (abs(x2 - x1) > abs(y2 - y1)) display.drawLine(x1, y1 + d, x2, y2 + d, GxEPD_BLACK);
    else display.drawLine(x1 + d, y1, x2 + d, y2, GxEPD_BLACK);
  }
}

void rysujGre(int y) {
  const int POLE = 120, ROZMIAR = 3 * POLE;
  const int lewo = (display.width() - ROZMIAR) / 2, gora = y + 70;

  u8g2Fonts.setFont(u8g2_font_ncenB18_te);
  naSrodku("Kółko i krzyżyk", y);
  u8g2Fonts.setFont(u8g2_font_ncenR14_te);
  String stan;
  if (wynik == 'R') stan = "Remis!";
  else if (wynik) stan = String("Wygrywa ") + wynik + "!";
  else if (!graczX.length() || !graczO.length()) stan = "Czekam na drugi telefon...";
  else stan = String("Ruch: ") + tura;
  naSrodku(stan.c_str(), y + 38);

  for (int i = 1; i < 3; i++) {
    display.fillRect(lewo + i * POLE - 2, gora, 4, ROZMIAR, GxEPD_BLACK);
    display.fillRect(lewo, gora + i * POLE - 2, ROZMIAR, 4, GxEPD_BLACK);
  }
  for (int i = 0; i < 9; i++) {
    int sx = lewo + (i % 3) * POLE + POLE / 2, sy = gora + (i / 3) * POLE + POLE / 2;
    const int R = 36;
    if (plansza[i] == 'X') {
      grubaLinia(sx - R, sy - R, sx + R, sy + R, 7);
      grubaLinia(sx - R, sy + R, sx + R, sy - R, 7);
    } else if (plansza[i] == 'O') {
      for (int r = R - 6; r <= R; r++) display.drawCircle(sx, sy, r, GxEPD_BLACK);
    }
  }
  int l = wygranaLinia();
  if (l >= 0) {
    auto srodek = [&](int p, bool poziomo) {
      return poziomo ? lewo + (p % 3) * POLE + POLE / 2 : gora + (p / 3) * POLE + POLE / 2;
    };
    int a = LINIE[l][0], b = LINIE[l][2];
    grubaLinia(srodek(a, true), srodek(a, false), srodek(b, true), srodek(b, false), 11);
  }

  u8g2Fonts.setFont(u8g2_font_ncenR14_te);
  String mecz = "X  " + String(wygraneX) + " : " + String(wygraneO) + "  O";
  if (remisy) mecz += "     remisy: " + String(remisy);
  naSrodku(mecz.c_str(), gora + ROZMIAR + 55);
}
