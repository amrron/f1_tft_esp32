/*
 * config.h — Konfigurasi utama (Optimasi URL Latest)
 */

#pragma once

#define AP_SSID_PORTAL   "F1_Display_Setup"
#define AP_PASS_PORTAL   "12345678"

#define WIB_OFFSET_SEC  (7 * 3600)

#define TFT_BLK_PIN   17

// ─────────────────────────────────────────
//  API base — Menggunakan Parameter Latest
// ─────────────────────────────────────────
#define OPENF1_BASE      "https://api.openf1.org/v1"
#define OPENF1_RESULTS   "https://api.openf1.org/v1/session_result?session_key=latest&position%3C=3"
#define OPENF1_STANDINGS "https://api.openf1.org/v1/championship_drivers?session_key=latest&position_current%3C=11"
#define OPENF1_DRIVERS   "https://api.openf1.org/v1/drivers?session_key=latest"
#define OPENF1_MEETING_LATEST  "https://api.openf1.org/v1/meetings?meeting_key=latest" // BARU

#define FETCH_DAYS_BACK    7
#define FETCH_DAYS_FORWARD 45

// ─────────────────────────────────────────
//  Timing
// ─────────────────────────────────────────
#define API_REFRESH_MS           (60UL * 60 * 1000)  // Refresh tiap 1 jam
#define RENDER_INTERVAL_MS       (30UL * 1000)       
#define MODE_SWITCH_MS           (15UL * 1000)       // Ganti mode tiap 15 detik
#define ANIM_FRAME_MS            20                  
#define ANIM_STEPS               12                  

// ─────────────────────────────────────────
//  F1 Warna tema
// ─────────────────────────────────────────
#define COLOR_BG        0x1082
#define COLOR_BG2       0x2104
#define COLOR_RED       0xF800
#define COLOR_WHITE     0xFFFF
#define COLOR_GREY      0x7BEF
#define COLOR_DARK_GREY 0x39E7
#define COLOR_YELLOW    0xFFE0
#define COLOR_GREEN     0x07E0
#define COLOR_CYAN      0x07FF
#define COLOR_ORANGE    0xFD20

// ─────────────────────────────────────────
//  Struct data
// ─────────────────────────────────────────
struct SessionInfo {
  char   name[20];    
  char   dayStr[4];   
  char   timeStr[6];  
  int    sessKey;     
  time_t startEpoch;
  time_t endEpoch;
  bool   valid;
};

struct RaceData {
  char        raceName[40];
  char        location[30];
  char        countryCode[4];
  char        dateRange[16];
  char        circuitImageUrl[120];
  SessionInfo sessions[5];
  int         sessionCount;
  int         raceSessionKey;
  time_t      raceEndEpoch;
  time_t      raceStartEpoch;
  int         meetingKey;
};

struct ResultData {
  int   position;
  char  driverName[30];
  char  driverAbbr[4];
  char  teamName[30];
  char  teamColor[8];
  float gap;
  char  gapStr[15];
  bool  valid;
};

struct StandingsData {
  int  driverNumber;
  int  points;
  char driverAbbr[4];
  bool valid;
};

static const char* DAY_NAMES[]   = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
static const char* MONTH_NAMES[] = {"Jan","Feb","Mar","Apr","May","Jun",
                                     "Jul","Aug","Sep","Oct","Nov","Dec"};