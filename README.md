# Printer Monitor (Moonraker / Klipper 3D Printer Monitor)

基于 [Qrome/printer-monitor](https://github.com/Qrome/printer-monitor)(David Payne)的 fork。使用 Wemos D1 Mini(ESP8266)和 0.96" SSD1306 OLED,在小屏幕上显示 3D 打印机的打印状态。**默认对接 Klipper 的 Moonraker**,也可通过编译选项切换为 Repetier Server。

## 与原项目的主要区别

- **默认使用 Moonraker**(`/printer/objects/query`,端口 7125)。`USE_REPETIER_CLIENT` 可切换到 Repetier;`OctoPrintClient.cpp` 仍在仓库中,但主程序不再引用。
- **支持两台打印机**:第二台留空即禁用;两台同时打印时按 `secondsBetweenPrinterSwitch`(默认 30 秒)轮流显示。
- **天气改用 [Open-Meteo](https://open-meteo.com/)**:免费,无需 API Key,在网页中填城市名和经纬度即可。天气文字支持英语、匈牙利语、德语、法语、西班牙语。
- **时区使用 POSIX 规则 + NTP**:网页下拉框提供 30 个地区,夏令时自动处理。默认 `UTC+8`(Kuala Lumpur)。
- **PlatformIO 构建**,配置存储使用 **LittleFS**(原项目为 Arduino IDE + SPIFFS)。
- 显示改进:网络请求推迟到帧切换动画结束后执行,避免动画卡顿;每屏显示时间 15 秒。

## 功能

- 显示打印进度:百分比、进度条、已打印时间、预计剩余时间、热端/热床温度
- 打印机空闲时可显示时钟和天气(可选);也可选择直接关闭屏幕
- 12 / 24 小时制时钟
- 打印时每 10 秒查询一次,空闲时每 60 秒查询一次
- 通过 mDNS 主机名(默认 `mainsail`)自动查找打印机地址
- 全部设置可在网页中修改,无需改 `Settings.h`
- ArduinoOTA 无线烧录,可设置 OTA 密码
- 网页基本认证(默认 `admin` / `password`,**请修改**)
- 网页上传编译好的固件(`/update`)
- D3 按钮:短按切换到下一屏
- 支持旋转显示(`INVERT_DISPLAY`)
- OctoPrint PSU Control 插件检测(仅 OctoPrint 客户端有效,Moonraker 不使用)

## 硬件

- Wemos D1 Mini
- 0.96" I2C OLED 128x64(SSD1306)
- (可选)1.3" I2C OLED(SH1106):在 `Settings.h` 中取消注释 `#define DISPLAY_SH1106`

接线:

| OLED | Wemos D1 Mini |
|------|---------------|
| SDA  | D2 |
| SCL  | D1(`Settings.h` 当前默认值;Easy Monitor Board 也用 D1。若按原项目接线则改为 D5) |
| VCC  | 5V |
| GND  | GND |

按钮(可选):一端接 **D3**,另一端接 **GND**(使用内部上拉,无需外接电阻)。短按切换到下一屏。D3 即 GPIO0,上电或复位时不要按住,否则会进入烧录模式。引脚可通过 `Settings.h` 的 `BUTTON_PIN` 修改。

I2C 地址默认 `0x3c`(`I2C_DISPLAY_ADDRESS`)。

![接线图](/images/printer_monitor_wiring.jpg)

3D 打印外壳(Qrome 设计):
- 0.96" OLED:https://www.thingiverse.com/thing:2884823
- 1.3" OLED:https://www.thingiverse.com/thing:2934049

## 编译与烧录(PlatformIO)

项目在 `printermonitor/` 目录下,环境名为 `d1_mini`。

```bash
cd printermonitor
pio run                  # 编译
pio run -t upload        # USB 烧录
pio device monitor       # 串口监视器(115200)
```

`platformio.ini` 要点:`platform = espressif8266`、`board = d1_mini`、`board_build.filesystem = littlefs`。依赖会自动安装:

- `tzapu/WiFiManager` ^2.0.17
- `thingpulse/ESP8266 and ESP32 OLED driver for SSD1306 displays` ^4.6.1
- ArduinoJson 5.x 已内置在 `printermonitor/libs/ArduinoJson`

编译选项在 `Settings.h` 中修改:

| 选项 | 作用 |
|------|------|
| `#define USE_REPETIER_CLIENT` | 使用 Repetier Server(默认 Moonraker) |
| `#define DISPLAY_SH1106` | 使用 1.3" SH1106 屏(默认 SSD1306) |
| `SCL_PIN` | SCL 引脚(`D1` 或 `D5`) |

### 无线更新

首次烧录后,可在网页菜单 **Firmware Update** 上传 `pio run` 生成的 `.pio/build/d1_mini/firmware.bin`,或使用 ArduinoOTA。

## 初次配置

1. 首次启动(或找不到上次的 WiFi)时,设备会开启 **WiFiManager 热点**,用手机连接后填写 WiFi 信息。
2. 连上后屏幕会显示分配到的 IP,浏览器打开即可进入网页界面。
3. 在 **Configure** 中填写 Moonraker 地址(不要带 `http://`)、端口(默认 7125)、可选 API Key / 认证信息、第二台打印机、时区、时钟格式等。
4. 在 **Weather** 中填写城市名、纬度、经度、单位(公制 / 英制)和语言。

说明:`Settings.h` 里的值只是**首次启动的默认值**。之后的修改保存在设备文件系统(`/conf.txt`)中;若想重新使用 `Settings.h` 的值,需要擦除文件系统或在网页中使用 **Reset Settings**。

### 网页菜单

`/`(状态)、`/configure`、`/configureweather`、`/systemreset`、`/forgetwifi`、`/update`。

<p align="center">
  <img src="/images/shot_01.png" width="200"/>
  <img src="/images/shot_02.png" width="200"/>
  <img src="/images/shot_03.png" width="200"/>
  <img src="/images/shot_04.png" width="200"/>
</p>

## Original Project
This is a fork of [Qrome/printer-monitor](https://github.com/Qrome/printer-monitor) created by David Payne.
The original project is licensed under the MIT License.

### Original Contributors
* David Payne -- Principal developer and architect
* Daniel Eichhorn -- Author of the TimeClient class and OLEDDisplayUi
* Florian Schutte -- added flip display to web interface
* Owen Carter -- Added PSU control setting

## License

The MIT License (MIT)

Copyright (c) 2018 David Payne

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
