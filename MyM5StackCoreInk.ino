#include "M5CoreInk.h"

#include <Wire.h>
#include "Adafruit_BMP280.h"
#include "ClosedCube_SHT31D.h"

#include <WiFi.h>
#include "env.h"

// TODO EEPROM.length() が 0 で読み書きできないのが解決したらまた試す。
//#include <EEPROM.h>
#include "MackerelClient.h"
MackerelHostMetric hostMetricsPool[10];
MackerelServiceMetric serviceMetricsPool[10];
MackerelClient mackerelClient(hostMetricsPool, 10, serviceMetricsPool, 10, mackerelApiKey);

#define VARSION = "MyM5StackCoreInk 0.0.3"
#define MY_SHT30_ADDRESS 0x44
#define MY_BMP280_ADDRESS 0x76

Ink_Sprite InkPageSprite(&M5.M5Ink);

RTC_TimeTypeDef RTCtime;
RTC_DateTypeDef RTCDate;

char flushStrBuf[64];

void putLog(char* message) {
  //  InkPageSprite.drawString(10, 180, message);
  //  InkPageSprite.pushSprite();
  Serial.println(message);
}

void flushTime() {
  M5.rtc.GetTime(&RTCtime);
  M5.rtc.GetData(&RTCDate);

  sprintf(flushStrBuf, "%d/%02d/%02d %02d:%02d:%02d",
          RTCDate.Year, RTCDate.Month, RTCDate.Date,
          RTCtime.Hours, RTCtime.Minutes, RTCtime.Seconds);

  InkPageSprite.drawString(10, 10, flushStrBuf);
  InkPageSprite.pushSprite();
}

ClosedCube_SHT31D sht3xd;
SHT31D sht3xResult;

Adafruit_BMP280 bmp; // I2C
float temperature; // C
uint32_t pressure; // Pa
float altitude; // m
// TODO get current seaLevelhPa
// https://www.data.jma.go.jp/obd/stats/data/mdrr/synopday/data1s.html
// 毎日の全国データ一覧表（日別値詳細版:2020年11月14日）
// 地点 現地平均  海面平均  最低海面値 最低海面起時
// 東京 1023.0  1026.0  1022.5  02:51
float seaLevelhPa = 1026.0; // hPa

void flushEnv() {
  InkPageSprite.drawString(5, 30, "BMP280");
  sprintf(flushStrBuf, "%2.2fC %4.1fhPa %4.0fm",
          temperature, pressure / 100.0f, altitude);
  InkPageSprite.drawString(10, 45, flushStrBuf);

  InkPageSprite.drawString(5, 60, "SHT30");
  if (sht3xResult.error == SHT3XD_NO_ERROR) {
    sprintf(flushStrBuf, "%2.2fC %2.1f%%",
            sht3xResult.t, sht3xResult.rh);
  } else {
    sprintf(flushStrBuf, "[ERROR] Code #%d", sht3xResult.error);
  }
  InkPageSprite.drawString(10, 75, flushStrBuf);

  InkPageSprite.pushSprite();
}

void updateEnv() {
  sht3xResult = sht3xd.periodicFetchData();

  temperature =  bmp.readTemperature();
  pressure = bmp.readPressure();
  altitude = bmp.readAltitude(seaLevelhPa);
}

void sendEnvToMackerel() {
  mackerelClient.addServiceMetric("sht30.t", sht3xResult.t);
  mackerelClient.addServiceMetric("sht30.rh", sht3xResult.rh);
  mackerelClient.postServiceMetrics("MyM5StackCoreInk");

  mackerelClient.addHostMetric("custom.sht30.t", sht3xResult.t);
  mackerelClient.addHostMetric("custom.sht30.rh", sht3xResult.rh);
  // 自宅のRaspberry PiがBME280からの値を bme280.* で送っているのでそれに合わせている。
  // が、温度と気圧のセンサが同じものなのかは知らない。
  // https://github.com/7474/RealDiceBot/blob/master/IoTEdgeDevice/files/usr/local/bin/bme280/bme280.sh
  mackerelClient.addHostMetric("custom.bme280.temperature", temperature);
  mackerelClient.addHostMetric("custom.bme280.pressure", pressure / 100.0f);
  mackerelClient.postHostMetrics();
}

void setupM5Ink() {
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
}

void setupWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  if (WiFi.begin(ssid, password) != WL_DISCONNECTED) {
    ESP.restart();
  }

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
  }
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

void setupEnv() {
  // https://github.com/closedcube/ClosedCube_SHT31D_Arduino/blob/master/examples/periodicmode/periodicmode.ino
  sht3xd.begin(MY_SHT30_ADDRESS);
  Serial.print("Serial #");
  Serial.println(sht3xd.readSerialNumber());
  if (sht3xd.periodicStart(SHT3XD_REPEATABILITY_HIGH, SHT3XD_FREQUENCY_10HZ) != SHT3XD_NO_ERROR)
    Serial.println("[ERROR] Cannot start periodic mode");
  putLog("SHT30 initialized.");

  // https://github.com/adafruit/Adafruit_BMP280_Library/blob/master/examples/bmp280test/bmp280test.ino
  if (!bmp.begin(MY_BMP280_ADDRESS)) {
    Serial.println(F("Could not find a valid BMP280 sensor, check wiring!"));
    while (1);
  }

  /* Default settings from datasheet. */
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */
  putLog("BMP280 initialized.");
}

// ホストIDは固定長だけれど何となく長めに取っておく。
char hostId[32];
void setupMackerel() {
  // TODO どっかにホストIDを永続化できるようになったら動的にホスト登録したいもんだ
  sprintf(hostId, "%s", mackerelHostId);
  mackerelClient.setHostId(hostId);
}

void setup() {
  // Grove for M5Stack CoreInk
  Wire.begin(32, 33);

  setupM5Ink();
  putLog("M5Ink initialized.");

  setupWiFi();
  putLog("Wi-Fi initialized.");

  setupTime();
  putLog("Time synced.");

  setupEnv();
  putLog("Env initialized.");

  setupMackerel();
  putLog("Mackerel initialized.");
}

void loop() {
  putLog("loop start.");
  updateEnv();
  flushEnv();
  flushTime();
  sendEnvToMackerel();
  putLog("loop end.");
  delay(60 * 1000);
}
