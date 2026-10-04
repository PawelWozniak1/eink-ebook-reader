// Strona pokazywana w przeglądarce w trybie WiFi (siec.ino).
// Zwykłe pliki TXT z Wolnych Lektur przygotowuje sama strona (funkcja przygotuj),
// tak jak przygotuj_ksiazke.py: nagłówek #CZYTNIK|tytuł|autor i znaczniki rozdziałów.
#pragma once

const char STRONA[] PROGMEM = R"rawliteral(<!doctype html>
<html lang="pl">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Czytnik</title>
<style>
body{font-family:system-ui,sans-serif;max-width:640px;margin:0 auto;padding:16px;background:#f4f2ec;color:#222}
h1{font-size:1.6em;margin:.2em 0 .6em}
section{background:#fff;border-radius:10px;padding:14px 16px;margin-bottom:16px;box-shadow:0 1px 3px #0002}
h2{font-size:1.15em;margin:0 0 .6em}
.k{display:flex;align-items:center;gap:10px;padding:8px 0;border-top:1px solid #eee}
.k:first-child{border-top:0}
.k div{flex:1}
.k small{color:#666}
button{font:inherit;padding:8px 14px;border:0;border-radius:8px;background:#2d5d8a;color:#fff;cursor:pointer}
button.usun{background:#b33;padding:6px 10px}
button:disabled{opacity:.5}
input[type=text],input[type=password]{font:inherit;width:100%;box-sizing:border-box;padding:8px;margin:4px 0 10px;border:1px solid #bbb;border-radius:6px}
progress{width:100%;height:14px}
.info{color:#555;font-size:.9em}
#stan{white-space:pre-line}
</style>
</head>
<body>
<h1>📖 Czytnik</h1>

<section>
<h2>Gra</h2>
<p class="info">Kółko i krzyżyk na dwa telefony połączone z czytnikiem.</p>
<p><button onclick="location.href='/gra'">Zagraj w kółko i krzyżyk</button></p>
</section>

<section>
<h2>Książki</h2>
<div id="lista">Wczytuję…</div>
<p class="info" id="miejsce"></p>
</section>

<section>
<h2>Dodaj książkę</h2>
<p class="info">Pobierz książkę z <b>wolnelektury.pl</b> w formacie <b>TXT</b> i wybierz ją tutaj (można kilka naraz).
Plik o tej samej nazwie zostanie zastąpiony.</p>
<input type="file" id="pliki" accept=".txt,text/plain" multiple>
<p><button id="wgraj">Wgraj</button></p>
<progress id="postep" value="0" max="1" hidden></progress>
<p id="stan"></p>
</section>

<section>
<h2>Domowa sieć WiFi</h2>
<p class="info" id="siecInfo"></p>
<form id="siec">
<label>Nazwa sieci<input type="text" name="ssid" id="ssid" autocomplete="off"></label>
<label>Hasło<input type="password" name="haslo" placeholder="puste = bez zmian"></label>
<button>Zapisz</button> <span id="siecStan"></span>
</form>
</section>

<section>
<h2>Aktualizacja programu</h2>
<p class="info">W Arduino IDE: <i>Szkic → Eksportuj skompilowany plik binarny</i>.
Potem wybierz tutaj plik <b>czytnik.ino.bin</b> z folderu <i>build</i> obok szkicu
(nie ten z „merged” w nazwie).</p>
<input type="file" id="bin" accept=".bin">
<p><button id="aktualizuj">Wgraj program</button></p>
<progress id="postepBin" value="0" max="1" hidden></progress>
<p id="stanBin"></p>
</section>

<script>
const $ = id => document.getElementById(id);
const kb = n => n >= 1048576 ? (n / 1048576).toFixed(1) + ' MB' : Math.round(n / 1024) + ' KB';

// czcionki czytnika nie mają tych znaków
const ZAMIANY = {'—': '-', '–': '-', '…': '...', '„': '"', '”': '"', '“': '"', '’': "'", '‘': "'",
                 '«': '"', '»': '"', ' ': ' '};
const PL = {'ą': 'a', 'ć': 'c', 'ę': 'e', 'ł': 'l', 'ń': 'n', 'ó': 'o', 'ś': 's', 'ź': 'z', 'ż': 'z'};

// bezpieczna nazwa pliku, np. "Pan Tadeusz" -> "pan_tadeusz"
function slug(s) {
  s = s.toLowerCase().replace(/[ąćęłńóśźż]/g, c => PL[c]).normalize('NFD').replace(/[̀-ͯ]/g, '');
  return s.replace(/[^a-z0-9]+/g, '_').replace(/^_+/, '').slice(0, 24).replace(/_+$/, '') || 'ksiazka';
}

// Zamienia TXT z Wolnych Lektur na format czytnika. Plik już przygotowany
// (z przygotuj_ksiazke.py, zaczyna się od #CZYTNIK|) zostaje bez zmian.
function przygotuj(tekst, nazwaPliku) {
  tekst = tekst.replace(/^﻿/, '').replace(/\r\n?/g, '\n');
  if (tekst.startsWith('#CZYTNIK|')) {
    const tytul = tekst.slice(0, tekst.indexOf('\n')).split('|')[1];
    return {tytul, tresc: tekst.endsWith('\n') ? tekst : tekst + '\n'};
  }

  let linie = tekst.split('\n');
  const stopka = linie.indexOf('-----');  // stopka licencyjna
  if (stopka >= 0) linie = linie.slice(0, stopka);
  linie = linie.map(l => l.replace(/[—–…„”“’‘«» ]/g, c => ZAMIANY[c]).replace(/\s+$/, ''));

  // początek pliku: autor, pusta linia, tytuł, podtytuły, ISBN, puste linie
  const autor = (linie[0] || '').trim();
  const tytul = (linie[2] || '').trim() || nazwaPliku.replace(/\.txt$/i, '');
  let i = 3;
  const podtytuly = [];
  while (i < linie.length && linie[i].trim() && !/^ISBN/.test(linie[i])) podtytuly.push(linie[i++].trim());
  while (i < linie.length && (!linie[i].trim() || /^ISBN/.test(linie[i]))) i++;

  const wynik = ['\x05', '\x05' + autor, '', '\x04' + tytul, ''];
  podtytuly.forEach(p => wynik.push('\x05' + p));
  wynik.push('', '', '\x05Tekst: wolnelektury.pl (domena publiczna)');

  // Proza (np. Lalka): każdy akapit to jedna długa linia, akapity oddziela pusta linia.
  // Wiersz (np. Pan Tadeusz): krótkie wersy, zwrotki oddziela pusta linia.
  const tekstKsiazki = linie.slice(i).filter(l => l.trim());
  const razem = tekstKsiazki.reduce((s, l) => s + l.length, 0);
  const wDlugich = tekstKsiazki.filter(l => l.length > 100).reduce((s, l) => s + l.length, 0);
  const proza = wDlugich > razem / 2;

  // Wolne Lektury dają co najmniej 3 puste linie przed nagłówkiem (akapity i zwrotki mają 1).
  // Krótki nagłówek tuż pod innym (np. "Zamek" pod "Księga druga") to podtytuł,
  // chyba że sam wygląda na rozdział (np. "I. Jak wygląda firma..." pod "Tom I").
  const ROZDZIAL = /^(rozdział|księga|część|tom|akt|scena|odsłona|epilog|prolog|zakończenie|wstęp)(?=[\s.:]|$)|^[IVXLC]+(\.|$)/i;
  let puste = 0, pierwsza = true, poNaglowku = false;
  for (; i < linie.length; i++) {
    const l = linie[i].replace(/\*([^*]+)\*/g, '$1');  // *wyróżnienie* -> wyróżnienie
    const t = l.trim();
    if (!t) { puste++; continue; }
    if (t.length < 100 && (puste >= 3 || (pierwsza && ROZDZIAL.test(t)))) {
      wynik.push((poNaglowku && !ROZDZIAL.test(t) ? '\x02' : '\x01') + t, '');
      poNaglowku = true;
    } else {
      if (proza) wynik.push('\x06' + t);   // akapit: wcięty i wyjustowany, bez odstępu
      else {
        if (puste && !poNaglowku) wynik.push('');  // odstęp między zwrotkami
        wynik.push(l);
      }
      poNaglowku = false;
    }
    puste = 0;
    pierwsza = false;
  }

  // jedna pusta linia zamiast kilku, bez pustych na końcu
  const czyste = [];
  for (const l of wynik) {
    if (l === '' && czyste.length && czyste[czyste.length - 1] === '') continue;
    czyste.push(l);
  }
  while (czyste.length && czyste[czyste.length - 1] === '') czyste.pop();

  const naglowek = '#CZYTNIK|' + tytul.replace(/\|/g, '/') + '|' + autor.replace(/\|/g, '/');
  return {tytul, tresc: naglowek + '\n' + czyste.join('\n') + '\n'};
}

function wyslij(url, dane, pasek) {
  return new Promise((ok, zle) => {
    const x = new XMLHttpRequest();
    x.open('POST', url);
    if (pasek) {
      pasek.hidden = false;
      pasek.value = 0;
      x.upload.onprogress = e => { if (e.lengthComputable) pasek.value = e.loaded / e.total; };
    }
    x.onload = () => x.status === 200 ? ok(x.responseText) : zle(new Error(x.responseText || 'Błąd ' + x.status));
    x.onerror = () => zle(new Error('Brak połączenia z czytnikiem'));
    x.send(dane);
  });
}

async function wczytaj() {
  try {
    const d = await (await fetch('/lista')).json();
    const lista = $('lista');
    lista.innerHTML = '';
    if (!d.ksiazki.length) lista.textContent = 'Brak książek - dodaj pierwszą poniżej.';
    for (const k of d.ksiazki) {
      const w = document.createElement('div');
      w.className = 'k';
      const opis = document.createElement('div');
      const t = document.createElement('b');
      t.textContent = k.tytul;
      const s = document.createElement('small');
      s.textContent = (k.autor ? k.autor + ' · ' : '') + kb(k.rozmiar);
      opis.append(t, document.createElement('br'), s);
      const usun = document.createElement('button');
      usun.className = 'usun';
      usun.textContent = 'Usuń';
      // bez confirm() i alert() - w okienku "Zaloguj się do sieci" na telefonie nie działają
      // (confirm od razu zwraca false), więc pytamy drugim kliknięciem
      const blad = document.createElement('small');
      blad.style.color = '#b33';
      opis.append(blad);
      usun.onclick = async () => {
        if (!usun.dataset.pewne) {
          usun.dataset.pewne = '1';
          usun.textContent = 'Na pewno?';
          setTimeout(() => { delete usun.dataset.pewne; usun.textContent = 'Usuń'; }, 4000);
          return;
        }
        usun.disabled = true;
        try {
          await wyslij('/usun', new URLSearchParams({plik: k.plik}));
          wczytaj();
        } catch (e) {
          blad.textContent = ' ' + e.message;
          usun.disabled = false;
        }
      };
      w.append(opis, usun);
      lista.append(w);
    }
    $('miejsce').textContent = 'Zajęte ' + kb(d.zajete) + ' z ' + kb(d.razem) + ', wolne ' + kb(d.razem - d.zajete) +
                               '. Wersja programu: ' + d.wersja + '.';
    $('ssid').value = d.siec;
    $('siecInfo').textContent = d.wlasna
      ? 'Teraz czytnik ma własną sieć „Czytnik”. Wpisz swoją domową sieć, żeby następnym razem łączył się z nią.'
      : 'Czytnik łączy się z tą siecią. Jeśli się nie uda, zrobi własną sieć „Czytnik” (hasło czytaj123).';
  } catch (e) {
    $('lista').textContent = 'Nie mogę pobrać listy: ' + e.message;
  }
}

$('wgraj').onclick = async () => {
  const pliki = [...$('pliki').files];
  if (!pliki.length) { $('stan').textContent = 'Najpierw wybierz plik.'; return; }
  $('wgraj').disabled = true;
  const wyniki = [];
  for (const p of pliki) {
    $('stan').textContent = 'Wgrywam: ' + p.name + '…';
    try {
      const {tytul, tresc} = przygotuj(await p.text(), p.name);
      const dane = new FormData();
      // nazwa z pliku, a nie z tytułu - bajki z jednego zbioru mają ten sam tytuł
      dane.append('plik', new Blob([tresc], {type: 'text/plain'}), slug(p.name.replace(/\.txt$/i, '')) + '.txt');
      await wyslij('/wgraj', dane, $('postep'));
      wyniki.push('✓ ' + tytul);
    } catch (e) {
      wyniki.push('✗ ' + p.name + ': ' + e.message);
    }
  }
  $('stan').textContent = wyniki.join('\n');
  $('postep').hidden = true;
  $('wgraj').disabled = false;
  $('pliki').value = '';
  wczytaj();
};

$('siec').onsubmit = async e => {
  e.preventDefault();
  try { $('siecStan').textContent = await wyslij('/wifi', new URLSearchParams(new FormData(e.target))); }
  catch (er) { $('siecStan').textContent = er.message; }
};

$('aktualizuj').onclick = async () => {
  const p = $('bin').files[0];
  if (!p || !p.name.endsWith('.bin')) { $('stanBin').textContent = 'Wybierz plik .bin'; return; }
  if (p.name.includes('merged')) { $('stanBin').textContent = 'To plik „merged” - wybierz czytnik.ino.bin'; return; }
  $('aktualizuj').disabled = true;
  $('stanBin').textContent = 'Wgrywam program… nie wyłączaj czytnika.';
  const dane = new FormData();
  dane.append('bin', p, p.name);
  try {
    await wyslij('/aktualizuj', dane, $('postepBin'));
    $('stanBin').textContent = 'Gotowe! Czytnik się uruchamia ponownie (WiFi się wyłączy).';
  } catch (e) {
    $('stanBin').textContent = e.message;
    $('aktualizuj').disabled = false;
  }
};

wczytaj();
</script>
</body>
</html>
)rawliteral";
