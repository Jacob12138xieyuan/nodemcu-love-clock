#pragma once
// 配置的数据结构，以及 LittleFS 上 config.json / wifi.json 的读写
#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>   // 需要 6.x 版本

#define MAX_EVENTS 10
#define MAX_PHOTOS 50

struct EventItem { char name[32]; uint8_t m, d; bool celebrate; };
struct PhotoItem { char file[16]; char cap[46]; };

struct Config {
  uint16_t sy = 0; uint8_t sm = 0, sd = 0;      // 在一起的日期，sy==0 表示未设置
  char  city[24] = "";
  float lat = 31.23f, lon = 121.47f;             // 天气用的经纬度
  uint8_t slideSec = 8;                          // 每页停留秒数
  uint8_t nightStart = 22, nightEnd = 7;         // 夜间时段
  uint8_t bright = 100, nightBright = 10;        // 亮度百分比
  uint8_t lang = 0;                              // 0=中文 1=English，屏幕和网页共用
  char  pass[16] = "";                           // 设置页密码，空表示不设
  char  testDate[11] = "";                       // 测试用：假装今天是这一天
  EventItem ev[MAX_EVENTS]; uint8_t nEv = 0;
  PhotoItem ph[MAX_PHOTOS]; uint8_t nPh = 0;
};

Config cfg;
// 最近使用的 Wi-Fi，最多 3 个，下标 0 是最近一次
#define MAX_WIFI 3
struct WifiEntry { char s[33]; char p[65]; };
WifiEntry wifis[MAX_WIFI];
uint8_t nWifi = 0;

// 界面语言：TR(中文, English) 按当前语言选择文字
#define EN() (cfg.lang == 1)
#define TR(zh, en) (cfg.lang == 1 ? (en) : (zh))

// 其他模块使用的标志
volatile bool gConfigChanged = false;   // 设置被保存过
volatile bool gQuotesChanged = false;   // 情话被保存过
volatile bool gUploading = false;       // 正在上传照片
uint32_t gRestartAt = 0;                // 非 0 时到点重启

// 按字节安全复制，不会把一个汉字截成两半
void safeCopy(char* dst, size_t n, const char* src) {
  if (!src) { dst[0] = 0; return; }
  size_t len = strlen(src);
  if (len >= n) {
    len = n - 1;
    while (len > 0 && (((uint8_t)src[len]) & 0xC0) == 0x80) len--;
  }
  memcpy(dst, src, len);
  dst[len] = 0;
}

bool parseDate(const char* s, int& y, int& m, int& d) {
  if (!s || !*s) return false;
  if (sscanf(s, "%d-%d-%d", &y, &m, &d) != 3) return false;
  return y >= 1900 && y <= 2200 && m >= 1 && m <= 12 && d >= 1 && d <= 31;
}

// 照片文件名只允许 p + 字母数字 + .jpg，防止路径穿越
bool validPhotoName(const char* s) {
  if (!s) return false;
  size_t n = strlen(s);
  if (n < 6 || n > 15 || s[0] != 'p') return false;
  if (strcmp(s + n - 4, ".jpg") != 0) return false;
  for (size_t i = 0; i < n - 4; i++) if (!isalnum((uint8_t)s[i])) return false;
  return true;
}

