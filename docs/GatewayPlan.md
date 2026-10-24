# GatewayPlan.md — Thiết kế hệ thống IoT phát hiện cháy sớm (Node H2 + Gateway H2/WROOM)

| | |
|---|---|
| Phiên bản | 0.1 — bản nháp chờ người dùng duyệt |
| Ngày | 2026-10-17 |
| Vai trò soạn | System Architect |
| Người đọc | Các agent triển khai: HW, FW-NODE, FW-GW-H2, FW-GW-HOST, CLOUD, QA, DOC |
| Phạm vi | 1 Node + 1 Gateway (PoC), thiết kế để mở rộng thêm Node |

---

## 0. Đọc trước khi làm bất cứ việc gì

### 0.1 Quy ước thẻ
| Thẻ | Nghĩa |
|---|---|
| `[DECIDED]` | Đã chốt. Không đổi nếu không cập nhật Decision Log (§2.4). |
| `[VERIFY]` | Thông tin chưa kiểm chứng trên phần cứng thật. Phải xác nhận trong Phase P0 và ghi kết quả vào `hardware/pinmap_verified.md`. |
| `[PLACEHOLDER]` | Giá trị khởi đầu, **phải** được hiệu chỉnh bằng thực nghiệm. Không được coi là kết luận khoa học. |
| `[DECISION-NEEDED]` | Cần người dùng quyết định (§14). Agent không tự quyết. |

### 0.2 Nguyên tắc làm việc
1. **Contract-first.** Các hợp đồng giao diện ở §4 (Zigbee), §5.1 (UART), §5.2 (MQTT) là bất biến trong lúc triển khai. Muốn đổi phải sửa tài liệu này trước, rồi mới sửa code.
2. **Fail-safe.** Mất liên lạc không bao giờ được hiển thị như "an toàn". Xem §7.5 và §8.
3. **Báo động cục bộ không phụ thuộc mạng.** LED/còi hoạt động được khi WiFi/MQTT chết (§6.3).
4. **Không invent số liệu.** Mọi ngưỡng đánh dấu `[PLACEHOLDER]` phải có log thực nghiệm đi kèm trước khi được ghi vào báo cáo.
5. **Giữ phần cứng đơn giản.** Dùng DevKit/module có sẵn chân, nối dây/perfboard. Không thiết kế PCB trừ khi `[DECISION-NEEDED] D6` được duyệt.

### 0.3 Nguồn tham chiếu trong project
`datn_nguyentrantien_20222184__3_07m.pdf` là một đồ án đã hoàn thiện dùng **cùng cặp phần cứng ESP32-H2 + ESP32 WROOM-32**, nhưng với **Thread/OpenThread** (H2 làm RCP qua UART/Spinel, WROOM-32 làm Border Router), không phải Zigbee.

| Dùng được | Không dùng được |
|---|---|
| Xác nhận WROOM-32 + H2 nối UART chạy ổn trong thực tế (UART2 = GPIO16/17 trên WROOM-32) | Số liệu hiệu năng Thread (join 170 ms, tự phục hồi < 2 s, trễ 0,5–1,2 s) **không** chứng minh được hiệu năng Zigbee của ta |
| MQTT: HiveMQ Cloud, TLS cổng 8883, QoS 1, topic `sensor/<node_id>`, dashboard subscribe `sensor/#` qua WSS | Kiến trúc RCP/Spinel: ta **không** dùng (H2 của ta chạy trọn stack Zigbee coordinator) |
| Cách xử lý CCS811 (nWAKE, bù nhiệt-ẩm bằng SHT31) | Mạch nguồn 3 pin AA / diode-OR 12 V: đã thay bằng §3.4 |
| Ngưỡng nhiệt 65 °C, xóa cảnh báo khi eCO₂ < 950 ppm, debounce 3 lần đọc, chu kỳ báo 10 s thường / 3 s khi có sự cố | |
| ESP-IDF v5.5.4 trên Windows + VS Code là môi trường đã chạy được với H2 | |
| Dashboard (HTML/JS/Chart.js) và bot Telegram trên Railway: có thể tái sử dụng nếu có mã nguồn `[DECISION-NEEDED] D5` | |

### 0.4 Các giả định bị thách thức (người dùng đã chốt, nhưng agent cần biết rủi ro)
1. **MQ-2 không phải cảm biến khói đúng nghĩa.** Nó là cảm biến khí (SnO₂), kém nhạy với hạt khói của cháy âm ỉ, cần sấy 24–48 h, heater 5 V ≈ 150 mA. Chính MQ-2 là lý do hệ thống phải có nguồn 5 V + dự phòng phức tạp. Đồ án tham chiếu dùng buồng khói quang điện (RE46C194) để tránh vấn đề này. → Test plan có thí nghiệm đối chứng (T-SYS-06). `[DECISION-NEEDED] D3`.
2. **Flame sensor (IR)** dễ báo giả do ánh nắng/đèn sợi đốt và **mù** với giai đoạn ngún, đúng giai đoạn đề tài muốn bắt sớm. → Chỉ dùng như tín hiệu xác nhận, không đủ để báo cháy một mình (§7.3).
3. **eCO₂/TVOC của CCS811 là giá trị ước tính** từ VOC, có trôi đáy, cần 20 phút ổn định mỗi lần bật và ~48 h burn-in lần đầu.
4. **Đồ án nói "phát hiện sớm bằng kết hợp nhiều cảm biến"** thì phải *chứng minh bằng số*: so fusion với từng cảm biến đơn lẻ về độ trễ phát hiện và số báo giả (T-SYS-05/06). Không có phép so sánh này, "thuật toán fusion" chỉ là tuyên bố.
5. **Gateway H2 chạy Zigbee coordinator bằng esp-zigbee-sdk** là đường ít người đi hơn Zigbee2MQTT. Do đó có cổng kiểm tra G0 (§10) và phương án lùi (§15).

---

## 1. Mục tiêu và phạm vi

### 1.1 Mục tiêu
Hệ thống báo cháy không dây giai đoạn sớm: Node đo nhiệt độ, độ ẩm, VOC/eCO₂, khí/khói, lửa → gửi Zigbee → Gateway hợp nhất dữ liệu, báo động tại chỗ (LED + còi) và đẩy lên cloud (dashboard + Telegram).

### 1.2 Yêu cầu chức năng (FR)
| ID | Yêu cầu |
|---|---|
| FR-1 | Node đo SHT31, CCS811, MQ-2, Flame và gửi định kỳ + theo sự kiện |
| FR-2 | Gateway nhận dữ liệu Zigbee, chạy fusion, xác định trạng thái NORMAL / CAUTION / WARNING / FIRE |
| FR-3 | Báo động tại chỗ: LED đỏ/vàng/xanh + còi active, có nút tắt tiếng |
| FR-4 | Publish MQTT lên HiveMQ Cloud; dashboard và Telegram nhận được |
| FR-5 | Phát hiện và báo lỗi: Node mất liên lạc, H2↔WROOM mất liên lạc, cảm biến hỏng, mất điện lưới |
| FR-6 | Chạy tiếp bằng pin khi mất điện lưới (Node và Gateway) |
| FR-7 | Tự phục hồi sau reset/mất điện/mất WiFi mà không cần can thiệp tay |

