// ============================================================
//  Obudowa czytnika DIY  —  e-Paper 7,5" Waveshare (800x480) w pionie
//  ESP32-S3, LiPo 405063, TP4056 USB-C, Pololu 3,3 V, joystick w klapce
//  Jednostki: mm
//
//  Uklad wspolrzednych: widok OD PRZODU (od strony ekranu), czytnik w pionie,
//  x w prawo, y w gore, z=0 to przednia powierzchnia, z rosnie do tylu.
//  Tasma FPC wychodzi z prawej krawedzi matrycy. Patrzysz jak przez ekran,
//  wiec uklad czesci jest taki sam jak na zdjeciu plytki od strony elementow.
//
//  Obrys = sam ekran + scianki. Cala elektronika miesci sie za matryca.
//
//  Czesci:
//    "front"    – przednia ramka ze sciankami (drukuj przodem do stolu)
//    "mid"      – plytka podporowa za panelem (0,8 mm)
//    szybka     – pleksi przed matryca, docinasz sama (wymiar wypisuje echo)
//    "lid"      – tylna klapka z rantem, joystickiem i gniazdami na czesci
//    "all"      – podglad zlozenia z makietami czesci
//    "exploded" – podglad rozlozony,  "layout" – rozmieszczenie czesci na klapce
//    "closed"   – zamknieta obudowa
// ============================================================

part = "exploded"; // [front, mid, lid, all, exploded, layout]

/* [Panel e-ink w pionie — ZMIERZ swoj egzemplarz!] */
panel_w   = 111.2;   // krotszy bok (poziomo)
panel_h   = 170.2;   // dluzszy bok (pionowo)
panel_t   = 1.2;
aa_w      = 97.92;   // obszar aktywny
aa_h      = 163.2;
aa_left   = 2.7;     // od lewej krawedzi panelu do obszaru aktywnego
aa_bottom = 3.5;     // od dolnej krawedzi panelu do obszaru aktywnego
fpc_center_y = 85.1; // srodek tasmy FPC na prawej krawedzi, liczony od dolu panelu
fpc_w     = 32;      // szerokosc tasmy + zapas

/* [Obudowa] */
wall      = 2.0;   // grubosc scianek bocznych
clr       = 0.3;   // luz wokol panelu
fpc_gap   = 2.5;   // miejsce na zagiecie tasmy FPC (prawa strona)
center_aa = true;  // obraz na srodku obudowy: dodatkowy pas z lewej rownowazy szersza ramke matrycy od strony FPC
front_t   = 1.2;   // grubosc przodu (ramki)
glass_t   = 1.0;   // szybka ochronna z pleksi przed matryca (0 = bez szybki)
glass_clr = 0.2;   // luz na grubosc szybki
pocket_t  = 1.4;   // gniazdo na panel (panel 1,2 + luz)
mid_t     = 0.8;   // plytka podporowa
cav       = 10.5;  // komora na elektronike: najwyzsza czesc to HAT z listwa 2x20 (~10 mm)
back_t    = 1.2;   // tylna scianka
corner_r  = 5;
front_round = 1.5; // zaokraglenie przedniej krawedzi obudowy (0 = ostra)
back_round  = 3.0; // zaokraglenie tylnej krawedzi: w reku czytnik wydaje sie cienszy
fit       = 0.15;  // luz pasowania rantu klapki
rim_t     = 1.2;   // grubosc rantu klapki
win_margin= 0.4;   // okno troche wieksze niz obszar aktywny
bevel     = 1.0;   // fazowanie okna od przodu

/* [Czesci — wymiary zmierzone, widok od przodu] */
esp_size = [27, 57];  esp_ant = 6;   // ESP32-S3 + modul wystajacy nad plytke
esp_side = 16;         // wolne miejsce po bokach na wtyki Dupont na pinach (przewody lutowane: 6)
bat_size = [50, 62];   bat_clr = 0.5; // LiPo 405063 (bez ramki, na tasme dwustronna)
hat_size = [31, 65];
tp_size  = [29, 18];   // TP4056, gniazdo USB-C po prawej
pol_size = [18, 12];   // Pololu S7V8F3
div_size = [25, 32];   // plytka z dzielnikami
joy_size = [26, 41];   // modul joysticka, piny na dolnej krawedzi

