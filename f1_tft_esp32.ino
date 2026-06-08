/*
 * F1 Race Schedule Display (Ditambah Fitur Smart Sleep Mode 22:00 - 06:00 WIB)
 */

#include <FS.h>
#include <SPIFFS.h> 
#include <WiFi.h>
#include <WiFiUdp.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include <NTPClient.h>
#include <Preferences.h>  
#include <WebServer.h>    
#include <DNSServer.h>    
#include "config.h"
#include "ui.h"
#include "api.h"

// ─────────────────────────────────────────
//  Globals
// ─────────────────────────────────────────
TFT_eSPI    tft    = TFT_eSPI();
TFT_eSprite sprite = TFT_eSprite(&tft);

WiFiUDP    ntpUDP;
NTPClient  timeClient(ntpUDP, "pool.ntp.org", WIB_OFFSET_SEC);

Preferences preferences;
WebServer   server(80);
DNSServer   dnsServer;

RaceData   nextRace;
RaceData   lastRace;
RaceData   latestRace; 

ResultData    currentResults[3];    // Penampung hasil podium top 3
StandingsData currentStandings[10];  // Penampung klasemen 10 besar

char       currentResultLabel[30]; 
int        currentResultSessKey = 0;

bool hasNextRace   = false;
bool hasLastRace   = false;
bool hasResults    = false;
bool hasStandings  = false; 
bool isScreenAsleep = false; // Status penanda layar sedang tidur

enum DisplayMode { MODE_NEXT_RACE, MODE_SESSION_RESULT, MODE_DRIVER_STANDINGS };
DisplayMode currentMode = MODE_NEXT_RACE;

unsigned long lastApiUpdate   = 0;
unsigned long lastModeSwitch  = 0;
unsigned long lastAnimFrame   = 0;
int           animStep        = 0;
bool          isTransitioning = false;

void initWifiPortal();
void handleRootPortal();
void handleSaveWifi();
void manageSleepMode();
bool isAnySessionLive(time_t nowEpoch);

void setup() {
  Serial.begin(115200);
  delay(500);

  // Inisialisasi Pin Backlight TFT sebagai OUTPUT
  pinMode(TFT_BLK_PIN, OUTPUT);
  digitalWrite(TFT_BLK_PIN, HIGH); // Nyalakan layar di awal boot

  tft.init();
  tft.setRotation(2);
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(MC_DATUM);

  sprite.setColorDepth(8); 
  sprite.createSprite(240, 240);
  sprite.setTextDatum(TL_DATUM);

  drawSplash();
  delay(1500);

  connectWiFi();

  timeClient.begin();
  for (int i = 0; i < 15; i++) {
    if (timeClient.update()) break;
    delay(500);
  }

  fetchAndUpdate();

  lastApiUpdate  = millis();
  lastModeSwitch = millis();
}

void loop() {
  unsigned long now = millis();
  timeClient.update();

  // Jalankan manajemen sleep mode pintar setiap loop
  manageSleepMode();

  // Jika layar sedang tidur, hentikan proses render dan transisi untuk menghemat CPU ESP32
  if (isScreenAsleep) {
    // Tetap refresh API di latar belakang setiap 1 jam meskipun layar mati
    if (now - lastApiUpdate > API_REFRESH_MS) {
      fetchAndUpdate();
      lastApiUpdate = now;
    }
    delay(100); // Beri jeda rileks pada processor
    return; 
  }

  if (now - lastApiUpdate > API_REFRESH_MS) {
    fetchAndUpdate();
    lastApiUpdate = now;
  }

  determineDisplayMode();

  if (isTransitioning) {
    if (now - lastAnimFrame > ANIM_FRAME_MS) {
      lastAnimFrame = now;
      animStep++;
      if (animStep >= ANIM_STEPS) {
        isTransitioning = false;
        animStep = 0;
        drawCurrentMode(-1); 
      } else {
        drawCurrentMode(animStep);
      }
    }
    return;
  }

  if (now - lastAnimFrame > RENDER_INTERVAL_MS) {
    lastAnimFrame = now;
    drawCurrentMode(-1);
  }
}

void connectWiFi() {
  preferences.begin("wifi-gate", false);
  String storedSsid = preferences.getString("ssid", "");
  String storedPass = preferences.getString("pass", "");

  if (storedSsid == "") {
    initWifiPortal();
    return;
  }

  drawStatusMessage("Connecting Saved Wi-Fi...", TFT_YELLOW);
  WiFi.mode(WIFI_STA);
  WiFi.begin(storedSsid.c_str(), storedPass.c_str());

  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 20) {
    delay(500);
    tries++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    drawStatusMessage("Wi-Fi Connected!", TFT_GREEN);
    delay(1000);
    preferences.end();
  } else {
    initWifiPortal();
  }
}