### 1.3 Yêu cầu phi chức năng (NFR) — mốc nghiệm thu
| ID | Chỉ tiêu | Giá trị |
|---|---|---|
| NFR-1 | Báo động tại chỗ sau khi Node phát report có điều kiện cháy | ≤ 3 s (không tính thời gian tích lũy xác nhận, §7.4) |
| NFR-2 | Cảm biến → hiển thị cảnh báo trên dashboard | ≤ 5 s (đồ án tham chiếu cũng đặt 5 s) |
| NFR-3 | Cảm biến → tin Telegram | ≤ 10 s |
| NFR-4 | Tỷ lệ gói Zigbee nhận được (PDR) trong 1 h, Node cách Gateway 3 m, 1 vách | ≥ 99 % |
| NFR-5 | Tự phục hồi sau mất nguồn Node hoặc Gateway | ≤ 60 s về trạng thái hoạt động bình thường |
| NFR-6 | Chạy pin dự phòng | ≥ 2 h (đạt), mục tiêu ≥ 6 h `[VERIFY]` |
| NFR-7 | Không có báo giả FIRE trong 24 h vận hành bình thường (kể cả nấu ăn, hơi nước, máy sấy tóc) | 0 lần FIRE |

### 1.4 Ngoài phạm vi (v1)
OTA, nhiều Node, mesh router, ứng dụng di động, chứng nhận chuẩn PCCC, PCB tùy chế.

---

## 2. Kiến trúc

### 2.1 Sơ đồ khối
```
 NODE_01                                GATEWAY                                          CLOUD
┌─────────────────────┐   Zigbee    ┌──────────────────────────────────────────┐
│ ESP32-H2 SuperMini  │  802.15.4   │ GW-H2: ESP32-H2 SuperMini                │
│  SHT31   (I2C)      │ ──────────► │   Zigbee Coordinator (esp-zigbee-sdk)    │
│  CCS811  (I2C)      │  ch.25      │   gom report → NDJSON                    │
│  MQ-2    (ADC)      │             │        │ UART1 115200 8N1 (TX,RX,GND)    │
│  Flame   (ADC+DO)   │             │        ▼                                 │      ┌───────────────┐
│ 5V adapter + UPS    │             │ GW-HOST: ESP32 WROOM-32 DevKit           │ WiFi │ HiveMQ Cloud  │
└─────────────────────┘             │   fusion + state machine + alarm         │ ───► │ TLS :8883     │
                                    │   WiFi/MQTT, NTP                         │      └──────┬────────┘
                                    │   LED R/Y/G · còi active · nút mute      │             │ WSS / MQTT
                                    │ 5V adapter + UPS                         │      Dashboard · Telegram bot
                                    └──────────────────────────────────────────┘
```

### 2.2 Vai trò từng khối
| Khối | Vai trò | **Không** làm |
|---|---|---|
| Node (H2) | Đọc 4 cảm biến, đóng gói thành attribute Zigbee, tự tăng tần suất báo khi nghi ngờ | Không quyết định "cháy". Không giữ lại/lọc dữ liệu |
| GW-H2 | Coordinator: lập mạng, cho Node join, nhận report, gom thành 1 dòng JSON/Node, gửi UART, phát heartbeat | Không fusion, không WiFi (H2 không có WiFi) |
| GW-HOST (WROOM-32) | Parse UART, fusion, state machine, điều khiển LED/còi, WiFi, MQTT, NTP, lệnh điều khiển | Không chạm vào stack Zigbee |
| Cloud | Phân phối, hiển thị, cảnh báo từ xa | Không là nguồn chân lý của trạng thái cháy; trạng thái do GW-HOST quyết |

### 2.3 Vì sao 2 chip tách biệt (lý do kỹ thuật)
- ESP32-C6 có **một radio 2.4 GHz dùng chung** cho WiFi và 802.15.4 (chia thời gian). Cộng đồng và tài liệu Espressif ghi nhận vai trò "coordinator chạy liên tục + WiFi" là *supported nhưng không ổn định*. H2 không có WiFi nên không bị tranh chấp radio.
- Zigbee2MQTT cần Linux + Node.js, không chạy được trên ESP32. Vì vậy không dùng CC2531/Zigbee2MQTT.
- Đồ án tham chiếu đã chứng minh cặp WROOM-32 + H2 qua UART hoạt động; ta kế thừa cấu hình vật lý đó (UART2 trên WROOM-32).

### 2.4 Decision Log
| ID | Quyết định | Trạng thái | Lý do / ghi chú |
|---|---|---|---|
| D-01 | Giao thức Node↔Gateway: Zigbee 3.0 | `[DECIDED]` bởi người dùng | Fallback Thread: §15 |
| D-02 | Gateway = H2 (coordinator) + WROOM-32 (host) | `[DECIDED]` | §2.3 |
| D-03 | Bỏ CC2531, Raspberry Pi, Zigbee2MQTT | `[DECIDED]` | Không chạy được trên ESP32 / tiết kiệm ~500k |
| D-04 | Node gửi **dữ liệu thô**; fusion chạy trên GW-HOST | `[DECIDED]` | Một nơi duy nhất chứa logic; sửa tham số không cần nạp lại Node |
| D-05 | Node là Zigbee End Device **không ngủ** (cấp nguồn lưới) | `[DECIDED]` | Đơn giản hơn router/sleepy; có ví dụ chính thức |
| D-06 | UART H2↔WROOM: NDJSON 115200 8N1 | `[DECIDED]` | Dễ debug bằng terminal |
| D-07 | Dùng DevKit/SuperMini + nối dây, không PCB | `[DECIDED]` bởi người dùng | `[DECISION-NEEDED] D6` nếu GVHD yêu cầu PCB |
| D-08 | Nguồn: adapter 5 V + module UPS 1S 18650 cho cả 2 thiết bị | **Chờ duyệt** `[DECISION-NEEDED] D1` | Thay thiết kế dual-power cũ vì lý do ở §3.4 |
| D-09 | Hợp đồng MQTT tương thích đồ án tham chiếu (§5.2) | `[DECIDED]` | Tái sử dụng dashboard/bot |
| D-10 | Còi active; LED nhấp nháy theo mức | `[DECIDED]` bởi người dùng | §8 |
| D-11 | Board: **ESP32-H2 SuperMini** cho Node và GW-H2; **ESP32 DevKit WROOM-32 (38 chân)** cho GW-HOST | `[DECIDED]` | §3.1 |
| D-12 | Toolchain: GW-H2 = ESP-IDF 5.5.x; Node và GW-HOST = Arduino-ESP32 3.x (Node có thể chuyển ESP-IDF) | `[DECIDED]` | §9 |

---

## 3. Phần cứng

### 3.1 Chọn board
| Vị trí | Board | Lý do |
|---|---|---|
| Node, GW-H2 | **ESP32-H2 SuperMini** (loại hàn sẵn header) | Cùng chip H2 như DevKit; có đúng 5 chân ADC (GPIO1–5) đủ cho Node; USB-C native; giá rẻ (~6 EUR) |
| GW-HOST | **ESP32 DevKit WROOM-32, 38 chân**, có CP2102/CH340 | Có UART2 (GPIO16/17), nhiều GPIO an toàn, nạp bằng USB dễ |

Lưu ý SuperMini:
- **Không có chip USB-UART**: Serial qua USB-CDC. Arduino IDE: bật `USB CDC On Boot`. `[VERIFY]`
- Lần nạp đầu thường phải **giữ nút BOOT** khi cắm USB-C.
- Chip H2 **không có WiFi**; không đưa 5 V vào GPIO (3,3 V logic).
- Pinout khác nhau giữa các nhà sản xuất. Đối chiếu silkscreen của board **thực tế** với §3.2 trước khi đấu dây `[VERIFY]`.
- Một số biến thể có đầu nối pin Li-ion + mạch sạc. **Để trống đầu nối này.** Không cấp 5 V vào chân `5V` đồng thời với USB-C nếu sơ đồ board không có diode chống ngược `[VERIFY]`: khi nạp firmware, rút nguồn 5 V ngoài.
- Mua **4 board H2** (2 dùng + 2 dự phòng). Có thể thêm 1 board **ESP32-H2-DevKitM-1 chính hãng** làm mốc chuẩn khi nghi ngờ lỗi clone (tùy chọn).

