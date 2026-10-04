// Tryb WiFi: strona w przeglądarce do wgrywania książek i aktualizacji programu.
//
// Czytnik najpierw łączy się z domową siecią (ustawia się ją na stronie albo niżej w DOMOWA_SIEC).
// Jeśli jej nie ma albo się nie uda, tworzy własną sieć "Czytnik" (hasło "czytaj123").
// Wtedy działa jak hotelowe WiFi: telefon sam pokazuje stronę czytnika ("Zaloguj się do sieci").
// W trybie WiFi program można też wgrać z Arduino IDE: Narzędzia > Port > czytnik (port sieciowy).

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <Update.h>
#include "strona.h"

// Domowa sieć wpisana na stałe, np. "MojaSiec" i "haslo123".
// Sieć zapisana na stronie czytnika ma pierwszeństwo.
const char* DOMOWA_SIEC = "";
const char* DOMOWE_HASLO = "";

const char* NAZWA_W_SIECI = "czytnik";  // http://czytnik.local
const char* SIEC_CZYTNIKA = "Czytnik";
const char* HASLO_CZYTNIKA = "czytaj123";

WebServer serwer(80);
DNSServer dns;  // we własnej sieci każdy adres prowadzi do czytnika
bool trasyGotowe = false;
bool laczenie = false;       // na ekranie "Łączę z siecią..."
bool wlasnaSiec = false;     // true = czytnik zrobił własną sieć
String adres;                // adres IP pokazywany na ekranie
unsigned long restartO = 0;  // kiedy zrestartować po aktualizacji (0 = nie)

// wgrywanie książki
const char* TYMCZASOWY = "/k/.wgrywany";
File plikWgrywany;
String nazwaWgrywana;
uint32_t wgrano = 0;
String bladWgrywania;

// wgrywanie programu
String bladProgramu;

void wlaczWifi() {
  zamknijKsiazke();  // książki mogą się zmienić, po wyjściu wczytamy je od nowa
  ekran = WIFI;
  laczenie = true;
  odswiez(true);

  String ssid = pamiec.isKey("ssid") ? pamiec.getString("ssid", "") : DOMOWA_SIEC;
  String haslo = pamiec.isKey("ssid") ? pamiec.getString("haslo", "") : DOMOWE_HASLO;
  WiFi.setHostname(NAZWA_W_SIECI);
  wlasnaSiec = true;
  if (ssid.length()) {
    Serial.printf("Lacze z %s\n", ssid.c_str());
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), haslo.c_str());
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) delay(100);
    wlasnaSiec = WiFi.status() != WL_CONNECTED;
  }
  if (wlasnaSiec) {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_AP);
    WiFi.softAP(SIEC_CZYTNIKA, HASLO_CZYTNIKA);
    adres = WiFi.softAPIP().toString();
    dns.start(53, "*", WiFi.softAPIP());
  } else {
    adres = WiFi.localIP().toString();
  }
  Serial.printf("WiFi: http://%s\n", adres.c_str());

  if (!trasyGotowe) {
    ustawTrasy();
    trasyGotowe = true;
  }
  serwer.begin();
  ArduinoOTA.setHostname(NAZWA_W_SIECI);
  ArduinoOTA.begin();  // uruchamia też mDNS (czytnik.local)
  MDNS.addService("http", "tcp", 80);

  laczenie = false;
  odswiez(true);
}

void wylaczRadio() {
  ArduinoOTA.end();
  MDNS.end();
  dns.stop();
  serwer.stop();
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  Serial.println("WiFi wylaczone");
}

void wylaczWifi() {
  wylaczRadio();
  wczytajListe();
  pokazMenu();
}

void obsluzWifi() {
  if (wlasnaSiec) dns.processNextRequest();
  serwer.handleClient();
  ArduinoOTA.handle();
  obsluzGre();
  if (restartO && millis() > restartO) ESP.restart();
}

