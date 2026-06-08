/*
 * ui.h — Semua fungsi rendering ke TFT 240x240 (Bersih Tanpa Peta)
 */

#pragma once
#include <TFT_eSPI.h>
#include <math.h>
#include "config.h"

extern TFT_eSPI    tft;
extern TFT_eSprite sprite;

// Helper: parse hex color "#RRGGBB" → 16-bit
static uint16_t hexToColor565(const char* hex) {
  if (!hex || hex[0] != '#' || strlen(hex) < 7) return TFT_WHITE;
  uint32_t rgb = strtol(hex + 1, nullptr, 16);
  uint8_t r = (rgb >> 16) & 0xFF;
  uint8_t g = (rgb >> 8)  & 0xFF;
  uint8_t b = (rgb)        & 0xFF;
  return tft.color565(r, g, b);
}

// Helper: countryCode → Badge
static void drawCountryBadge(int x, int y, const char* code) {
  sprite.fillRoundRect(x, y, 44, 26, 4, COLOR_DARK_GREY);
  sprite.drawRoundRect(x, y, 44, 26, 4, COLOR_GREY);
  sprite.setTextColor(COLOR_WHITE, COLOR_DARK_GREY);
  sprite.setTextSize(1);
  sprite.setFreeFont(nullptr);
  sprite.setTextDatum(MC_DATUM);
  sprite.drawString(code, x + 22, y + 13);
  sprite.setTextDatum(TL_DATUM);
}

// Helper: gambar podium P1/P2/P3
static void drawPodium(int x, int y, int pos, uint16_t color) {
  int w = 20, h = 0;
  if (pos == 1) h = 30;
  else if (pos == 2) h = 22;
  else h = 16;

  sprite.fillRect(x, y + (30 - h), w, h, color);
  sprite.drawRect(x, y + (30 - h), w, h, COLOR_GREY);

  char num[3];
  snprintf(num, sizeof(num), "%d", pos);
  sprite.setTextColor(COLOR_WHITE);
  sprite.setTextDatum(MC_DATUM);
  sprite.setTextSize(1);
  sprite.drawString(num, x + w/2, y + (30 - h) + h/2);
  sprite.setTextDatum(TL_DATUM);
}

// Helper: countdown "Xd Xh Xm"
static void formatCountdown(time_t secondsLeft, char* buf, size_t len) {
  if (secondsLeft <= 0) {
    strlcpy(buf, "ONGOING", len);
    return;
  }
  long d = secondsLeft / 86400;
  long h = (secondsLeft % 86400) / 3600;
  long m = (secondsLeft % 3600) / 60;

  if (d > 0)      snprintf(buf, len, "%ldd %ldh %ldm", d, h, m);
  else if (h > 0) snprintf(buf, len, "%ldh %ldm", h, m);
  else            snprintf(buf, len, "%ldm", m);
}

// Splash screen
void drawSplash() {
  sprite.fillSprite(COLOR_BG);
  sprite.fillRect(0, 0, 240, 50, COLOR_RED);
  sprite.setTextColor(COLOR_WHITE);
  sprite.setTextDatum(MC_DATUM);
  sprite.setTextSize(3);
  sprite.drawString("F1", 120, 25);

  sprite.setTextSize(1);
  sprite.drawString("2026 SEASON CALENDAR", 120, 80);
  sprite.drawFastHLine(20, 95, 200, COLOR_RED);

  sprite.setTextColor(COLOR_GREY);
  sprite.drawString("Race Schedule Display", 120, 120);
  sprite.drawString("ESP32 + ST7789 240x240", 120, 140);
  sprite.setTextColor(COLOR_DARK_GREY);
  sprite.drawString("by OpenF1 API", 120, 180);
  sprite.setTextDatum(TL_DATUM);
  sprite.pushSprite(0, 0);
}