### 3.2 Node — BOM và đấu dây
| # | Linh kiện | Ghi chú |
|---|---|---|
| 1 | ESP32-H2 SuperMini | |
| 2 | SHT31 breakout (địa chỉ 0x44) | ADDR → GND |
| 3 | CCS811 breakout (địa chỉ 0x5A) | **nWAKE nối GND cố định** (Node cấp điện lưới, không cần ngủ). nRESET để trống |
| 4 | MQ-2 module 4 chân (VCC/GND/AO/DO) | Cấp 5 V, heater ~150 mA, sấy 24–48 h |
| 5 | Flame sensor module LM393 (VCC/GND/AO/DO) | Cấp 5 V |
| 6 | 2 × điện trở 22 kΩ + 1 × 100 nF cho mỗi đường analog (MQ-2, Flame) | Chia áp 1:2 |
| 7 | Tụ 470 µF/10 V trên rail 5 V gần MQ-2 | `[PLACEHOLDER]` giảm sụt áp khi heater bật |
| 8 | Nguồn: §3.4 | |

**Pin map Node** `[VERIFY]` (tránh GPIO2, 3, 8, 9, 25 là chân strapping; GPIO26/27 là USB; GPIO23/24 mặc định UART0):
| Chức năng | GPIO | Ghi chú |
|---|---|---|
| ADC MQ-2 (sau chia áp) | **GPIO1** | ADC1 |
| ADC Flame AO (sau chia áp) | **GPIO4** | ADC1 |
| ADC dự trữ | GPIO5 | |
| I2C SDA | **GPIO10** | SHT31 + CCS811 dùng chung bus |
| I2C SCL | **GPIO11** | |
| Flame DO (tùy chọn, đường nhanh) | GPIO14 | ngắt |
| SHT31 ALERT (tùy chọn, đường nhanh) | GPIO22 | ngắt phần cứng khi quá ngưỡng nhiệt |
| LED trạng thái | LED onboard | `[VERIFY]` GPIO13 trên nhiều SuperMini |

**Chia áp analog** (áp dụng cho cả MQ-2 và Flame, bất kể nguồn cấp):
```
AO ──[22k]──┬──[22k]── GND        node giữa → GPIOx và → [100nF] → GND
            └────────► ADC
Hệ số: DIV_RATIO = 2.0  →  mV_sensor = mV_adc × 2.0   (AO tối đa 5 V → 2,5 V tại ADC)
```
- ADC của H2 là 12 bit, vùng tuyến tính tốt tới ~3,1 V ở suy hao 12 dB. Dùng `analogReadMilliVolts()` (có hiệu chuẩn eFuse), trung bình 16 mẫu. `[VERIFY]`
- Nhiều breakout SHT31/CCS811 đã có điện trở kéo I2C sẵn. **Kiểm tra trước**; chỉ thêm 4,7 kΩ ngoài nếu bus không có kéo lên. Điện trở kéo tổng hợp không nhỏ hơn ~1,5 kΩ.
- I2C chạy 100 kHz. CCS811 dùng clock stretching, có báo cáo lỗi giao tiếp trên một số dòng ESP32 `[VERIFY]` trong P0: nếu lỗi, hạ xuống 50 kHz hoặc dùng I2C bit-bang.
- **Thứ tự quan trọng với CCS811:** nWAKE phải LOW mới giao tiếp được; sau khi bật phải chờ ổn định; ghi `ENV_DATA` (nhiệt độ, độ ẩm từ SHT31) mỗi chu kỳ để bù.

### 3.3 Gateway — BOM và đấu dây
| # | Linh kiện | Ghi chú |
|---|---|---|
| 1 | ESP32-H2 SuperMini (GW-H2) | |
| 2 | ESP32 DevKit WROOM-32 38 chân (GW-HOST) | |
| 3 | 3 LED 5 mm đỏ/vàng/xanh + 3 điện trở 220 Ω | ~6 mA mỗi LED, nằm trong giới hạn an toàn của GPIO; không cần transistor |
| 4 | **Còi active** loại module 3 chân (VCC/GND/I/O) có sẵn transistor | **Kiểm tra mức kích: low-level hay high-level trigger** (§3.3.1) |
| 5 | 1 nút nhấn | Mute / test |
| 6 | 2 điện trở 10 kΩ + 15 kΩ | Chia áp cảm biến có điện lưới (PWR_SENSE) |
| 7 | Nguồn: §3.4 | |

**Pin map GW-H2** `[VERIFY]`
| Chức năng | GPIO | Ghi chú |
|---|---|---|
| UART1 TX → WROOM RX | **GPIO10** | qua điện trở nối tiếp 220 Ω (tùy chọn) |
| UART1 RX ← WROOM TX | **GPIO11** | |
| LED trạng thái (cửa sổ join) | LED onboard | |

**Pin map GW-HOST** (WROOM-32, tránh GPIO 0, 2, 12, 15 là strapping; GPIO34–39 chỉ input)
| Chức năng | GPIO | Ghi chú |
|---|---|---|
| UART2 RX ← H2 TX (GPIO10) | **GPIO16** | |
| UART2 TX → H2 RX (GPIO11) | **GPIO17** | |
| LED đỏ (FIRE) | GPIO25 | → 220 Ω → LED → GND |
| LED vàng (CAUTION/WARNING/FAULT) | GPIO26 | |
| LED xanh (NORMAL) | GPIO27 | |
| Còi | **GPIO33** | không có glitch lúc boot |
| Nút mute/test | GPIO32 | `INPUT_PULLUP`, nhấn = LOW |
| PWR_SENSE (có điện lưới) | GPIO35 | chỉ input, ADC1; chia áp từ 5 V adapter (**trước** module UPS) |

```
Đấu dây UART (chéo TX/RX, chung GND, cả hai 3,3 V logic):
  GW-H2 GPIO10(TX) ──►── GPIO16(RX2) GW-HOST
  GW-H2 GPIO11(RX) ──◄── GPIO17(TX2) GW-HOST
  GND ───────────────────  GND

PWR_SENSE:  5V_ADAPTER ─[10k]─┬─[15k]─ GND ;  node giữa → GPIO35   (5 V → 3,0 V)
```

#### 3.3.1 Còi active: các bẫy cần tránh
- Module còi 5 V thường là **low-level trigger** (kêu khi chân I/O = LOW). Nếu không xử lý, còi kêu ngay lúc boot. → Đặt mức tắt vào GPIO33 **trước** mọi khởi tạo khác. Hằng số cấu hình `BUZZER_ACTIVE_LEVEL`.
- Nếu dùng còi trần 2 chân thay module: NPN 2N2222/S8050, điện trở nền 1 kΩ, và **diode 1N4148 ngược chiều song song còi** nếu còi là loại điện từ (có cuộn dây).
- Còi active chỉ ON/OFF; âm sắc khác nhau thể hiện bằng **nhịp bíp** (§8), không bằng tần số.

### 3.4 Nguồn — thiết kế đơn giản hóa `[DECISION-NEEDED] D1`
**Cả Node và Gateway dùng cùng một chuỗi:**
```
Adapter 5 V / 2 A ──► [Module UPS 1S 18650 (IP5306: sạc + boost 5 V, có power-path)] ──► rail 5 V ──► chân 5V/VIN của board
                                                  │
                                       1 × pin 18650 có mạch bảo vệ (PCM)
Tap PWR_SENSE (chỉ Gateway) lấy từ đầu vào adapter, trước module UPS.
```
**Vì sao thay thiết kế dual-power 2S/3S + diode-OR đã bàn trước đó:**
1. **TP4056 và IP5306 chỉ dùng cho 1 cell Li-ion (1S).** Không thể sạc hay tăng áp từ 2–3 cell nối tiếp như tài liệu cũ nêu. Giữ 2S/3S buộc phải dùng bộ sạc cân bằng + BMS riêng: phức tạp, trái với nguyên tắc "đơn giản".
2. SuperMini và DevKit WROOM-32 đều nhận 5 V và có LDO 3,3 V onboard → **không cần** buck 12 V→5 V (TPS54321) hay AMS1117 riêng.
3. IP5306 có power-path (cấp tải từ đầu vào khi cắm điện, tự chuyển sang boost từ pin khi mất điện) → **không cần diode-OR** tự chế và không cần tính Vf Schottky.