void applyJson(JsonObject o) {
  if (o.containsKey("start")) {
    int y, m, d;
    if (parseDate(o["start"] | "", y, m, d)) { cfg.sy = y; cfg.sm = m; cfg.sd = d; }
    else { cfg.sy = 0; cfg.sm = 0; cfg.sd = 0; }
  }
  if (o.containsKey("city")) safeCopy(cfg.city, sizeof cfg.city, o["city"] | "");
  if (o.containsKey("lat")) cfg.lat = o["lat"] | cfg.lat;
  if (o.containsKey("lon")) cfg.lon = o["lon"] | cfg.lon;
  if (o.containsKey("slide")) cfg.slideSec = constrain((int)(o["slide"] | 8), 3, 60);
  if (o.containsKey("nightStart")) cfg.nightStart = constrain((int)(o["nightStart"] | 22), 0, 23);
  if (o.containsKey("nightEnd")) cfg.nightEnd = constrain((int)(o["nightEnd"] | 7), 0, 23);
  if (o.containsKey("bright")) cfg.bright = constrain((int)(o["bright"] | 100), 5, 100);
  if (o.containsKey("nightBright")) cfg.nightBright = constrain((int)(o["nightBright"] | 10), 0, 100);
  if (o.containsKey("lang")) cfg.lang = constrain((int)(o["lang"] | 0), 0, 1);
  if (o.containsKey("pass")) safeCopy(cfg.pass, sizeof cfg.pass, o["pass"] | "");
  if (o.containsKey("newPass")) safeCopy(cfg.pass, sizeof cfg.pass, o["newPass"] | "");
  if (o.containsKey("testDate")) {
    int y, m, d;
    const char* t = o["testDate"] | "";
    if (parseDate(t, y, m, d)) safeCopy(cfg.testDate, sizeof cfg.testDate, t);
    else cfg.testDate[0] = 0;
  }
  if (o.containsKey("events")) {
    cfg.nEv = 0;
    for (JsonObject e : o["events"].as<JsonArray>()) {
      if (cfg.nEv >= MAX_EVENTS) break;
      EventItem& x = cfg.ev[cfg.nEv];
      safeCopy(x.name, sizeof x.name, e["n"] | "");
      x.m = constrain((int)(e["m"] | 1), 1, 12);
      x.d = constrain((int)(e["d"] | 1), 1, 31);
      x.celebrate = e["c"] | true;
      cfg.nEv++;
    }
  }
  if (o.containsKey("photos")) {
    cfg.nPh = 0;
    for (JsonObject p : o["photos"].as<JsonArray>()) {
      if (cfg.nPh >= MAX_PHOTOS) break;
      const char* f = p["f"] | "";
      if (!validPhotoName(f)) continue;
      PhotoItem& x = cfg.ph[cfg.nPh];
      safeCopy(x.file, sizeof x.file, f);
      safeCopy(x.cap, sizeof x.cap, p["c"] | "");
      cfg.nPh++;
    }
  }
}

// forWeb=true 时不返回密码本身，只告诉网页"有没有设密码"
void buildJson(DynamicJsonDocument& d, bool forWeb) {
  char buf[12];
  if (cfg.sy) snprintf(buf, sizeof buf, "%04u-%02u-%02u", cfg.sy, cfg.sm, cfg.sd);
  else buf[0] = 0;
  d["start"] = buf;
  d["city"] = cfg.city;
  d["lat"] = cfg.lat;
  d["lon"] = cfg.lon;
  d["slide"] = cfg.slideSec;
  d["nightStart"] = cfg.nightStart;
  d["nightEnd"] = cfg.nightEnd;
  d["bright"] = cfg.bright;
  d["nightBright"] = cfg.nightBright;
  d["testDate"] = cfg.testDate;
  d["lang"] = cfg.lang;
  if (forWeb) d["hasPass"] = cfg.pass[0] != 0;
  else d["pass"] = cfg.pass;
  JsonArray ea = d.createNestedArray("events");
  for (uint8_t i = 0; i < cfg.nEv; i++) {
    JsonObject e = ea.createNestedObject();
    e["n"] = cfg.ev[i].name; e["m"] = cfg.ev[i].m; e["d"] = cfg.ev[i].d; e["c"] = cfg.ev[i].celebrate;
  }
  JsonArray pa = d.createNestedArray("photos");
  for (uint8_t i = 0; i < cfg.nPh; i++) {
    JsonObject p = pa.createNestedObject();
    p["f"] = cfg.ph[i].file; p["c"] = cfg.ph[i].cap;
  }
}

