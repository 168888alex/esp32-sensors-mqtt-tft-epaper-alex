# ESP32 電子紙 MQTT 交接紀錄

更新日期：2026-09-21

## 目前使用專案

- 專案資料夾：`D:\esp32\40_epaper_mqtt`
- 主程式：`D:\esp32\40_epaper_mqtt\40_epaper_mqtt.ino`
- 本次因工作區未找到 `39_epaper_dht`，以已驗證的 `36_EPAPER` 電子紙感測器程式為基礎整合 MQTT。
- 最近一次上傳：COM5，ESP32-D0WD-V3，Hash verified，成功。

## 電子紙硬體設定

- 型號：微雪 Waveshare 2.9 吋三色電子紙 V4 / Rev2.1
- 解析度：296 x 128，程式採橫向座標配置
- DIN / MOSI：GPIO23
- SCK：GPIO18
- CS：GPIO27
- DC：GPIO26（由 `epd2in9b_V4` 驅動使用）
- RST：GPIO25（由 `epd2in9b_V4` 驅動使用）
- BUSY：GPIO34（由 `epd2in9b_V4` 驅動使用）
- SPI 初始化：`SPI.begin(18, -1, 23, 27)`
- 電子紙驅動檔：`epd2in9b_V4.cpp/.h`、`epdif.cpp/.h`

## 感測器與燈號腳位

- DHT11：GPIO14
- 光敏電阻：GPIO33，12-bit ADC，換算為 0～100%
- 綠色燈號：GPIO15
- 黃色燈號：GPIO2
- 紅色燈號：GPIO4

## 顯示與更新行為

- 電子紙顯示溫度、濕度、亮度三欄，各欄有圖示、數值與單位。
- DHT11 無效時溫度/濕度欄顯示紅色 `!`。
- 光敏電阻讀值無效時亮度欄顯示紅色 `!`。
- 電子紙感測資料每 60 秒讀取並更新一次。
- 最上方為紅底白字 `ESP32 SENSOR`，標題已往下調整避免上緣裁切。

## Wi-Fi 設定

- SSID：`Alex0116`
- Password：已寫入程式，但交接紀錄不明文保存；如需修改請編輯 `WIFI_PASSWORD`。
- 模式：`WIFI_STA`
- Wi-Fi 連線逾時：約 20 秒，斷線時會自動重連。

## MQTT 設定

- 函式庫：PubSubClient 2.8，已複製到 `D:\esp32\libraries\PubSubClient`
- Broker：`mqttgo.io`
- Port：`1883`（未加密 MQTT）
- Client ID：`esp32-epaper-` 加 ESP32 eFuse MAC 產生
- 感測資料上傳主題：`alex/class301/data`
- MQTT 重新連線檢查間隔：5 秒
- 感測資料發布時機：每次電子紙 60 秒更新後發布一次

感測資料 JSON 格式：

```json
{"temp":25,"humi":60,"light":75,"dhtValid":true,"lightValid":true}
```

感測器無效時對應數值為 `-1`，並以 `dhtValid` / `lightValid` 標示有效性。

## MQTT 燈號控制主題

| 燈號 | GPIO | MQTT topic |
|---|---:|---|
| 綠燈 | 15 | `alex/class301/ctrl/gled` |
| 黃燈 | 2 | `alex/class301/ctrl/yled` |
| 紅燈 | 4 | `alex/class301/ctrl/led4` |

控制 payload 可使用：

```json
{"state":"on"}
```

或：

```json
{"state":"off"}
```

程式也接受純文字 `on` / `off`。MQTT 連線成功後會訂閱以上三個控制主題。

## 編譯與上傳

```powershell
$cli='C:\Users\user\AppData\Local\Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe'
& $cli --config-dir 'C:\Users\user\AppData\Local\Arduino15' compile --fqbn 'esp32:esp32:esp32' --libraries 'D:\esp32\libraries' --output-dir 'D:\esp32\40_epaper_mqtt\build' 'D:\esp32\40_epaper_mqtt'
& $cli --config-dir 'C:\Users\user\AppData\Local\Arduino15' upload -p COM5 --fqbn 'esp32:esp32:esp32' --input-dir 'D:\esp32\40_epaper_mqtt\build'
```

## 後續檢查重點

1. 若電子紙畫面未更新，先確認 BUSY、RST、DC 及 SPI 腳位接線。
2. 若 Wi-Fi 連不上，確認 SSID 與 `WIFI_PASSWORD`。
3. 若 MQTT 無資料，確認 broker `mqttgo.io:1883` 可連線，並訂閱 `alex/class301/data`。
4. 若燈號不動，確認 GPIO15、GPIO2、GPIO4 的 LED 電路有效位準，並檢查控制 topic 與 payload。
5. MQTT 目前使用未加密 port 1883，正式部署時應評估帳號、密碼與 TLS。
