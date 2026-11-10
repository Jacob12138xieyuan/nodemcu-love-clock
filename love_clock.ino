// 爱的小时钟：NodeMCU(ESP8266) + 1.3 寸 ST7789 IPS 屏
// 页面轮播：时钟天气 / 在一起天数 / 倒数日 / 照片 / 情话，纪念日当天全屏庆祝
// 所有设置（含 Wi-Fi）都在手机网页里完成，见 README.md
#include <ESP8266WiFi.h>
#include <LittleFS.h>
#include "config_store.h"
#include "display.h"
#include "weather.h"
#include "web_page.h"
#include "web_ui.h"

#define PIN_BTN 4    // GPIO4 = D2，可选：接一个按钮到 GND，长按 5 秒清除 Wi-Fi 和密码

enum Mode : uint8_t { M_NONE, M_SETUP, M_INFO, M_PAGES, M_CELEB };
Mode mode = M_NONE;
bool needFull = true;
uint8_t page = P_CLOCK;
uint32_t lastSwitch = 0, lastBL = 0, wxNextTry = 0, lastEv = 0, lastFrame = 0, btnDown = 0;
bool special = false;

void handleButton() {
  if (digitalRead(PIN_BTN) == LOW) {
    if (!btnDown) btnDown = millis();
    if (millis() - btnDown > 5000) {
      LittleFS.remove("/wifi.json");
      cfg.pass[0] = 0; saveConfig();
      drawMsg("已清除 Wi-Fi 和密码", "正在重启…");
      delay(800);
      ESP.restart();
    }
  } else btnDown = 0;
}

void setup() {
  Serial.begin(115200);
  if (!LittleFS.begin()) { LittleFS.format(); LittleFS.begin(); }
  loadConfig();
  loadWifi();
  displayInit();
  pinMode(PIN_BTN, INPUT_PULLUP);   // 必须在 SPI 初始化之后设置
  configTime(8 * 3600, 0, "ntp.aliyun.com", "cn.pool.ntp.org");   // 北京时间
  netInit();
  webInit();
  randomSeed(RANDOM_REG32);   // 硬件随机数，每次开机顺序不同
}

void loop() {
  uint32_t now = millis();
  netLoop();
  handleButton();
  if (gRestartAt && now > gRestartAt) ESP.restart();

  if (gConfigChanged) {
    gConfigChanged = false;
    needFull = true; wxNextTry = 0; lastBL = 0; lastEv = 0;
  }
  if (lastBL == 0 || now - lastBL > 20000) { lastBL = now ? now : 1; applyBacklight(); }

  // 天气：每 30 分钟一次，失败 2 分钟后重试
  if (!gUploading && WiFi.status() == WL_CONNECTED && now >= wxNextTry) {
    bool ok = fetchWeather();
    wxNextTry = millis() + (ok ? 30UL * 60000UL : 120000UL);
    if (ok && mode == M_PAGES && page == P_CLOCK) needFull = true;
  }

  if (lastEv == 0 || now - lastEv > 1000) { lastEv = now ? now : 1; special = findSpecial(); }

  Mode want;
  if (infoUntil > now) want = M_INFO;
  else if (apActive && WiFi.status() != WL_CONNECTED && !timeValid()) want = M_SETUP;
  else if (special) want = M_CELEB;
  else want = M_PAGES;
  if (want != mode) { mode = want; needFull = true; page = P_CLOCK; }

  switch (mode) {
    case M_SETUP:
      if (needFull) { needFull = false; drawMsg("请连接热点", AP_NAME, "浏览器打开", "192.168.4.1"); }
      break;
    case M_INFO:
      if (needFull) {
        needFull = false;
        drawMsg("Wi-Fi 已连接", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str(), "love-clock.local");
      }
      break;
    case M_CELEB:
      if (needFull) { needFull = false; drawCelebrate(); }
      if (now - lastFrame > 70) { lastFrame = now; tickCelebrate(); }
      break;
    case M_PAGES:
      if (needFull) {
        needFull = false;
        if (!pageEnabled(page)) page = P_CLOCK;
        drawPage(page); lastSwitch = now;
      } else if (now - lastSwitch >= (uint32_t)cfg.slideSec * 1000UL) {
        page = nextPage(page);
        drawPage(page); lastSwitch = now;
      }
      if (page == P_CLOCK) tickClock();
      if (page == P_QUOTE && now - lastFrame > 80) { lastFrame = now; tickQuote(); }
      break;
    default: break;
  }
  delay(2);
}