void loadConfig() {
  File f = LittleFS.open("/config.json", "r");
  if (!f) return;
  DynamicJsonDocument d(8192);
  if (deserializeJson(d, f) == DeserializationError::Ok) applyJson(d.as<JsonObject>());
  f.close();
}

bool saveConfig() {
  DynamicJsonDocument d(8192);
  buildJson(d, false);
  File f = LittleFS.open("/config.json", "w");
  if (!f) return false;
  serializeJson(d, f);
  f.close();
  return true;
}

void loadWifi() {
  nWifi = 0;
  File f = LittleFS.open("/wifi.json", "r");
  if (!f) return;
  DynamicJsonDocument d(1536);
  if (deserializeJson(d, f) == DeserializationError::Ok) {
    if (d.is<JsonArray>()) {
      for (JsonObject o : d.as<JsonArray>()) {
        if (nWifi >= MAX_WIFI) break;
        const char* s = o["s"] | "";
        if (!*s) continue;
        safeCopy(wifis[nWifi].s, sizeof wifis[nWifi].s, s);
        safeCopy(wifis[nWifi].p, sizeof wifis[nWifi].p, o["p"] | "");
        nWifi++;
      }
    } else {   // 旧版本只存一组：{"s":..,"p":..}
      const char* s = d["s"] | "";
      if (*s) {
        safeCopy(wifis[0].s, sizeof wifis[0].s, s);
        safeCopy(wifis[0].p, sizeof wifis[0].p, d["p"] | "");
        nWifi = 1;
      }
    }
  }
  f.close();
}

void saveWifi() {
  if (!nWifi) { LittleFS.remove("/wifi.json"); return; }
  DynamicJsonDocument d(1536);
  JsonArray a = d.to<JsonArray>();
  for (uint8_t i = 0; i < nWifi; i++) {
    JsonObject o = a.createNestedObject();
    o["s"] = wifis[i].s; o["p"] = wifis[i].p;
  }
  File f = LittleFS.open("/wifi.json", "w");
  if (!f) return;
  serializeJson(d, f);
  f.close();
}

// 新增或更新一个 Wi-Fi，放到最前面；超过 3 个时淘汰最旧的
void addWifi(const char* s, const char* p) {
  int at = -1;
  for (uint8_t i = 0; i < nWifi; i++) if (!strcmp(wifis[i].s, s)) { at = i; break; }
  if (at < 0) { if (nWifi < MAX_WIFI) nWifi++; at = nWifi - 1; }
  for (int i = at; i > 0; i--) wifis[i] = wifis[i - 1];
  safeCopy(wifis[0].s, sizeof wifis[0].s, s);
  safeCopy(wifis[0].p, sizeof wifis[0].p, p);
  saveWifi();
}

void removeWifi(uint8_t i) {
  if (i >= nWifi) return;
  for (uint8_t k = i; k + 1 < nWifi; k++) wifis[k] = wifis[k + 1];
  nWifi--;
  saveWifi();
}

// ---- 连按两次 RST 进入配网 ----
// 开机时在 RTC 内存里写标记，3 秒内没再次复位就清掉；再次复位时发现标记还在，就是连按
#define DR_MAGIC 0xD0B1E5E7UL
bool gDoubleReset = false, drArmed = false;
void drArm() {
  uint32_t m = 0;
  ESP.rtcUserMemoryRead(0, &m, sizeof m);
  gDoubleReset = (m == DR_MAGIC) && ESP.getResetInfoPtr()->reason == REASON_EXT_SYS_RST;
  m = DR_MAGIC;
  ESP.rtcUserMemoryWrite(0, &m, sizeof m);
  drArmed = true;
}
void drTick() {
  if (drArmed && millis() > 3000) {
    uint32_t m = 0;
    ESP.rtcUserMemoryWrite(0, &m, sizeof m);
    drArmed = false;
  }
}
