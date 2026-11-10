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
  fnt.setFont(font); fnt.setForegroundColor(col); fnt.drawUTF8(x, y, s);
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

int clockDrawnMin = -1;
void drawClockDigits() {
  Today t = getToday();
  tft.fillRect(0, 52, 240, 76, COL_BG);
  char b[8];
  if (timeValid()) snprintf(b, sizeof b, "%02d:%02d", t.hh, t.mm); else strcpy(b, "--:--");
  txtC(b, 118, COL_FG, F_BIG);
  clockDrawnMin = t.mm;
}
void drawClockPage() {
  tft.fillScreen(COL_BG);
  Today t = getToday();
  static const char* WD[] = {"日", "一", "二", "三", "四", "五", "六"};
  static const char* WDE[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
  char b[40];
  if (timeValid() || cfg.testDate[0]) {
    if (EN()) snprintf(b, sizeof b, "%s, %s %d", WDE[t.wd], MON_EN[t.m - 1], t.d);
    else snprintf(b, sizeof b, "%d月%d日  周%s", t.m, t.d, WD[t.wd]);
  } else strcpy(b, TR("等待校时…", "Syncing time..."));
  txtC(b, 34, COL_DIM, F_CN);
  drawClockDigits();
  tft.drawFastHLine(30, 142, 180, C565(50, 50, 60));
  if (wx.ok) {
    uint8_t k = wxKind(wx.code);
    drawWxIcon(k, 62, 186);
    char tb[8]; snprintf(tb, sizeof tb, "%d", wx.temp);
    int w = txtW(tb, F_MID);
    putTxt(tb, 112, 196, COL_FG, F_MID);
    tft.drawCircle(112 + w + 6, 172, 3, COL_FG);
    putTxt("C", 112 + w + 12, 196, COL_FG, F_CN);
    snprintf(b, sizeof b, "%s  %s", wxText(k), cfg.city);
    putTxt(b, 112, 224, COL_DIM, F_CN);
  } else {
    txtC(TR("天气获取中…", "Loading weather..."), 196, COL_DIM, F_CN);
  }
}
void tickClock() {   // 每分钟只刷新数字
  Today t = getToday();
  if (t.mm != clockDrawnMin) drawClockDigits();
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
