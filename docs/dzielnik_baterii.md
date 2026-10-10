# Dzielnik napięcia do pomiaru baterii

Bez tej płytki czytnik działa normalnie, tylko nie pokazuje stanu baterii. Z nią w menu biblioteki widać procent naładowania albo błyskawicę podczas ładowania, a przy rozładowanej baterii (ok. 3,3 V) czytnik sam się usypia. Kod jest w [`firmware/czytnik/bateria.ino`](../firmware/czytnik/bateria.ino).

## Po co dzielnik

Bateria ma do 4,2 V, a USB 5 V. Wejście ADC w ESP32-S3 przyjmuje maksymalnie ok. 3,1 V, więc napięcie trzeba podzielić na pół. Robią to dwa jednakowe rezystory połączone szeregowo.

```
   wejście
      │
    [100k]   górny
      │
      ├──────► GPIO        połowa napięcia
      │
    [100k]   dolny
      │
     GND
```

Potrzebne są dwa takie dzielniki:

| Dzielnik | Wejście | Wyjście | Co mierzy |
|---|---|---|---|
| bateria | TP4056 **OUT+** | **GPIO1** | napięcie baterii (1,6–2,1 V na pinie) |
| ładowarka | TP4056 **IN+** (5 V z USB) | **GPIO2** | czy ładowarka jest wpięta (ok. 2,5 V albo 0 V) |

## Części

- 4 rezystory 100 kΩ, 1/4 W, THT. Pasuje też inna wartość w zakresie ok. 47k–470k, byle w parze były dwie jednakowe sztuki. Przy rezystorach 5% kup 8 sztuk i dobierz pary multimetrem.
- Kondensator ceramiczny 100 nF (nadruk `104`), uspokaja odczyt ADC.
- Kawałek płytki uniwersalnej, raster 2,54 mm, ok. 2×2 cm (najlepiej dwustronnej, z metalizowanymi otworami).

Nie używaj 1 kΩ ani innych małych wartości. Dzielnik baterii pobiera prąd cały czas, także w uśpieniu: 2×100 kΩ to ok. 20 µA, a 2×1 kΩ to ok. 2 mA, które rozładowałyby baterię w kilka tygodni.

## Ułożenie na płytce

```
   5V (z IN+)          VBAT (z OUT+)
       │                    │
     [R3]                 [R1]
       │                    │
       ●──┐              ┌──●──[C]──┐
       │  │              │  │       │
     [R4] │              │ [R2]     │
       │  │              │  │       │
   ····●··│··············│··●·······●···· GND (od spodu płytki) ──► GND ESP
          ▼              ▼
        GPIO2          GPIO1
```

1. Wlutuj R1 nad R2 i R3 nad R4, w dwóch kolumnach. Wspólna nóżka w każdej parze to środek dzielnika.
2. Kondensator wlutuj między środek dzielnika baterii (R1/R2) a masę. Nie ma polaryzacji.
3. Dolne końce R2, R4 i kondensatora połącz razem jako masę. Poprowadź ją od spodu płytki, wtedy nie krzyżuje się z odczepami do GPIO.
4. Płytkę przyklej tuż obok pinów 1 i 2 ESP. Przewody do GPIO mają być krótkie, bo rezystory 100 kΩ mają dużą impedancję i długi przewód zbiera zakłócenia. Długie mogą być przewody wejściowe z TP4056.

## Sprawdzenie przed podłączeniem do ESP

Najpierw przylutuj tylko wejścia i masę, a przewodów do GPIO jeszcze nie. Zmierz multimetrem względem GND:

- środek R1/R2: połowa napięcia baterii, czyli ok. 1,6–2,1 V,
- środek R3/R4: ok. 2,5 V przy wpiętej ładowarce, 0 V bez niej.

Jeśli zobaczysz ok. 4 V albo 5 V, coś jest źle połączone. Takie napięcie spali pin ESP, więc najpierw znajdź błąd. Gdy wartości się zgadzają, dolutuj przewody do GPIO1 i GPIO2.

## Pierwsze uruchomienie

Wgraj program i wejdź do menu. Obok napisu „Biblioteka” powinna być ikona baterii z procentem. Po wpięciu ładowarki ekran sam się odświeży i pojawi się błyskawica z napisem `ładuje`, a po naładowaniu `pełna`.
