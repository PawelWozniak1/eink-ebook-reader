// Strona gry w kółko i krzyżyk (gra.ino). Bez alert/confirm - w okienku
// "Zaloguj się do sieci" na telefonie nie działają.
#pragma once

const char GRA[] PROGMEM = R"rawliteral(<!doctype html>
<html lang="pl">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Kółko i krzyżyk</title>
<style>
body{font-family:system-ui,sans-serif;max-width:420px;margin:0 auto;padding:16px;background:#f4f2ec;color:#222;text-align:center}
h1{font-size:1.4em;margin:.2em 0 .4em}
#info{font-size:1.2em;min-height:2.6em;margin:.4em 0}
#plansza{display:grid;grid-template-columns:repeat(3,1fr);gap:8px;margin:0 auto;max-width:330px}
#plansza button{aspect-ratio:1;font:bold 3em system-ui,sans-serif;border:0;border-radius:10px;background:#fff;
  box-shadow:0 1px 3px #0003;color:#222;cursor:pointer;padding:0}
#plansza button.X{color:#2d5d8a}
#plansza button.O{color:#b33}
#plansza button.wygrana{background:#ffe9a8}
button.zwykly{font:inherit;padding:10px 18px;border:0;border-radius:8px;background:#2d5d8a;color:#fff;cursor:pointer;margin-top:16px}
#wynik{color:#555;margin-top:12px}
#blad{color:#b33;min-height:1.2em}
a{color:#2d5d8a}
</style>
</head>
<body>
<h1>Kółko i krzyżyk</h1>
<div id="info">Łączę z czytnikiem…</div>
<div id="plansza"></div>
<p id="blad"></p>
<button class="zwykly" id="nowa" hidden>Nowa gra</button>
<p id="wynik"></p>
<p><a href="/">← wróć do czytnika</a></p>

<script>
const $ = id => document.getElementById(id);

// identyfikator tego telefonu - po odświeżeniu strony gramy dalej tym samym znakiem
let id = '';
try { id = localStorage.getItem('graId') || ''; } catch (e) {}
if (!id) {
  id = Math.random().toString(36).slice(2, 10);
  try { localStorage.setItem('graId', id); } catch (e) {}
}

const LINIE = [[0,1,2],[3,4,5],[6,7,8],[0,3,6],[1,4,7],[2,5,8],[0,4,8],[2,4,6]];
const pola = [];
for (let i = 0; i < 9; i++) {
  const b = document.createElement('button');
  b.onclick = () => wyslij('/gra/ruch', {pole: i});
  $('plansza').append(b);
  pola.push(b);
}

async function wyslij(url, dane) {
  $('blad').textContent = '';
  try {
    const r = await fetch(url, {method: 'POST', body: new URLSearchParams({id, ...dane})});
    if (!r.ok) $('blad').textContent = await r.text();
  } catch (e) {
    $('blad').textContent = 'Brak połączenia z czytnikiem';
  }
  odswiez();
}

$('nowa').onclick = () => wyslij('/gra/nowa', {});

async function odswiez() {
  let s;
  try {
    s = await (await fetch('/gra/stan?id=' + id)).json();
  } catch (e) {
    $('info').textContent = 'Brak połączenia z czytnikiem…';
    return;
  }
  const linia = LINIE.find(l => s.plansza[l[0]] !== ' ' && l.every(p => s.plansza[p] === s.plansza[l[0]]));
  pola.forEach((b, i) => {
    const c = s.plansza[i];
    b.textContent = c === 'X' ? '✕' : c === 'O' ? '◯' : '';
    b.className = (c !== ' ' ? c : '') + (linia && linia.includes(i) ? ' wygrana' : '');
  });

  const znak = z => z === 'X' ? '✕' : '◯';
  let info;
  if (!s.ja) info = 'Grają już dwie osoby - możesz oglądać.';
  else if (s.wynik === 'R') info = 'Remis!';
  else if (s.wynik) info = s.wynik === s.ja ? '🎉 Wygrałeś!' : 'Przegrana… może rewanż?';
  else if (s.graczy < 2) info = 'Grasz jako ' + znak(s.ja) + '. Czekamy na drugi telefon…';
  else info = 'Grasz jako ' + znak(s.ja) + '. ' + (s.tura === s.ja ? '<b>Twój ruch!</b>' : 'Ruch przeciwnika…');
  $('info').innerHTML = info;

  $('nowa').hidden = !s.ja || !s.wynik;
  $('wynik').textContent = '✕ ' + s.wygraneX + ' : ' + s.wygraneO + ' ◯' + (s.remisy ? ', remisy: ' + s.remisy : '');
}

odswiez();
setInterval(odswiez, 700);
</script>
</body>
</html>
)rawliteral";