void rysujEkranWifi() {
  u8g2Fonts.setFont(u8g2_font_ncenB24_te);
  naSrodku("Tryb WiFi", 80);
  display.drawLine(MARGINES, 100, display.width() - MARGINES_PRAWY, 100, GxEPD_BLACK);

  int y = 170;
  if (laczenie) {
    u8g2Fonts.setFont(u8g2_font_ncenR14_te);
    naSrodku("Włączam WiFi...", y);
    return;
  }

  // ktoś gra w kółko i krzyżyk - zamiast instrukcji plansza
  if (graNaEkranie()) {
    u8g2Fonts.setFont(u8g2_font_ncenR12_te);
    naSrodku(("http://" + adres + "/gra").c_str(), 135);
    rysujGre(y);
    rysujStopke((String("Dowolny przycisk - wyjście      wersja ") + WERSJA).c_str());
    return;
  }

  if (wlasnaSiec) {
    u8g2Fonts.setFont(u8g2_font_ncenR14_te);
    naSrodku("Połącz telefon z siecią WiFi:", y);
    u8g2Fonts.setFont(u8g2_font_ncenB18_te);
    naSrodku(SIEC_CZYTNIKA, y + 40);
    u8g2Fonts.setFont(u8g2_font_ncenR14_te);
    naSrodku((String("hasło: ") + HASLO_CZYTNIKA).c_str(), y + 75);
  } else {
    u8g2Fonts.setFont(u8g2_font_ncenR14_te);
    naSrodku("Czytnik jest w sieci:", y);
    u8g2Fonts.setFont(u8g2_font_ncenB18_te);
    naSrodku(WiFi.SSID().c_str(), y + 40);
  }

  y += 160;
  u8g2Fonts.setFont(u8g2_font_ncenR14_te);
  naSrodku("Otwórz w przeglądarce:", y);
  u8g2Fonts.setFont(u8g2_font_ncenB18_te);
  naSrodku(("http://" + adres).c_str(), y + 45);
  u8g2Fonts.setFont(u8g2_font_ncenR12_te);
  naSrodku((String("albo http://") + NAZWA_W_SIECI + ".local").c_str(), y + 85);

  if (wlasnaSiec) {
    u8g2Fonts.setFont(u8g2_font_ncenR10_te);
    naSrodku("Telefon może sam pokazać stronę (\"Zaloguj się do sieci\").", y + 150);
    naSrodku("Jeśli strona się nie otwiera: wyłącz dane komórkowe,", y + 175);
    naSrodku("a gdy telefon zapyta o sieć bez internetu - zostań połączony.", y + 195);
    naSrodku("Na stronie możesz wpisać domową sieć WiFi,", y + 235);
    naSrodku("wtedy czytnik będzie łączył się z nią.", y + 255);
  }

  rysujStopke((String("Dowolny przycisk - wyjście      wersja ") + WERSJA).c_str());
}

// --- strona ---

void ustawTrasy() {
  serwer.on("/", HTTP_GET, []() {
    // bez tego przeglądarka potrafi pokazać starą stronę (ze starym przygotowywaniem książek)
    serwer.sendHeader("Cache-Control", "no-store");
    serwer.send_P(200, "text/html; charset=utf-8", STRONA);
  });
  serwer.on("/lista", HTTP_GET, wyslijListe);
  serwer.on("/wgraj", HTTP_POST, []() {
    if (bladWgrywania.length()) serwer.send(400, "text/plain; charset=utf-8", bladWgrywania);
    else serwer.send(200, "text/plain", "OK");
  }, odbierzKsiazke);
  serwer.on("/usun", HTTP_POST, usunKsiazke);
  serwer.on("/wifi", HTTP_POST, zapiszSiec);
  ustawTrasyGry();
  serwer.on("/aktualizuj", HTTP_POST, []() {
    if (bladProgramu.length() || Update.hasError() || !Update.isFinished()) {
      String b = bladProgramu.length() ? bladProgramu : String(Update.errorString());
      serwer.send(500, "text/plain; charset=utf-8", "Błąd aktualizacji: " + b);
    } else {
      serwer.send(200, "text/plain", "OK");
      restartO = millis() + 1000;
    }
  }, odbierzProgram);
  serwer.onNotFound([]() {
    // we własnej sieci telefon sprawdza internet (np. /generate_204) - odsyłamy go na stronę
    // czytnika, wtedy sam ją pokaże jako "logowanie do sieci"
    if (wlasnaSiec) {
      serwer.sendHeader("Location", "http://" + adres + "/");
      serwer.send(302, "text/plain", "");
    } else {
      serwer.send(404, "text/plain", "Nie ma");
    }
  });
}

String jsonTekst(const String& s) {
  String w = "\"";
  for (const char* c = s.c_str(); *c; c++) {
    if (*c == '"' || *c == '\\') w += '\\';
    if ((uint8_t)*c >= 32) w += *c;
  }
  return w + "\"";
}

void wyslijListe() {
  wczytajListe();
  String j = "{\"ksiazki\":[";
  for (size_t i = 0; i < ksiazki.size(); i++) {
    if (i) j += ",";
    j += "{\"plik\":";
    j += jsonTekst(ksiazki[i].plik);
    j += ",\"tytul\":";
    j += jsonTekst(ksiazki[i].tytul);
    j += ",\"autor\":";
    j += jsonTekst(ksiazki[i].autor);
    j += ",\"rozmiar\":";
    j += String(ksiazki[i].dlugosc);
    j += "}";
  }
  j += "],\"zajete\":";
  j += String((unsigned)LittleFS.usedBytes());
  j += ",\"razem\":";
  j += String((unsigned)LittleFS.totalBytes());
  j += ",\"siec\":";
  j += jsonTekst(pamiec.getString("ssid", ""));
  j += ",\"wlasna\":";
  j += wlasnaSiec ? "true" : "false";
  j += ",\"wersja\":";
  j += jsonTekst(WERSJA);
  j += "}";
  serwer.send(200, "application/json", j);
}

