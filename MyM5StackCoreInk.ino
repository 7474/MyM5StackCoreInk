#include "M5CoreInk.h"

#include <Wire.h>
#include "Adafruit_BMP280.h"
#include "ClosedCube_SHT31D.h"

#include <WiFi.h>
#include "env.h"

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
Adafruit_BMP280 bmp; // I2C
// C
float temperature;
// Pa
uint32_t pressure;
// m
float altitude;

void flushEnv() {
  sprintf(flushStrBuf, "%2.2fC %4.1fhPa %4.0fm",
          temperature, pressure / 100.0f, altitude);
  InkPageSprite.drawString(5, 30, "BMP280");
  InkPageSprite.drawString(10, 45, flushStrBuf);
  InkPageSprite.drawString(5, 60, "SHT30");
  InkPageSprite.drawString(10, 75, "TODO");
  InkPageSprite.pushSprite();
}

void updateEnv() {
  temperature =  bmp.readTemperature();
  pressure = bmp.readPressure();
  // TODO get current seaLevelhPa
  altitude = bmp.readAltitude();
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
  putLog("M5Ink initialized.");

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  if (WiFi.begin(ssid, password) != WL_DISCONNECTED) {
    ESP.restart();
  }

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
  }
  putLog("Wi-Fi initialized.");

  setupTime();
  putLog("Time synced.");

  // Grove for M5Stack CoreInk
  Wire.begin(32, 33);

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

void scani2c() {
  byte error, address;
  int nDevices;

  Serial.println("Scanning...");

  nDevices = 0;
  for (address = 1; address < 127; address++ )
  {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0)
    {
      Serial.print("I2C device found at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.print(address, HEX);
      Serial.println("  !");

      nDevices++;
    }
    else if (error == 4)
    {
      Serial.print("Unknown error at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.println(address, HEX);
    }
  }
  if (nDevices == 0)
    Serial.println("No I2C devices found\n");
  else
    Serial.println("done\n");
}

void loop() {
  putLog("loop start.");
  updateEnv();
  flushEnv();
  flushTime();
  putLog("loop end.");
  //  scani2c();
  delay(15000);
}