**Hạn chế đã biết của IP5306, phải kiểm thử:**
- **Tự tắt đầu ra** khi dòng tải < ~45–50 mA kéo dài ~32 s. Node (MQ-2 heater ~150 mA) và Gateway (WiFi) đều vượt ngưỡng này, nhưng **phải đo dòng thực** (T-PWR-03). Nếu thấp hơn: thêm tải giả hoặc dùng biến thể I2C cho phép tắt tính năng này.
- Có thể **sụt áp thoáng qua** khi chuyển từ điện lưới sang pin. Thêm tụ 470–1000 µF sau module UPS nếu ESP32 reset (T-PWR-02).
- Module UPS giá rẻ **thường không có bảo vệ pin đầy đủ.** Dùng cell 18650 **có mạch bảo vệ**, mua từ nguồn đáng tin; không sạc không giám sát khi đang dev.

**Ước lượng thời gian chạy pin** `[VERIFY]` (đo thực tế thay cho công thức này):
`t ≈ (3,7 V × C_mAh × 0,85) / (5 V × I_tải_mA)`; ví dụ 3000 mAh, tải 200 mA → khoảng 9 h. Đo dòng thực của Node (tải lớn nhất là MQ-2) và Gateway.

### 3.5 Quy tắc RF và bố trí (dù là nối dây)
- GW-H2 và GW-HOST **cách nhau ≥ 10 cm**, anten hướng ra hai phía khác nhau; cả hai đều ở 2,4 GHz.
- Không đặt anten gần kim loại, pin, hoặc dây nguồn dài. MQ-2 nóng: tránh đặt sát H2 và SHT31 (nhiệt làm lệch đo nhiệt độ).
- Đặt SHT31 xa MQ-2 và xa mạch UPS (nóng) để đo nhiệt chính xác.
- Chọn kênh Zigbee ≥ 25 MHz cách kênh WiFi của router (§4.1).

---

## 4. Mạng Zigbee và mô hình dữ liệu (HỢP ĐỒNG BẤT BIẾN)

### 4.1 Tham số mạng
| Tham số | Giá trị | Ghi chú |
|---|---|---|
| Vai trò GW-H2 | Coordinator | |
| Vai trò Node | End Device, rx-on-when-idle (không ngủ) | D-05 |
| Kênh | **25** (hằng số `ZB_CHANNEL`) | Sạch với WiFi kênh 1–11. **Nếu router dùng kênh 13**, kênh 25/26 bị chồng: đổi sang 15 hoặc 20. Kiểm tra bằng WiFi scanner `[VERIFY]` |
| PAN ID | cố định (`ZB_PAN_ID`) | Tránh trùng mạng khác |
| Bảo mật | Zigbee 3.0 mặc định, khóa mạng ngẫu nhiên lưu NVS | Dùng cho PoC; ghi rõ giới hạn trong báo cáo |
| Cửa sổ join | **Đóng** mặc định. Mở 180 s khi: (a) boot lần đầu chưa có Node nào, (b) có lệnh `permit_join`, (c) giữ nút 5 s | Tránh để mạng mở vĩnh viễn |
| Quy tắc khi Node không join được | Xóa flash Node (erase) rồi nạp lại; xóa NVS coordinator nếu đổi mạng | Lỗi phổ biến theo tài liệu Arduino-ESP32 |

### 4.2 Endpoint Node (hợp đồng)
Cài đặt có thể dùng lớp Arduino-ESP32 (`ZigbeeTempSensor`, `ZigbeeAnalog`, …) hoặc esp-zigbee-sdk trực tiếp `[VERIFY]`; **hợp đồng** là bảng sau:

| EP | Cluster | Attribute | Kiểu / đơn vị |
|---|---|---|---|
| 10 | Temperature Measurement 0x0402 | MeasuredValue | int16, 0,01 °C |
| 10 | Relative Humidity 0x0405 | MeasuredValue | uint16, 0,01 %RH |
| 11 | Analog Input 0x000C | PresentValue | float, **mq2_mv** (mV tại ngõ ra cảm biến, đã nhân DIV_RATIO) |
| 12 | Analog Input 0x000C | PresentValue | float, **flame_mv** |
| 13 | Analog Input 0x000C | PresentValue | float, **eco2_ppm** |
| 14 | Analog Input 0x000C | PresentValue | float, **tvoc_ppb** |
| 15 | Analog Input 0x000C | PresentValue | float, **flags** (bitfield, §4.3) |

Mỗi Analog Input đặt `Description` = tên trường ở trên.

### 4.3 Bitfield `flags` (EP15)
| Bit | Nghĩa |
|---|---|
| 0 | SHT31 lỗi đọc |
| 1 | CCS811 lỗi đọc / đang ổn định |
| 2 | MQ-2 đang sấy (chưa đáng tin) |
| 3 | Flame DO đang kích |
| 4 | Pre-alert cục bộ đang bật (Node đang báo 3 s) |
| 5 | Giá trị ADC chạm trần/đáy (cảm biến mất kết nối) |

### 4.4 Chính sách báo cáo của Node
| Trạng thái | Chu kỳ | Điều kiện chuyển |
|---|---|---|
| Thường | **10 s** | |
| Pre-alert | **3 s**, và gửi ngay khi vào pre-alert | Vào khi thỏa **bất kỳ**: T ≥ 45 °C, dT/dt ≥ 5 °C/phút, mq2 tăng ≥ 50 % so với nền, Flame DO kích, eCO₂ ≥ 1000 ppm `[PLACEHOLDER]` |
| Thoát pre-alert | | 3 lần đọc liên tiếp dưới ngưỡng (debounce, theo đồ án tham chiếu) |

**Quy tắc cứng:** ngưỡng pre-alert của Node phải **thấp hơn hoặc bằng** ngưỡng CAUTION của Gateway. Node chỉ đổi *tốc độ báo*, không bao giờ chặn hay làm mất một report.

Mỗi report gửi đủ EP10–15 trong một loạt (burst) < 300 ms.

### 4.5 Quy trình join và nhận diện
1. Node mở, join vào mạng có kênh đã định, lưu thông tin mạng vào NVS.
2. GW-H2 phát `{"t":"join",...}` qua UART. GW-HOST gán `node_id` theo bảng `nodes_map` (địa chỉ IEEE → tên). Node lạ → `Node_<4 ký tự cuối IEEE>`.
3. Sau mất điện, Node tự rejoin bằng thông tin NVS (kỳ vọng vài giây `[VERIFY]`).

---

## 5. Hợp đồng giao diện

### 5.1 UART GW-H2 ↔ GW-HOST (NDJSON)
- 115200 baud, 8N1, mỗi bản tin là **một dòng JSON kết thúc `\n`**, ≤ 256 byte. Dòng parse lỗi → bỏ qua và tăng bộ đếm `parse_err`.
- Mức logic 3,3 V cả hai phía.

**H2 → HOST**
```json
{"t":"hello","fw":"gwh2-0.1.0","ch":25,"pan":"0x1A62"}
{"t":"hb","up":123,"nodes":1}
{"t":"join","ieee":"0011223344556677","sa":"0x1234"}
{"t":"leave","ieee":"0011223344556677"}
{"t":"rpt","ieee":"0011223344556677","seq":42,"temp":28.4,"hum":61.2,"eco2":410,"tvoc":5,"mq2_mv":1234.0,"flame_mv":4980.0,"flags":0,"rssi":-60,"lqi":200}
```
- `hello`: gửi khi boot. `hb`: **mỗi 2 s**.
- `rpt`: GW-H2 giữ cache theo Node, **gom** các attribute đến trong cửa sổ 300 ms thành **một** `rpt` mỗi Node; `seq` tăng dần để đo mất gói.

