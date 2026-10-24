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
- 打印完成 / 出错提醒页(见下)
- D3 按钮:短按切换到下一屏;提醒页显示时短按关闭提醒
- 支持旋转显示(`INVERT_DISPLAY`)
- OctoPrint PSU Control 插件检测(仅 OctoPrint 客户端有效,Moonraker 不使用)

## 打印完成 / 出错提醒

打印机状态从"打印中 / 暂停"变为"完成"或"出错"时,屏幕会切换到全屏提醒页(仅 Moonraker 客户端支持):

| 类型 | 显示内容 | 闪烁 | 保持时间 |
|------|----------|------|----------|
| 完成 | `Print Complete`、文件名、总打印耗时 | 整屏反色闪 3 次后常亮 | 5 分钟 |
| 出错 | `PRINT ERROR`、Klipper 的错误信息(最多 60 字符)、文件名 | 整屏反色闪 1 分钟后常亮 | 15 分钟 |

- 配置了两台打印机时,文件名前会带 `1:` 或 `2:` 标明是哪一台。
- 关闭提醒:短按 D3 按钮、到时间自动关闭,或同一台打印机开始新的打印。关闭后自动回到时钟 / 打印页 / 休眠。
- 提醒显示期间,页面轮播、时钟切换和休眠都会暂停。
- 只有"打印中 / 暂停 → 完成 / 出错"的状态跳变才会触发:取消打印不提醒;开机时打印机本来就处于完成或出错状态,也不提醒。
- 打印中每 10 秒查询一次,所以提醒最多比实际晚约 10 秒出现。
- 出错信息来自 Klipper 的 `print_stats.message`,没有该信息时只显示文件名。
- Repetier 模式不支持提醒页。

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

### 可选外设

所有外设都占用独立引脚,互不影响;引脚可在 `Settings.h` 中修改。

| 外设 | 元件接法 | Wemos 引脚 | 启用方式 |
|------|----------|-----------|----------|
| 按钮 | 一端接引脚,另一端接 GND(内部上拉,不需要电阻) | D3 | 始终启用 |
| 压电片(无源) | 两根线分别接引脚和 GND(不需要电阻、三极管) | D6 | `#define USE_BUZZER` |
| 报警 LED(红色) | LED 正极(长脚)→ 220~330Ω 电阻 → 引脚;LED 负极(短脚)→ GND | D7 | `#define USE_ALERT_LED`(默认已启用) |

报警 LED 接线示意:

```
 D7 ──[ 220~330Ω ]──▶|── GND
                    LED
              (长脚朝电阻,短脚接 GND)
```

- **LED 极性**:长脚是正极,接电阻;短脚(外壳有平边的一侧)是负极,接 GND。接反不会烧毁,但不亮。
- **电阻不能省**:3.3V 下红色 LED 经 220Ω 约 6mA,在 ESP8266 单脚 12mA 的限度内,可以直接驱动。不加电阻会烧 LED 或 GPIO。
- **LED 行为**:仅在出错提醒时闪烁,与屏幕反色同步(0.4 秒一次),一直闪到提醒被关闭(屏幕 1 分钟后转为常亮,LED 不会停),按 D3 按钮关闭提醒即熄灭。可与压电片同时启用。

压电片行为:仅在出错提醒时响,前 20 秒与屏幕闪烁同步,每 0.4 秒一声,按 D3 关闭提醒即静音,完成提醒不响。请使用无源的压电片;有源蜂鸣器电流较大,需要三极管驱动,不适合这种直接接法。

按钮注意:D3 即 GPIO0,上电或复位时不要按住,否则会进入烧录模式。

**网页测试按钮**:启用了 LED 或压电片后,网页 **Configure** 页面底部会出现 "Hardware Test" 区域,两个按钮相互独立,可以不触发真实的打印出错就检查接线:
- **Test LED (blink 3 s)**:LED 每 0.4 秒一闪,持续 3 秒(仅启用 `USE_ALERT_LED` 时显示)。
- **Test Buzzer (3 beeps)**:响三声,每声 0.3 秒、间隔 0.3 秒(仅启用 `USE_BUZZER` 时显示)。

测试是非阻塞的,不影响屏幕和打印监视;设置了网页密码时需要登录才能使用。

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
| `#define USE_BUZZER` | 编译压电片蜂鸣支持(默认关闭,关闭时相关代码完全不参与编译) |
| `#define USE_ALERT_LED` | 编译外接红色报警 LED 支持(**默认启用**;未接 LED 时注释掉即可,关闭后相关代码不参与编译) |
| `ALERT_LED_PIN` | 报警 LED 引脚(默认 `D7`) |
| `BUZZER_PIN` / `BUZZER_FREQ_HZ` | 压电片引脚(默认 `D6`)和频率(默认 2700Hz) |

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
