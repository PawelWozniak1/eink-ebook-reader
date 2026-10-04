// ============================================================
//  Obudowa czytnika DIY  —  e-Paper 7,5" Waveshare 13504 (800x480)
//  ESP32-S3-DevKitC-1, LiPo 523450, TP4056 USB-C, 1 przycisk z tylu
//  Jednostki: mm
//
//  Uklad wspolrzednych: widok OD PRZODU (od strony ekranu),
//  x w prawo, y w gore, z=0 to przednia powierzchnia, z rosnie do tylu.
//  Wszystkie pozycje czesci podajesz wiec tak, jak widzisz je przez ekran.
//
//  Czesci:
//    "front" – przednia ramka ze sciankami (drukuj przodem do stolu)
//    "mid"   – plytka podporowa za panelem (0,8 mm)
//    "lid"   – tylna klapka z rantem, przyciskiem i gniazdami na czesci
//    "all"   – podglad zlozenia,  "exploded" – podglad rozlozony
// ============================================================

part = "exploded"; // [front, mid, lid, all, exploded]

/* [Panel e-ink — ZMIERZ swoj egzemplarz!] */
panel_w   = 170.2;
panel_h   = 111.2;
panel_t   = 1.2;
aa_w      = 163.2;   // obszar aktywny
aa_h      = 97.92;
aa_left   = 3.5;     // od lewej krawedzi panelu do obszaru aktywnego
aa_bottom = 10.58;   // od dolnej krawedzi (strona tasmy FPC) do obszaru aktywnego
fpc_center_x = 85.1; // srodek tasmy FPC liczony od lewej krawedzi panelu
fpc_w     = 32;      // szerokosc tasmy + zapas

/* [Obudowa] */
wall      = 2.0;   // grubosc scianek bocznych
clr       = 0.3;   // luz wokol panelu
fpc_gap   = 2.5;   // dodatkowe miejsce na zagiecie tasmy FPC (dol)
front_t   = 1.2;   // grubosc przodu (ramki)
pocket_t  = 1.4;   // gniazdo na panel (panel 1,2 + luz)
mid_t     = 0.8;   // plytka podporowa
cav       = 6.0;   // wysokosc komory na elektronike
back_t    = 1.2;   // tylna scianka
corner_r  = 5;
fit       = 0.15;  // luz pasowania rantu klapki
rim_t     = 1.2;   // grubosc rantu klapki
win_margin= 0.4;   // okno troche wieksze niz obszar aktywny
bevel     = 1.0;   // fazowanie okna od przodu

/* [Przycisk z tylu] */
btn_x0    = 50;    // poczatek klawisza (zawias) — x
btn_cy    = 103;   // srodek klawisza — y
btn_len   = 16;
btn_w     = 10;
slot      = 1.0;   // szerokosc szczeliny wokol klawisza
sw_h      = 4.3;   // wysokosc tact switcha z popychaczem (6x6x4.3)
sw_size   = 6.2;

/* [Polozenie czesci (lewy dolny rog, widok od przodu)] */
hat_pos = [55, 6];    hat_size = [65, 30.2];
bat_pos = [10, 50];   bat_size = [50, 34];
tp_size = [25, 19];
esp_size= [69, 26];

$fn = 32;

// ---------- wartosci wyliczone ----------
iw = panel_w + 2*clr;
ih = panel_h + 2*clr + fpc_gap;
W  = iw + 2*wall;
H  = ih + 2*wall;
px = wall + clr;                 // panel – lewy dolny rog
py = wall + clr + fpc_gap;
z_mid = front_t + pocket_t;
z_cav = z_mid + mid_t;
z_lid = z_cav + cav;
T     = z_lid + back_t;
rim_in = wall + fit + rim_t;     // wewnetrzna krawedz rantu

esp_pos = [W - rim_in - 0.5 - esp_size[0], 80];
tp_pos  = [W - rim_in - 0.5 - tp_size[0], 45];

port_z  = 5.4;   // wysokosc srodka gniazd USB-C (od przodu)
ports = [ // [srodek y, dlugosc, wysokosc]
  [esp_pos[1] + esp_size[1]/2, 24, 6.5],   // ESP32 (oba gniazda)
  [tp_pos[1]  + tp_size[1]/2,  11, 6.5]    // TP4056
];

z_snap = z_cav + cav*0.65;

// przycisk – mostek z przelacznikiem
sw_x   = btn_x0 + btn_len - 4;
cb_z0  = z_cav + 0.1;
cb_z1  = cb_z0 + 1.2;
sw_tip = cb_z1 - 0.6 + sw_h;
pin_l  = z_lid - sw_tip - 0.2;
assert(pin_l >= 0.3, "Przelacznik za wysoki: zmniejsz sw_h albo zwieksz cav");
echo(str("Wymiary zewnetrzne: ", W, " x ", H, " x ", T, " mm"));
echo(str("Dlugosc popychacza: ", pin_l, " mm"));