// Status / loading message
void drawStatusMessage(const char* msg, uint16_t color) {
  sprite.fillSprite(COLOR_BG);
  static int spinFrame = 0;
  spinFrame = (spinFrame + 1) % 8;
  for (int i = 0; i < 8; i++) {
    int angle = i * 45;
    int sx = 120 + 20 * cos(radians(angle));
    int sy = 100 + 20 * sin(radians(angle));
    uint16_t c = (i == spinFrame) ? COLOR_RED : COLOR_DARK_GREY;
    sprite.fillCircle(sx, sy, 3, c);
  }
  sprite.setTextColor(color);
  sprite.setTextDatum(MC_DATUM);
  sprite.setTextSize(1);
  sprite.drawString(msg, 120, 150);
  sprite.setTextDatum(TL_DATUM);
  sprite.pushSprite(0, 0);
}

// No data screen
void drawNoDataScreen(const char* msg) {
  sprite.fillSprite(COLOR_BG);
  sprite.setTextColor(COLOR_GREY);
  sprite.setTextDatum(MC_DATUM);
  sprite.setTextSize(1);
  sprite.drawString("NO DATA", 120, 100);
  sprite.drawString(msg, 120, 130);
  sprite.setTextDatum(TL_DATUM);
  sprite.pushSprite(0, 0);
}

// NEXT RACE screen (Bentuk Gambar Bulat/Sirkuit Dihapus Total)
void drawNextRaceScreen(const RaceData& rd, time_t nowEpoch, int animStep) {
  int offsetX = 0;
  if (animStep >= 0) {
    offsetX = 240 - (240 * animStep / ANIM_STEPS);
  }

  sprite.fillSprite(COLOR_BG);

  // Header
  sprite.fillRect(offsetX + 0, 0, 240, 40, COLOR_RED);
  sprite.setTextColor(COLOR_WHITE);
  sprite.setTextDatum(ML_DATUM);
  sprite.setTextSize(1);
  sprite.drawString("NEXT RACE", offsetX + 8, 20);

  sprite.setTextDatum(MR_DATUM);
  sprite.drawString(rd.dateRange, offsetX + 232, 20);
  sprite.setTextDatum(TL_DATUM);

  // Teks Informasi Identitas Grand Prix
  sprite.setTextColor(COLOR_WHITE);
  char raceName[32];
  strlcpy(raceName, rd.raceName, sizeof(raceName));
  sprite.drawString(raceName, offsetX + 8, 48);

  sprite.setTextColor(COLOR_GREY);
  char locStr[40];
  snprintf(locStr, sizeof(locStr), "%s", rd.location);
  sprite.drawString(locStr, offsetX + 8, 63);

  // Gambar Badge Negara Saja (Gambar Lingkaran Bulat Sirkuit Sampingnya Sudah Dibuang)
  drawCountryBadge(offsetX + 8, 78, rd.countryCode);

  sprite.drawFastHLine(offsetX + 0, 112, 240, COLOR_DARK_GREY);

  // Jadwal Rangkaian Sesi Kontinu
  const int SESSION_Y_START = 118;
  const int SESSION_H       = 20;

  for (int i = 0; i < rd.sessionCount && i < 5; i++) {
    const SessionInfo& si = rd.sessions[i];
    int y = SESSION_Y_START + i * SESSION_H;

    bool isRace = strcmp(si.name, "Race") == 0;
    if (isRace) {
      sprite.fillRect(offsetX + 0, y - 2, 240, SESSION_H, COLOR_BG2);
      sprite.drawFastHLine(offsetX + 0, y - 2, 3, COLOR_RED);
    }

    uint16_t nameColor = isRace ? COLOR_RED : COLOR_WHITE;
    sprite.setTextColor(nameColor);
    sprite.setTextDatum(ML_DATUM);
    sprite.drawString(si.name, offsetX + 10, y + 8);

    sprite.setTextColor(COLOR_GREY);
    sprite.drawString(si.dayStr, offsetX + 80, y + 8);

    sprite.setTextColor(COLOR_WHITE);
    sprite.setTextDatum(MR_DATUM);
    sprite.drawString(si.timeStr, offsetX + 232, y + 8);
    sprite.setTextDatum(TL_DATUM);
  }

  // Bar Hitung Mundur Sesi Utama
  sprite.drawFastHLine(0, 218, 240, COLOR_DARK_GREY);
  sprite.fillRect(0, 219, 240, 21, COLOR_BG2);

  char countdown[24];
  time_t secondsLeft = rd.raceStartEpoch - nowEpoch;
  formatCountdown(secondsLeft, countdown, sizeof(countdown));

  sprite.setTextColor(COLOR_YELLOW);
  sprite.setTextDatum(ML_DATUM);
  sprite.drawString("Race in:", 8, 229);

  sprite.setTextColor(COLOR_WHITE);
  sprite.setTextDatum(MR_DATUM);
  sprite.drawString(countdown, 232, 229);
  sprite.setTextDatum(TL_DATUM);

  sprite.pushSprite(0, 0);
}