// zostawia tylko a-z, 0-9, _ i -, dokleja .txt ("" gdy nic nie zostało)
String bezpiecznaNazwa(String n) {
  n.toLowerCase();
  if (n.endsWith(".txt")) n.remove(n.length() - 4);
  String w;
  for (const char* c = n.c_str(); *c && w.length() < 24; c++) {
    if ((*c >= 'a' && *c <= 'z') || (*c >= '0' && *c <= '9') || *c == '_' || *c == '-') w += *c;
  }
  return w.length() ? w + ".txt" : "";
}

// Plik zapisuje się najpierw pod tymczasową nazwą, żeby przerwane wgrywanie
// nie zepsuło książki, która już była.
void odbierzKsiazke() {
  HTTPUpload& u = serwer.upload();
  if (u.status == UPLOAD_FILE_START) {
    nazwaWgrywana = bezpiecznaNazwa(u.filename);
    bladWgrywania = nazwaWgrywana.length() ? "" : "Zła nazwa pliku";
    wgrano = 0;
    LittleFS.remove(TYMCZASOWY);
    if (!bladWgrywania.length()) {
      plikWgrywany = LittleFS.open(TYMCZASOWY, "w");
      if (!plikWgrywany) bladWgrywania = "Nie mogę utworzyć pliku";
    }
    Serial.printf("Wgrywam %s\n", nazwaWgrywana.c_str());
  } else if (u.status == UPLOAD_FILE_WRITE) {
    if (bladWgrywania.length()) return;
    if (wgrano == 0 && (u.currentSize < strlen(NAGLOWEK) || memcmp(u.buf, NAGLOWEK, strlen(NAGLOWEK)) != 0)) {
      bladWgrywania = "To nie jest plik przygotowany dla czytnika";
    } else if (plikWgrywany.write(u.buf, u.currentSize) != u.currentSize) {
      bladWgrywania = "Brak miejsca w pamięci czytnika";
    }
    wgrano += u.currentSize;
  } else if (u.status == UPLOAD_FILE_END) {
    plikWgrywany.close();
    if (!bladWgrywania.length() && wgrano == 0) bladWgrywania = "Pusty plik";
    if (bladWgrywania.length()) {
      LittleFS.remove(TYMCZASOWY);
      Serial.printf("Blad: %s\n", bladWgrywania.c_str());
      return;
    }
    String cel = "/k/" + nazwaWgrywana;
    LittleFS.remove(cel);
    if (!LittleFS.rename(TYMCZASOWY, cel)) bladWgrywania = "Nie mogę zapisać pliku";
    Serial.printf("Zapisano %s (%u B)\n", cel.c_str(), (unsigned)wgrano);
  } else if (u.status == UPLOAD_FILE_ABORTED) {
    plikWgrywany.close();
    LittleFS.remove(TYMCZASOWY);
    bladWgrywania = "Przerwano wgrywanie";
  }
}

// usuwa tylko plik, który jest na liście książek (nazwa dokładnie taka jak z /lista)
void usunKsiazke() {
  String plik = serwer.arg("plik");
  wczytajListe();
  if (plik.length() && indeksPliku(plik) >= 0 && LittleFS.remove("/k/" + plik)) {
    pamiec.remove(kluczZakladki(plik).c_str());
    Serial.printf("Usunieto %s\n", plik.c_str());
    serwer.send(200, "text/plain", "OK");
  } else {
    serwer.send(404, "text/plain; charset=utf-8", "Nie ma takiej książki");
  }
}

void zapiszSiec() {
  String ssid = serwer.arg("ssid");
  String haslo = serwer.arg("haslo");
  ssid.trim();
  // puste hasło przy tej samej sieci = bez zmian
  if (ssid != pamiec.getString("ssid", "") || haslo.length()) pamiec.putString("haslo", haslo);
  pamiec.putString("ssid", ssid);
  serwer.send(200, "text/plain; charset=utf-8", ssid.length()
    ? "Zapisano. Czytnik połączy się z tą siecią przy następnym włączeniu WiFi."
    : "Usunięto domową sieć.");
}

void odbierzProgram() {
  HTTPUpload& u = serwer.upload();
  if (u.status == UPLOAD_FILE_START) {
    Serial.printf("Aktualizacja: %s\n", u.filename.c_str());
    bladProgramu = "";
    // plik "merged" zawiera bootloader i zepsułby czytnik
    if (u.filename.indexOf("merged") >= 0) bladProgramu = "to plik merged, wybierz czytnik.ino.bin";
    else if (!Update.begin(UPDATE_SIZE_UNKNOWN)) bladProgramu = Update.errorString();
  } else if (u.status == UPLOAD_FILE_WRITE) {
    if (bladProgramu.length()) return;
    if (Update.write(u.buf, u.currentSize) != u.currentSize) bladProgramu = Update.errorString();
  } else if (u.status == UPLOAD_FILE_END) {
    if (bladProgramu.length()) Update.abort();
    else if (!Update.end(true)) bladProgramu = Update.errorString();
    Serial.printf("Aktualizacja: %s\n", bladProgramu.length() ? bladProgramu.c_str() : "OK");
  } else if (u.status == UPLOAD_FILE_ABORTED) {
    Update.abort();
    bladProgramu = "przerwano wgrywanie";
  }
}