/* [Joystick w klapce — od lewego dolnego rogu modulu, widok od przodu (z pinami na dole)] */
// Plytka ma ok. 37 mm, ponizej wystaja piny (razem 41). Gora plytki to y = 41.
joy_holes = [[4.5, 17.5], [21.5, 17.5], [4.5, 38], [21.5, 38]];  // otwory montazowe (ZMIERZ)
joy_stem  = [13, 21];      // os grzybka: 20 mm od gory plytki
joy_hole  = 10.5;          // otwor na galke 8 mm + luz na przechylanie
joy_btns  = [[8, 32], [19, 32]];  // SET i RST: 9 mm od gory plytki
joy_knob_h = 9.2;          // wierzch galki nad plytka joysticka
joy_out   = 1.5;           // ile galka ma wystawac ponad klapke
joy_btn_h = 2.1;           // wierzch guzikow SET/RST nad plytka

/* [Klawisze SET/RST w klapce: sprezynujace jezyczki z popychaczem] */
key_w     = 6;             // szerokosc klawisza
key_len   = 6.5;           // dlugosc od zawiasu (w strone grzybka) do konca
key_slot  = 0.8;           // szczelina wokol klawisza
leather_t = 0.5;           // zaglebienie na latke ze skory nad klawiszami (0 = bez)

/* [Gniazda USB-C] */
esp_usb_x = 18.4;          // srodek gniazda USB (prawego, nie COM) od lewej krawedzi plytki ESP
usb_w = 12.5;              // otwor na wtyczke (oslonka wtyczki ~12 x 6)
usb_h = 6.5;

$fn = 32;

joy_stand = joy_knob_h - joy_out - back_t;   // odstep plytki joysticka od klapki (wysokosc slupkow)

// ---------- wartosci wyliczone ----------
pad = center_aa ? panel_w - 2*aa_left - aa_w + fpc_gap : 0;  // ~10,4 mm
iw = panel_w + 2*clr + fpc_gap + pad;
ih = panel_h + 2*clr;
W  = iw + 2*wall;
H  = ih + 2*wall;
px = wall + clr + pad;           // panel – lewy dolny rog
py = wall + clr;
z_glass = front_t;                                         // szybka lezy na ramce
z_pan = front_t + (glass_t > 0 ? glass_t + glass_clr : 0);  // panel za szybka
z_mid = z_pan + pocket_t;
// szybka wypelnia cale wnetrze obok pasa z lewej, wiec nie przesuwa sie na boki
glass_size = [W - 2*wall - (pad > 0 ? pad - clr : 0) - 0.2, ih - 0.2];
z_cav = z_mid + mid_t;
z_lid = z_cav + cav;
T     = z_lid + back_t;
rim_in = wall + fit + rim_t;     // wewnetrzna krawedz rantu

// polozenie czesci (lewy dolny rog); prawa kolumna dosunieta do rantu
R = W - rim_in - 0.5;            // prawa granica miejsca na czesci
U = H - rim_in - 0.5;            // gorna granica
hat_pos = [R - hat_size[0], py + fpc_center_y - hat_size[1]/2];   // srodkiem na wysokosci FPC
// ESP jak najblizej srodka, ale tak, zeby prawe wtyki nie wchodzily na HAT
esp_pos = [min(W/2 - esp_size[0]/2, hat_pos[0] - 1.2 - 1 - esp_side - esp_size[0]), rim_in + 0.4];
div_pos = [rim_in + 0.5, esp_pos[1] + esp_size[1] + esp_ant + 3];  // nad lewa listwa, blisko GPIO1/GPIO2
tp_pos  = [R - tp_size[0],  U - tp_size[1]];
pol_pos = [R - pol_size[0], tp_pos[1] - 3 - pol_size[1]];
joy_pos = [rim_in + 0.5, U - joy_size[1]];
bat_pos = [joy_pos[0] + joy_size[0] + 0.5 + bat_clr, U - 3 - bat_size[1]];  // u gory, 3 mm na wyprowadzenia

// gniazda: [sciana, srodek wzdluz scianki, dlugosc, wysokosc srodka z]
usb_esp_z = z_lid - 1.6;          // ESP lezy modulem na klapce, gniazda sa po stronie modulu
usb_tp_z  = z_lid - 1.6 - 1.6;    // TP4056 lezy plytka na klapce
ports = [
  ["b", esp_pos[0] + esp_usb_x, usb_w, usb_esp_z],              // ESP: tylko gniazdo USB
  ["r", tp_pos[1] + tp_size[1]/2, usb_w, usb_tp_z]               // TP4056: ladowanie
];

z_snap = z_cav + cav*0.65;
snaps = [ // [sciana, poczatek wzdluz scianki]
  ["l", 25], ["l", H/2 - 7], ["l", H - 39],
  ["r", 25], ["r", hat_pos[1] + hat_size[1] + 2],
  ["t", W/2 - 7]
];