// ─────────────────────────────────────────
//  LAST RESULT screen (Fixed Kotak Posisi & Nama Driver)
// ─────────────────────────────────────────
void drawLastResultScreen(const RaceData& rd, const ResultData results[3], const char* sessionLabel, bool hasResults, int animStep) {
  int offsetX = 0;
  if (animStep >= 0) {
    offsetX = -240 + (240 * animStep / ANIM_STEPS);
  }

  sprite.fillSprite(COLOR_BG);

  // Header Bar
  sprite.fillRect(offsetX + 0, 0, 240, 40, COLOR_RED);
  sprite.setTextColor(COLOR_WHITE);
  sprite.setTextDatum(ML_DATUM);
  
  char headerLabel[30];
  if (sessionLabel && strlen(sessionLabel) > 0) {
    snprintf(headerLabel, sizeof(headerLabel), "%s RESULT", sessionLabel);
    for (int _i = 0; headerLabel[_i]; _i++)
      if (headerLabel[_i] >= 'a' && headerLabel[_i] <= 'z')
        headerLabel[_i] -= 32;
  } else {
    strlcpy(headerLabel, "SESSION RESULT", sizeof(headerLabel));
  }
  sprite.drawString(headerLabel, offsetX + 8, 20);
  sprite.setTextDatum(MR_DATUM);
  sprite.drawString(rd.dateRange, offsetX + 232, 20);
  sprite.setTextDatum(TL_DATUM);

  // Data Lintasan Teks
  sprite.setTextColor(COLOR_WHITE);
  sprite.drawString(rd.raceName, offsetX + 8, 48);
  sprite.setTextColor(COLOR_GREY);
  sprite.drawString(rd.location, offsetX + 8, 63);

  sprite.drawFastHLine(0, 80, 240, COLOR_DARK_GREY);

  if (!hasResults) {
    sprite.setTextColor(COLOR_GREY);
    sprite.setTextDatum(MC_DATUM);
    sprite.drawString("Results not available", 120 + offsetX, 150);
    sprite.setTextDatum(TL_DATUM);
    sprite.pushSprite(0, 0);
    return;
  }

  // Komponen Visualisasi Podium Top 3 
  int podiumY = 82;
  const ResultData* p1 = nullptr;
  const ResultData* p2 = nullptr;
  const ResultData* p3 = nullptr;
  for (int i = 0; i < 3; i++) {
    if (!results[i].valid) continue;
    if (results[i].position == 1) p1 = &results[i];
    if (results[i].position == 2) p2 = &results[i];
    if (results[i].position == 3) p3 = &results[i];
  }

  uint16_t c1 = p1 ? hexToColor565(p1->teamColor) : COLOR_GREY;
  uint16_t c2 = p2 ? hexToColor565(p2->teamColor) : COLOR_GREY;
  uint16_t c3 = p3 ? hexToColor565(p3->teamColor) : COLOR_GREY;

  drawPodium(offsetX + 50,  podiumY, 2, c2);
  drawPodium(offsetX + 78,  podiumY, 1, c1);
  drawPodium(offsetX + 106, podiumY, 3, c3);

  sprite.setTextSize(1);
  sprite.setTextDatum(MC_DATUM);
  if (p2) { sprite.setTextColor(c2); sprite.drawString(p2->driverAbbr, offsetX + 60, podiumY - 8); }
  if (p1) { sprite.setTextColor(c1); sprite.drawString(p1->driverAbbr, offsetX + 88, podiumY - 8); }
  if (p3) { sprite.setTextColor(c3); sprite.drawString(p3->driverAbbr, offsetX + 116, podiumY - 8); }
  sprite.setTextDatum(TL_DATUM);

  int listY = podiumY + 36;
  sprite.drawFastHLine(0, listY, 240, COLOR_DARK_GREY);
  listY += 4;

  // Render Baris Pembalap
  const ResultData* ordered[3] = {p1, p2, p3};
  const char*    medals[3]      = {"P1", "P2", "P3"};

  for (int i = 0; i < 3; i++) {
    const ResultData* r = ordered[i];
    if (!r) continue;

    int y = listY + i * 36;
    if (i == 0) sprite.fillRect(offsetX + 0, y - 1, 240, 34, 0x1904);

    // 🟩 KOTAK WARNA DIISI URUTAN FINISH (P1, P2, P3)
    uint16_t tc = hexToColor565(r->teamColor);
    sprite.fillRect(offsetX + 6, y + 5, 24, 22, tc);
    sprite.setTextColor(COLOR_WHITE);
    sprite.setTextDatum(MC_DATUM);
    sprite.drawString(medals[i], offsetX + 18, y + 16);

    // Baris Atas: Singkatan Nama Driver (misal VER / NOR) atau nomor jika gagal
    sprite.setTextColor(COLOR_WHITE);
    sprite.setTextDatum(ML_DATUM);
    sprite.drawString(r->driverAbbr, offsetX + 38, y + 10);

    // 🟩 BARIS BAWAH: Diubah dari "Driver X" menjadi Full Name (Nama Lengkap) Hasil API asli
    sprite.setTextColor(COLOR_GREY);
    char shortName[24];
    strlcpy(shortName, r->driverName, sizeof(shortName));
    sprite.drawString(shortName, offsetX + 38, y + 24);

    // Gap Waktu
    sprite.setTextColor(i == 0 ? COLOR_YELLOW : COLOR_GREY);
    sprite.setTextDatum(MR_DATUM);
    sprite.drawString(r->gapStr, offsetX + 234, y + 16);
    sprite.setTextDatum(TL_DATUM);

    if (i < 2) sprite.drawFastHLine(offsetX + 36, y + 33, 200, COLOR_DARK_GREY);
  }

  sprite.pushSprite(0, 0);
}

