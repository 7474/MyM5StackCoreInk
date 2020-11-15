# MyM5StackCoreInk

koudenpa's M5Stack CoreInk.

It reads the temperature and humidity from the environmental sensor and sends it to [Mackerel](https://mackerel.io/).

Sending to Mackerel uses the [library](https://github.com/7474/ArduinoMackerelClient) I'm making now.

## Target

- [M5Stack CoreInk 開発キット（1.5インチ Einkディスプレイ）](https://www.switch-science.com/catalog/6735/)
- [M5Stack用環境センサユニット ver.2（ENV II）](https://www.switch-science.com/catalog/6344/)

## Setup

Set environment variables.
```
cp env.h.sample env.h
# And edit it.
```

## Thanks

- https://github.com/closedcube/ClosedCube_SHT31D_Arduino
- https://github.com/adafruit/Adafruit_BMP280_Library
