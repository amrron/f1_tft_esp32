/*
 * api.h — Fetch & parse OpenF1 API (Ultra Optimized via Latest Parameter)
 */

#pragma once
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <string.h>
#include <time.h>
#include "config.h"

// Struct internal ringkas untuk menampung cache driver grid terbaru di RAM lokal fungsi
struct _DriverCache {
  int driverNumber;
  char fullName[30];
  char acronym[4];
  char teamColor[8];
};

static void epochToDateParam(time_t epoch, char* buf, size_t len) {
  struct tm* t = gmtime(&epoch);
  snprintf(buf, len, "%04d-%02d-%02d", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday);
}

static time_t parseIso8601(const char* s) {
  if (!s || strlen(s) < 19) return 0;
  struct tm t = {};
  int tzH = 0, tzM = 0;
  char tzSign = '+';
  sscanf(s, "%4d-%2d-%2dT%2d:%2d:%2d%c%2d:%2d",
    &t.tm_year, &t.tm_mon, &t.tm_mday, &t.tm_hour, &t.tm_min, &t.tm_sec, &tzSign, &tzH, &tzM);
  t.tm_year -= 1900; t.tm_mon -= 1; t.tm_isdst = 0;
  time_t epoch = mktime(&t);
  int offsetSec = tzH * 3600 + tzM * 60;
  if (tzSign == '+') epoch -= offsetSec;
  else               epoch += offsetSec;
  return epoch;
}

static void epochToTimeStr(time_t epoch, char* buf, size_t len) {
  time_t local = epoch + WIB_OFFSET_SEC;
  struct tm* t = gmtime(&local);
  snprintf(buf, len, "%02d:%02d", t->tm_hour, t->tm_min);
}

static void epochToDayStr(time_t epoch, char* buf, size_t len) {
  time_t local = epoch + WIB_OFFSET_SEC;
  struct tm* t = gmtime(&local);
  strncpy(buf, DAY_NAMES[t->tm_wday], len);
}

static void formatDateRange(time_t start, time_t end, char* buf, size_t len) {
  time_t ls = start + WIB_OFFSET_SEC; time_t le = end + WIB_OFFSET_SEC;
  struct tm* ts = gmtime(&ls); int dayS = ts->tm_mday;
  struct tm* te = gmtime(&le); int dayE = te->tm_mday; int mon = te->tm_mon;
  snprintf(buf, len, "%02d-%02d %s", dayS, dayE, MONTH_NAMES[mon]);
}

static const char* friendlyName(const char* n) {
  if (strstr(n, "Practice 1"))        return "FP1";
  if (strstr(n, "Practice 2"))        return "FP2";
  if (strstr(n, "Practice 3"))        return "FP3";
  if (strstr(n, "Sprint Qualifying")) return "SQ";
  if (strstr(n, "Sprint"))            return "Sprint";
  if (strstr(n, "Qualifying"))        return "Quali";
  if (strstr(n, "Race"))              return "Race";
  return n;
}

struct _Meet { int key; char name[36]; char location[24]; char cc[4]; time_t dateStart; time_t dateEnd; };
struct _Sess { int meetKey; int sessKey; char name[24]; time_t start; time_t end; bool cancelled; };

#define MAX_M 6
#define MAX_S 30

static DeserializationError httpGetJson(const char* url, const JsonDocument& filter, JsonDocument& doc) {
  HTTPClient http;
  http.begin(url);
  http.setTimeout(12000);
  http.addHeader("User-Agent", "ESP32-F1Display/1.0");
  int code = http.GET();
  if (code != HTTP_CODE_OK) { http.end(); return DeserializationError::NoMemory; }
  WiFiClient* stream = http.getStreamPtr();
  DeserializationError err = deserializeJson(doc, *stream, DeserializationOption::Filter(filter));
  http.end();
  return err;
}

static int fetchMeetings(_Meet* meets, time_t fromEpoch, time_t toEpoch) {
  char fromStr[12], toStr[12];
  epochToDateParam(fromEpoch, fromStr, sizeof(fromStr));
  epochToDateParam(toEpoch,   toStr,   sizeof(toStr));
  char url[200];
  snprintf(url, sizeof(url), "%s/meetings?year=2026&date_start%%3E=%s&date_end%%3C=%s", OPENF1_BASE, fromStr, toStr);

  StaticJsonDocument<256> filter; 
  filter[0]["meeting_key"] = true; filter[0]["meeting_name"] = true; filter[0]["location"] = true;
  filter[0]["country_code"] = true; filter[0]["date_start"] = true; filter[0]["date_end"] = true; filter[0]["is_cancelled"] = true;

  DynamicJsonDocument doc(3072);
  if (httpGetJson(url, filter, doc)) return 0;
  int n = 0;
  for (JsonVariant m : doc.as<JsonArray>()) {
    if (n >= MAX_M) break;
    if (m["is_cancelled"].as<bool>()) continue;
    _Meet& mi = meets[n];
    mi.key = m["meeting_key"].as<int>();
    strlcpy(mi.name, m["meeting_name"] | "", sizeof(mi.name));
    strlcpy(mi.location, m["location"] | "", sizeof(mi.location));
    strlcpy(mi.cc, m["country_code"] | "", sizeof(mi.cc));
    mi.dateStart = parseIso8601(m["date_start"] | "");
    mi.dateEnd = parseIso8601(m["date_end"] | "");
    n++;
  }
  return n;
}