void initWifiPortal() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(AP_SSID_PORTAL, AP_PASS_PORTAL);
  IPAddress apIP(192, 168, 4, 1);
  dnsServer.start(53, "*", apIP);
  WiFi.scanNetworks();

  server.on("/", handleRootPortal);
  server.on("/save", handleSaveWifi);
  server.onNotFound(handleRootPortal);
  server.begin();

  while (true) {
    dnsServer.processNextRequest();
    server.handleClient();
    drawWifiPortalScreen(AP_SSID_PORTAL, AP_PASS_PORTAL, "192.168.4.1");
    delay(10);
  }
}

void handleRootPortal() {
  String html = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<style>body{font-family:Arial,sans-serif;margin:20px;background:#f0f2f5;} h2{color:#f80000;} input,select{width:100%;padding:10px;margin:8px 0;box-sizing:border-box;} button{background:#07e000;color:white;padding:12px;border:none;width:100%;font-size:16px;cursor:pointer;}</style>";
  html += "</head><body><h2>F1 Display — Wi-Fi Setup</h2>";
  html += "<form action='/save' method='POST'><label>Pilih Wi-Fi Sekitar:</label><select name='ssid'>";
  int n = WiFi.scanComplete();
  if (n <= 0) html += "<option value=''>Tidak ada Wi-Fi terdeteksi. Refresh!</option>";
  else { for (int i = 0; i < n; ++i) html += "<option value='" + WiFi.SSID(i) + "'>" + WiFi.SSID(i) + "</option>"; }
  html += "</select><label>Password Wi-Fi:</label><input type='password' name='pass' placeholder='Masukkan password Wi-Fi'><button type='submit'>CONNECT DEVICE</button></form></body></html>";
  server.send(200, "text/html", html);
}

void handleSaveWifi() {
  String reqSsid = server.arg("ssid"); String reqPass = server.arg("pass");
  if (reqSsid != "") {
    server.send(200, "text/html", "<!DOCTYPE html><html><body><h2>Setup Sukses! ESP32 Rebooting...</h2></body></html>");
    delay(1000);
    preferences.putString("ssid", reqSsid); preferences.putString("pass", reqPass); preferences.end();
    delay(500); ESP.restart();
  }
}

void fetchAndUpdate() {
  drawStatusMessage("Loading Schedule...", TFT_CYAN);
  time_t nowEpoch = timeClient.getEpochTime();
  fetchRaceData(nowEpoch, nextRace, lastRace, hasNextRace, hasLastRace);

  hasResults   = false;
  hasStandings = false;

  drawStatusMessage("Loading Event Name...", TFT_CYAN);
  fetchLatestMeeting(latestRace);

  drawStatusMessage("Caching Drivers...", TFT_CYAN);
  _DriverCache driverCache[24]; 
  int cacheCount = loadDriverCache(driverCache, 24);

  if (cacheCount > 0) {
    drawStatusMessage("Loading Latest Podium...", TFT_CYAN);
    hasResults = fetchSessionResults(currentResults, driverCache, cacheCount);

    drawStatusMessage("Loading Standings...", TFT_CYAN);
    hasStandings = fetchDriverStandings(currentStandings, driverCache, cacheCount);
  }

  strlcpy(currentResultLabel, "LATEST", sizeof(currentResultLabel));
  Serial.printf("[FETCH-DONE] Results=%d, Standings=%d\n", hasResults, hasStandings);
}

void determineDisplayMode() {
  if (!hasNextRace && !hasLastRace) { currentMode = MODE_NEXT_RACE; return; }

  unsigned long now = millis();
  if (now - lastModeSwitch > MODE_SWITCH_MS) {
    DisplayMode nextMode = currentMode;
    if (currentMode == MODE_NEXT_RACE) {
      nextMode = hasResults ? MODE_SESSION_RESULT : (hasStandings ? MODE_DRIVER_STANDINGS : MODE_NEXT_RACE);
    } else if (currentMode == MODE_SESSION_RESULT) {
      nextMode = hasStandings ? MODE_DRIVER_STANDINGS : MODE_NEXT_RACE;
    } else if (currentMode == MODE_DRIVER_STANDINGS) {
      nextMode = MODE_NEXT_RACE;
    }
    if (nextMode != currentMode) {
      currentMode = nextMode; lastModeSwitch = now; isTransitioning = true; animStep = 0; lastAnimFrame = now;
    }
  }
}

void drawCurrentMode(int anim) {
  if (currentMode == MODE_NEXT_RACE) {
    // Mode Jadwal: Tetap melihat kalender masa depan (nextRace atau lastRace)
    if (hasNextRace) drawNextRaceScreen(nextRace, timeClient.getEpochTime(), anim);
    else if (hasLastRace) drawNextRaceScreen(lastRace, timeClient.getEpochTime(), anim);
    else drawNoDataScreen("No race data");
  } 
  else if (currentMode == MODE_SESSION_RESULT) {
    // 🟩 FIXED: Gunakan objek latestRace agar Nama GP sinkron dengan hasil podium terkini
    drawLastResultScreen(latestRace, currentResults, currentResultLabel, hasResults, anim);
  } 
  else if (currentMode == MODE_DRIVER_STANDINGS) {
    // 🟩 FIXED: Gunakan objek latestRace agar Nama GP sinkron dengan data klasemen terkini
    drawStandingsScreen(latestRace, currentStandings, anim);
  }
}

// ─────────────────────────────────────────
//  FUNGSI BARU: Manajemen Sleep Mode Pintar
// ─────────────────────────────────────────
void manageSleepMode() {
  time_t nowEpoch = timeClient.getEpochTime();
  
  // Ambil data jam lokal WIB (jam 0 - 23)
  time_t localTime = nowEpoch + WIB_OFFSET_SEC;
  struct tm* t = gmtime(&localTime);
  int currentHour = t->tm_hour;

  // Cek rentang waktu normal tidur: Jam 10 Malam (22) s/d Jam 6 Pagi (sebelum jam 6)
  bool isSleepWindow = (currentHour >= 22 || currentHour < 6);

  // Periksa apakah saat ini ada sesi latihan/kualifikasi/balapan yang sedang berlangsung
  bool sessionIsLive = isAnySessionLive(nowEpoch);

  if (isSleepWindow && !sessionIsLive) {
    // Masuk ke mode sleep jika belum dalam kondisi tidur
    if (!isScreenAsleep) {
      Serial.println("[SLEEP] Memasuki Sleep Mode. Mematikan Backlight TFT.");
      digitalWrite(TFT_BLK_PIN, LOW); // Matikan lampu latar layar (TFT Gelap Gulita)
      isScreenAsleep = true;
    }
  } else {
    // Bangunkan layar jika waktu sudah pagi ATAU balapan live sedang dimulai
    if (isScreenAsleep) {
      Serial.println("[SLEEP] Layar Bangun! Menyalakan Backlight TFT.");
      digitalWrite(TFT_BLK_PIN, HIGH); // Nyalakan kembali lampu latar layar
      isScreenAsleep = false;
      
      // Paksa render ulang layar saat bangun agar info up-to-date
      drawCurrentMode(-1); 
    }
  }
}

// Fungsi scanning waktu pengecekan ada tidaknya sesi yang sedang live berjalan saat ini
bool isAnySessionLive(time_t nowEpoch) {
  // Cek seluruh rangkaian sesi pada objek nextRace
  if (hasNextRace) {
    for (int i = 0; i < nextRace.sessionCount; i++) {
      if (nextRace.sessions[i].valid) {
        // Jika epoch sekarang berada di antara waktu start dan end sebuah sesi
        if (nowEpoch >= nextRace.sessions[i].startEpoch && nowEpoch <= nextRace.sessions[i].endEpoch) {
          Serial.printf("[LIVE-TRACK] Sesi AKTIF terdeteksi di nextRace: %s\n", nextRace.sessions[i].name);
          return true; 
        }
      }
    }
  }

  // Cek juga rangkaian sesi pada objek lastRace (antisipasi jika race weekend baru selesai berjalan)
  if (hasLastRace) {
    for (int i = 0; i < lastRace.sessionCount; i++) {
      if (lastRace.sessions[i].valid) {
        if (nowEpoch >= lastRace.sessions[i].startEpoch && nowEpoch <= lastRace.sessions[i].endEpoch) {
          Serial.printf("[LIVE-TRACK] Sesi AKTIF terdeteksi di lastRace: %s\n", lastRace.sessions[i].name);
          return true;
        }
      }
    }
  }

  return false; // Tidak ada sesi yang live saat ini
}