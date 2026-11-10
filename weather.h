#pragma once
// Open-Meteo 天气（免费，无需注册）
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include "display.h"

bool fetchWeather() {
  if (WiFi.status() != WL_CONNECTED) return false;
  BearSSL::WiFiClientSecure client;
  client.setInsecure();
  client.setBufferSizes(1024, 512);
  HTTPClient http;
  http.setTimeout(8000);
  http.useHTTP10(true);   // 避免分块传输，方便直接解析
  char url[220];
  snprintf(url, sizeof url,
           "https://api.open-meteo.com/v1/forecast?latitude=%.3f&longitude=%.3f"
           "&current=temperature_2m,weather_code&timezone=Asia%%2FShanghai",
           cfg.lat, cfg.lon);
  if (!http.begin(client, url)) return false;
  int code = http.GET();
  if (code != 200) { http.end(); return false; }
  StaticJsonDocument<96> filter;
  filter["current"]["temperature_2m"] = true;
  filter["current"]["weather_code"] = true;
  StaticJsonDocument<192> doc;
  DeserializationError err = deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filter));
  http.end();
  if (err) return false;
  JsonObject cur = doc["current"];
  if (cur.isNull() || !cur.containsKey("temperature_2m")) return false;
  wx.temp = (int)roundf(cur["temperature_2m"].as<float>());
  wx.code = cur["weather_code"] | 0;
  wx.ok = true;
  return true;
}