static int fetchSessions(_Sess* sess, time_t fromEpoch, time_t toEpoch) {
  char fromStr[12], toStr[12];
  epochToDateParam(fromEpoch, fromStr, sizeof(fromStr));
  epochToDateParam(toEpoch,   toStr,   sizeof(toStr));
  char url[200];
  snprintf(url, sizeof(url), "%s/sessions?year=2026&date_start%%3E=%s&date_end%%3C=%s", OPENF1_BASE, fromStr, toStr);

  StaticJsonDocument<192> filter;
  filter[0]["meeting_key"] = true; filter[0]["session_key"] = true; filter[0]["session_name"] = true;
  filter[0]["date_start"] = true; filter[0]["date_end"] = true; filter[0]["is_cancelled"] = true;

  DynamicJsonDocument doc(5120);
  if (httpGetJson(url, filter, doc)) return 0;
  int n = 0;
  for (JsonVariant s : doc.as<JsonArray>()) {
    if (n >= MAX_S) break;
    _Sess& se = sess[n];
    se.meetKey = s["meeting_key"].as<int>();   se.sessKey = s["session_key"].as<int>();
    se.cancelled = s["is_cancelled"].as<bool>(); strlcpy(se.name, s["session_name"] | "", sizeof(se.name));
    se.start = parseIso8601(s["date_start"] | ""); se.end = parseIso8601(s["date_end"] | "");
    n++;
  }
  return n;
}

static void buildRaceData(RaceData& rd, const _Meet& mi, const _Sess* sess, int sCount) {
  strlcpy(rd.raceName, mi.name, sizeof(rd.raceName));
  strlcpy(rd.location, mi.location, sizeof(rd.location));
  strlcpy(rd.countryCode, mi.cc, sizeof(rd.countryCode));
  rd.circuitImageUrl[0] = '\0'; rd.meetingKey = mi.key; rd.sessionCount = 0; rd.raceSessionKey = 0; rd.raceEndEpoch = 0; rd.raceStartEpoch = 0;
  formatDateRange(mi.dateStart, mi.dateEnd, rd.dateRange, sizeof(rd.dateRange));

  for (int j = 0; j < sCount && rd.sessionCount < 5; j++) {
    const _Sess& se = sess[j];
    if (se.meetKey != mi.key || se.cancelled || strncmp(se.name, "Day", 3) == 0) continue;
    SessionInfo& si = rd.sessions[rd.sessionCount];
    strlcpy(si.name, friendlyName(se.name), sizeof(si.name));
    epochToDayStr(se.start, si.dayStr, sizeof(si.dayStr)); epochToTimeStr(se.start, si.timeStr, sizeof(si.timeStr));
    si.sessKey = se.sessKey; si.startEpoch = se.start; si.endEpoch = se.end; si.valid = true;
    if (strcmp(se.name, "Race") == 0) { rd.raceSessionKey = se.sessKey; rd.raceEndEpoch = se.end; rd.raceStartEpoch = se.start; }
    rd.sessionCount++;
  }
}

bool fetchRaceData(time_t nowEpoch, RaceData& nextRace, RaceData& lastRace, bool& hasNext, bool& hasLast) {
  hasNext = false; hasLast = false;
  time_t fromEpoch = nowEpoch - (time_t)FETCH_DAYS_BACK * 86400; time_t toEpoch = nowEpoch + (time_t)FETCH_DAYS_FORWARD * 86400;
  _Meet* meets = (_Meet*)malloc(MAX_M * sizeof(_Meet)); _Sess* sess = (_Sess*)malloc(MAX_S * sizeof(_Sess));
  if (!meets || !sess) { free(meets); free(sess); return false; }

  int mCount = fetchMeetings(meets, fromEpoch, toEpoch);
  int sCount = (mCount > 0) ? fetchSessions(sess, fromEpoch, toEpoch) : 0;
  if (sCount == 0) { free(meets); free(sess); return false; }

  int nextIdx = -1, lastIdx = -1; time_t nextStart = 0, lastEnd = 0;
  for (int i = 0; i < mCount; i++) {
    time_t rStart = 0, rEnd = 0;
    for (int j = 0; j < sCount; j++) {
      if (sess[j].meetKey == meets[i].key && !sess[j].cancelled && strcmp(sess[j].name, "Race") == 0) {
        rStart = sess[j].start; rEnd = sess[j].end; break;
      }
    }
    if (rStart == 0) continue;
    if (rEnd <= nowEpoch) { if (rEnd > lastEnd) { lastEnd = rEnd; lastIdx = i; } }
    else { if (nextStart == 0 || rStart < nextStart) { nextStart = rStart; nextIdx = i; } }
  }
  if (nextIdx >= 0) { buildRaceData(nextRace, meets[nextIdx], sess, sCount); hasNext = true; }
  if (lastIdx >= 0) { buildRaceData(lastRace, meets[lastIdx], sess, sCount); hasLast = true; }
  free(meets); free(sess);
  return (hasNext || hasLast);
}

