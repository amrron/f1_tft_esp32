# F1 Race Display - ESP32 + ST7789 240x240
## Dokumentasi proyek

Display F1 ini menampilkan jadwal race berikutnya, hasil sesi terbaru, dan klasemen pembalap dari OpenF1 API. Kode juga sudah memiliki portal Wi-Fi bawaan untuk konfigurasi jaringan serta smart sleep mode yang mematikan backlight pada jam malam jika tidak ada sesi yang sedang live.

## Fitur utama

- Mode `NEXT RACE` untuk menampilkan race weekend berikutnya atau race weekend terakhir yang masih relevan.
- Mode `SESSION RESULT` untuk menampilkan podium top 3 dari sesi terbaru.
- Mode `DRIVER CHAMPIONSHIP` untuk menampilkan klasemen pembalap top 10.
- Portal Wi-Fi lokal jika perangkat belum punya kredensial tersimpan atau gagal konek ke Wi-Fi.
- Smart sleep mode pada jam 22:00 sampai 06:00 WIB, kecuali ada sesi yang sedang berlangsung.
- Animasi slide saat berpindah mode.

## Komponen yang dibutuhkan

| Komponen | Keterangan |
| --- | --- |
| ESP32 DevKit | Board utama |
| TFT ST7789 240x240 | Layar 1.3" atau 1.54" |
| Kabel jumper | 7 kabel |
| Power supply | USB 5V atau 3.3V regulated |

## Wiring ESP32 ke ST7789

```
ST7789 Pin   ->   ESP32 GPIO
----------------------------
VCC          ->   3.3V
GND          ->   GND
SCL / SCLK   ->   GPIO 18
SDA / MOSI   ->   GPIO 23
RES / RST    ->   GPIO 4
DC / A0      ->   GPIO 2
CS           ->   GPIO 15
BLK          ->   GPIO 17
```

> Penting: ST7789 harus memakai 3.3V.

## Library yang dibutuhkan

Install via Arduino IDE Library Manager:

1. TFT_eSPI by Bodmer
2. ArduinoJson 6.x by Benoit Blanchon
3. NTPClient by Fabrice Weinberg

Library `WiFi`, `HTTPClient`, `WiFiUDP`, `Preferences`, `WebServer`, dan `DNSServer` berasal dari ESP32 core.

## Konfigurasi TFT_eSPI

Setelah install TFT_eSPI, sesuaikan `User_Setup.h` agar cocok dengan wiring di atas. Konfigurasi yang dipakai proyek ini adalah:

```cpp
#define ST7789_DRIVER

#define TFT_WIDTH  240
#define TFT_HEIGHT 240

#define TFT_MOSI  23
#define TFT_SCLK  18
#define TFT_CS    15
#define TFT_DC     2
#define TFT_RST    4

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF
#define SMOOTH_FONT

#define SPI_FREQUENCY 40000000
```

## Konfigurasi proyek

Edit [config.h](config.h) untuk menyesuaikan nilai penting berikut:

- `AP_SSID_PORTAL` dan `AP_PASS_PORTAL` untuk hotspot setup lokal.
- `TFT_BLK_PIN` untuk kontrol backlight, saat ini `GPIO 17`.
- `WIB_OFFSET_SEC` sudah diset ke UTC+7.
- `API_REFRESH_MS` untuk interval refresh data OpenF1.
- `MODE_SWITCH_MS` untuk pergantian mode tampilan.
- `FETCH_DAYS_BACK` dan `FETCH_DAYS_FORWARD` untuk rentang pencarian race.

## Alur kerja perangkat

Saat boot, perangkat menampilkan splash screen, menyalakan backlight, lalu mencoba koneksi Wi-Fi dari kredensial yang tersimpan di `Preferences`.

Jika belum ada kredensial atau koneksi gagal, perangkat masuk ke portal Wi-Fi dengan SSID `F1_Display_Setup` dan password `12345678`. Dari portal itu, user memilih jaringan, memasukkan password, lalu perangkat menyimpan kredensial dan reboot.

Setelah online, perangkat:

1. Sinkron waktu dari NTP.
2. Mengambil race meeting, sessions, results, dan standings dari OpenF1.
3. Merender mode tampilan aktif ke TFT.
4. Memperbarui data secara berkala.

## Mode tampilan

### NEXT RACE

Mode default. Menampilkan nama event, lokasi, kode negara, daftar sesi, dan countdown menuju race start. Jika race berikutnya belum ditemukan, perangkat akan memakai race terakhir yang masih relevan.

### SESSION RESULT

Menampilkan hasil podium top 3 dari endpoint session result terbaru. Tampilan ini memakai nama event terbaru dari `meetings?meeting_key=latest` agar judul hasil sinkron dengan event yang sedang diproses.

### DRIVER CHAMPIONSHIP

Menampilkan klasemen pembalap top 10 dari endpoint championship drivers. Data diurutkan ulang di sketch karena API beta tidak selalu mengembalikan urutan yang konsisten.

### Pergantian mode

- `NEXT RACE` tetap jadi mode utama.
- Setelah data hasil atau klasemen tersedia, perangkat akan berputar antar mode setiap 15 detik.
- Transisi antar mode memakai animasi slide.

## Smart sleep mode

Perangkat mematikan backlight pada jam 22:00 sampai 06:00 WIB jika tidak ada sesi yang sedang live. Jika ada sesi yang sedang berlangsung, layar tetap menyala.

## API yang digunakan

| Endpoint | Fungsi |
| --- | --- |
| `https://api.openf1.org/v1/meetings?year=2026&date_start%3E=...&date_end%3C=...` | Mencari meeting dalam rentang tanggal |
| `https://api.openf1.org/v1/sessions?year=2026&date_start%3E=...&date_end%3C=...` | Mengambil sesi untuk meeting terpilih |
| `https://api.openf1.org/v1/meetings?meeting_key=latest` | Mengambil event terbaru untuk header result dan standings |
| `https://api.openf1.org/v1/session_result?session_key=latest&position%3C=3` | Mengambil top 3 hasil sesi terbaru |
| `https://api.openf1.org/v1/championship_drivers?session_key=latest&position_current%3C=11` | Mengambil klasemen pembalap top 10 |
| `https://api.openf1.org/v1/drivers?session_key=latest` | Cache nama driver, acronym, dan warna tim |

Catatan: kode saat ini masih mengacu ke season `2026`.

## Timing yang dipakai

| Konstanta | Nilai default |
| --- | --- |
| `API_REFRESH_MS` | 1 jam |
| `RENDER_INTERVAL_MS` | 30 detik |
| `MODE_SWITCH_MS` | 15 detik |
| `ANIM_FRAME_MS` | 20 ms |
| `ANIM_STEPS` | 12 frame |

## Struktur file

```
f1_tft_esp32/
├── f1_tft_esp32.ino
├── config.h
├── api.h
└── ui.h
```

## Cara upload

1. Buka `f1_tft_esp32.ino` di Arduino IDE.
2. Pilih board `ESP32 Dev Module`.
3. Pilih port yang sesuai.
4. Upload sketch.

## Troubleshooting

| Masalah | Solusi |
| --- | --- |
| Layar kosong atau putih | Cek wiring, `User_Setup.h`, dan `TFT_BLK_PIN` |
| Wi-Fi tidak konek | Pastikan kredensial tersimpan benar atau masuk portal setup |
| Portal setup tidak muncul | Cek AP `F1_Display_Setup` dan password `12345678` |
| Data result kosong | Sesi terbaru mungkin belum punya hasil atau API belum tersedia |
| Tampilan tidak berganti | Cek `MODE_SWITCH_MS` dan pastikan data hasil/standings berhasil dimuat |
| Jam salah | Tunggu NTP sinkron atau cek koneksi internet |

## Catatan penggunaan

- Semua waktu ditampilkan dalam WIB (UTC+7).
- Data hasil dan klasemen hanya akan muncul jika request OpenF1 berhasil dan cache driver berhasil dimuat.
- Untuk hasil terbaik, biarkan perangkat online saat start agar NTP dan OpenF1 bisa diambil sekali di awal.
