#pragma once
// 屏幕驱动、日期计算、各页面的绘制
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <TJpg_Decoder.h>
#include <time.h>
#include "config_store.h"

// ---- 引脚（和 PDF 教程一致）：SCL=D5 SDA=D7 RES=D4 DC=D3 BL=D1，CS 已接地 ----
// 直接写 GPIO 编号：D1=5, D3=0, D4=2，这样不依赖开发板选择
#define PIN_BL 5
Adafruit_ST7789 tft(-1, 0, 2);   // CS 不用, DC=GPIO0(D3), RST=GPIO2(D4)
U8G2_FOR_ADAFRUIT_GFX fnt;
U8G2_FOR_ADAFRUIT_GFX fnt2;   // 绑定到临时画布，用来生成放大的中文

// 如果照片颜色发红发蓝，把这个改成 true
#define JPG_SWAP false

#define F_CN  u8g2_font_wqy16_t_gb2312
#define F_BIG u8g2_font_logisoso54_tf
#define F_MID u8g2_font_logisoso32_tf
#define F_HUGE u8g2_font_logisoso78_tn   // 只有数字

constexpr uint16_t C565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}
const uint16_t COL_BG = 0x0000, COL_FG = 0xFFFF;
const uint16_t COL_PINK = C565(255, 105, 155), COL_DIM = C565(140, 140, 150);
const uint16_t COL_GOLD = C565(255, 200, 70), COL_BLUE = C565(90, 170, 255);
const uint16_t COL_GRAY = C565(170, 175, 190);

// ================= 日期计算 =================
long daysFromCivil(int y, int m, int d) {   // 1970-01-01 为 0
  y -= m <= 2;
  long era = (y >= 0 ? y : y - 399) / 400;
  unsigned yoe = (unsigned)(y - era * 400);
  unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097 + (long)doe - 719468;
}
int daysInMonth(int y, int m) {
  static const uint8_t dm[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if (m == 2 && ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0)) return 29;
  return dm[m - 1];
}
bool timeValid() { return time(nullptr) > 1700000000; }

struct Today { int y, m, d, hh, mm, wd; long days; };
Today getToday() {
  Today t;
  time_t n = time(nullptr);
  tm lt; localtime_r(&n, &lt);
  t.y = lt.tm_year + 1900; t.m = lt.tm_mon + 1; t.d = lt.tm_mday;
  t.hh = lt.tm_hour; t.mm = lt.tm_min;
  int y, m, d;
  if (cfg.testDate[0] && parseDate(cfg.testDate, y, m, d)) { t.y = y; t.m = m; t.d = d; }
  t.days = daysFromCivil(t.y, t.m, t.d);
  t.wd = (int)((t.days + 4) % 7);   // 0=周日
  return t;
}

// ================= 基础绘制 =================
void putTxt(const char* s, int x, int y, uint16_t col, const uint8_t* font) {
  fnt.setFont(font); fnt.setFontMode(0); fnt.setForegroundColor(col); fnt.drawUTF8(x, y, s);
}
int txtW(const char* s, const uint8_t* font) { fnt.setFont(font); return fnt.getUTF8Width(s); }
void txtC(const char* s, int y, uint16_t col, const uint8_t* font, int cx = 120) {
  putTxt(s, cx - txtW(s, font) / 2, y, col, font);
}

void drawHeart(int cx, int cy, int r, uint16_t col) {
  tft.fillCircle(cx - r, cy, r, col);
  tft.fillCircle(cx + r, cy, r, col);
  tft.fillTriangle(cx - (r * 19) / 10, cy + r / 3, cx + (r * 19) / 10, cy + r / 3, cx, cy + (r * 26) / 10, col);
}

// 按像素宽度折行，返回行数
// 英文在空格处换行，中文逐字换行
int wrapLines(const char* s, int maxW, char L[][64], int maxL) {
  int nl = 0, cl = 0, sp = -1;
  char cur[64], tmp[72];
  const uint8_t* p = (const uint8_t*)s;
  fnt.setFont(F_CN);
  while (*p && nl < maxL) {
    int clen = (*p < 0x80) ? 1 : (*p < 0xE0) ? 2 : (*p < 0xF0) ? 3 : 4;
    int rem = strlen((const char*)p);
    if (clen > rem) clen = rem;
    if (cl == 0 && *p == ' ') { p++; continue; }   // 行首空格跳过
    memcpy(tmp, cur, cl); memcpy(tmp + cl, p, clen); tmp[cl + clen] = 0;
    if (cl + clen >= 63 || (cl > 0 && fnt.getUTF8Width(tmp) > maxW)) {
      if (sp > 0 && *p < 0x80 && *p != ' ') {      // 在最后一个空格处断开，剩下的词留到下一行
        memcpy(L[nl], cur, sp); L[nl][sp] = 0; nl++;
        int rest = cl - sp - 1;
        memmove(cur, cur + sp + 1, rest); cl = rest; sp = -1;
        continue;
      }
      cur[cl] = 0; strcpy(L[nl++], cur); cl = 0; sp = -1; continue;
    }
    if (*p == ' ') sp = cl;
    memcpy(cur + cl, p, clen); cl += clen; p += clen;
  }
  if (cl > 0 && nl < maxL) { cur[cl] = 0; strcpy(L[nl++], cur); }
  return nl;
}