**HOST → H2**
```json
{"t":"cmd","op":"permit_join","sec":180}
{"t":"cmd","op":"reset_net"}
{"t":"cmd","op":"ping"}
```
- H2 đáp `{"t":"ack","op":"..."}`.

**Giám sát liên kết:** HOST coi H2 mất liên lạc nếu không nhận `hb` trong **6 s** → trạng thái lỗi `H2_LINK_LOST` (§7.5).

### 5.2 MQTT (HiveMQ Cloud) — tương thích đồ án tham chiếu
- Kết nối **TLS, cổng 8883**, chứng chỉ CA của HiveMQ (ISRG Root X1). Thông tin đăng nhập nằm trong `secrets.h` (**git-ignore**, không commit).
- Client ID: `gateway-01`.

| Topic | Hướng | QoS | Retain | Nội dung |
|---|---|---|---|---|
| `sensor/<node_id>` | GW → cloud | 1 | không | Telemetry mỗi report |
| `alert/<node_id>` | GW → cloud | 1 | không | **Sự kiện** khi trạng thái đổi |
| `status/gateway` | GW → cloud | 1 | **có** | Sức khỏe Gateway; **LWT** `{"online":false}` |
| `cmd/gateway` | cloud → GW | 1 | không | Lệnh điều khiển |

**`sensor/<node_id>`** — giữ nguyên các khóa của đồ án tham chiếu (`node_id`, `temperature`, `humidity`, `co2`, `tvoc`, `last_seen`), thêm khóa mới:
```json
{"node_id":"Node_01","temperature":28.4,"humidity":61.2,"co2":410,"tvoc":5,
 "mq2_mv":1234.0,"flame_mv":4980.0,"flags":0,"rssi":-60,
 "score":0.12,"state":"NORMAL","fault":0,"last_seen":"<xem D5>"}
```
**`alert/<node_id>`**
```json
{"node_id":"Node_01","state":"FIRE","prev":"WARNING","score":0.71,
 "reason":["score","flame_confirmed"],"temperature":68.5,"ts":1760000000}
```
**`status/gateway`**
```json
{"online":true,"fw":"host-0.1.0","power":"mains","zb_ch":25,"uptime":3600,
 "h2_link":"ok","nodes":{"Node_01":"online"},"mute":false}
```
**`cmd/gateway`**
```json
{"cmd":"permit_join","sec":180}   {"cmd":"mute","sec":300}   {"cmd":"test_alarm"}   {"cmd":"reset_latch"}
```
- `last_seen`: định dạng phải khớp bộ parse của dashboard tham chiếu. **Kiểm tra mã JS của dashboard trước khi chốt** `[DECISION-NEEDED] D5`. Mặc định nếu không có dashboard cũ: epoch giây (số nguyên).
- Thời gian lấy từ NTP. Chưa đồng bộ NTP → dùng `uptime` và đặt `"ts":0`, **không** bịa thời gian.

---

## 6. Thiết kế firmware

### 6.1 Node (FW-NODE)
Vòng lặp không chặn (không `delay()` dài), tick 100 ms:
```
boot: init I2C → SHT31 → CCS811 (nWAKE đã nối GND) → ADC → Zigbee ED join
loop:
  mỗi 1 s : đọc SHT31, ADC (mq2, flame), CCS811 khi sẵn sàng (data-ready)
            ghi ENV_DATA vào CCS811 từ SHT31
            cập nhật flags (§4.3), dT/dt, đánh giá pre-alert (§4.4)
  mỗi chu kỳ báo (10 s | 3 s): cập nhật attribute EP10–15 và gửi report
  ngắt GPIO14 / GPIO22 (nếu bật): vào pre-alert + gửi ngay
  watchdog: TWDT 10 s; mất kết nối coordinator → thử rejoin (backoff 5→60 s)
```
- Cảm biến lỗi: đặt cờ tương ứng, **vẫn tiếp tục gửi** phần còn lại. Không treo.
- MQ-2 sấy: bit `flags[2]=1` trong **24 h đầu cộng dồn** (đếm lưu NVS) hoặc tới khi người vận hành xác nhận qua cấu hình `[PLACEHOLDER]`.

### 6.2 GW-H2 (FW-GW-H2, ESP-IDF)
- Khởi tạo NVS; lập mạng coordinator trên `ZB_CHANNEL`; quản lý cửa sổ join (§4.1).
- Đăng ký handler báo cáo attribute (kiểu `ESP_ZB_CORE_REPORT_ATTR_CB_ID` của esp-zigbee-sdk `[VERIFY]` theo phiên bản SDK dùng); gom theo Node trong cửa sổ 300 ms → `rpt`.
- Phát `hb` mỗi 2 s, `hello` lúc boot; xử lý `cmd`.
- Không có logic nghiệp vụ khác. Giữ code nhỏ, dễ kiểm chứng.

### 6.3 GW-HOST (FW-GW-HOST, Arduino-ESP32)
Các task FreeRTOS độc lập (**task báo động không bao giờ bị chặn bởi mạng**):
| Task | Chu kỳ | Việc |
|---|---|---|
| `uart_rx` | sự kiện | Đọc dòng, parse JSON, đẩy hàng đợi `q_rpt`; theo dõi `hb` |
| `fusion` | theo report + tick 1 s | Chạy §7, cập nhật trạng thái, kiểm tra timeout (Node offline, H2 link) |
| `alarm` | 50 ms | Đọc trạng thái, điều khiển LED/còi theo §8, xử lý nút mute |
| `net` | nền | WiFi, NTP, MQTT (kết nối lại backoff 1→60 s), hàng đợi publish có giới hạn, nhận `cmd` |
| `power` | 1 s | Đọc PWR_SENSE, phát hiện mất điện lưới |
- TWDT bật; đăng ký tối thiểu `alarm` và `fusion`.
- Hàng đợi publish đầy: **bỏ telemetry cũ, không bỏ `alert`**.
- WiFi chưa lên: hệ thống **vẫn** fusion và báo động cục bộ.

---

## 7. Thuật toán fusion (chạy trên GW-HOST)

Triển khai thành **thư viện C++ thuần, không phụ thuộc phần cứng**, để chạy được trên PC bằng log CSV (replay). Mọi tham số nằm trong `fusion_params.h`. **Tất cả giá trị bên dưới là `[PLACEHOLDER]`.**

### 7.1 Đầu vào và đại lượng dẫn xuất
T, RH, eCO₂, TVOC, mq2_mv, flame_mv, flags. Dẫn xuất:
- `dTdt`: tốc độ tăng nhiệt (°C/phút), tính trên cửa sổ 60 s từ giá trị đã lọc trung vị 3 mẫu.
- `mq2_rel = (mq2_mv − MQ2_BASE) / MQ2_BASE`. `MQ2_BASE` lưu trong NVS sau hiệu chuẩn CAL-01 (không khí sạch, sau burn-in ≥ 24 h). v1 **không** tự trôi nền.
- `flame_drop = (FLAME_BASE − flame_mv) / FLAME_BASE` (module thường giảm áp khi thấy lửa; **xác nhận chiều tăng/giảm bằng thực nghiệm**). Hoặc `flame_do`.

