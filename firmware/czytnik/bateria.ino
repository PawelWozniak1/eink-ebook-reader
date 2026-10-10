// Bateria: pomiar napięcia, procent naładowania i wykrywanie ładowarki.
// Wskaźnik jest tylko w menu, przy czytaniu nic nie przeszkadza.
//
// Potrzebne są dwa dzielniki napięcia z rezystorów 100 kΩ (razem 4 sztuki):
//
//   TP4056 OUT+ ──[100k]──┬──[100k]── GND      środek -> GPIO1 (połowa napięcia baterii)
//   TP4056 IN+  ──[100k]──┬──[100k]── GND      środek -> GPIO2 (ok. 2,5 V, gdy jest ładowarka)
//
// Wystarczą dowolne dwa jednakowe rezystory w parze (od ok. 47k do 470k), ważne, żeby dzieliły na pół.
// Dzielnik baterii cały czas pobiera ok. 20 µA - przy 1000 mAh to bez znaczenia.
// Gdy dzielników nie ma, napięcie wychodzi bliskie zeru i wskaźnik się nie pokazuje.

const int PIN_BATERII = 1;
const int PIN_LADOWARKI = 2;
const int BATERIA_ROZLADOWANA = 3300;  // mV - poniżej czytnik sam się usypia, żeby chronić baterię
const int BATERIA_PELNA = 4180;        // mV przy ładowarce - powyżej pokazujemy "pełna"

int zmierzoneMv = 0;          // ostatni pomiar, z niego rysuje się wskaźnik
bool zmierzonaLadowarka = false;

// napięcie baterii w mV (średnia z kilku pomiarów, bo ADC szumi)
int napiecieBaterii() {
  uint32_t suma = 0;
  for (int i = 0; i < 16; i++) suma += analogReadMilliVolts(PIN_BATERII);
  return suma / 16 * 2;  // dzielnik dzieli napięcie na pół
}

bool jestLadowarka() {
  return analogReadMilliVolts(PIN_LADOWARKI) > 1500;
}

bool jestPomiar() {
  return zmierzoneMv > 2500;  // bez dzielnika pin pokazuje prawie 0 V
}

void zmierzBaterie() {
  zmierzoneMv = napiecieBaterii();
  zmierzonaLadowarka = jestLadowarka();
}

// napięcie LiPo -> procent; krzywa rozładowania nie jest liniowa, więc tabela
int procentBaterii(int mV) {
  static const int16_t tabela[][2] = {
    {4200, 100}, {4100, 90}, {4000, 78}, {3900, 65}, {3800, 50}, {3750, 40},
    {3700, 30}, {3650, 20}, {3600, 12}, {3500, 5}, {3300, 0}};
  const int ile = sizeof(tabela) / sizeof(tabela[0]);
  if (mV >= tabela[0][0]) return 100;
  for (int i = 1; i < ile; i++) {
    if (mV >= tabela[i][0]) {
      int p = tabela[i][1] + (mV - tabela[i][0]) * (tabela[i - 1][1] - tabela[i][1]) /
                             (tabela[i - 1][0] - tabela[i][0]);
      return (p + 2) / 5 * 5;  // co 5%, żeby liczba nie skakała przy każdym odświeżeniu
    }
  }
  return 0;
}

// Rysuje ikonkę baterii z opisem ("87%", "ładuje", "pełna"); x to lewy brzeg, y linia bazowa tekstu.
void rysujBaterie(int x, int y) {
  if (!jestPomiar()) return;

  char opis[16];
  if (zmierzonaLadowarka) strcpy(opis, zmierzoneMv >= BATERIA_PELNA ? "pełna" : "ładuje");
  else snprintf(opis, sizeof(opis), "%d%%", procentBaterii(zmierzoneMv));

  const int SZER_IKONY = 26, WYS_IKONY = 13, ODSTEP = 6;
  int gora = y - WYS_IKONY + 1;
  display.drawRect(x, gora, SZER_IKONY, WYS_IKONY, GxEPD_BLACK);
  display.fillRect(x + SZER_IKONY, gora + 4, 3, WYS_IKONY - 8, GxEPD_BLACK);  // biegun
  if (zmierzonaLadowarka) {
    // błyskawica
    display.fillTriangle(x + 15, gora + 1, x + 8, gora + 7, x + 14, gora + 7, GxEPD_BLACK);
    display.fillTriangle(x + 12, gora + 6, x + 18, gora + 6, x + 11, gora + 12, GxEPD_BLACK);
  } else {
    int wypelnienie = (SZER_IKONY - 4) * procentBaterii(zmierzoneMv) / 100;
    display.fillRect(x + 2, gora + 2, wypelnienie, WYS_IKONY - 4, GxEPD_BLACK);
  }
  u8g2Fonts.setFont(u8g2_font_ncenR10_te);
  u8g2Fonts.drawUTF8(x + SZER_IKONY + 3 + ODSTEP, y, opis);
}

// Wywoływane z loop(): w menu po włożeniu albo wyjęciu ładowarki odświeża ekran,
// a przy rozładowanej baterii (na każdym ekranie) pokazuje komunikat i usypia czytnik.
void pilnujBaterii() {
  static unsigned long ostatnio = 0;
  static int niskich = 0;  // ile pomiarów z rzędu poniżej progu
  if (ekran == WIFI || millis() - ostatnio < 2000) return;
  ostatnio = millis();

  int mV = napiecieBaterii();
  if (mV <= 2500) return;  // brak dzielnika - nie ma czego pilnować
  bool ladowarka = jestLadowarka();

  if (ladowarka != zmierzonaLadowarka) {
    Serial.printf("Ladowarka: %s\n", ladowarka ? "podlaczona" : "odlaczona");
    if (ekran == MENU) {
      odswiez(false);  // odswiez() mierzy baterię na nowo
      return;
    }
    zmierzonaLadowarka = ladowarka;  // przy czytaniu nic nie pokazujemy, tylko zapamiętujemy
  }

  // jeden niski pomiar to może być chwilowy spadek napięcia, dopiero trzy z rzędu się liczą
  niskich = (!ladowarka && mV < BATERIA_ROZLADOWANA) ? niskich + 1 : 0;
  if (niskich >= 3) {
    Serial.printf("Bateria rozladowana (%d mV), usypiam\n", mV);
    pokazKomunikat("Bateria rozładowana", "podłącz ładowarkę i wciśnij MID");
    display.hibernate();
    zasnij();
  }
}