// ---------- pomocnicze ----------
module rbox(x, y, w, h, r, z0, z1)
  translate([x, y, z0]) linear_extrude(z1 - z0)
    offset(r) offset(-r) square([w, h]);

module port_cuts() {
  for (p = ports) {
    yc = p[0]; l = p[1]; hh = p[2];
    hull() for (s = [-1, 1])
      translate([W - rim_in - 1, yc + s*(l - hh)/2, port_z])
        rotate([0, 90, 0]) cylinder(d = hh, h = rim_in + 2);
  }
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
  wx = px + aa_left - win_margin;
  wy = py + aa_bottom - win_margin;
  ww = aa_w + 2*win_margin;
  wh = aa_h + 2*win_margin;
  difference() {
    rbox(0, 0, W, H, corner_r, 0, z_lid);
    translate([wall, wall, front_t]) cube([iw, ih, z_lid]);
    // okno z fazowaniem
    hull() {
      translate([wx, wy, front_t - 0.01]) cube([ww, wh, 0.1]);
      translate([wx - bevel, wy - bevel, -0.01]) cube([ww + 2*bevel, wh + 2*bevel, 0.01]);
    }
    // rowki zatrzaskow
    for (y = [wall, H - wall])
      translate([wall + 5, y, z_snap]) rotate([0, 90, 0]) cylinder(r = 0.65, h = iw - 10);
    port_cuts();
  }
}

// ---------- PLYTKA PODPOROWA ----------
module mid() {
  translate([px + 0.1, py + 0.1, z_mid]) cube([panel_w - 0.2, panel_h - 0.2, mid_t]);
}

// ---------- TYLNA KLAPKA ----------
module lid() {
  difference() {
    union() {
      // dno
      rbox(0, 0, W, H, corner_r, z_lid, T);
      // rant
      difference() {
        translate([wall + fit, wall + fit, z_cav]) cube([iw - 2*fit, ih - 2*fit, cav + 0.01]);
        translate([rim_in, rim_in, z_cav - 1]) cube([W - 2*rim_in, H - 2*rim_in, cav + 2]);
        // wyciecie na tasme FPC
        translate([px + fpc_center_x - (fpc_w + 6)/2, wall - 1, z_cav - 1])
          cube([fpc_w + 6, rim_t + fit + 2, cav + 1]);
      }
      // zatrzaski
      for (x = [25, W - 25 - 14])
        translate([x, wall + fit + 0.15, z_snap]) rotate([0, 90, 0]) cylinder(r = 0.6, h = 14);
      for (x = [25, W/2 - 7, W - 25 - 14])
        translate([x, H - wall - fit - 0.15, z_snap]) rotate([0, 90, 0]) cylinder(r = 0.6, h = 14);
      // gniazda na czesci
      locator(hat_pos, hat_size);
      locator(bat_pos, bat_size);
      locator(esp_pos, esp_size);
      locator(tp_pos,  tp_size);
      // mostek z przelacznikiem
      for (s = [-1, 1])
        translate([sw_x - 4, btn_cy + s*(btn_w/2 + slot + 3.5) - 1.5, cb_z0])
          cube([8, 3, z_lid - cb_z0]);
      translate([sw_x - 4, btn_cy - btn_w/2 - slot - 5, cb_z0])
        cube([8, btn_w + 2*slot + 10, cb_z1 - cb_z0]);
    }
    // szczelina w ksztalcie U wokol klawisza
    difference() {
      translate([btn_x0, btn_cy - btn_w/2 - slot, z_lid - 1])
        cube([btn_len + slot, btn_w + 2*slot, back_t + 2]);
      translate([btn_x0 - 1, btn_cy - btn_w/2, z_lid - 2])
        cube([btn_len + 1, btn_w, back_t + 4]);
    }
    // klawisz lekko wpuszczony (cienszy = latwiej sie ugina)
    translate([btn_x0, btn_cy - btn_w/2, T - 0.4]) cube([btn_len, btn_w, 1]);
    // gniazdo na przelacznik w mostku
    translate([sw_x - sw_size/2, btn_cy - sw_size/2, cb_z1 - 0.6]) cube([sw_size, sw_size, 1]);
    // otwory USB
    port_cuts();
    // wciecie do podwazania klapki
    translate([-1, H/2 - 6, T - 0.8]) cube([3, 12, 1]);
  }
  // popychacz + wyczuwalna kropka
  translate([sw_x, btn_cy, z_lid - pin_l]) cylinder(d = 3, h = pin_l + 0.01);
  translate([sw_x, btn_cy, T - 0.41]) cylinder(d = 3, h = 0.3);
}

// ---------- wybor ----------
if (part == "front") front();
else if (part == "mid") translate([0, 0, -z_mid]) mid();
else if (part == "lid") translate([0, H, T]) rotate([180, 0, 0]) lid(); // dnem do stolu
else if (part == "all") { color("#555") front(); color("tan") mid(); color("#888") lid(); }
else {
  color("#555") front();
  color("tan") translate([0, 0, 15]) mid();
  color("#888") translate([0, 0, 30]) lid();
}