// ---------- kontrola ukladu ----------
function box(p, s, m = 0) = [p[0] - m, p[1] - m, p[0] + s[0] + m, p[1] + s[1] + m];
function hit(a, b) = a[0] < b[2] && b[0] < a[2] && a[1] < b[3] && b[1] < a[3];
loc = 1.2;  // ramka gniazda wystaje tyle poza czesc
footprints = [ // [nazwa, obrys czesci, obrys z ramka]
  ["ESP",      box([esp_pos[0] - esp_side, esp_pos[1]], [esp_size[0] + 2*esp_side, esp_size[1] + esp_ant]),
               box([esp_pos[0] - esp_side, esp_pos[1] - loc], [esp_size[0] + 2*esp_side, esp_size[1] + esp_ant + 2*loc])],
  ["dzielnik", box(div_pos, div_size), box(div_pos, div_size, loc)],
  ["HAT",      box(hat_pos, hat_size), box(hat_pos, hat_size, loc)],
  ["TP4056",   box(tp_pos, tp_size),   box(tp_pos, tp_size, loc)],
  ["Pololu",   box(pol_pos, pol_size), box(pol_pos, pol_size, loc)],
  ["bateria",  box(bat_pos, bat_size, bat_clr), box(bat_pos, bat_size, bat_clr)],
  ["joystick", box(joy_pos, joy_size), box(joy_pos, joy_size)]
];
for (i = [0 : len(footprints) - 1]) {
  f = footprints[i][1];
  assert(f[0] >= rim_in && f[1] >= rim_in && f[2] <= W - rim_in && f[3] <= H - rim_in,
         str(footprints[i][0], " wychodzi poza obudowe"));
  for (j = [i + 1 : len(footprints) - 1])
    if (j > i && j < len(footprints))
      assert(!hit(footprints[i][2], footprints[j][2]), str(footprints[i][0], " nachodzi na ", footprints[j][0]));
}
assert(joy_stand + 1.6 + 1.5 < cav, "Joystick za gleboko: zmniejsz joy_stand albo zwieksz cav");
key_pin = joy_stand - joy_btn_h - 0.2;   // dlugosc popychacza od spodu klapki do guzika
assert(key_pin >= 0.3, "Guziki SET/RST za wysokie: zmniejsz joy_btn_h albo zwieksz joy_stand");
echo(str("Popychacze SET/RST: ", key_pin, " mm"));
echo(str("Wymiary zewnetrzne: ", W, " x ", H, " x ", T, " mm"));
if (glass_t > 0) echo(str("Szybka do wyciecia: ", glass_size[0], " x ", glass_size[1], " x ", glass_t, " mm"));

// ---------- pomocnicze ----------
// zewnetrzna bryla z zaokraglonymi krawedziami z przodu i z tylu
function inset_at(z) =
  z < front_round ? front_round - sqrt(pow(front_round, 2) - pow(front_round - z, 2)) :
  z > T - back_round ? back_round - sqrt(pow(back_round, 2) - pow(z - (T - back_round), 2)) : 0;
module outer() {
  n = 8;
  zs = concat([for (i = [0 : n]) front_round * i / n],
              [for (i = [0 : n]) T - back_round + back_round * i / n]);
  hull() for (z = zs) {
    i = inset_at(min(max(z, 0), T));
    translate([i, i, min(max(z, 0), T - 0.01)]) linear_extrude(0.01)
      offset(r = corner_r - i) offset(delta = -(corner_r - i)) square([W - 2*i, H - 2*i]);
  }
}
module body(z0, z1) intersection() { outer(); translate([-1, -1, z0]) cube([W + 2, H + 2, z1 - z0]); }

module rbox(x, y, w, h, r, z0, z1)
  translate([x, y, z0]) linear_extrude(z1 - z0)
    offset(r) offset(-r) square([w, h]);

module port_cuts() {
  for (p = ports) {
    c = p[1]; l = p[2]; zc = p[3];
    if (p[0] == "r")
      hull() for (s = [-1, 1])
        translate([W - rim_in - 1, c + s*(l - usb_h)/2, zc]) rotate([0, 90, 0]) cylinder(d = usb_h, h = rim_in + 2);
    else
      hull() for (s = [-1, 1])
        translate([c + s*(l - usb_h)/2, -1, zc]) rotate([-90, 0, 0]) cylinder(d = usb_h, h = rim_in + 2);
  }
}