// 放大中文：先把 16 像素的字画到 1 位画布上，再按倍数逐行放大画到屏幕。
// cx 为水平中心，base 为放大后的基线。太宽放不下时自动退回 1 倍。
// 放大倍数为 num/den（如 3/2 = 1.5 倍）。最近邻取样，整数倍时和之前完全一样。
void bigCNx(const char* s, int cx, int base, uint16_t col, int num, int den) {
  fnt.setFont(F_CN);
  int w = fnt.getUTF8Width(s);
  if (w * num / den > 236 && num > den * 3 / 2) { num = 3; den = 2; }   // 2 倍放不下先试 1.5 倍
  if (w * num / den > 236) { num = 1; den = 1; }
  const int h = 20, cw = w + 2;
  GFXcanvas1 cv(cw, h);
  if (!cv.getBuffer()) { putTxt(s, cx - w / 2, base, col, F_CN); return; }
  cv.fillScreen(0);
  fnt2.begin(cv);
  fnt2.setFontMode(1);
  fnt2.setFont(F_CN);
  fnt2.setForegroundColor(1);
  fnt2.drawUTF8(0, 15, s);
  int dw = (cw * num + den - 1) / den, dh = (h * num + den - 1) / den;
  int x0 = cx - (w * num / den) / 2, y0 = base - (15 * num) / den;
  for (int dy = 0; dy < dh; dy++) {
    int sy = dy * den / num;
    int dx = 0;
    while (dx < dw) {
      if (cv.getPixel(dx * den / num, sy)) {
        int d0 = dx;
        while (dx < dw && cv.getPixel(dx * den / num, sy)) dx++;
        tft.fillRect(x0 + d0, y0 + dy, dx - d0, 1, col);
      } else dx++;
    }
  }
}
void bigCN(const char* s, int cx, int base, uint16_t col, int sc = 2) { bigCNx(s, cx, base, col, sc, 1); }

// ================= 天气 =================
struct Weather { bool ok = false; int temp = 0; int code = 0; } wx;