### 7.2 Điểm thành phần và điểm tổng
Mỗi điểm `s_i = clamp((x − x_low) / (x_high − x_low), 0, 1)`.
| i | Đại lượng x | x_low | x_high | Trọng số w |
|---|---|---|---|---|
| T | T (°C) | 35 | 65 | 0,25 |
| dT | dTdt (°C/phút) | 2 | 8 | 0,15 |
| CO2 | eCO₂ (ppm) | 800 | 1500 | 0,15 |
| TVOC | TVOC (ppb) | 50 | 400 | 0,10 |
| MQ2 | mq2_rel | 0,2 | 1,0 | 0,20 |
| FL | flame_drop | 0,1 | 0,5 | 0,15 |

`S = Σ w_i · s_i` (Σ w = 1). Cảm biến có cờ lỗi/sấy bị **loại khỏi tổng và chuẩn hóa lại trọng số**; đồng thời phát `fault` (không phải cháy).

### 7.3 Luật ghi đè (override)
Tổng có trọng số có thể làm loãng một chỉ báo rất mạnh, nên có luật cứng:
| Luật | Điều kiện | Kết quả |
|---|---|---|
| R1 | T ≥ 65 °C trong ≥ 2 report liên tiếp (ngưỡng của đồ án tham chiếu) | FIRE |
| R2 | Lửa xác nhận: Flame kích ≥ 3 report liên tiếp **và** (s_T > 0,2 hoặc s_MQ2 > 0,2 hoặc s_CO2 > 0,2) | FIRE |
| R3 | dTdt ≥ 10 °C/phút giữ ≥ 30 s **và** T ≥ 40 °C | tối thiểu WARNING |
Flame **một mình không bao giờ** đủ cho FIRE (báo giả do nắng/đèn).

### 7.4 Máy trạng thái và chống báo giả
| Mức | Vào khi (S ≥ θ_up) | Ra khi (S < θ_down) |
|---|---|---|
| CAUTION | 0,25 | 0,15 |
| WARNING | 0,45 | 0,35 |
| FIRE | 0,65 | 0,55, **và latch** |
- **Leo mức:** cần `N_UP = 2` report liên tiếp ≥ ngưỡng. **Hạ mức:** cần `N_DOWN = 3` report liên tiếp < ngưỡng thoát (khớp debounce của đồ án tham chiếu).
- **FIRE là latch:** chỉ xóa khi điều kiện dưới ngưỡng thoát liên tục ≥ 60 s **và** có lệnh `reset_latch` (nút giữ 3 s hoặc MQTT). `[PLACEHOLDER]`
- Trạng thái chính được tính theo từng Node; trạng thái hệ thống = mức cao nhất.

### 7.5 Lỗi (FAULT) — lớp phủ, tách biệt với cháy
| Lỗi | Điều kiện |
|---|---|
| `NODE_OFFLINE` | không có `rpt` > 3 × chu kỳ báo (30 s ở chế độ thường) |
| `H2_LINK_LOST` | không có `hb` > 6 s |
| `SENSOR_FAULT` | cờ lỗi từ Node |
| `MAINS_LOST` | PWR_SENSE thấp > 3 s |
**Ưu tiên hiển thị:** FIRE > FAULT > WARNING > CAUTION > NORMAL.
**Fail-safe:** nếu Node offline khi trạng thái đang ≥ WARNING, **giữ nguyên trạng thái, không hạ mức**.

### 7.6 Quy trình hiệu chỉnh (bắt buộc trước khi chốt tham số)
- **CAL-01:** Node chạy ≥ 24 h trong không khí sạch → ghi `MQ2_BASE`, `FLAME_BASE`, đáy CCS811.
- **CAL-02:** Thu log CSV các kịch bản (T-SYS) → chạy replay → chỉnh `x_low/x_high/w/θ`.
- Mọi tham số chốt phải kèm tên file log đã dùng.

---

## 8. Hành vi cảnh báo cục bộ

| Trạng thái | LED | Còi (active) |
|---|---|---|
| NORMAL | Xanh sáng liên tục | Tắt |
| CAUTION | Vàng sáng liên tục | Tắt |
| WARNING | Vàng nháy 2 Hz | Bíp ngắn 100 ms mỗi 5 s |
| **FIRE** | **Đỏ nháy 4 Hz** | **Bíp 500 ms bật / 500 ms tắt, liên tục** |
| FAULT (không có FIRE) | Đỏ và vàng nháy luân phiên 1 Hz | 2 bíp ngắn mỗi 60 s |
| MAINS_LOST | thêm: vàng nháy chậm 0,5 Hz chồng lên trạng thái hiện tại | 1 bíp/5 phút |
| Khởi động | cả 3 LED sáng 1 s rồi tắt (kiểm tra LED) | 1 bíp ngắn |

- **Nút:** nhấn ngắn = tắt tiếng còi `MUTE_S = 300` s (LED **không** tắt; hết thời gian mà vẫn còn FIRE thì còi kêu lại). Giữ 3 s = `reset_latch`. Giữ 5 s = mở cửa sổ join. Nút nhấn khi khởi động = test toàn bộ LED/còi.
- Còi phải được đặt về mức tắt ngay đầu `setup()`.

---

## 9. Toolchain, repo, quy ước

| Thành phần | Công cụ |
|---|---|
| GW-H2 | **ESP-IDF v5.5.x** + esp-zigbee-sdk (đồ án tham chiếu dùng 5.5.4). Dùng PlatformIO hoặc VS Code extension `[VERIFY]` tương thích phiên bản SDK |
| Node | Arduino-ESP32 **3.x**: Board `ESP32H2 Dev Module`, `Tools → Zigbee mode: Zigbee ED (end device)`, partition scheme có Zigbee, `USB CDC On Boot: Enabled` `[VERIFY]`. Nếu lớp Arduino thiếu tính năng → chuyển ESP-IDF |
| GW-HOST | Arduino-ESP32 3.x, `PubSubClient` hoặc `esp-mqtt`, `ArduinoJson`, `WiFiClientSecure` |
| Công cụ gỡ lỗi | Serial monitor, MQTT Explorer, WiFi scanner |

**Cấu trúc repo**
```
fire-alarm/
  docs/GatewayPlan.md            # tài liệu này (nguồn chân lý)
  hardware/pinmap_verified.md    # kết quả xác minh P0, BOM thực, ảnh đấu dây
  common/                        # constants.h, schema JSON, mô tả giao thức
  firmware/node_h2/
  firmware/gw_h2/                # ESP-IDF
  firmware/gw_host/              # Arduino
  firmware/lib/fusion/           # thư viện C++ thuần + test PC
  cloud/                         # dashboard, bot Telegram
  tests/                         # log, script replay, kết quả
```
**Quy tắc:** không `delay()` dài trong luồng chính; không hard-code WiFi/MQTT trong repo (`secrets.h` git-ignore, kèm `secrets.example.h`); log có tiền tố `[MODULE]`; mọi hằng số vật lý (DIV_RATIO, kênh, ngưỡng) nằm trong `config.h` / `fusion_params.h`.

---

## 10. Phân việc cho agent

**Định nghĩa hoàn thành (DoD) chung:** code trong repo + chạy được + log bằng chứng trong `tests/` + cập nhật `pinmap_verified.md` hoặc tài liệu này nếu hợp đồng thay đổi.

### Phase P0 — Spike (cổng G0, tối đa ~5 ngày làm việc)
| ID | Agent | Việc | Tiêu chí chấp nhận |
|---|---|---|---|
| T-001 | HW | Mua linh kiện (§3.2, §3.3). Đối chiếu pinout SuperMini thực với tài liệu | `pinmap_verified.md` hoàn chỉnh, ghi lại mọi chỗ khác biệt |
| T-002 | FW-GW-H2 + FW-NODE | Coordinator (IDF) + Node ED (Arduino) với **giá trị giả**: join, Node gửi EP10–15, coordinator in ra UART/serial | S1: build OK · S2: join OK · S3: nhận đủ 6 trường · S4: 1000 report, mất ≤ 1 % |
| T-003 | FW-GW-H2 | Rút nguồn Node/Gateway rồi cấp lại | S5: tự rejoin, báo cáo lại ≤ 60 s |
**Cổng G0:** S1–S5 đạt → tiếp P1. Không đạt sau ~5 ngày → kích hoạt §15.

