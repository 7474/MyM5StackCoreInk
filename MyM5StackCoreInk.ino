#include <WiFi.h>
#include "M5CoreInk.h"

#include "env.h"

Ink_Sprite InkPageSprite(&M5.M5Ink);

RTC_TimeTypeDef RTCtime;
RTC_DateTypeDef RTCDate;

char timeStrbuff[64];

void flushTime() {
  M5.rtc.GetTime(&RTCtime);
  M5.rtc.GetData(&RTCDate);

  sprintf(timeStrbuff, "%d/%02d/%02d %02d:%02d:%02d",
          RTCDate.Year, RTCDate.Month, RTCDate.Date,
          RTCtime.Hours, RTCtime.Minutes, RTCtime.Seconds);

  InkPageSprite.drawString(10, 100, timeStrbuff);
  InkPageSprite.pushSprite();
}

void setupTime() {
  // https://wak-tech.com/archives/833
  configTime(9 * 3600L, 0, "ntp.nict.jp", "time.google.com", "ntp.jst.mfeed.ad.jp");//NTPの設定
  struct tm timeInfo;
  getLocalTime(&timeInfo);

  RTCtime.Hours = timeInfo.tm_hour;
  RTCtime.Minutes = timeInfo.tm_min;
  RTCtime.Seconds = timeInfo.tm_sec;
  M5.rtc.SetTime(&RTCtime);

  RTCDate.Year = timeInfo.tm_year + 1900;
  RTCDate.Month = timeInfo.tm_mon + 1;
  RTCDate.Date = timeInfo.tm_mday;
  M5.rtc.SetData(&RTCDate);
}

void setup() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  if (WiFi.begin(ssid, password) != WL_DISCONNECTED) {
    ESP.restart();
  }

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
  }

  M5.begin();
  if ( !M5.M5Ink.isInit())
  {
    Serial.printf("Ink Init faild");
    while (1) delay(100);
  }
  M5.M5Ink.clear();
  delay(1000);
  //creat ink refresh Sprite
  if ( InkPageSprite.creatSprite(0, 0, 200, 200, true) != 0 )
  {
    Serial.printf("Ink Sprite creat faild");
  }
  setupTime();
}

void loop() {
  flushTime();
  delay(15000);
}
