#pragma once
// Wi-Fi 管理（热点配网 + 连接路由器）和设置网页的后端接口
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <DNSServer.h>
#include "display.h"
#include "web_page.h"

#define AP_NAME "LoveClock-Setup"
#define MDNS_NAME "love-clock"

ESP8266WebServer server(80);
DNSServer dns;
bool apActive = false;
uint32_t staUpSince = 0, staDownSince = 0, infoUntil = 0;
extern int quoteN;

IPAddress AP_IP(192, 168, 4, 1);

// ---------- 热点 ----------
void startAP() {
  if (apActive) return;
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(AP_IP, AP_IP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(AP_NAME);
  dns.start(53, "*", AP_IP);
  apActive = true;
}
void stopAP() {
  if (!apActive) return;
  dns.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  apActive = false;
}

// ---------- 鉴权 ----------
bool authed() {
  if (!cfg.pass[0]) return true;
  return server.header("X-Pass") == String(cfg.pass);
}
#define REQUIRE_AUTH do { if (!authed()) { server.send(401, "text/plain", "auth"); return; } } while (0)

// ---------- 接口 ----------
void hRoot() { server.send_P(200, "text/html; charset=utf-8", INDEX_HTML); }

void hStatus() {
  StaticJsonDocument<384> d;
  bool c = WiFi.status() == WL_CONNECTED;
  d["sta"] = c;
  d["ssid"] = c ? WiFi.SSID() : String("");
  d["ip"] = c ? WiFi.localIP().toString() : String("");
  d["ap"] = apActive;
  d["auth"] = cfg.pass[0] != 0;
  FSInfo fi; LittleFS.info(fi);
  d["free"] = (uint32_t)(fi.totalBytes - fi.usedBytes);
  d["total"] = (uint32_t)fi.totalBytes;
  if (timeValid()) {
    time_t n = time(nullptr); tm t; localtime_r(&n, &t);
    char b[24]; snprintf(b, sizeof b, "%04d-%02d-%02d %02d:%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min);
    d["time"] = b;
  } else d["time"] = "";
  String out; serializeJson(d, out);
  server.send(200, "application/json", out);
}

void hGetConfig() {
  REQUIRE_AUTH;
  String out;
  {
    DynamicJsonDocument d(8192);
    buildJson(d, true);
    out.reserve(measureJson(d) + 1);
    serializeJson(d, out);
  }
  server.send(200, "application/json", out);
}

void hPostConfig() {
  REQUIRE_AUTH;
  if (!server.hasArg("plain")) { server.send(400, "text/plain", "no body"); return; }
  if (server.arg("plain").length() > 7000) { server.send(413, "text/plain", "too large"); return; }
  {
    DynamicJsonDocument d(8192);
    if (deserializeJson(d, server.arg("plain")) != DeserializationError::Ok) { server.send(400, "text/plain", "bad json"); return; }
    applyJson(d.as<JsonObject>());
  }
  bool ok = saveConfig();
  gConfigChanged = true;
  server.send(ok ? 200 : 500, "application/json", ok ? "{\"ok\":true}" : "{\"ok\":false}");
}

void hGetQuotes() {
  REQUIRE_AUTH;
  File f = LittleFS.open("/quotes.txt", "r");
  if (!f) { server.send(200, "text/plain; charset=utf-8", ""); return; }
  server.streamFile(f, "text/plain; charset=utf-8");
  f.close();
}
void hPostQuotes() {
  REQUIRE_AUTH;
  String body = server.arg("plain");
  if (body.length() > 6000) { server.send(413, "text/plain", "too large"); return; }
  File f = LittleFS.open("/quotes.txt", "w");
  if (!f) { server.send(500, "text/plain", "fs"); return; }
  f.print(body);
  f.close();
  quoteN = -1;
  gQuotesChanged = true; gConfigChanged = true;
  server.send(200, "application/json", "{\"ok\":true}");
}

// ---- 照片 ----
File upFile; bool upOk = false;
void hUpload() {
  HTTPUpload& u = server.upload();
  if (u.status == UPLOAD_FILE_START) {
    gUploading = true; upOk = false;
    if (authed() && validPhotoName(u.filename.c_str())) {
      upFile = LittleFS.open("/" + u.filename, "w");
      upOk = (bool)upFile;
    }
  } else if (u.status == UPLOAD_FILE_WRITE) {
    if (upOk && upFile) { if (upFile.write(u.buf, u.currentSize) != u.currentSize) upOk = false; }
  } else if (u.status == UPLOAD_FILE_END) {
    if (upFile) upFile.close();
    if (!upOk) LittleFS.remove("/" + u.filename);
    gUploading = false;
  } else if (u.status == UPLOAD_FILE_ABORTED) {
    if (upFile) upFile.close();
    LittleFS.remove("/" + u.filename);
    upOk = false; gUploading = false;
  }
}
void hUploadDone() {
  server.send(upOk ? 200 : 500, "application/json", upOk ? "{\"ok\":true}" : "{\"ok\":false}");
}
void hPhotoDelete() {
  REQUIRE_AUTH;
  String f = server.arg("f");
  if (!validPhotoName(f.c_str())) { server.send(400, "text/plain", "bad name"); return; }
  LittleFS.remove("/" + f);
  server.send(200, "application/json", "{\"ok\":true}");
}
void hPhotoGet() {
  String f = server.arg("f");
  if (!validPhotoName(f.c_str())) { server.send(400, "text/plain", "bad name"); return; }
  File file = LittleFS.open("/" + f, "r");
  if (!file) { server.send(404, "text/plain", "none"); return; }
  server.sendHeader("Cache-Control", "max-age=86400");
  server.streamFile(file, "image/jpeg");
  file.close();
}

// ---- Wi-Fi ----
void hWifiScan() {
  REQUIRE_AUTH;
  int n = WiFi.scanComplete();
  if (n == WIFI_SCAN_FAILED) {
    WiFi.scanNetworks(true, false);
    server.send(200, "application/json", "{\"scanning\":true}");
    return;
  }
  if (n == WIFI_SCAN_RUNNING) { server.send(200, "application/json", "{\"scanning\":true}"); return; }
  DynamicJsonDocument d(2048);
  JsonArray a = d.createNestedArray("list");
  for (int i = 0; i < n && i < 15; i++) {
    if (WiFi.SSID(i).length() == 0) continue;
    JsonObject o = a.createNestedObject();
    o["s"] = WiFi.SSID(i);
    o["r"] = WiFi.RSSI(i);
    o["e"] = WiFi.encryptionType(i) != ENC_TYPE_NONE;
  }
  WiFi.scanDelete();
  String out; serializeJson(d, out);
  server.send(200, "application/json", out);
}
void hWifiSet() {
  REQUIRE_AUTH;
  StaticJsonDocument<256> d;
  if (deserializeJson(d, server.arg("plain")) != DeserializationError::Ok) { server.send(400, "text/plain", "bad json"); return; }
  const char* s = d["s"] | "";
  if (!*s) { server.send(400, "text/plain", "no ssid"); return; }
  addWifi(s, d["p"] | "");
  WiFi.disconnect();
  WiFi.begin(wifis[0].s, wifis[0].p);
  staDownSince = millis();
  server.send(200, "application/json", "{\"ok\":true}");
}
void hWifiList() {
  REQUIRE_AUTH;
  DynamicJsonDocument d(512);
  JsonArray a = d.createNestedArray("list");
  for (uint8_t i = 0; i < nWifi; i++) a.add(wifis[i].s);
  String out; serializeJson(d, out);
  server.send(200, "application/json", out);
}
void hWifiDel() {
  REQUIRE_AUTH;
  StaticJsonDocument<64> d;
  if (deserializeJson(d, server.arg("plain")) != DeserializationError::Ok) { server.send(400, "text/plain", "bad json"); return; }
  removeWifi((uint8_t)(d["i"] | 255));
  server.send(200, "application/json", "{\"ok\":true}");
}
void hWifiForget() {
  REQUIRE_AUTH;
  nWifi = 0; saveWifi();
  server.send(200, "application/json", "{\"ok\":true}");
  gRestartAt = millis() + 800;
}

void hNotFound() {
  if (apActive) {   // 配网时把所有请求重定向到设置页（强制门户）
    server.sendHeader("Location", "http://192.168.4.1/", true);
    server.send(302, "text/plain", "");
  } else {
    server.send(404, "text/plain", "not found");
  }
}

// ---------- 初始化与循环 ----------
void webInit() {
  server.collectHeaders("X-Pass");
  server.on("/", HTTP_GET, hRoot);
  server.on("/api/status", HTTP_GET, hStatus);
  server.on("/api/config", HTTP_GET, hGetConfig);
  server.on("/api/config", HTTP_POST, hPostConfig);
  server.on("/api/quotes", HTTP_GET, hGetQuotes);
  server.on("/api/quotes", HTTP_POST, hPostQuotes);
  server.on("/api/photo", HTTP_POST, hUploadDone, hUpload);
  server.on("/api/photo/delete", HTTP_POST, hPhotoDelete);
  server.on("/ph", HTTP_GET, hPhotoGet);
  server.on("/api/wifi/scan", HTTP_GET, hWifiScan);
  server.on("/api/wifi", HTTP_POST, hWifiSet);
  server.on("/api/wifi/list", HTTP_GET, hWifiList);
  server.on("/api/wifi/del", HTTP_POST, hWifiDel);
  server.on("/api/wifi/forget", HTTP_POST, hWifiForget);
  server.onNotFound(hNotFound);
  server.begin();
}

// mDNS 只在连上路由器后再启动，只有热点时启动它在部分版本上不稳定
bool mdnsOn = false;

bool forcedSetup = false;   // 连按两次 RST 进入的配网模式

// 先扫描一次，按“最近使用”的顺序依次尝试周围能搜到的已保存 Wi-Fi
bool connectSaved() {
  WiFi.mode(WIFI_STA);
  drawMsg(TR("正在搜索 Wi-Fi", "Searching Wi-Fi..."));
  int found = WiFi.scanNetworks();
  uint8_t order[MAX_WIFI], n = 0;
  for (uint8_t i = 0; i < nWifi; i++)
    for (int k = 0; k < found; k++)
      if (WiFi.SSID(k) == wifis[i].s) { order[n++] = i; break; }
  WiFi.scanDelete();
  if (!n) { order[0] = 0; n = 1; }   // 一个都没搜到（可能是隐藏网络）：只试最近的那个
  for (uint8_t j = 0; j < n; j++) {
    WiFi.begin(wifis[order[j]].s, wifis[order[j]].p);
    drawMsg(TR("正在连接 Wi-Fi", "Connecting Wi-Fi..."), wifis[order[j]].s);
    uint32_t t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < 12000) { delay(200); drTick(); }
    if (WiFi.status() == WL_CONNECTED) return true;
    WiFi.disconnect();
  }
  return false;
}

void netInit() {
  WiFi.persistent(false);
  WiFi.hostname(MDNS_NAME);
  if (gDoubleReset) { forcedSetup = true; startAP(); return; }
  if (!nWifi) { startAP(); return; }
  if (!connectSaved()) startAP();   // 都连不上就开热点，等待重新配置
}

void netLoop() {
  server.handleClient();
  if (apActive) dns.processNextRequest();
  uint32_t now = millis();
  if (mdnsOn) MDNS.update();
  if (forcedSetup && now > 300000 && WiFi.status() != WL_CONNECTED) ESP.restart();   // 配网模式 5 分钟没人操作，自动回到正常模式
  if (WiFi.status() == WL_CONNECTED) {
    if (!mdnsOn) {
      mdnsOn = MDNS.begin(MDNS_NAME);
      if (mdnsOn) MDNS.addService("http", "tcp", 80);
    }
    staDownSince = 0;
    if (!staUpSince) { staUpSince = now ? now : 1; infoUntil = now + 8000; }
    if (apActive && now - staUpSince > 60000) stopAP();   // 连上后留 60 秒给手机看结果，再关热点
  } else {
    staUpSince = 0;
    if (!staDownSince) staDownSince = now ? now : 1;
    if (!apActive && nWifi && now - staDownSince > 120000) startAP();   // 断网超过 2 分钟，重新开热点
  }
}