### Phase P1 — Liên kết
| ID | Agent | Việc | Chấp nhận | Phụ thuộc |
|---|---|---|---|---|
| T-010 | FW-GW-H2 | NDJSON qua UART1, `hello`/`hb`/`join`/`rpt`, nhận `cmd` | Đúng §5.1; terminal thấy đủ bản tin | G0 |
| T-011 | FW-GW-HOST | Parse UART, tính trạng thái giả lập, điều khiển LED/còi theo §8 | Mọi trạng thái §8 hiển thị đúng bằng lệnh giả lập | — |
| T-012 | FW-GW-HOST / CLOUD | WiFi + MQTT TLS, `sensor/*`, `status/gateway` (LWT), `cmd/gateway` | MQTT Explorer thấy đúng schema §5.2; rút WiFi → tự nối lại | T-010 |

### Phase P2 — Cảm biến thật
| ID | Agent | Việc | Chấp nhận |
|---|---|---|---|
| T-020 | FW-NODE | Driver SHT31, CCS811 (+ENV_DATA), ADC mq2/flame (mV, DIV_RATIO), flags | Đọc ổn định 1 h; số liệu hợp lý; cờ lỗi đúng khi rút cảm biến |
| T-021 | FW-NODE | Gắn giá trị thật vào EP10–15; chu kỳ 10 s/3 s, pre-alert | Chuyển 10 s ↔ 3 s đúng điều kiện §4.4 |

### Phase P3 — Fusion
| ID | Agent | Việc | Chấp nhận |
|---|---|---|---|
| T-030 | FW-GW-HOST | Thư viện fusion §7 + test đơn vị trên PC + công cụ replay CSV | Test: mọi luật R1–R3, hysteresis, latch, loại cảm biến lỗi |
| T-031 | QA | Thu log CSV các kịch bản (dùng `sensor/#`) | File log có nhãn kịch bản + thời điểm |
| T-032 | QA + FW | CAL-01, CAL-02, chốt tham số | Bảng tham số kèm log nguồn; T-SYS-05/06 đạt |

### Phase P4 — Nguồn và lỗi
| ID | Agent | Việc | Chấp nhận |
|---|---|---|---|
| T-040 | HW | Tích hợp module UPS; PWR_SENSE; sự kiện mất điện lưới | T-PWR-01..04 đạt |
| T-041 | QA | Bộ kiểm thử gây lỗi (§11.3) | T-FLT-01..06 đạt |

### Phase P5 — Cloud
| ID | Agent | Việc | Chấp nhận |
|---|---|---|---|
| T-050 | CLOUD | Dashboard: thêm `mq2_mv`, `flame_mv`, `state`, `fault`, `power`; Telegram theo `alert/#` | NFR-2, NFR-3 đạt |

### Phase P6 — Tài liệu
| ID | Agent | Việc |
|---|---|---|
| T-060 | DOC | Sơ đồ đấu dây, sơ đồ khối, lưu đồ; chương báo cáo. Sơ đồ KiCad chỉ làm nếu D6 yêu cầu |

---

## 11. Kế hoạch kiểm thử

### 11.1 Liên kết và mạng
| ID | Kiểm thử | Đạt khi |
|---|---|---|
| T-NET-01 | Join lần đầu | Node join ≤ 30 s sau khi mở cửa sổ |
| T-NET-02 | PDR 1 h, chu kỳ 10 s, 3 m, 1 vách | ≥ 99 % (NFR-4), tính bằng `seq` |
| T-NET-03 | PDR khi WiFi của GW-HOST đang tải nặng | Không tệ hơn T-NET-02 quá 1 điểm % (kiểm tra nhiễu 2,4 GHz) |
| T-NET-04 | Rút nguồn Node, cấp lại | Rejoin ≤ 60 s (NFR-5) |
| T-NET-05 | Rút nguồn GW-H2, cấp lại | Node tự về mạng, `rpt` tiếp tục ≤ 60 s |
| T-NET-06 | Đổi kênh WiFi router sang kênh gần Zigbee | PDR không giảm dưới 99 % hoặc có ghi chú đổi kênh |

### 11.2 Nguồn
| ID | Kiểm thử | Đạt khi |
|---|---|---|
| T-PWR-01 | Rút adapter khi đang chạy (Node) | Không reset ESP32; báo cáo liên tục |
| T-PWR-02 | Rút/cắm adapter lặp 20 lần (cả hai thiết bị) | Không reset, không treo (nếu reset → thêm tụ §3.4) |
| T-PWR-03 | Đo dòng tải thực Node và Gateway | Cả hai > 50 mA liên tục (nếu không: xử lý tự tắt IP5306) |
| T-PWR-04 | Chạy pin đến khi tắt | Ghi thời gian; ≥ 2 h (NFR-6) |
| T-PWR-05 | Gateway phát hiện mất điện lưới | `status/gateway.power = "battery"` ≤ 5 s, LED chồng đúng |

### 11.3 Gây lỗi
| ID | Gây lỗi | Hành vi đúng |
|---|---|---|
| T-FLT-01 | Tắt Node | `NODE_OFFLINE` sau ~30 s; **không** hiện xanh an toàn |
| T-FLT-02 | Tháo dây UART giữa H2 và WROOM | `H2_LINK_LOST` ≤ 6 s |
| T-FLT-03 | Tắt WiFi | LED/còi vẫn hoạt động; MQTT tự nối lại khi có WiFi |
| T-FLT-04 | Rút SHT31 / CCS811 | Cờ lỗi; fusion vẫn chạy với cảm biến còn lại |
| T-FLT-05 | Node offline khi đang WARNING/FIRE | Giữ nguyên mức, không hạ |
| T-FLT-06 | Làm hỏng gói JSON UART (nhiễu giả lập) | Bỏ dòng lỗi, `parse_err` tăng, không crash |

### 11.4 Cảm biến và hệ thống
> **An toàn:** thử cháy ở nơi thoáng, có bình chữa cháy/nước, người giám sát, không dùng chất lỏng dễ cháy, quy mô nhỏ (giấy, nhang, bật lửa có kiểm soát).

| ID | Kiểm thử | Đạt khi |
|---|---|---|
| T-SNS-01 | SHT31 so với nhiệt kế tham chiếu | Sai số trong dải datasheet |
| T-SNS-02 | ADC: cấp áp biết trước qua chia áp | Sai số mV ≤ 3 % sau hiệu chuẩn |
| T-SNS-03 | MQ-2 đáp ứng với khói nhang/giấy | Có đáp ứng lặp lại, ghi biên độ |
| T-SNS-04 | Flame: bật lửa ở 30 cm, ánh nắng, đèn sợi đốt | Ghi tỷ lệ kích đúng/sai (căn cứ cho luật R2) |
| T-SYS-01 | Nhiệt tăng (máy sấy tóc) | WARNING/FIRE theo R1/R3; NFR-1, NFR-2 |
| T-SYS-02 | Khói/âm ỉ (nhang, giấy cháy chậm trong hộp) | Đạt ít nhất WARNING; ghi thời gian phát hiện |
| T-SYS-03 | Lửa nhỏ có kiểm soát | FIRE nhờ R2 hoặc điểm tổng |
| T-SYS-04 | **Chống báo giả:** nấu ăn/hơi nước/sấy tóc không vào FIRE; 24 h vận hành thường | 0 FIRE (NFR-7) |
| T-SYS-05 | **So sánh fusion vs từng cảm biến đơn** (ngưỡng đơn lẻ) trên cùng log | Bảng: độ trễ phát hiện và số báo giả của từng phương án |
| T-SYS-06 | **Đối chứng MQ-2** so với cảm biến khói/CO khác (nếu D3 được duyệt) | Ghi nhận khả năng bắt cháy âm ỉ của MQ-2 |
| T-SYS-07 | Độ trễ đầu-cuối: cảm biến → còi, → dashboard, → Telegram | NFR-1/2/3, đo bằng đồng hồ + log thời gian |