int findActiveOrJustFinishedSession(const RaceData& rd, time_t nowEpoch) {
  int bestIdx = -1; time_t bestEnd = 0;
  for (int i = 0; i < rd.sessionCount; i++) {
    const SessionInfo& si = rd.sessions[i];
    if (si.valid && si.endEpoch != 0 && si.endEpoch <= nowEpoch && si.endEpoch > bestEnd) {
      bestEnd = si.endEpoch; bestIdx = i;
    }
  }
  return bestIdx;
}

// ─────────────────────────────────────────
//  FUNGSI BARU: Pre-load Driver Grid Terbaru (Hanya 1x HTTP Request)
// ─────────────────────────────────────────
static int loadDriverCache(_DriverCache* cache, int maxCache) {
  StaticJsonDocument<192> filter;
  filter[0]["driver_number"] = true; filter[0]["full_name"] = true;
  filter[0]["name_acronym"] = true;  filter[0]["team_colour"] = true;

  DynamicJsonDocument doc(6144); // Cukup untuk menampung satu grid pembalap aktif terbaru
  if (httpGetJson(OPENF1_DRIVERS, filter, doc)) return 0;

  JsonArray arr = doc.as<JsonArray>();
  int count = 0;
  for (JsonVariant d : arr) {
    if (count >= maxCache) break;
    
    // Cegah duplikasi token data driver dari API
    int dNum = d["driver_number"].as<int>();
    bool isDup = false;
    for(int c=0; c<count; c++) { if(cache[c].driverNumber == dNum) { isDup = true; break; } }
    if(isDup) continue;

    _DriverCache& dc = cache[count];
    dc.driverNumber = dNum;
    strlcpy(dc.fullName, d["full_name"] | "---", sizeof(dc.fullName));
    strlcpy(dc.acronym,  d["name_acronym"] | "---", sizeof(dc.acronym));
    
    const char* rawColor = d["team_colour"] | "FFFFFF";
    if (rawColor[0] == '#') strlcpy(dc.teamColor, rawColor, sizeof(dc.teamColor));
    else snprintf(dc.teamColor, sizeof(dc.teamColor), "#%s", rawColor);
    
    count++;
  }
  Serial.printf("[CACHE] Berhasil memuat %d data driver unik\n", count);
  return count;
}

// ─────────────────────────────────────────
//  Fetch Session Results — Menggunakan Data Cache RAM
// ─────────────────────────────────────────
bool fetchSessionResults(ResultData results[3], const _DriverCache* cache, int cacheCount) {
  for (int i = 0; i < 3; i++) results[i].valid = false;

  StaticJsonDocument<128> filter;
  filter[0]["position"] = true; filter[0]["driver_number"] = true; filter[0]["gap_to_leader"] = true;

  DynamicJsonDocument doc(1536);
  if (httpGetJson(OPENF1_RESULTS, filter, doc)) return false;

  JsonArray arr = doc.as<JsonArray>();
  if (arr.size() == 0) return false;

  int count = 0;
  for (JsonVariant r : arr) {
    if (count >= 3) break;
    int pos = r["position"].as<int>();
    if (pos > 3) continue;

    ResultData& rd = results[count];
    rd.position = pos;
    rd.valid = true;
    int driverNum = r["driver_number"].as<int>();
    
    // Set parameter fallback standar
    snprintf(rd.driverAbbr, sizeof(rd.driverAbbr), "%d", driverNum);
    snprintf(rd.driverName, sizeof(rd.driverName), "Driver %d", driverNum);
    strlcpy(rd.teamColor, "#FFFFFF", sizeof(rd.teamColor));

    // Ambil data langsung dari cache lokal RAM tanpa tembak HTTP Request lagi!
    for (int i = 0; i < cacheCount; i++) {
      if (cache[i].driverNumber == driverNum) {
        strlcpy(rd.driverName, cache[i].fullName, sizeof(rd.driverName));
        strlcpy(rd.driverAbbr, cache[i].acronym,  sizeof(rd.driverAbbr));
        strlcpy(rd.teamColor,  cache[i].teamColor, sizeof(rd.teamColor));
        break;
      }
    }

    float gap = r["gap_to_leader"] | 0.0f; rd.gap = gap;
    if (rd.position == 1) strlcpy(rd.gapStr, "WINNER", sizeof(rd.gapStr));
    else snprintf(rd.gapStr, sizeof(rd.gapStr), "+%.3f", gap);
    count++;
  }
  return count > 0;
}