/*
 * Tambahkan fungsi ini di bagian paling bawah file ui.h Anda
 */

// ─────────────────────────────────────────
//  FUNGSI BARU: Gambar Layar Driver Standings
// ─────────────────────────────────────────
void drawStandingsScreen(const RaceData& rd, const StandingsData standings[10], int animStep) {
  int offsetX = 0;
  if (animStep >= 0) {
    offsetX = -240 + (240 * animStep / ANIM_STEPS);
  }

  sprite.fillSprite(COLOR_BG);

  // Header Bar
  sprite.fillRect(offsetX + 0, 0, 240, 40, COLOR_RED);
  sprite.setTextColor(COLOR_WHITE);
  sprite.setTextDatum(ML_DATUM);
  sprite.drawString("DRIVER CHAMPIONSHIP", offsetX + 8, 20);
  sprite.setTextDatum(TL_DATUM);

  // Sub-header pembagi kolom tabel
  int startY = 46;
  sprite.setTextColor(COLOR_GREY);
  sprite.drawString("POS", offsetX + 8, startY);
  sprite.drawString("DRIVER", offsetX + 52, startY);
  sprite.drawString("NUM", offsetX + 130, startY);
  sprite.setTextDatum(TR_DATUM);
  sprite.drawString("PTS", offsetX + 232, startY);
  sprite.setTextDatum(TL_DATUM);

  sprite.drawFastHLine(offsetX + 0, 60, 240, COLOR_DARK_GREY);

  // Gambar 10 Baris Klasemen Pembalap
  int rowY = 65;
  int rowHeight = 15;

  for (int i = 0; i < 10; i++) {
    int y = rowY + (i * rowHeight);
    const StandingsData& sd = standings[i];

    if (!sd.valid) continue;

    // Beri highlight warna emas/BG terang khusus untuk peringkat pertama
    if (i == 0) {
      sprite.setTextColor(COLOR_YELLOW);
    } else {
      sprite.setTextColor(COLOR_WHITE);
    }

    // Urutan posisi
    char posStr[4];
    snprintf(posStr, sizeof(posStr), "%02d", i + 1);
    sprite.drawString(posStr, offsetX + 10, y);

    // Inisial Nama (Abbr)
    sprite.drawString(sd.driverAbbr, offsetX + 52, y);

    // Nomor Pembalap
    sprite.setTextColor(COLOR_GREY);
    char numStr[4];
    snprintf(numStr, sizeof(numStr), "#%d", sd.driverNumber);
    sprite.drawString(numStr, offsetX + 130, y);

    // Points Current (Rata Kanan)
    if (i == 0) sprite.setTextColor(COLOR_YELLOW);
    else sprite.setTextColor(COLOR_WHITE);
    
    sprite.setTextDatum(TR_DATUM);
    char ptsStr[6];
    snprintf(ptsStr, sizeof(ptsStr), "%d", sd.points);
    sprite.drawString(ptsStr, offsetX + 232, y);
    sprite.setTextDatum(TL_DATUM);
  }

  // Footer dekoratif tipis
  sprite.drawFastHLine(0, 222, 240, COLOR_DARK_GREY);
  sprite.setTextColor(COLOR_DARK_GREY);
  sprite.drawString("OpenF1 Realtime Standings", 8, 227);

  sprite.pushSprite(0, 0);
}