---

## 12. Đăng ký rủi ro

| ID | Rủi ro | Xác suất | Ảnh hưởng | Giảm thiểu / kích hoạt |
|---|---|---|---|---|
| R-01 | Coordinator H2 (esp-zigbee-sdk) không nhận report ổn định / API Arduino thiếu | TB | Cao | Cổng G0; nhận report bằng callback IDF; lùi §15 |
| R-02 | Xung đột 2,4 GHz giữa WiFi (WROOM) và Zigbee (H2) trong cùng hộp | TB | TB | §3.5, §4.1; T-NET-03/06 |
| R-03 | IP5306 tự tắt khi tải nhẹ / sụt áp lúc chuyển nguồn | TB | Cao | T-PWR-02/03; tụ lớn, tải giả |
| R-04 | MQ-2 kém nhạy với cháy âm ỉ; trôi nền | Cao | TB | CAL-01/02; T-SYS-06; D3 |
| R-05 | CCS811 lỗi I2C (clock stretching) | TB | TB | Hạ tốc độ I2C / bit-bang; cờ lỗi |
| R-06 | Flame báo giả nhiều | Cao | TB | Luật R2 (không đủ một mình) |
| R-07 | Pin Li-ion không bảo vệ gây nguy hiểm | Thấp | Rất cao | Cell có PCM, không sạc không giám sát |
| R-08 | Board SuperMini clone khác pinout/lỗi | TB | TB | P0 đối chiếu; mua dự phòng |
| R-09 | Báo giả FIRE làm mất niềm tin hệ thống | TB | Cao | N_UP, R1–R3, T-SYS-04 |
| R-10 | Thời gian: spike kéo dài, ăn vào tiến độ | TB | Cao | Giới hạn P0 ≈ 5 ngày, rồi quyết định lùi |

---

## 13. Errata — những điểm sai/lỗi thời trong các tài liệu trước (01–07 và bản tổng kết HTML)

Các tài liệu này do các phiên trước tạo. **Agent không dùng chúng làm nguồn cho các mục sau:**

| # | Nội dung cũ | Đúng là |
|---|---|---|
| 1 | TP4056 sạc 2–3 cell nối tiếp | TP4056 chỉ cho 1 cell |
| 2 | IP5306 nhận đầu vào pin 8,4–12,6 V, tăng áp từ 2S/3S | IP5306 dùng 1 cell; xem §3.4 |
| 3 | Dual-power cần diode-OR Schottky + adapter 5,3 V + buck 12 V→5 V + AMS1117 | Không cần với DevKit/SuperMini; module UPS có power-path (§3.4) |
| 4 | ESP32-C6 "dual-core 160 MHz" | C6 một nhân RISC-V 160 MHz |
| 5 | GPIO34 là chân ADC của Node (tài liệu firmware 06) | GPIO34 chỉ có trên WROOM-32; Node dùng H2, ADC = GPIO1–5 |
| 6 | CC2531 "Zigbee 3.0" | Stack cũ hơn; và đã bị loại khỏi thiết kế |
| 7 | Zigbee2MQTT chạy trên ESP32 WROOM-32 | Không thể; cần Linux + Node.js |
| 8 | "Vai trò coordinator chỉ làm được bằng ESP-IDF" | Arduino-ESP32 3.x có chế độ coordinator; nhưng đường nhận report từ cluster tùy ý phải kiểm chứng ở P0 `[VERIFY]` |
| 9 | Thời gian chuyển nguồn "< 1 ms", "ESP32 không reset" (tài liệu 05) | Chưa đo. Chỉ là giả định; kiểm chứng bằng T-PWR-01/02 |
| 10 | Node đo % pin bằng chia áp ADC (tài liệu 06) | Module UPS không cho đọc cell qua chân; thay bằng PWR_SENSE trên Gateway |
| 11 | Ngân sách ~1,65–2,4 triệu và các con số linh kiện | Ước lượng cũ; tính lại theo BOM §3.2/§3.3 sau khi chốt D1 |

---

## 14. Quyết định cần người dùng xác nhận

| ID | Câu hỏi | Khuyến nghị của Architect |
|---|---|---|
| **D1** | Chấp nhận đơn giản hóa nguồn: adapter 5 V + module UPS 1S 18650 cho cả hai thiết bị (thay dual-power 2S/3S)? | **Chấp nhận.** Thiết kế cũ có lỗi nền tảng (§3.4, §13) |
| **D2** | Giữ Zigbee, với cổng G0 và phương án lùi Thread? | Giữ Zigbee. Chấp nhận phương án lùi nếu G0 trượt |
| **D3** | Có thêm đối chứng cảm biến khói quang điện (như đồ án tham chiếu) với MQ-2? | Có, ít nhất ở mức thí nghiệm T-SYS-06, vì nó quyết định đề tài "cháy sớm/âm ỉ" có đứng vững hay không |
| **D4** | Fusion chạy trên GW-HOST (WROOM-32)? | Chấp nhận |
| **D5** | Có mã nguồn dashboard + bot Telegram của đồ án tham chiếu để tái sử dụng? Định dạng `last_seen` họ dùng là gì? | Nếu có: tái sử dụng để tiết kiệm nhiều ngày; đọc JS trước khi chốt §5.2 |
| **D6** | GVHD có yêu cầu PCB/sơ đồ nguyên lý tùy chế không? | Nếu có: thêm Phase "carrier board" sau khi hệ thống chạy trên DevKit. Không thiết kế PCB trước khi G0–G2 đạt |
| **D7** | Mục tiêu thời gian chạy pin dự phòng | Đề xuất ≥ 2 h (đạt), ≥ 6 h (mục tiêu) |

---

## 15. Phương án lùi

### Plan T — Thread/OpenThread (phương án lùi ưu tiên)
- **Kích hoạt:** G0 trượt sau ~5 ngày, hoặc PDR < 95 % lặp lại không khắc phục được.
- **Lý do ưu tiên:** đồ án tham chiếu đã chứng minh đúng cặp phần cứng (WROOM-32 + H2 RCP) hoạt động, có tài liệu quá trình triển khai.
- **Thay đổi:** Node → OpenThread (ESP-IDF) gửi CoAP JSON; GW-H2 → firmware RCP; GW-HOST → OpenThread Border Router + CoAP server + MQTT. Giao diện UART của §5.1 **được thay bằng Spinel**.
- **Không đổi:** hợp đồng MQTT (§5.2), fusion (§7), cảnh báo cục bộ (§8), nguồn (§3.4), cloud.
- **Chi phí:** phải học ESP-IDF cho cả Node và Host.

### Plan P — Raspberry Pi + Zigbee2MQTT
- **Kích hoạt:** cả hai Plan Zigbee-ESP và Plan T đều không khả thi.
- **Thay đổi:** Gateway = Raspberry Pi Zero 2 W + USB Zigbee dongle hiện đại (ưu tiên dòng EFR32/CC2652 được Zigbee2MQTT khuyến nghị, **không** dùng CC2531) + Mosquitto + script fusion Python; LED/còi nối thẳng GPIO Pi.
- **Chi phí:** thêm SD card, bảo trì OS, tiêu thụ điện cao hơn; mất lợi thế "chỉ dùng ESP32".

---
*Hết GatewayPlan.md v0.1. Mọi thay đổi hợp đồng phải cập nhật §2.4 (Decision Log) và ghi ngày.*