// walec wzdluz scianki: grooves w ramce i garby na rancie
module along(side, start, len, r, inset) {
  if (side == "l") translate([inset, start, z_snap]) rotate([-90, 0, 0]) cylinder(r = r, h = len);
  if (side == "r") translate([W - inset, start, z_snap]) rotate([-90, 0, 0]) cylinder(r = r, h = len);
  if (side == "t") translate([start, H - inset, z_snap]) rotate([0, 90, 0]) cylinder(r = r, h = len);
  if (side == "b") translate([start, inset, z_snap]) rotate([0, 90, 0]) cylinder(r = r, h = len);
}

module locator(pos, size, h = 1.0, t = 0.8, c = 0.4, gap = 8) {
  difference() {
    translate([pos[0]-c-t, pos[1]-c-t, z_lid-h])
      cube([size[0]+2*(c+t), size[1]+2*(c+t), h]);
    translate([pos[0]-c, pos[1]-c, z_lid-h-1])
      cube([size[0]+2*c, size[1]+2*c, h+2]);
    // przerwy na kable w polowie bokow
    translate([pos[0]+size[0]/2-gap/2, pos[1]-c-t-1, z_lid-h-1]) cube([gap, size[1]+2*(c+t)+2, h+2]);
    translate([pos[0]-c-t-1, pos[1]+size[1]/2-gap/2, z_lid-h-1]) cube([size[0]+2*(c+t)+2, gap, h+2]);
  }
}

// ---------- PRZOD ----------
module front() {
  front_shell();
  // pas z lewej obok matrycy: oparcie dla panelu i plytki podporowej
  if (pad > 0) translate([wall - 0.01, wall - 0.01, front_t - 0.01]) cube([pad - clr + 0.01, ih + 0.02, z_cav - front_t + 0.01]);
}

module front_shell() {
  wx = px + aa_left - win_margin;
  wy = py + aa_bottom - win_margin;
  ww = aa_w + 2*win_margin;
  wh = aa_h + 2*win_margin;
  difference() {
    body(0, z_lid);
    translate([wall, wall, front_t]) cube([iw, ih, z_lid]);
    // wciecie do podwazania klapki (w gornej krawedzi scianki)
    translate([-1, H/2 - 6, z_lid - 0.8]) cube([wall/2 + 1, 12, 1]);
    // okno z fazowaniem
    hull() {
      translate([wx, wy, front_t - 0.01]) cube([ww, wh, 0.1]);
      translate([wx - bevel, wy - bevel, -0.01]) cube([ww + 2*bevel, wh + 2*bevel, 0.01]);
    }
    // rowki zatrzaskow
    along("l", wall + 5, ih - 10, 0.65, wall);
    along("r", wall + 5, ih - 10, 0.65, wall);
    along("t", wall + 5, iw - 10, 0.65, wall);
    port_cuts();
  }
}

// ---------- PLYTKA PODPOROWA ----------
module glass() {
  translate([W - wall - 0.1 - glass_size[0], wall + 0.1, z_glass]) cube([glass_size[0], glass_size[1], glass_t]);
}

module mid() {
  translate([px + 0.1, py + 0.1, z_mid]) cube([panel_w - 0.2, panel_h - 0.2, mid_t]);
}