// ─────────────────────────────────────────
//  Gambar Layar Konfigurasi Wi-Fi
// ─────────────────────────────────────────
void drawWifiPortalScreen(const char* ssid, const char* pass, const char* ipStr) {
  sprite.fillSprite(COLOR_BG);

  // Header Bar Portal
  sprite.fillRect(0, 0, 240, 40, COLOR_RED);
  sprite.setTextColor(COLOR_WHITE);
  sprite.setTextDatum(MC_DATUM);
  sprite.setTextSize(1);
  sprite.drawString("WI-FI CONFIG PORTAL", 120, 20);
  sprite.setTextDatum(TL_DATUM);

  // Instruksi Koneksi
  sprite.setTextColor(COLOR_WHITE);
  sprite.drawString("1. Connect to Wi-Fi Hotspot:", 10, 55);
  
  // Kotak Informasi SSID & PASS AP Lokal
  sprite.fillRoundRect(10, 75, 220, 55, 6, COLOR_BG2);
  sprite.drawRoundRect(10, 75, 220, 55, 6, COLOR_GREY);
  
  sprite.setTextColor(COLOR_YELLOW);
  char ssidBuf[40];  snprintf(ssidBuf, sizeof(ssidBuf), "SSID : %s", ssid);
  char passBuf[40];  snprintf(passBuf, sizeof(passBuf), "PASS : %s", pass);
  sprite.drawString(ssidBuf, 20, 85);
  sprite.drawString(passBuf, 20, 105);

  // Instruksi Langkah Kedua
  sprite.setTextColor(COLOR_WHITE);
  sprite.drawString("2. Open Browser & Go To IP:", 10, 145);

  // Kotak Informasi IP Address
  sprite.fillRoundRect(10, 165, 220, 35, 6, COLOR_BG2);
  sprite.drawRoundRect(10, 165, 220, 35, 6, COLOR_GREEN);
  
  sprite.setTextColor(COLOR_GREEN);
  sprite.setTextDatum(MC_DATUM);
  sprite.setTextSize(2); // Perbesar teks IP agar scannable
  sprite.drawString(ipStr, 120, 182);
  
  // Reset format text
  sprite.setTextSize(1);
  sprite.setTextDatum(TL_DATUM);

  sprite.setTextColor(COLOR_GREY);
  sprite.drawString("Set your internet Wi-Fi there.", 15, 215);

  sprite.pushSprite(0, 0);
}