import sys
import unittest
from pathlib import Path

KORZEN = Path(__file__).parent.parent
sys.path.insert(0, str(KORZEN / "symulator"))

from czytnik import DOL_TEKSTU, SZEROKOSC, WYSOKOSC, Czytnik, Ksiazka  # noqa: E402
from u8g2_font import U8g2Font, _CzytnikBitow, _literal_c  # noqa: E402

FONTY = sorted((KORZEN / "symulator" / "fonty").glob("*.u8g2"))
KSIAZKI = KORZEN / "ksiazki_do_wgrania"


class TestDekoderaFontow(unittest.TestCase):
    def test_literal_c(self):
        self.assertEqual(_literal_c(r'"\101\x42" "C\\\""'), b'ABC\\"')

    def test_czytnik_bitow_od_najmlodszego_bitu(self):
        b = _CzytnikBitow(bytes([0b10110101, 0b00000011]), 0)
        self.assertEqual(b.bez_znaku(3), 0b101)
        self.assertEqual(b.bez_znaku(7), 0b1110110)  # przez granicę bajtów
        self.assertEqual(b.ze_znakiem(2), 0b00 - 2)  # ze znakiem: wartość minus 2^(n-1)

    def test_wszystkie_fonty_maja_polskie_litery(self):
        self.assertEqual(len(FONTY), 6)
        for plik in FONTY:
            font = U8g2Font.z_pliku(plik)
            for znak in "aąćęłńóśźżAĄĆĘŁŃÓŚŹŻ":
                with self.subTest(font=plik.stem, znak=znak):
                    g = font.glif(ord(znak))
                    self.assertIsNotNone(g)
                    self.assertTrue(g.piksele)
                    self.assertGreater(g.dx, 0)

    def test_szerokosc_tekstu(self):
        font = U8g2Font.z_pliku(FONTY[0])
        self.assertEqual(font.szerokosc(""), 0)
        self.assertLess(font.szerokosc("Litwo"), font.szerokosc("Litwo! Ojczyzno"))
        # spacja na końcu nie ma bitmapy, więc liczy się jej przesunięcie
        self.assertGreater(font.szerokosc("a "), font.szerokosc("a"))


class TestSkladuStron(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.pan_tadeusz = Czytnik(Ksiazka.z_pliku(KSIAZKI / "pan_tadeusz.txt"))
        cls.treny = Ksiazka.z_pliku(KSIAZKI / "treny.txt")

    def test_naglowek(self):
        self.assertEqual(self.pan_tadeusz.ksiazka.tytul, "Pan Tadeusz")
        self.assertEqual(self.pan_tadeusz.ksiazka.autor, "Adam Mickiewicz")

    def test_rozdzialy_i_rodzaj_tekstu(self):
        self.assertEqual(len(self.pan_tadeusz.ksiegi), 13)  # 12 ksiąg i Epilog
        self.assertFalse(self.pan_tadeusz.proza)
        self.assertEqual(self.pan_tadeusz.tytul_rozdzialu(12), "Epilog")

    def test_strony_pokrywaja_caly_tekst(self):
        strony = self.pan_tadeusz.strony
        self.assertEqual(strony[0], 0)
        self.assertTrue(all(a < b for a, b in zip(strony, strony[1:])))
        self.assertEqual(self.pan_tadeusz.uloz_strone(strony[-1]), self.pan_tadeusz.dlugosc)

    def test_kazdy_rozdzial_od_nowej_strony(self):
        for k, s in zip(self.pan_tadeusz.ksiegi, self.pan_tadeusz.pierwsza_strona_ksiegi):
            self.assertEqual(self.pan_tadeusz.strony[s], k)

    def test_wieksza_czcionka_to_wiecej_stron(self):
        ile = [len(Czytnik(self.treny, r).strony) for r in range(4)]
        self.assertEqual(ile, sorted(ile))
        self.assertLess(ile[0], ile[3])

    def test_rysowanie_strony(self):
        obraz = self.pan_tadeusz.rysuj_strone(1)
        self.assertEqual(obraz.size, (SZEROKOSC, WYSOKOSC))
        tekst = obraz.crop((0, 0, SZEROKOSC, DOL_TEKSTU))
        self.assertEqual(tekst.getextrema()[0], 0)  # jest jakiś czarny piksel
        # strona tytułowa nie ma stopki
        stopka = self.pan_tadeusz.rysuj_strone(0).crop((0, DOL_TEKSTU, SZEROKOSC, WYSOKOSC))
        self.assertEqual(stopka.getextrema()[0], 1)  # same białe piksele


if __name__ == "__main__":
    unittest.main()
