# F1 Race Schedule Display — ESP32 + ST7789 240×240
## Dokumentasi Lengkap

---

## 📦 Komponen yang Dibutuhkan

| Komponen | Keterangan |
|----------|------------|
| ESP32 DevKit v1 | Board utama |
| TFT ST7789 240×240 | Layar 1.3" atau 1.54" |
| Kabel jumper | 7 kabel |
| Power supply | USB 5V atau 3.3V regulated |

---

## 🔌 Wiring ESP32 → ST7789

```
ST7789 Pin   →   ESP32 GPIO
─────────────────────────────
VCC          →   3.3V
GND          →   GND
SCL (SCLK)   →   GPIO 18
SDA (MOSI)   →   GPIO 23
RES (RST)    →   GPIO 4
DC  (A0)     →   GPIO 2
CS           →   GPIO 15
BLK          →   3.3V  (atau GPIO + resistor 100Ω untuk kontrol brightness)
```

> ⚠️ **Penting:** ST7789 menggunakan 3.3V. Jangan sambungkan ke 5V!

---

## 📚 Library yang Dibutuhkan

Install via **Arduino IDE → Library Manager**:

1. **TFT_eSPI** by Bodmer  
2. **ArduinoJson** by Benoit Blanchon — pilih versi **6.x**  
3. **NTPClient** by Fabrice Weinberg  

Library `WiFi`, `HTTPClient`, `WiFiUDP` sudah built-in di ESP32 core.

---

## ⚙️ Konfigurasi TFT_eSPI (WAJIB)

Setelah install TFT_eSPI, buka file:
```
Windows: C:\Users\[nama]\Documents\Arduino\libraries\TFT_eSPI\User_Setup.h
Mac/Linux: ~/Arduino/libraries/TFT_eSPI/User_Setup.h
```

Hapus semua `#define` driver yang ada, lalu ganti dengan:

```cpp
// Driver
#define ST7789_DRIVER

// Ukuran layar
#define TFT_WIDTH  240
#define TFT_HEIGHT 240

// Pin mapping
#define TFT_MOSI  23
#define TFT_SCLK  18
#define TFT_CS    15
#define TFT_DC     2
#define TFT_RST    4
// #define TFT_BLK  -1   // -1 = tidak dikontrol software

// Font yang dibutuhkan
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF
#define SMOOTH_FONT

// SPI speed
#define SPI_FREQUENCY  40000000
```

---

## 🛠️ Konfigurasi Proyek

Edit file **`config.h`**:

```cpp
// WiFi Anda
#define WIFI_SSID   "NamaWiFiAnda"
#define WIFI_PASS   "PasswordWiFiAnda"
```

Itu saja yang perlu diubah! Sisanya sudah dikonfigurasi untuk WIB (UTC+7).

---

## 📁 Struktur File

```
f1_tft_esp32/
├── f1_tft_esp32.ino   ← Main sketch (upload ini)
├── config.h           ← Konfigurasi WiFi, pin, warna, struct data
├── api.h              ← Fetch & parse OpenF1 API
└── ui.h               ← Semua fungsi rendering layar
```

---

## 🚀 Cara Upload

1. Buka `f1_tft_esp32.ino` di Arduino IDE
2. Pilih Board: **ESP32 Dev Module**
3. Pilih Port COM yang sesuai
4. Klik **Upload**

---

## 🖥️ Tampilan & Logika

### Mode 1: NEXT RACE (default)
```
┌────────────────────────────────────────┐
│ NEXT RACE              05 - 07 Jun     │  ← Header merah
│ Grand Prix de Monaco                   │
│ Monte Carlo                   [MON]    │  ← Badge negara + circuit shape
├────────────────────────────────────────┤
│ FP1    Fri   18:30                     │
│ FP2    Fri   22:00                     │
│ FP3    Sat   17:30                     │
│ Quali  Sat   21:00                     │
│▐Race   Sun   20:00                     │  ← Highlight merah
├────────────────────────────────────────┤
│ Race in:              2d 14h 30m       │  ← Countdown
└────────────────────────────────────────┘
```

### Mode 2: LAST RESULT (muncul 2 hari setelah race)
```
┌────────────────────────────────────────┐
│ RACE RESULT            05 - 07 Jun     │  ← Header merah
│ Grand Prix de Monaco                   │
│ Monte Carlo                            │
├────────────────────────────────────────┤
│         [P2]  [P1]  [P3]              │  ← Visual podium berwarna tim
│         NOR   VER   LEC               │
├────────────────────────────────────────┤
│ [P1] VER  Max Verstappen      WINNER  │
│ [P2] NOR  Lando Norris        +1.234  │
│ [P3] LEC  Charles Leclerc     +4.567  │
└────────────────────────────────────────┘
```

### Logika pergantian:
- **Default**: Mode Next Race
- **Dalam 2 hari setelah race selesai**: bergantian antara Next Race ↔ Last Result setiap **15 detik** (bisa diubah di `config.h`: `MODE_SWITCH_MS`)
- **Transisi**: animasi slide (Next Race slide dari kanan, Result slide dari kiri)

---

## ⏱️ Semua waktu ditampilkan dalam WIB (UTC+7)

---

## 🔧 Troubleshooting

| Masalah | Solusi |
|---------|--------|
| Layar putih/tidak menyala | Cek User_Setup.h, cek wiring pin |
| WiFi tidak konek | Cek SSID/password di config.h |
| API tidak merespons | Cek koneksi internet, coba restart |
| JSON error | ESP32 kehabisan RAM, restart |
| Waktu salah | NTP belum sync, tunggu beberapa detik |
| Flicker layar | Normal di frame pertama, sprite buffer aktif |

---

## 🔋 Konsumsi Daya

- ESP32 aktif WiFi: ~160mA @ 3.3V
- ST7789 240×240: ~20mA
- **Total: ~180mA** → USB 5V aman

---

## 📡 API yang Digunakan

| Endpoint | Keterangan |
|----------|------------|
| `api.openf1.org/v1/meetings?year=2026` | Daftar semua race weekend |
| `api.openf1.org/v1/sessions?year=2026` | Semua sesi (FP1, Quali, Race, dll) |
| `api.openf1.org/v1/session_result?session_key=X&position%3C=3` | Top 3 hasil |

Data direfresh setiap **10 menit** (configurable di `API_REFRESH_MS`).

---

## 💡 Tips & Kustomisasi

```cpp
// Di config.h — ubah sesuai kebutuhan:

#define API_REFRESH_MS    (10UL * 60 * 1000)  // Refresh tiap 10 menit
#define MODE_SWITCH_MS    (15UL * 1000)        // Ganti tampilan tiap 15 detik  
#define RESULT_SHOW_DURATION_SEC (2 * 24 * 3600L)  // Tampilkan hasil 2 hari
```