uint8_t wxKind(int c) {
  if (c == 0) return 0;
  if (c <= 2) return 1;
  if (c == 3) return 2;
  if (c == 45 || c == 48) return 3;
  if ((c >= 51 && c <= 67) || (c >= 80 && c <= 82)) return 4;
  if ((c >= 71 && c <= 77) || c == 85 || c == 86) return 5;
  if (c >= 95) return 6;
  return 2;
}
const char* WX_ZH[] = {"晴", "多云", "阴", "雾", "雨", "雪", "雷雨"};
const char* WX_EN[] = {"Clear", "Partly", "Cloudy", "Fog", "Rain", "Snow", "Storm"};
const char* wxText(uint8_t k) { return EN() ? WX_EN[k] : WX_ZH[k]; }
const char* MON_EN[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

void drawCloud(int x, int y, uint16_t col) {
  tft.fillCircle(x - 11, y + 5, 8, col);
  tft.fillCircle(x, y - 1, 12, col);
  tft.fillCircle(x + 12, y + 5, 8, col);
  tft.fillRect(x - 11, y + 5, 24, 9, col);
}
void drawSun(int x, int y, int r, bool rays) {
  tft.fillCircle(x, y, r, COL_GOLD);
  if (!rays) return;
  for (int i = 0; i < 8; i++) {
    float a = i * 0.785398f;
    tft.drawLine(x + cosf(a) * (r + 5), y + sinf(a) * (r + 5), x + cosf(a) * (r + 11), y + sinf(a) * (r + 11), COL_GOLD);
  }
}
void drawWxIcon(uint8_t k, int x, int y) {
  switch (k) {
    case 0: drawSun(x, y, 11, true); break;
    case 1: drawSun(x + 10, y - 10, 8, false); drawCloud(x - 2, y + 4, COL_GRAY); break;
    case 2: drawCloud(x, y, COL_GRAY); break;
    case 3:
      for (int i = 0; i < 3; i++) tft.fillRoundRect(x - 20 + i * 4, y - 12 + i * 11, 36, 4, 2, COL_GRAY);
      break;
    case 4:
      drawCloud(x, y - 8, COL_GRAY);
      for (int i = 0; i < 3; i++) tft.drawLine(x - 10 + i * 10, y + 10, x - 14 + i * 10, y + 20, COL_BLUE);
      break;
    case 5:
      drawCloud(x, y - 8, COL_GRAY);
      for (int i = 0; i < 3; i++) { tft.fillCircle(x - 10 + i * 10, y + 12 + (i & 1) * 6, 2, COL_FG); }
      break;
    default:
      drawCloud(x, y - 8, C565(110, 115, 130));
      tft.fillTriangle(x, y + 4, x - 6, y + 14, x + 1, y + 14, COL_GOLD);
      tft.fillTriangle(x + 1, y + 12, x - 3, y + 22, x + 7, y + 10, COL_GOLD);
  }
}

// ================= 事件 / 纪念日 =================
struct EvInfo { const char* name; int days; int m, d; bool cel; bool isStart; };
#define START_NAME TR("在一起纪念日", "Anniversary")

int collectEvents(EvInfo* out, const Today& t) {
  int n = 0;
  for (uint8_t i = 0; i < cfg.nEv; i++) {
    EvInfo& e = out[n++];
    e.name = cfg.ev[i].name; e.m = cfg.ev[i].m; e.d = cfg.ev[i].d;
    e.cel = cfg.ev[i].celebrate; e.isStart = false;
    int y = t.y;
    if (daysFromCivil(y, e.m, e.d) < t.days) y++;
    e.days = (int)(daysFromCivil(y, e.m, e.d) - t.days);
  }
  if (cfg.sy) {
    EvInfo& e = out[n++];
    e.name = START_NAME; e.m = cfg.sm; e.d = cfg.sd; e.cel = true; e.isStart = true;
    int y = max(t.y, (int)cfg.sy + 1);          // 当年不算纪念日
    if (daysFromCivil(y, e.m, e.d) < t.days) y++;
    e.days = (int)(daysFromCivil(y, e.m, e.d) - t.days);
  }
  // 按剩余天数排序
  for (int i = 0; i < n; i++)
    for (int j = i + 1; j < n; j++)
      if (out[j].days < out[i].days) { EvInfo tmp = out[i]; out[i] = out[j]; out[j] = tmp; }
  return n;
}

EvInfo gSpecial;   // 今天的庆祝事件
bool findSpecial() {
  if (!timeValid() && !cfg.testDate[0]) return false;
  Today t = getToday();
  EvInfo ev[MAX_EVENTS + 1];
  int n = collectEvents(ev, t);
  for (int i = 0; i < n; i++)
    if (ev[i].days == 0 && ev[i].cel) { gSpecial = ev[i]; return true; }
  return false;
}

// ================= 情话 =================
int quoteN = -1;
int quoteCount() {
  if (quoteN >= 0) return quoteN;
  quoteN = 0;
  File f = LittleFS.open("/quotes.txt", "r");
  if (!f) return 0;
  while (f.available()) {
    String s = f.readStringUntil('\n'); s.trim();
    if (s.length()) quoteN++;
  }
  f.close();
  return quoteN;
}
bool quoteAt(int idx, char* out, size_t n) {
  File f = LittleFS.open("/quotes.txt", "r");
  if (!f) return false;
  int k = 0; bool ok = false;
  while (f.available()) {
    String s = f.readStringUntil('\n'); s.trim();
    if (!s.length()) continue;
    if (k++ == idx) { safeCopy(out, n, s.c_str()); ok = true; break; }
  }
  f.close();
  return ok;
}

// ================= 页面 =================
enum Page : uint8_t { P_CLOCK, P_TOGETHER, P_COUNT, P_PHOTO, P_QUOTE, P_N };

bool pageEnabled(uint8_t p) {
  switch (p) {
    case P_TOGETHER: return cfg.sy != 0;
    case P_COUNT:    return cfg.nEv > 0 || cfg.sy != 0;
    case P_PHOTO:    return cfg.nPh > 0;
    case P_QUOTE:    return quoteCount() > 0;
    default:         return true;
  }
}
uint8_t nextPage(uint8_t p) {
  for (int i = 0; i < P_N; i++) {
    p = (p + 1) % P_N;
    if (pageEnabled(p)) return p;
  }
  return P_CLOCK;
}

// "天"/"days" 单位及其放大 2 倍后的宽度
const char* dayUnit(long n) { return EN() ? (n == 1 ? "day" : "days") : "天"; }
int unitW(const char* u) { return EN() ? txtW(u, F_CN) * 2 : 32; }

// ================= 时钟页：6 种样式（cfg.clockStyle：0=A 默认）=================
// 0 A 超大数字  1 B 上下分行  2 C 圆润卡片  3 D 表盘  4 E 进度信息  5 F 爱心
#define CLOCK_STYLES 6
#define F_L62  u8g2_font_logisoso62_tn
#define F_FUB49 u8g2_font_fub49_tn
#define F_FUB35 u8g2_font_fub35_tf
#define F_FUB30 u8g2_font_fub30_tf
#define F_FUB25 u8g2_font_fub25_tf
#define F_FUB20 u8g2_font_fub20_tf
#define F_FUB14 u8g2_font_fub14_tf

const uint16_t COL_LINE = C565(60, 60, 72), COL_CARD = C565(28, 30, 40);

// 透明背景写字（putTxt 是实心背景，卡片和爱心上要用这个）
void tt(const char* s, int x, int y, uint16_t col, const uint8_t* font) {
  fnt.setFont(font); fnt.setFontMode(1); fnt.setForegroundColor(col); fnt.drawUTF8(x, y, s);
}
void ttC(const char* s, int y, uint16_t col, const uint8_t* font, int cx = 120) { tt(s, cx - txtW(s, font) / 2, y, col, font); }
void thickLine(int x0, int y0, int x1, int y1, int w, uint16_t col) {
  int n = max(abs(x1 - x0), abs(y1 - y0)); if (n < 1) n = 1;
  for (int i = 0; i <= n; i++) tft.fillCircle(x0 + (x1 - x0) * i / n, y0 + (y1 - y0) * i / n, w / 2, col);
}

// 小号天气图标，用在底部天气行
void drawWxMini(uint8_t k, int x, int y) {
  auto cloud = [&](int cx, int cy, uint16_t c) {
    tft.fillCircle(cx - 7, cy + 3, 5, c); tft.fillCircle(cx, cy - 2, 7, c); tft.fillCircle(cx + 7, cy + 3, 5, c);
    tft.fillRect(cx - 7, cy + 3, 15, 6, c);
  };
  switch (k) {
    case 0:
      tft.fillCircle(x, y, 6, COL_GOLD);
      for (int i = 0; i < 8; i++) { float a = i * 0.785398f; tft.drawLine(x + cosf(a) * 9, y + sinf(a) * 9, x + cosf(a) * 13, y + sinf(a) * 13, COL_GOLD); }
      break;
    case 1: tft.fillCircle(x + 6, y - 6, 5, COL_GOLD); cloud(x - 1, y + 2, COL_GRAY); break;
    case 2: cloud(x, y, COL_GRAY); break;
    case 3: for (int i = 0; i < 3; i++) tft.fillRoundRect(x - 12 + i * 3, y - 8 + i * 7, 22, 3, 1, COL_GRAY); break;
    case 4: cloud(x, y - 5, COL_GRAY); for (int i = 0; i < 3; i++) tft.drawLine(x - 6 + i * 6, y + 7, x - 9 + i * 6, y + 13, COL_BLUE); break;
    case 5: cloud(x, y - 5, COL_GRAY); for (int i = 0; i < 3; i++) tft.fillCircle(x - 6 + i * 6, y + 8 + (i & 1) * 4, 1, COL_FG); break;
    default: cloud(x, y - 5, C565(110, 115, 130)); tft.fillTriangle(x, y + 3, x - 4, y + 10, x + 1, y + 10, COL_GOLD);
  }
}

// 温度：数字 + 小圆圈 + C，返回结束的 x
int drawTemp(int x, int base, uint16_t col, uint16_t colC, const uint8_t* font) {
  char tb[8]; snprintf(tb, sizeof tb, "%d", wx.temp);
  int w = txtW(tb, font); fnt.setFont(font); int asc = fnt.getFontAscent();
  tt(tb, x, base, col, font);
  tft.drawCircle(x + w + 5, base - (asc * 3) / 4 - 1, 3, col);
  tt("C", x + w + 11, base, colC, F_CN);
  return x + w + 20;
}
int tempW(const uint8_t* font) { char tb[8]; snprintf(tb, sizeof tb, "%d", wx.temp); return txtW(tb, font) + 20; }

void wxCondLine(char* b, size_t n) {
  if (cfg.city[0]) snprintf(b, n, "%s \xC2\xB7 %s", wxText(wxKind(wx.code)), cfg.city);
  else snprintf(b, n, "%s", wxText(wxKind(wx.code)));
}

int clockDrawnMin = -1;

void clockTimeStr(char* b, uint16_t& col, uint16_t normal) {
  Today t = getToday();
  if (timeValid()) { snprintf(b, 8, "%02d:%02d", t.hh, t.mm); col = normal; }
  else { strcpy(b, "00:00"); col = COL_DIM; }
}
void dateFull(char* b, size_t n) {
  Today t = getToday();
  static const char* WD[] = {"日", "一", "二", "三", "四", "五", "六"};
  static const char* WDE[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
  if (timeValid() || cfg.testDate[0]) {
    if (EN()) snprintf(b, n, "%s, %s %d", WDE[t.wd], MON_EN[t.m - 1], t.d);
    else snprintf(b, n, "%d月%d日  周%s", t.m, t.d, WD[t.wd]);
  } else snprintf(b, n, "%s", TR("等待校时…", "Syncing time..."));
}
void dateMD(char* b, size_t n) {
  Today t = getToday();
  if (EN()) snprintf(b, n, "%s %d", MON_EN[t.m - 1], t.d); else snprintf(b, n, "%d月%d日", t.m, t.d);
}
void weekS(char* b, size_t n) {
  Today t = getToday();
  static const char* WD[] = {"日", "一", "二", "三", "四", "五", "六"};
  static const char* WDE[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
  if (EN()) snprintf(b, n, "%s", WDE[t.wd]); else snprintf(b, n, "周%s", WD[t.wd]);
}
void wxLoading(int y) { ttC(TR("天气获取中…", "Loading weather..."), y, COL_DIM, F_CN); }

// ---- 每分钟刷新的部分 ----
void clockDynamic(uint8_t st) {
  Today t = getToday();
  char b[40]; uint16_t col;
  switch (st) {
    case 1:   // B 上下分行
      tft.fillRect(0, 4, 144, 186, COL_BG);
      snprintf(b, sizeof b, "%02d", timeValid() ? t.hh : 0); tt(b, 18, 92, timeValid() ? COL_FG : COL_DIM, F_HUGE);
      snprintf(b, sizeof b, "%02d", timeValid() ? t.mm : 0); tt(b, 18, 184, timeValid() ? COL_PINK : COL_DIM, F_HUGE);
      break;
    case 2:   // C 圆润卡片
      tft.fillRect(0, 58, 240, 60, COL_BG);
      clockTimeStr(b, col, COL_FG); ttC(b, 112, col, F_FUB49);
      break;
    case 3: { // D 表盘
      const int cx = 120, cy = 104, R = 90;
      tft.fillCircle(cx, cy, R - 14, COL_BG);
      drawHeart(cx, cy - R + 26, 5, COL_PINK);
      dateMD(b, sizeof b); char w[16]; weekS(w, sizeof w); strcat(b, " "); strcat(b, w);
      ttC(b, cy + 48, COL_DIM, F_CN);
      float ah = ((t.hh % 12) + t.mm / 60.0f) * PI / 6, am = t.mm * PI / 30;
      thickLine(cx, cy, cx + sinf(ah) * 46, cy - cosf(ah) * 46, 5, COL_FG);
      thickLine(cx, cy, cx + sinf(am) * 70, cy - cosf(am) * 70, 3, COL_PINK);
      tft.fillCircle(cx, cy, 5, COL_GOLD);
      break; }
    case 4: { // E 进度信息
      tft.fillRect(0, 44, 240, 68, COL_BG);
      clockTimeStr(b, col, COL_FG); ttC(b, 108, col, F_L62);
      tft.fillRect(0, 118, 240, 40, COL_BG);
      int pct = (t.hh * 60 + t.mm) * 100 / 1440;
      tft.fillRoundRect(20, 124, 200, 8, 4, C565(45, 47, 58));
      tft.fillRoundRect(20, 124, max(8, 200 * pct / 100), 8, 4, COL_PINK);
      snprintf(b, sizeof b, EN() ? "Today %d%%" : "今天已过 %d%%", pct);
      ttC(b, 152, COL_DIM, F_CN);
      break; }
    case 5:   // F 爱心
      tft.fillRect(62, 76, 116, 30, COL_PINK);
      clockTimeStr(b, col, COL_FG); ttC(b, 102, col, F_FUB35);
      break;
    default:  // A 超大数字
      tft.fillRect(0, 64, 240, 92, COL_BG);
      clockTimeStr(b, col, COL_FG); ttC(b, 150, col, F_HUGE);
  }
  clockDrawnMin = t.mm;
}

// ---- 整页（不随分钟变化的部分）----
void drawClockStatic(uint8_t st) {
  char b[48], w[16]; uint8_t k = wxKind(wx.code);
  switch (st) {
    case 1:   // B
      tft.drawFastVLine(146, 26, 158, COL_LINE);
      drawHeart(190, 40, 8, COL_PINK);
      { Today t = getToday(); snprintf(b, sizeof b, "%02d.%02d", t.m, t.d); ttC(b, 110, COL_FG, F_FUB20, 190); }
      weekS(w, sizeof w); ttC(w, 140, COL_PINK, F_CN, 190);
      tft.drawFastHLine(20, 200, 200, COL_LINE);
      if (wx.ok) {
        drawWxMini(k, 40, 219);
        int x = drawTemp(68, 227, COL_FG, COL_DIM, F_FUB20);
        wxCondLine(b, sizeof b); tt(b, x + 6, 226, COL_DIM, F_CN);
      } else wxLoading(222);
      break;
    case 2: { // C
      dateFull(b, sizeof b);
      int pw = txtW(b, F_CN) + 28;
      tft.fillRoundRect(120 - pw / 2, 12, pw, 28, 14, COL_PINK);
      ttC(b, 32, COL_FG, F_CN);
      tft.fillRoundRect(14, 134, 100, 88, 14, COL_CARD);
      tft.fillRoundRect(126, 134, 100, 88, 14, COL_CARD);
      if (wx.ok) {
        drawWxIcon(k, 64, 160);
        int x = 64 - tempW(F_FUB30) / 2;
        drawTemp(x, 208, COL_FG, COL_DIM, F_FUB30);
        drawHeart(176, 156, 6, COL_PINK);
        ttC(wxText(k), 194, COL_FG, F_CN, 176);
        ttC(cfg.city, 214, COL_DIM, F_CN, 176);
      } else { ttC(TR("天气", "Weather"), 182, COL_DIM, F_CN, 64); ttC("--", 204, COL_DIM, F_CN, 64); drawHeart(176, 170, 8, COL_PINK); }
      break; }
    case 3: { // D
      const int cx = 120, cy = 104, R = 90;
      tft.drawCircle(cx, cy, R, C565(70, 70, 84)); tft.drawCircle(cx, cy, R - 1, C565(70, 70, 84));
      for (int i = 0; i < 60; i++) {
        float a = i * PI / 30; int r0; uint16_t c;
        if (i % 5 == 0) { r0 = R - 12; c = (i % 15 == 0) ? COL_FG : COL_GRAY; } else { r0 = R - 6; c = C565(80, 80, 94); }
        int x0 = cx + sinf(a) * r0, y0 = cy - cosf(a) * r0, x1 = cx + sinf(a) * (R - 3), y1 = cy - cosf(a) * (R - 3);
        tft.drawLine(x0, y0, x1, y1, c);
        if (i % 5 == 0) { tft.drawLine(x0 + 1, y0, x1 + 1, y1, c); }
      }
      tft.drawFastHLine(40, 214, 160, COL_LINE);
      if (wx.ok) {
        drawWxMini(k, 40, 226);
        int x = drawTemp(62, 235, COL_FG, COL_DIM, F_FUB14);
        wxCondLine(b, sizeof b); tt(b, x + 8, 234, COL_DIM, F_CN);
      } else wxLoading(232);
      break; }
    case 4: { // E
      dateMD(b, sizeof b); tt(b, 16, 32, COL_FG, F_CN);
      weekS(w, sizeof w); tt(w, 224 - txtW(w, F_CN), 32, COL_PINK, F_CN);
      static const char* NZ[] = {"一", "二", "三", "四", "五", "六", "日"};
      static const char* NE[] = {"M", "T", "W", "T", "F", "S", "S"};
      int today = (getToday().wd + 6) % 7;
      for (int i = 0; i < 7; i++) {
        int x = 30 + i * 30;
        if (i == today) tft.fillCircle(x, 176, 12, COL_PINK); else tft.drawCircle(x, 176, 12, C565(70, 70, 84));
        ttC(EN() ? NE[i] : NZ[i], 182, i == today ? COL_FG : COL_DIM, F_CN, x);
      }
      if (wx.ok) {
        drawWxMini(k, 40, 216);
        int x = drawTemp(66, 224, COL_FG, COL_DIM, F_FUB20);
        wxCondLine(b, sizeof b); tt(b, x + 8, 222, COL_DIM, F_CN);
      } else wxLoading(220);
      break; }
    case 5: { // F 爱心
      drawHeart(120, 62, 46, COL_PINK);
      drawHeart(16, 26, 5, COL_GOLD); drawHeart(224, 22, 4, COL_PINK); drawHeart(216, 168, 5, COL_GOLD); drawHeart(24, 168, 4, COL_PINK);
      dateFull(b, sizeof b); ttC(b, 208, COL_FG, F_CN);
      if (wx.ok) {
        if (cfg.city[0]) snprintf(b, sizeof b, "%s %d\xC2\xB0""C \xC2\xB7 %s", wxText(k), wx.temp, cfg.city);
        else snprintf(b, sizeof b, "%s %d\xC2\xB0""C", wxText(k), wx.temp);
        ttC(b, 232, COL_DIM, F_CN);
      } else wxLoading(232);
      break; }
    default: { // A
      drawHeart(120, 18, 4, COL_PINK);
      dateFull(b, sizeof b); ttC(b, 52, COL_DIM, F_CN);
      tft.drawFastHLine(24, 164, 192, COL_LINE);
      if (wx.ok) {
        drawWxIcon(k, 48, 198);
        drawTemp(84, 214, COL_FG, COL_FG, F_MID);
        wxCondLine(b, sizeof b); tt(b, 216 - txtW(b, F_CN), 214, COL_DIM, F_CN);
      } else wxLoading(204);
    }
  }
}

void drawClockPage() {
  tft.fillScreen(COL_BG);
  uint8_t st = cfg.clockStyle < CLOCK_STYLES ? cfg.clockStyle : 0;
  drawClockStatic(st);
  clockDynamic(st);
}
void tickClock() {   // 每分钟只刷新时间部分
  Today t = getToday();
  if (t.mm != clockDrawnMin) clockDynamic(cfg.clockStyle < CLOCK_STYLES ? cfg.clockStyle : 0);
}

void drawTogetherPage() {
  tft.fillScreen(COL_BG);
  Today t = getToday();
  long days = t.days - daysFromCivil(cfg.sy, cfg.sm, cfg.sd) + 1;
  if (days < 1) days = 1;
  drawHeart(120, 22, 9, COL_PINK);
  bigCN(TR("我们在一起", "Together for"), 120, 80, COL_FG, 2);
  char b[24];
  snprintf(b, sizeof b, "%ld", days);
  const uint8_t* nf = txtW(b, F_HUGE) <= 150 ? F_HUGE : F_BIG;
  int w = txtW(b, nf);
  const char* unit = dayUnit(days);
  int uw = unitW(unit);
  int x = 120 - (w + 8 + uw) / 2;
  putTxt(b, x, 176, COL_PINK, nf);
  bigCN(unit, x + w + 8 + uw / 2, 176, COL_FG, 2);
  int Y = t.y - cfg.sy, M = t.m - cfg.sm, D = t.d - cfg.sd;
  if (D < 0) { M--; int pm = t.m - 1, py = t.y; if (pm < 1) { pm = 12; py--; } D += daysInMonth(py, pm); }
  if (M < 0) { Y--; M += 12; }
  if (Y < 0) { Y = 0; M = 0; D = 0; }
  if (EN()) snprintf(b, sizeof b, "%dy %dm %dd", Y, M, D);
  else snprintf(b, sizeof b, "%d年%d个月%d天", Y, M, D);
  txtC(b, 224, COL_DIM, F_CN);
}

// 倒数日：事件名 2 倍大，天数最大最醒目，下面一条进度条表示"离得多近"
void drawCountPage() {
  tft.fillScreen(COL_BG);
  Today t = getToday();
  EvInfo ev[MAX_EVENTS + 1];
  int n = collectEvents(ev, t);
  if (n == 0) { txtC(TR("请在网页里添加纪念日", "Add events on the web page"), 130, COL_FG, F_CN); return; }
  char b[48];
  drawHeart(120, 14, 4, COL_PINK);
  bigCN(ev[0].name, 120, 62, COL_FG, 2);
  if (ev[0].days == 0) {
    bigCN(TR("就是今天！", "It's today!"), 120, 150, COL_GOLD, 2);
  } else {
    txtC(TR("还有", "in"), 88, COL_DIM, F_CN);
    snprintf(b, sizeof b, "%d", ev[0].days);
    int w = txtW(b, F_HUGE);
    const char* unit = dayUnit(ev[0].days);
    int uw = unitW(unit);
    int x = 120 - (w + 8 + uw) / 2;
    putTxt(b, x, 176, COL_GOLD, F_HUGE);
    bigCN(unit, x + w + 8 + uw / 2, 176, COL_FG, 2);
    int pct = ev[0].days >= 365 ? 0 : (365 - ev[0].days) * 100 / 365;
    tft.fillRoundRect(30, 188, 180, 6, 3, C565(50, 50, 60));
    if (pct > 0) tft.fillRoundRect(30, 188, max(6, 180 * pct / 100), 6, 3, COL_GOLD);
  }
  if (EN()) snprintf(b, sizeof b, "%s %d", MON_EN[ev[0].m - 1], ev[0].d);
  else snprintf(b, sizeof b, "%d月%d日", ev[0].m, ev[0].d);
  txtC(b, 216, COL_DIM, F_CN);
  if (n > 1) {
    if (EN()) snprintf(b, sizeof b, "Then %s, %dd", ev[1].name, ev[1].days);
    else snprintf(b, sizeof b, "之后 %s %d天", ev[1].name, ev[1].days);
    txtC(b, 235, C565(100, 100, 110), F_CN);
  }
}

bool jpgOut(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bmp) {
  if (y >= tft.height()) return false;
  tft.drawRGBBitmap(x, y, bmp, w, h);
  return true;
}
uint8_t phIdx = 0;
void drawPhotoPage() {
  if (cfg.nPh == 0) return;
  if (phIdx >= cfg.nPh) phIdx = 0;
  if (cfg.nPh > 1) { uint8_t k; do { k = random(cfg.nPh); } while (k == phIdx); phIdx = k; }   // 随机，不连续重复
  PhotoItem& p = cfg.ph[phIdx];
  char path[20]; snprintf(path, sizeof path, "/%s", p.file);
  tft.fillScreen(COL_BG);
  if (LittleFS.exists(path)) TJpgDec.drawFsJpg(0, 0, path, LittleFS);
  if (p.cap[0]) {
    tft.fillRect(0, 208, 240, 32, COL_BG);
    txtC(p.cap, 230, COL_FG, F_CN);
  }
}

// 情话页：每次随机一句；中间的爱心跳动，两侧有爱心慢慢飘升
struct Qf { int16_t x, y; uint8_t r, sp; uint16_t col; } qf[4];
int lastQuote = -1;

void drawQuotePage() {
  tft.fillScreen(COL_BG);
  drawHeart(120, 36, 9, COL_PINK);
  for (int i = 0; i < 4; i++) {
    qf[i].x = (i % 2) ? random(216, 231) : random(10, 25);
    qf[i].y = random(80, 236);
    qf[i].r = random(3, 5);
    qf[i].sp = random(1, 3);
    qf[i].col = (i == 0) ? COL_GOLD : COL_PINK;
  }
  int n = quoteCount();
  if (n == 0) return;
  int idx = 0;
  if (n > 1) { do { idx = random(n); } while (idx == lastQuote); }
  lastQuote = idx;
  char q[128];
  if (!quoteAt(idx, q, sizeof q)) return;
  char L[6][64];
  int nl = wrapLines(q, 117, L, 6);                 // 117 = 176 / 1.5
  if (nl <= 5) {                                    // 放大 1.5 倍，每行约 7 个字
    int lineH = 30, y0 = 150 - (nl - 1) * lineH / 2 + 8;
    for (int i = 0; i < nl; i++) bigCNx(L[i], 120, y0 + i * lineH, COL_FG, 3, 2);
  } else {                                          // 句子太长，退回原大小
    nl = wrapLines(q, 176, L, 5);
    int lineH = 26, y0 = 140 - (nl - 1) * lineH / 2 + 6;
    for (int i = 0; i < nl; i++) txtC(L[i], y0 + i * lineH, COL_FG, F_CN);
  }
}

void tickQuote() {
  static uint8_t ph = 0, lastR = 0;
  ph = (ph + 1) % 16;
  uint8_t r = 9 + (ph < 8 ? ph : 16 - ph) / 3;      // 爱心大小在 9~11 之间变化
  if (r != lastR) {
    lastR = r;
    tft.fillRect(90, 20, 60, 52, COL_BG);
    drawHeart(120, 36, r, COL_PINK);
  }
  for (int i = 0; i < 4; i++) {
    Qf& f = qf[i];
    tft.fillRect(f.x - 2 * f.r - 1, f.y - f.r - 1, 4 * f.r + 2, (f.r * 38) / 10 + 3, COL_BG);
    f.y -= f.sp;
    if (f.y < 76) f.y = 238;
    drawHeart(f.x, f.y, f.r, f.col);
  }
}

void drawPage(uint8_t p) {
  switch (p) {
    case P_CLOCK:    drawClockPage(); break;
    case P_TOGETHER: drawTogetherPage(); break;
    case P_COUNT:    drawCountPage(); break;
    case P_PHOTO:    drawPhotoPage(); break;
    case P_QUOTE:    drawQuotePage(); break;
  }
}

// ================= 庆祝页（爱心飘动） =================
struct Fl { int16_t x, y; uint8_t r, sp; uint16_t col; bool top; } fl[10];

void drawCelebrate() {
  tft.fillScreen(COL_BG);
  Today t = getToday();
  char b[48];
  if (gSpecial.isStart) {
    int years = t.y - cfg.sy;
    txtC(TR("今天是我们的", "Today is our"), 96, COL_FG, F_CN);
    snprintf(b, sizeof b, "%d", years);
    txtC(b, 158, COL_PINK, F_BIG);
    txtC(TR("周年纪念日", "year anniversary"), 184, COL_FG, F_CN);
  } else {
    txtC(TR("今天是", "Today is"), 100, COL_FG, F_CN);
    bigCN(gSpecial.name, 120, 152, COL_PINK, 2);
    txtC(TR("记得要开心哦", "Have a happy day!"), 188, COL_GOLD, F_CN);
  }
  for (int i = 0; i < 10; i++) {
    fl[i].top = (i % 2 == 0);
    fl[i].x = random(14, 226);
    fl[i].y = fl[i].top ? random(6, 46) : random(198, 236);
    fl[i].r = random(3, 6);
    fl[i].sp = random(1, 3);
    fl[i].col = (i % 3 == 0) ? COL_GOLD : COL_PINK;
  }
}
void tickCelebrate() {
  for (int i = 0; i < 10; i++) {
    Fl& f = fl[i];
    tft.fillRect(f.x - 2 * f.r - 1, f.y - f.r - 1, 4 * f.r + 2, (f.r * 38) / 10 + 3, COL_BG);
    f.y -= f.sp;
    if (f.top && f.y < 6) f.y = 46;
    if (!f.top && f.y < 198) f.y = 236;
    drawHeart(f.x, f.y, f.r, f.col);
  }
}

// ================= 提示屏 =================
void drawMsg(const char* a, const char* b = "", const char* c = "", const char* d = "") {
  tft.fillScreen(COL_BG);
  txtC(a, 70, COL_PINK, F_CN);
  txtC(b, 110, COL_FG, F_CN);
  txtC(c, 150, COL_FG, F_CN);
  txtC(d, 190, COL_DIM, F_CN);
}

void applyBacklight() {
  int pct = cfg.bright;
  if (timeValid() && cfg.nightStart != cfg.nightEnd) {
    int h = getToday().hh;
    bool night = cfg.nightStart > cfg.nightEnd ? (h >= cfg.nightStart || h < cfg.nightEnd)
                                               : (h >= cfg.nightStart && h < cfg.nightEnd);
    if (night) pct = cfg.nightBright;
  }
  analogWrite(PIN_BL, map(pct, 0, 100, 0, 1023));
}

void displayInit() {
  pinMode(PIN_BL, OUTPUT);
  analogWrite(PIN_BL, 700);
  tft.init(240, 240, SPI_MODE2);       // 无 CS 脚的 ST7789 必须用 MODE2，否则花屏
  tft.setSPISpeed(32000000);           // 不稳定就改小，比如 20000000
  tft.setRotation(2);                  // 转 180 度；方向不对就改回 0
  tft.fillScreen(COL_BG);
  fnt.begin(tft);
  fnt.setFontMode(1);                   // 透明背景
  TJpgDec.setJpgScale(1);
  TJpgDec.setSwapBytes(JPG_SWAP);
  TJpgDec.setCallback(jpgOut);
}