// ---------- TYLNA KLAPKA ----------
module lid() {
  difference() {
    union() {
      // dno
      body(z_lid, T);
      // rant
      difference() {
        translate([wall + fit, wall + fit, z_cav]) cube([iw - 2*fit, ih - 2*fit, cav + 0.01]);
        translate([rim_in, rim_in, z_cav - 1]) cube([W - 2*rim_in, H - 2*rim_in, cav + 2]);
        // wyciecie na tasme FPC (prawa strona)
        translate([W - rim_in - 1, py + fpc_center_y - (fpc_w + 6)/2, z_cav - 1])
          cube([rim_t + fit + 2, fpc_w + 6, cav + 1]);
      }
      // zatrzaski
      for (s = snaps) along(s[0], s[1], 14, 0.6, wall + fit + 0.15);
      // gniazda na czesci
      locator([esp_pos[0], esp_pos[1]], [esp_size[0], esp_size[1] + esp_ant]);
      locator(div_pos, div_size);
      locator(hat_pos, hat_size);
      locator(tp_pos,  tp_size);
      locator(pol_pos, pol_size, gap = 6);
      // slupki joysticka z kolkami w otwory montazowe
      for (h = joy_holes) translate([joy_pos[0] + h[0], joy_pos[1] + h[1], 0]) {
        translate([0, 0, z_lid - joy_stand]) cylinder(d = 4.5, h = joy_stand + 0.01);
        translate([0, 0, z_lid - joy_stand - 1.8]) cylinder(d = 2.4, h = 1.81);
      }
    }
    // otwory joysticka
    translate([joy_pos[0] + joy_stem[0], joy_pos[1] + joy_stem[1], z_lid - 1]) cylinder(d = joy_hole, h = back_t + 2, $fn = 48);
    // klawisze SET/RST: szczelina w ksztalcie U, zawias od strony grzybka
    for (b = joy_btns) translate([joy_pos[0] + b[0], joy_pos[1] + b[1], 0]) difference() {
      translate([-key_w/2 - key_slot, 2.5 - key_len, z_lid - 1]) cube([key_w + 2*key_slot, key_len + key_slot, back_t + 2]);
      translate([-key_w/2, 2.5 - key_len - 1, z_lid - 2]) cube([key_w, key_len + 1, back_t + 4]);
    }
    // plytkie okienko na latke ze skory nad klawiszami
    if (leather_t > 0) {
      bx = [for (b = joy_btns) b[0]];  by = [for (b = joy_btns) b[1]];
      m = key_slot + 1.5;
      rbox(joy_pos[0] + min(bx) - key_w/2 - m, joy_pos[1] + min(by) + 2.5 - key_len - 0.8,
           max(bx) - min(bx) + key_w + 2*m, max(by) - min(by) + key_len + key_slot + 2.3,
           1.5, T - leather_t, T + 1);
    }
    // otwory USB
    port_cuts();
    // wciecie do podwazania klapki
    translate([-1, H/2 - 6, T - 0.8]) cube([3, 12, 1]);
  }
  // popychacze na klawiszach SET/RST
  for (b = joy_btns) translate([joy_pos[0] + b[0], joy_pos[1] + b[1], z_lid - key_pin]) cylinder(d = 2.5, h = key_pin + 0.01);
}

// ---------- makiety czesci (tylko podglad) ----------
module dummies() {
  module d(p, s, h, c) color(c, 0.85) translate([p[0], p[1], z_lid - h]) cube([s[0], s[1], h]);
  d(esp_pos, [esp_size[0], esp_size[1] + esp_ant], 5, "#222");
  for (x = [esp_pos[0] - esp_side, esp_pos[0] + esp_size[0]])  // miejsce na wtyki przy listwach
    d([x, esp_pos[1]], [esp_side, esp_size[1]], 2.5, "orange");
  d(div_pos, div_size, 4, "darkgreen");
  d(hat_pos, hat_size, 10, "royalblue");
  d(tp_pos,  tp_size, 4.5, "steelblue");
  d(pol_pos, pol_size, 3, "seagreen");
  d(bat_pos, bat_size, 4.5, "silver");
  color("green", 0.85) translate([joy_pos[0], joy_pos[1], z_lid - joy_stand - 1.6]) cube([joy_size[0], joy_size[1], 1.6]);
}

// ---------- wybor ----------
// Os z rosnie do tylu, wiec w prawoskretnym ukladzie OpenSCAD widok od przodu bylby
// lustrzany. Odbicie w x sprawia, ze to, co jest "po prawej" w zmiennych, jest po prawej
// naprawde, gdy patrzysz na ekran.
module real() translate([W, 0, 0]) mirror([1, 0, 0]) children();

if (part == "front") real() front();
else if (part == "mid") translate([0, 0, -z_mid]) mid();
else if (part == "lid") translate([0, H, T]) rotate([180, 0, 0]) real() lid(); // dnem do stolu
else if (part == "all") real() { color("#555", 0.35) front(); if (glass_t > 0) color("lightblue", 0.5) glass(); color("tan") mid(); dummies(); color("#888") lid(); }
// schemat ukladu we wspolrzednych z widoku od przodu (ogladany z gory, klapka przezroczysta)
else if (part == "closed") real() {
  color("#4a4f55") front(); color("#5a6067") lid();
  color("#e4e5df") translate([px, py, z_pan]) cube([panel_w, panel_h, panel_t]);  // matryca
}
else if (part == "layout") { dummies(); color("#888", 0.25) lid(); }
else real() {
  color("#555") front();
  if (glass_t > 0) color("lightblue", 0.6) translate([0, 0, 8]) glass();
  color("tan") translate([0, 0, 15]) mid();
  translate([0, 0, 30]) { color("#888") lid(); dummies(); }
}