// ─────────────────────────────────────────
//  Fetch Driver Standings — Menggunakan Data Cache RAM
// ─────────────────────────────────────────
bool fetchDriverStandings(StandingsData standings[10], const _DriverCache* cache, int cacheCount) {
  for (int i = 0; i < 10; i++) standings[i].valid = false;

  StaticJsonDocument<144> filter;
  filter[0]["driver_number"] = true; filter[0]["points_current"] = true;

  DynamicJsonDocument doc(3072);
  if (httpGetJson(OPENF1_STANDINGS, filter, doc)) return false;

  JsonArray arr = doc.as<JsonArray>();
  int rawCount = arr.size();
  if (rawCount == 0) return false;

  struct TempDriver { int driverNumber; int points; };
  TempDriver* tempList = (TempDriver*)malloc(rawCount * sizeof(TempDriver));
  if (!tempList) return false;

  for (int i = 0; i < rawCount; i++) {
    tempList[i].driverNumber = arr[i]["driver_number"].as<int>();
    tempList[i].points = arr[i]["points_current"].as<int>();
  }

  // Sortir manual point_current terbesar-terkecil (Mengatasi bugs sorting internal API Beta)
  for (int i = 0; i < rawCount - 1; i++) {
    for (int j = 0; j < rawCount - i - 1; j++) {
      if (tempList[j].points < tempList[j+1].points) {
        TempDriver temp = tempList[j]; tempList[j] = tempList[j+1]; tempList[j+1] = temp;
      }
    }
  }

  int count = 0;
  for (int i = 0; i < rawCount && count < 10; i++) {
    bool duplicate = false;
    for(int d=0; d<count; d++) { if(standings[d].driverNumber == tempList[i].driverNumber) { duplicate = true; break; } }
    if(duplicate) continue;

    StandingsData& sd = standings[count];
    sd.driverNumber = tempList[i].driverNumber;
    sd.points = tempList[i].points;
    sd.valid = true;
    snprintf(sd.driverAbbr, sizeof(sd.driverAbbr), "%d", sd.driverNumber);

    // Ambil inisial nama driver langsung dari cache RAM lokal
    for (int c = 0; c < cacheCount; c++) {
      if (cache[c].driverNumber == sd.driverNumber) {
        strlcpy(sd.driverAbbr, cache[c].acronym, sizeof(sd.driverAbbr));
        break;
      }
    }
    count++;
  }
  free(tempList);
  return count > 0;
}

// ─────────────────────────────────────────
//  FUNGSI BARU: Fetch Spesifik Data Event Meeting Terbaru
// ─────────────────────────────────────────
bool fetchLatestMeeting(RaceData& latestRace) {
  StaticJsonDocument<256> filter; 
  filter[0]["meeting_key"]  = true;
  filter[0]["meeting_name"] = true;
  filter[0]["location"]     = true;
  filter[0]["country_code"] = true;
  filter[0]["date_start"]   = true;
  filter[0]["date_end"]     = true;

  DynamicJsonDocument doc(2048);
  if (httpGetJson(OPENF1_MEETING_LATEST, filter, doc)) return false;

  JsonArray arr = doc.as<JsonArray>();
  if (arr.size() == 0) return false;

  JsonVariant m = arr[0];
  latestRace.meetingKey = m["meeting_key"].as<int>();
  strlcpy(latestRace.raceName, m["meeting_name"] | "", sizeof(latestRace.raceName));
  strlcpy(latestRace.location, m["location"]     | "", sizeof(latestRace.location));
  strlcpy(latestRace.countryCode, m["country_code"] | "", sizeof(latestRace.countryCode));
  latestRace.sessionCount = 0; // Tidak perlu memuat daftar sub-sesi untuk efisiensi RAM

  time_t start = parseIso8601(m["date_start"] | "");
  time_t end   = parseIso8601(m["date_end"]   | "");
  formatDateRange(start, end, latestRace.dateRange, sizeof(latestRace.dateRange));

  Serial.printf("[API] Sukses Memuat Nama Meeting Terbaru: %s\n", latestRace.raceName);
  return true;
}