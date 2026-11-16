# DEV_CODING_PLAN.md — Kế hoạch triển khai mã nguồn (Firmware & Software)

Hệ thống IoT phát hiện cháy sớm: **Node ESP32-H2** → Zigbee → **Gateway = ESP32-H2 (coordinator) + ESP32 WROOM-32 (host)** → MQTT/TLS → Dashboard + Telegram.

| | |
|---|---|
| Phiên bản | 0.1 (nháp chờ người dùng duyệt) |
| Ngày | 2026-10-24 |
| Vai trò soạn | Senior Embedded & IoT Software Architect |
| Căn cứ | `GatewayPlan.md` v0.1 (hợp đồng giao diện), `GW_SCHEMATIC_SPEC.md` |
| Người thực thi | Con người + agent sinh code (**DeepSeek chạy trên OpenCode**) |
| Giai đoạn đầu | **Thử nghiệm trên breadboard** bằng các ứng dụng kiểm tra (`bringup/`) |

---

## 0. Cách dùng tài liệu này với OpenCode + DeepSeek

### 0.1 Vì sao cần kỷ luật nghiêm hơn bình thường
Agent sinh code có ba kiểu lỗi lặp lại: (1) **bịa API** (đặc biệt `esp-zigbee-sdk`, vốn ít tài liệu); (2) **bịa thanh ghi/datasheet** của cảm biến; (3) **báo "xong" khi chưa build/chưa chạy**. Toàn bộ tài liệu này được thiết kế để chặn ba lỗi đó: mỗi task nhỏ, có chữ ký hàm cố định, có dữ kiện datasheet được cấp sẵn, và có lệnh nghiệm thu cụ thể.

### 0.2 Vòng lặp cho mỗi task (bắt buộc)
```
1. Mở nhánh:        git switch -c feat/<TaskID>-<ten-ngan>
2. Nạp ngữ cảnh:    đưa cho agent ĐÚNG các file ở cột "Ngữ cảnh" (không đưa cả repo)
3. Dán prompt:      dùng mẫu §8.1; điền Task ID, file cần tạo, chữ ký hàm từ §5
4. Agent làm:       chỉ trong phạm vi file đã liệt kê
5. Cổng build:      agent PHẢI chạy lệnh build/test và dán đầu ra thật; lỗi thì sửa tiếp, không "giải thích"
6. Người duyệt:     đọc diff theo checklist §1.8 (5 phút)
7. Chạy phần cứng:  với task HIL, người nạp board, chạy tools/serial_check.py, lưu log vào tests/results/
8. Hợp nhất:        git merge + tag khi xong cả Phase (phase-N-done)
```

### 0.3 Quy tắc dừng: agent phải DỪNG và hỏi người, không tự quyết, khi
- Cần một API của IDF/Zigbee mà **không tìm thấy** trong header đã cài (xem §1.4 về `API_NOTES.md`).
- Chân GPIO thực tế **khác** bảng pin map.
- Một bài test đơn vị fail và cách "sửa" duy nhất là sửa/xóa test hoặc nới ngưỡng.
- Muốn đổi **hợp đồng** (UART NDJSON, endpoint Zigbee, topic MQTT) so với `GatewayPlan.md` §4–§5.
- Muốn thêm thư viện bên thứ ba chưa có trong §3.

### 0.4 Lưu trữ từ xa (GitHub)
- Remote `origin` là repo public: `https://github.com/leonardo42ni/fire-alarm-multisensor.git`.
- Sau mỗi task đã được duyệt: commit trên nhánh task, merge vào `master`, rồi `git push origin master`.
- Khi hết Phase: tạo tag `phase-N-done` trên commit merge rồi `git push origin master --tags`.
- Trước khi push: `git status` phải sạch; không có `secrets.h`, `.env`, build output hoặc log chứa bí mật.
- Không tự push/merge/tag nếu người dùng chưa cho phép.

---

## 1. Nguyên tắc lập trình & kỷ luật mã nguồn

### 1.1 Quy tắc cứng (vi phạm = từ chối PR)
| # | Quy tắc | Cách kiểm tra |
|---|---|---|
| H1 | **Cấm `delay()`/`vTaskDelay()` dài trong luồng xử lý logic.** Chờ định kỳ chỉ dùng `vTaskDelayUntil` hoặc chờ trên queue/timeout. Không có vòng `while` bận | `grep -rn "vTaskDelay(" firmware/*/components firmware/*/main` chỉ cho phép trong task có chu kỳ cố định và có chú thích `/* periodic */` |
| H2 | Mọi vòng lặp task phải gọi `esp_task_wdt_reset()` (task được giám sát) | Review + test T-404 |
| H3 | **Không cấp phát heap trong ISR**, không gọi hàm không an toàn ISR. ISR chỉ `xQueueSendFromISR`/`xTaskNotifyFromISR` | Review |
| H4 | Mọi buffer có kích thước cố định và kiểm tra tràn. Không `sprintf`, `strcpy`, `gets`. Chỉ `snprintf`, `strlcpy`-tương đương | `grep -rnE "\b(sprintf|strcpy|strcat|gets)\(" firmware/` = 0 kết quả |
| H5 | Mọi hàm có thể lỗi trả `esp_err_t` (hoặc `proto_err_t`); **không nuốt lỗi** (`ESP_ERROR_CHECK` chỉ dùng lúc khởi tạo không thể phục hồi; còn lại phải xử lý) | Review |
| H6 | **Không có bí mật trong git** (§1.5) | `git grep -nE "(PASSWORD|PASS|TOKEN)\s*=\s*\"[^\"]+\"" ` = 0 ngoài `*.example.*` |
| H7 | Hằng số vật lý (chân, hệ số, ngưỡng, thời gian) chỉ nằm trong `config.h` hoặc `fusion_params.h`. Không số "ma thuật" trong code | Review |
| H8 | Không sửa/xóa test để cho qua. Không nới ngưỡng test mà không ghi lý do trong PR | Review |
| H9 | Mỗi file ≤ ~300 dòng; mỗi hàm ≤ ~60 dòng; một module một trách nhiệm | Review |
| H10 | Kênh UART giao thức **chỉ** chứa NDJSON. Log luôn đi qua kênh console riêng (§1.6) | Test T-115 |
| H11 | Trước khi dùng API ESP-IDF/Zigbee, phải xác minh chữ ký trong header cài thật và ghi vào `docs/API_NOTES.md` | Review: PR không có dòng mới trong `API_NOTES.md` mà dùng API mới → từ chối |

### 1.2 Mô hình đồng thời
- **Node:** 4 task (`sensor`, `report`, `zb`, `cli`) + ISR cho Flame DO / SHT31 ALERT. Dữ liệu chia sẻ qua `node_snapshot` bảo vệ bằng mutex (thời gian giữ khóa < 1 ms, không gọi API chặn khi đang giữ).
- **GW-H2:** 2 task (`zb`, `bridge`). Callback Zigbee **không** ghi UART; chỉ đẩy sự kiện vào queue `q_zb_evt`. `bridge` mới là nơi duy nhất ghi UART.
- **GW-HOST:** bảng ưu tiên (số lớn = ưu tiên cao):

| Task | Ưu tiên | Stack | Chu kỳ / kích hoạt | Giám sát TWDT |
|---|---|---|---|---|
| `alarm` | 7 | 3072 | 50 ms (`vTaskDelayUntil`) | Có |
| `input` | 6 | 2048 | 10 ms | Có |
| `uart_rx` | 6 | 4096 | chờ UART event, timeout 100 ms | Có |
| `fusion` | 5 | 4096 | chờ `q_rpt`, timeout 1000 ms (để chạy tick) | Có |
| `power` | 3 | 3072 | 1000 ms | Không |
| `mqtt_pub` | 4 | 6144 | chờ `q_pub`, timeout 500 ms | Không (mạng có thể treo hợp lệ) |
| esp-mqtt nội bộ | 5 | 8192 (cấu hình `task.stack_size`) | sự kiện | Không |

**Nguyên tắc ưu tiên:** `alarm` cao nhất trong số task người dùng, **không bao giờ** chờ mạng, không bao giờ lấy khóa mà task mạng có thể giữ lâu.

### 1.3 Fail-safe & tự phục hồi
| Tình huống | Hành vi bắt buộc |
|---|---|
| Mất WiFi / MQTT | Fusion + còi/LED vẫn chạy. Kết nối lại theo backoff 1→2→4→…→60 s (±10 % jitter). Hàng đợi publish có giới hạn; đầy thì bỏ telemetry cũ nhất, **không bỏ `alert`** |
| Mất heartbeat H2 > 6 s | Đặt lỗi `H2_LINK_LOST`; nếu đang ≥ WARNING thì **giữ mức** |
| Node không báo > `max(30 s, 3×chu kỳ)` | Đặt `NODE_OFFLINE`; **không hạ mức** nếu đang ≥ WARNING |
| Task treo > 10 s | TWDT panic → reset. Khi boot lại: ghi `reset_reason`, bíp 3 lần sau self-test, đưa vào `status/gateway` |
| Mất điện lưới | `MAINS_LOST`, vẫn chạy bằng pin; báo MQTT khi có mạng |
| Lệnh/bản tin UART hỏng | Bỏ dòng, tăng `parse_err`, **không crash**, không reset |
| Khởi động | Đầu ra (còi, LED) về mức TẮT **trước mọi thứ khác**, kể cả trước NVS/WiFi |

### 1.4 Chống "bịa API" — `docs/API_NOTES.md`
Mỗi API ESP-IDF/Zigbee/driver được dùng lần đầu phải có một dòng:
```
| API | Chữ ký thật (copy từ header) | File:dòng | Phiên bản IDF/lib | Task dùng |
```
Cách tìm: `grep -rn "esp_zb_bdb_open_network" $IDF_PATH/components managed_components/ 2>/dev/null`. Nếu `grep` không ra → API **không tồn tại** ở phiên bản này → áp dụng quy tắc dừng §0.3.

### 1.5 Cấu hình tập trung & cách ly bí mật
- Mỗi firmware có `main/config.h` (hằng số, chân, thời gian). Giá trị chia sẻ giữa firmware nằm ở `firmware/common/shared_config.h`.
- **GW-HOST:** `main/secrets.h` (git-ignore) + `main/secrets.example.h` (commit). `config.h` bắt buộc có:
  ```c
  #if !__has_include("secrets.h")
  #error "Thieu main/secrets.h: sao chep tu secrets.example.h va dien thong tin"
  #endif
  #include "secrets.h"
  ```
  `secrets.h` định nghĩa: `WIFI_SSID, WIFI_PASS, MQTT_HOST, MQTT_PORT(8883), MQTT_USER, MQTT_PASS, MQTT_CLIENT_ID("gateway-01")`.
- **Cloud:** `.env` (git-ignore) + `.env.example`. Mỗi thành phần dùng **credential HiveMQ riêng**: gateway (publish+subscribe), dashboard (chỉ subscribe), bot Telegram (chỉ subscribe). Thiết lập quyền trên HiveMQ Cloud Access Management `[VERIFY]`.
- **Dashboard là mã chạy trong trình duyệt → mọi thứ trong đó là công khai.** Chỉ được dùng credential subscribe-only.

### 1.6 Kênh log và kênh giao thức
| Firmware | Console/log | Kênh giao thức |
|---|---|---|
| Node, GW-H2 (SuperMini) | **USB-Serial-JTAG** (`CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y`) `[VERIFY]` | GW-H2: UART1 (GPIO10/11), chỉ NDJSON |
| GW-HOST (DevKit) | UART0 qua cầu USB của DevKit | UART2 (GPIO16/17), chỉ NDJSON |

Định dạng log: `ESP_LOGx("<TAG>", ...)` với TAG ∈ {`SNS`,`ZB`,`BRG`,`UART`,`FUS`,`ALM`,`PWR`,`NET`,`MQTT`,`CMD`}. Dòng kiểm tra phần cứng (bring-up) dùng định dạng cố định để máy đọc: `BU|<TEST>|k=v|k=v|PASS` hoặc `...|FAIL|reason=...`.

### 1.7 Độ lệch so với `GatewayPlan.md` / `GW_SCHEMATIC_SPEC.md` (phải cập nhật hai file đó)
| # | Nội dung cũ | Nội dung mới (theo yêu cầu hiện tại) | Hành động |
|---|---|---|---|
| X1 | Toolchain: Arduino cho Node và Host (D-12) | **ESP-IDF 5.5.x cho cả 3 firmware** (người dùng chỉ định `esp-zigbee-sdk`; một toolchain; agent chạy CLI ổn định hơn) | Cập nhật D-12 |
| X2 | 3 LED rời đỏ/vàng/xanh | **1 LED RGB cathode chung**: R=GPIO25, G=GPIO26, B=GPIO27 `[ASSUMPTION: thứ tự RGB; xác nhận]`. Vàng = R+G. FAULT = **xanh dương nháy** | Sửa `GW_SCHEMATIC_SPEC` §3.5; sửa bảng GatewayPlan §8 |
| X3 | Còi module 3 chân | **Còi trần 5 V qua NPN** (GPIO33). Loại còi (active/passive) **chưa xác nhận**; mặc định active (bật/tắt). Hỗ trợ passive qua cờ cấu hình | Sửa `GW_SCHEMATIC_SPEC` §3.5 (xem §1.7.1) |
| X4 | "Zigbee NCP" | Thực chất là **coordinator chạy ứng dụng đầy đủ + cầu UART**, không phải NCP/RCP. Tên module: `gw_h2` | Chỉ là thuật ngữ |
| X5 | Dùng cJSON | Dùng **bộ parse JSON phẳng tự viết** (không phụ thuộc, test được trên PC) | — |
| X6 | Nút: hành động theo thời gian nhấn tức thời | Hành động tính **khi nhả nút** theo thời lượng giữ (tránh xung đột 3 s/5 s) | — |

**1.7.1 Mạch còi trần (breadboard):** `GPIO33 → 1 kΩ → Base(NPN S8050/2N2222)`; **10 kΩ từ Base xuống GND** (giữ còi tắt khi GPIO nổi lúc reset); `Emitter → GND`; `Collector → còi(−)`; `còi(+) → +5V`; **diode 1N4148 ngược chiều song song còi** (bắt buộc nếu còi điện từ; còi áp điện active có thể bỏ nhưng cứ lắp). Kiểm tra dòng còi ≤ ~30 mA `[VERIFY]` (nếu lớn hơn, dùng transistor công suất lớn hơn).
**LED RGB:** R qua 220 Ω; G và B qua ~100 Ω vì Vf của G/B ≈ 3,0–3,2 V nên sáng yếu ở 3,3 V `[VERIFY: đo dòng, mỗi chân ≤ 8 mA]`. Màu vàng cần cân bằng độ rộng xung (hằng số `RGB_YELLOW_R_DUTY`, `RGB_YELLOW_G_DUTY`).

### 1.8 Checklist duyệt diff (người, ~5 phút/PR)
1. Có `delay` dài / vòng bận / `while(1)` không có chờ? 2. Có task nào không `esp_task_wdt_reset()`? 3. Có `malloc`/`printf`/log trong ISR? 4. Buffer có kiểm tra độ dài không? 5. Mọi `esp_err_t` có được xử lý? 6. Có số ma thuật ngoài `config.h`? 7. Có API mới chưa vào `API_NOTES.md`? 8. Test có bị sửa/nới không? 9. Đầu ra (còi/LED) có ở mức tắt khi lỗi/khởi động không? 10. Có bí mật trong diff không?

---

## 2. Cây thư mục mã nguồn

```
fire-alarm/
├─ AGENTS.md                         # luật cho agent (nội dung ở §8.2)
├─ README.md
├─ .gitignore                        # build/, sdkconfig, managed_components/, secrets.h, .env, node_modules/, tests/results/*.log (giữ .gitkeep)
├─ docs/
│  ├─ GatewayPlan.md   GW_SCHEMATIC_SPEC.md   DEV_CODING_PLAN.md
│  ├─ API_NOTES.md                   # API đã xác minh (H11)
│  ├─ ENV.md                         # lệnh cài đặt/phiên bản thật
│  └─ pinmap_verified.md             # kết quả đối chiếu pinout thật
├─ firmware/
│  ├─ common/                        # C thuần, dùng chung 3 firmware + test PC
│  │  ├─ proto_ndjson.h/.c           # §5.1
│  │  ├─ shared_config.h             # ZB_CHANNEL, ZB_PAN_ID, EP_*, ngưỡng giao thức
│  │  ├─ error.h  version.h
│  │  └─ CMakeLists.txt              # IDF component HOẶC thư viện CMake thường (phát hiện ESP_PLATFORM)
│  ├─ lib_fusion/                    # C thuần, logic nghiệp vụ (PC-testable)
│  │  ├─ filters.h/.c  prealert.h/.c  fusion_params.h  fusion.h/.c    # §5.2
│  │  ├─ logic/ fault.h/.c  alarm_pattern.h/.c  button_logic.h/.c  backoff.h/.c
│  │  └─ CMakeLists.txt
│  ├─ node_h2/                       # ESP-IDF, target esp32h2
│  │  ├─ CMakeLists.txt  sdkconfig.defaults  partitions.csv
│  │  ├─ main/ app_main.c  config.h  CMakeLists.txt  idf_component.yml
│  │  └─ components/ i2c_bus/ sht31/ ccs811/ adc_sensors/ flame_gpio/ node_state/ report_sched/ zb_node/ node_cli/
│  ├─ gw_h2/                         # ESP-IDF, target esp32h2
│  │  ├─ CMakeLists.txt  sdkconfig.defaults  partitions.csv
│  │  ├─ main/ app_main.c  config.h  idf_component.yml
│  │  └─ components/ zb_coord/ node_cache/ uart_bridge/
│  ├─ gw_host/                       # ESP-IDF, target esp32
│  │  ├─ CMakeLists.txt  sdkconfig.defaults
│  │  ├─ main/ app_main.c  config.h  secrets.example.h  (secrets.h: git-ignore)
│  │  └─ components/ sys_state/ uart_link/ node_registry/ fusion_ctrl/ alarm_out/ power_mon/ net_wifi/ net_time/ net_mqtt/ payload/ cmd_handler/
│  └─ bringup/                       # ứng dụng kiểm tra breadboard (dùng lại component thật)
│     ├─ bu_i2c_scan/ bu_sht31/ bu_ccs811/ bu_adc/ bu_flame_do/ bu_node_all/        # target esp32h2
│     ├─ bu_uart_h2/                                                                  # target esp32h2
│     ├─ bu_rgb/ bu_buzzer/ bu_button/ bu_pwr_sense/ bu_uart_host/ bu_wifi_mqtt/      # target esp32
├─ cloud/
│  ├─ dashboard/      index.html  app.js  style.css  vendor/ (mqtt.min.js, chart.min.js lưu cục bộ)
│  └─ telegram_bot/   package.json  index.js  src/ (mqtt_sub.js, format.js, dedupe.js)  .env.example
├─ tests/
│  ├─ host/           CMakeLists.txt  test_util.h  test_proto.c  test_filters.c  test_prealert.c  test_fusion.c  test_logic.c  tools: fusion_replay.c
│  ├─ vectors/        *.csv (đầu vào) + *.golden.csv (kết quả mong đợi)
│  └─ results/        .gitkeep (log HIL, ghi đè được)
└─ tools/
   ├─ serial_check.py  gw_sim.py  replay.py  pdr_report.py  build_all.sh  build_all.ps1
```
**Ghi chú:** (a) `common/` và `bringup/` là hai thư mục thêm so với yêu cầu ban đầu; cần thiết để chia sẻ mã giao thức và để test breadboard dùng lại đúng driver thật. (b) `lib_fusion/` được dùng làm "lõi logic thuần" (kể cả `logic/`) dù tên chỉ nhắc fusion.

---

## 3. Môi trường & phiên bản

| Hạng mục | Lựa chọn | Ghi chú |
|---|---|---|
| ESP-IDF | **v5.5.x** (đồ án tham chiếu đã chạy 5.5.4 với H2) | Ghim phiên bản thật vào `docs/ENV.md` |
| Build | `idf.py` (CLI) | **Không** dùng PlatformIO: hỗ trợ H2/IDF mới thường chậm |
| Target | `esp32h2` (node_h2, gw_h2), `esp32` (gw_host) | `idf.py set-target ...` |
| Zigbee | `espressif/esp-zigbee-lib` + `espressif/esp-zboss-lib` qua IDF Component Manager | Ghim phiên bản vào `dependencies.lock` rồi commit file đó `[VERIFY]` |
| Test PC | CMake + trình biên dịch C11 (gcc/clang), CTest | Không phụ thuộc thư viện ngoài |
| Python | 3.10+, `pyserial` | tools |
| Node.js | 20 LTS | Telegram bot |

**`sdkconfig.defaults` tối thiểu** `[VERIFY từng khóa bằng menuconfig]`:
```
# node_h2 / gw_h2
CONFIG_IDF_TARGET="esp32h2"
CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y
CONFIG_FREERTOS_HZ=1000
CONFIG_ZB_ENABLED=y
CONFIG_ZB_RADIO_NATIVE=y
CONFIG_ZB_ZED=y            # node_h2   (gw_h2: CONFIG_ZB_ZCZR=y)
CONFIG_PARTITION_TABLE_CUSTOM=y
CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="partitions.csv"
CONFIG_ESP_TASK_WDT_EN=y
CONFIG_ESP_TASK_WDT_TIMEOUT_S=10
CONFIG_ESP_TASK_WDT_PANIC=y

# gw_host
CONFIG_IDF_TARGET="esp32"
CONFIG_FREERTOS_HZ=1000
CONFIG_ESP_TASK_WDT_EN=y
CONFIG_ESP_TASK_WDT_TIMEOUT_S=10
CONFIG_ESP_TASK_WDT_PANIC=y
CONFIG_MBEDTLS_CERTIFICATE_BUNDLE=y
CONFIG_ESP_WIFI_STATIC_RX_BUFFER_NUM=10
```
**`partitions.csv` cho Zigbee** `[VERIFY theo ví dụ chính thức]`:
```
# Name,     Type, SubType, Offset,  Size
nvs,        data, nvs,     ,        0x6000
phy_init,   data, phy,     ,        0x1000
zb_storage, data, fat,     ,        16K
zb_fct,     data, fat,     ,        1K
factory,    app,  factory, ,        1500K
```

---

## 4. Phân rã Phase và ma trận Task

**Quy ước cột:** `Spec` = mục trong §5 mô tả từng hàm. `Ngữ cảnh` = file đưa cho agent. `DoD` = Definition of Done (lệnh + kết quả quan sát được). `HIL` = cần phần cứng.

### Phase 0 — Môi trường, khung dự án, cổng G0
| ID | Module / file | Mô tả cụ thể | Input → Output | Phụ thuộc | DoD |
|---|---|---|---|---|---|
| T-001 | `docs/ENV.md` | Cài ESP-IDF 5.5.x; build+nạp `hello_world` cho H2 SuperMini (console USB-Serial-JTAG) và DevKit WROOM-32 | — → ghi lệnh thật + phiên bản | — | **HIL:** cả hai board in `chip=... rev=...` qua `esp_chip_info`; `ENV.md` có đủ lệnh để người khác lặp lại |
| T-002 | Cây repo §2, `AGENTS.md`, `.gitignore`, 3 project IDF rỗng | Tạo khung; mỗi project `app_main` in tên + `FW_VERSION` + reset reason | — → 3 project build được | T-001 | `tools/build_all.sh` (hoặc `.ps1`) build cả 3 → exit 0; `git status` sạch; `secrets.h` không bị track |
| T-003 | `config.h` ×3, `shared_config.h`, `secrets.example.h`, `error.h`, `version.h` | Khai báo toàn bộ hằng số theo §5 (chân, chu kỳ, ngưỡng); `#error` khi thiếu `secrets.h` | — → header | T-002 | Xóa `secrets.h` → build gw_host dừng với thông báo rõ; có `secrets.h` → build OK |
| T-004 | `tests/host/` | CMake + `test_util.h` (`ASSERT_TRUE/EQ_INT/NEAR(a,b,eps)`, bộ đếm, `return` code) + 1 test mẫu | — → `ctest` chạy được | T-002 | `cmake -S tests/host -B build_host && cmake --build build_host && ctest --test-dir build_host` → 100 % pass; cố tình làm test sai → ctest fail |
| T-005 | `tools/serial_check.py` | Đọc cổng serial, lọc dòng `BU|...`, kiểm biểu thức mong đợi, timeout, mã thoát 0/1; `--selftest` đọc file log mẫu | log → PASS/FAIL | T-002 | `python tools/serial_check.py --selftest` in `SELFTEST PASS`; `--expect "BU\|SHT31\|.*\|PASS" --timeout 20` hoạt động trên log mẫu |
| **T-006** | `firmware/gw_h2`, `firmware/node_h2`, `docs/API_NOTES.md` | **Spike G0:** coordinator (gw_h2) lập mạng kênh 25; ED (node_h2) join và gửi EP10–15 bằng **giá trị giả**; gw_h2 in giá trị nhận được ra console. Điền `API_NOTES.md` | 2 board H2 → log nhận report | T-001, T-002 | **Cổng G0 (GatewayPlan §10):** S1 build, S2 join ≤ 30 s, S3 nhận đủ 6 trường, S4 1000 report mất ≤ 1 %, S5 rút nguồn Node/GW rồi cấp lại → báo cáo lại ≤ 60 s. Log lưu `tests/results/g0_*.log`. **Trượt → dừng, kích hoạt phương án lùi (GatewayPlan §15)** |

> **Chạy song song:** Phase 1 (cảm biến, breadboard) **không phụ thuộc** G0 và có thể làm cùng lúc với T-006. Phase 2 trở đi **phải chờ G0 đạt**.
>
> **Ghi chú V5 (T-003):** `ZB_TASK_STACK`/`ZB_TASK_PRIO` đang trùng giá trị ở `node_h2` và `gw_h2`; giữ cục bộ theo §5, có thể gom về `shared_config.h` khi cần.

### Phase 1 — Driver & lớp vật lý (breadboard + ứng dụng kiểm tra)

**Sơ đồ đấu breadboard (đối chiếu trước khi nạp):**
```
NODE (H2 SuperMini)                         GATEWAY
 GPIO10 SDA ── SHT31 SDA, CCS811 SDA         H2 GPIO10 TX ─(220Ω)→ WROOM GPIO16 RX2
 GPIO11 SCL ── SHT31 SCL, CCS811 SCL         H2 GPIO11 RX ←(220Ω)─ WROOM GPIO17 TX2     GND chung
 3V3 → SHT31 VIN, CCS811 VCC                 WROOM GPIO25→220Ω→LED_R ; GPIO26→100Ω→LED_G ; GPIO27→100Ω→LED_B ; cathode chung→GND
 CCS811 nWAKE → GND ; ADDR → GND (0x5A)      WROOM GPIO33→1k→NPN base (10k base→GND), còi (+5V), diode ngược
 SHT31 ADDR → GND (0x44)                     WROOM GPIO32 ← nút → GND  (INPUT_PULLUP)
 5V → MQ-2 VCC, Flame VCC                    WROOM GPIO35 ← [10k/15k] ← 5V adapter (trước UPS)
 MQ-2 AO ─22k─┬─22k─GND ; nút giữa→GPIO1 & 100nF→GND
 Flame AO ─22k─┬─22k─GND ; nút giữa→GPIO4 & 100nF→GND ; Flame DO → GPIO14 (tùy chọn)
```
| ID | Module / file | Mô tả cụ thể | Input → Output | Phụ thuộc | DoD |
|---|---|---|---|---|---|
| T-101 | `node_h2/components/i2c_bus`, `bringup/bu_i2c_scan` | Spec §5.3.1. Quét 0x08–0x77, in địa chỉ tìm thấy | SDA/SCL → danh sách địa chỉ | T-002 | **HIL:** `BU|I2C|found=0x44,0x5A|PASS`; rút CCS811 → `found=0x44` và vẫn `PASS` có cảnh báo; không treo |
| T-102 | `components/sht31`, `bringup/bu_sht31` | Spec §5.3.2 (CRC-8, đọc 6 byte, công thức quy đổi) | I2C → `float T,RH` | T-101 | **HIL:** 100 lần đọc, 0 lỗi CRC; `T∈[-10,60]`, `RH∈[0,100]`; so nhiệt kế tham chiếu sai ≤ ±1 °C; log `BU|SHT31|T=..|RH=..|PASS` |
| T-103 | `components/ccs811`, `bringup/bu_ccs811` | Spec §5.3.3 (HW_ID, APP_START, MEAS_MODE, ENV_DATA, ERROR_ID, hồi phục) | I2C + T,RH → eCO2, TVOC | T-101, T-102 | **HIL:** `HW_ID=0x81`; 60 mẫu mode 1; ghi `ENV_DATA` mỗi mẫu; sau khi ngắt nguồn CCS811 rồi cấp lại, driver tự khởi tạo lại ≤ 10 s; `BU|CCS811|eco2=..|tvoc=..|stable=0/1|PASS` |
| T-104 | `components/adc_sensors`, `bringup/bu_adc` | Spec §5.3.4. ADC oneshot + hiệu chuẩn, trung bình 16 mẫu, nhân `ADC_DIV_RATIO`, cờ lỗi thấp/cao | GPIO1, GPIO4 → mV cảm biến | T-002 | **HIL:** cấp 1,65 V (hai điện trở bằng nhau từ 3V3) vào GPIO1 → đọc 1650 ± 3 % (mV tại pin) và cờ OK; nối GND → cờ `LOW`; nối 3V3 qua 0 Ω → không vượt 3,3 V, cờ `HIGH` nếu ≥ 3050 mV |
| T-105 | `components/flame_gpio`, `bringup/bu_flame_do` | Spec §5.3.5. GPIO14 ngắt cạnh, debounce 50 ms, queue từ ISR | DO → sự kiện | T-002 | **HIL:** che/lộ nguồn hồng ngoại → mỗi lần đổi trạng thái đúng 1 sự kiện (không dội); `BU|FLAME|events=N|PASS` |
| T-106 | `components/node_state`, `bringup/bu_node_all` | Spec §5.3.6. Snapshot + cờ (bit §4.3 GatewayPlan) + bộ đếm sấy MQ-2 lưu NVS (`BENCH_MODE` rút ngắn) | tất cả cảm biến → 1 dòng/giây | T-102..T-105 | **HIL:** 30 phút chạy liên tục, không reset; mỗi giây 1 dòng `BU|NODE|T=..|RH=..|eco2=..|tvoc=..|mq2=..|flame=..|flags=0x..`; rút từng cảm biến → bit cờ tương ứng bật ≤ 3 s |
| T-111 | `gw_host/components/alarm_out` (rgb), `bringup/bu_rgb` | Spec §5.5.5a. LEDC 3 kênh 8-bit 5 kHz; `rgb_set(r,g,b)`; màu có tên | — → LED | T-002 | **HIL:** chuỗi Đỏ,Xanh lá,Xanh dương,Vàng,Trắng,Tắt (1 s mỗi màu); đo dòng mỗi chân ≤ 8 mA; `BU|RGB|PASS` do người xác nhận bằng phím |
| T-112 | `alarm_out` (buzzer), `bringup/bu_buzzer` | Spec §5.5.5b. Đầu ra OFF ngay đầu `app_main`; `buzzer_set()`; chế độ passive qua LEDC (tùy chọn) | — → còi | T-002 | **HIL:** nhấn reset nhiều lần → còi **không** kêu lúc boot; chuỗi bíp 100/100/100/700 ms; dòng còi đo được |
| T-113 | `lib_fusion/logic/button_logic`, `alarm_out` (button), `bringup/bu_button` | Spec §5.2.6 + §5.5.5c. Debounce tích phân + phân loại theo thời lượng giữ khi nhả | GPIO32 → sự kiện | T-004 | **Test PC:** `test_logic` đạt (xung dội 5 ms không tạo sự kiện; giữ 0,3/3,5/6 s → SHORT/RESET/JOIN). **HIL:** in `BU|BTN|ev=SHORT|ms=..` đúng |
| T-114 | `power_mon`, `bringup/bu_pwr_sense` | Spec §5.5.6. ADC1 GPIO35, 16 mẫu, hiệu chuẩn, trễ + debounce | GPIO35 → `mains_ok` | T-002 | **HIL:** cắm/rút adapter → đổi trạng thái ≤ 5 s, không dao động khi điện áp ở giữa; in `BU|PWR|mv_pin=..|v_adp=..|mains=1/0|PASS` |
| T-115 | `gw_h2/components/uart_bridge` (thô), `gw_host/components/uart_link` (thô), `bringup/bu_uart_h2`, `bringup/bu_uart_host` | Spec §5.4.3 + §5.5.1. Ping-pong NDJSON: host gửi `{"t":"cmd","op":"ping"}`, H2 đáp `{"t":"ack","op":"ping"}`; thử tràn dòng | UART1↔UART2 | T-201 (codec) | **HIL:** 10 000 vòng ping-pong ở 115200, 0 lỗi; chèn dòng 400 byte → bị bỏ, vòng sau vẫn đúng; console của H2 **không** lẫn vào UART1 (H10) |

### Phase 2 — Giao thức truyền thông nội bộ (**chờ G0**)
| ID | Module / file | Mô tả cụ thể | Input → Output | Phụ thuộc | DoD |
|---|---|---|---|---|---|
| T-201 | `common/proto_ndjson` + `tests/host/test_proto.c` | Spec §5.1. Mã hóa/giải mã mọi bản tin; bộ ghép dòng | struct ↔ dòng NDJSON | T-004 | `ctest`: ≥ 25 ca (hợp lệ, thiếu trường, sai kiểu, quá dài, ký tự lạ, tràn buffer, khóa trùng, khoảng trắng, thứ tự khóa đảo) đều đúng; chạy dưới `-fsanitize=address,undefined` không lỗi |
| T-202 | `node_h2/components/zb_node` | Spec §5.3.7. Tạo EP10–15, join, rejoin có backoff, đặt attribute + gửi report | `node_snapshot` → Zigbee | T-006, T-106 | **HIL:** Node gửi giá trị **thật**; gw_h2 (từ T-203) in đúng giá trị; rút nguồn Node → tự rejoin ≤ 60 s |
| T-203 | `gw_h2/components/zb_coord`, `node_cache` | Spec §5.4.1–§5.4.2. Lập mạng, cửa sổ join, handler report, ánh xạ short↔IEEE, gom 300 ms | Zigbee → `q_zb_evt` | T-006 | **HIL:** 1 `rpt` duy nhất mỗi chu kỳ Node (không nhân đôi, không thiếu trường khi mạng tốt); `seq` tăng đều |
| T-204 | `gw_h2/components/uart_bridge` (đầy đủ) | Spec §5.4.3. `hello`, `hb` 2 s, `join/leave/rpt`, xử lý `cmd` (`permit_join`, `reset_net`, `ping`) | `q_zb_evt` → UART1 | T-201, T-203 | **HIL:** terminal thấy đủ bản tin đúng schema (GatewayPlan §5.1); gửi `permit_join` → onboard LED bật trong cửa sổ |
| T-205 | `gw_host/components/uart_link` (đầy đủ) | Spec §5.5.1. Đọc UART2, ghép dòng, giải mã, đẩy `q_rpt`; theo dõi `hb`; gửi `cmd` | UART2 → `q_rpt`, lỗi liên kết | T-201, T-115 | **HIL:** rút dây TX của H2 → `H2_LINK_LOST` sau 6 ± 1 s; cắm lại → hết lỗi ≤ 3 s; `parse_err` đếm đúng khi chèn rác |
| T-206 | `gw_host/components/node_registry` | Spec §5.5.2. IEEE→`node_id`, `last_rpt_ms`, offline, thống kê `seq` (mất gói) | `rpt` → bảng node | T-205 | **Test PC** (tách logic thuần) + **HIL:** tắt Node → `NODE_OFFLINE` sau 30 ± 2 s; bật lại → xóa lỗi ở report đầu |
| T-207 | `tools/pdr_report.py` | Đọc log, tính PDR theo `seq`, độ trễ, rssi | log → báo cáo | T-205 | **HIL (NFR-4):** 1 giờ, Node cách 3 m, 1 vách, chu kỳ 10 s → PDR ≥ 99 %; lưu báo cáo |
| T-208 | kiểm thử phục hồi | Rút nguồn Node/GW-H2/WROOM lần lượt | — | T-202..T-206 | **HIL:** mỗi trường hợp về trạng thái bình thường ≤ 60 s, không can thiệp tay (NFR-5) |

### Phase 3 — Logic biên & thuật toán Fusion (làm trên PC trước, không cần phần cứng)
| ID | Module / file | Mô tả cụ thể | Input → Output | Phụ thuộc | DoD |
|---|---|---|---|---|---|
| T-301 | `lib_fusion/filters` + `test_filters.c` | Spec §5.2.1: `ma_*`, `med3_*`, `rate_*` (hồi quy tuyến tính) | mẫu → giá trị lọc, dT/dt | T-004 | `ctest`: dốc 10 °C/phút → `rate_get` = 10 ± 0,5; <3 mẫu hoặc <20 s → `false`; tràn `millis` (wrap 32-bit) vẫn đúng; ASAN/UBSAN sạch |
| T-302 | `lib_fusion/prealert` + `test_prealert.c` + tích hợp `report_sched` | Spec §5.2.2 + §5.3.8: vào pre-alert ngay, thoát sau 3 lần bình thường | `node_state` → cờ pre-alert, chu kỳ 3 s/10 s | T-301, T-106 | `ctest` đủ ca; **HIL:** hơ máy sấy → chu kỳ chuyển 10→3 s ≤ 2 s và có report tức thời; hết nóng 3 chu kỳ → về 10 s |
| T-303 | `lib_fusion/fusion` (chấm điểm) + `fusion_params.h` | Spec §5.2.3: tính `s_i`, trọng số, chuẩn hóa theo kênh hợp lệ | `fusion_in_t` → `score` | T-301 | `ctest`: ví dụ tay ở §6.2 (VD-1..VD-3) khớp ± 0,001 |
| T-304 | `fusion` (máy trạng thái, override, latch) + `test_fusion.c` | Spec §5.2.4–§5.2.5: ngưỡng + trễ, `N_UP/N_DOWN`, R1–R3, latch | điểm → `fstate_t` | T-303 | `ctest`: toàn bộ kịch bản S1–S9 (§6.2) đạt; số lần đổi trạng thái khi S dao động quanh ngưỡng ≤ 1 |
| T-305 | `lib_fusion/logic/fault` + test | Spec §5.2.7: mặt nạ lỗi, ưu tiên hiển thị | sự kiện → mặt nạ | T-004 | `ctest`: ưu tiên FIRE > FAULT > WARNING > CAUTION > NORMAL |
| T-306 | `gw_host/components/fusion_ctrl`, `tools/replay.py`, `tests/host/fusion_replay.c`, `tests/vectors/*.csv` | Spec §5.5.3: 1 `fusion_t` mỗi node, baseline NVS, tick 1 s, trạng thái hệ thống = max | `q_rpt` → `sys_state` | T-304, T-206 | `replay.py` chạy `tests/vectors/*.csv` qua `fusion_replay` → khớp file `.golden.csv`; trên board với `gw_sim.py` cho cùng kết quả |
| T-307 | `node_cli`, lệnh `cal_baseline` ở host | Spec §5.3.9 + §5.5.8: lưu `MQ2_BASE`, `FLAME_BASE` (host) và `mq2_base` (node) | lệnh → NVS | T-306 | **HIL:** gọi lệnh khi trạng thái NORMAL → giá trị lưu NVS và còn sau reset; khi trạng thái ≠ NORMAL → bị từ chối |

### Phase 4 — Cảnh báo & ngoại vi Gateway
| ID | Module / file | Mô tả cụ thể | Input → Output | Phụ thuộc | DoD |
|---|---|---|---|---|---|
| T-401 | `lib_fusion/logic/alarm_pattern` + test | Spec §5.2.6: động cơ bước (`pattern_tick`), bộ phân giải ưu tiên `alarm_resolve()` | (state, faults, mute, mains) → (led_pat, buz_pat) | T-004, T-305 | `ctest`: bảng chân trị đầy đủ (mọi tổ hợp state×fault×mute×mains); FIRE luôn thắng; mute chỉ tắt còi, không tắt LED |
| T-402 | `alarm_out` (task) | Spec §5.5.5d: task 50 ms, ghép LED + còi + lớp phủ MAINS_LOST, TWDT | `sys_state` → đầu ra | T-111, T-112, T-401 | **HIL:** dùng `gw_sim.py` ép từng trạng thái → LED/còi đúng bảng §8 GatewayPlan (đã sửa X2); độ trễ từ đổi trạng thái đến đổi đầu ra ≤ 100 ms |
| T-403 | `alarm_out` (input) + `sys_state` | Spec §5.5.5c–e: ánh xạ nút (SHORT=mute 300 s/hủy mute, RESET=reset latch, JOIN=mở join), tín hiệu phản hồi, self-test khi giữ nút lúc boot | sự kiện nút → hành động | T-113, T-402 | **HIL:** FIRE → nhấn ngắn → còi tắt, LED vẫn nháy; sau 300 s còi kêu lại; giữ 3–5 s nhả → reset latch **chỉ** khi đủ điều kiện, nếu từ chối thì 3 chớp đỏ nhanh |
| T-404 | `app_main` (host), TWDT, reset reason | Spec §5.5.9: thứ tự khởi động, đăng ký TWDT, bíp mã reset | — | T-402 | **HIL:** cố tình treo task `fusion` (lệnh debug `CONFIG_DEBUG_HANG`) → reset sau ≤ 10 s; sau boot còi bíp 3 lần; `status` có `reset_reason`; đầu ra không kích hoạt trong lúc reset |

> **Ghi chú R4 (T-003):** Khi đọc `NODES_MAP` (T-206/T-404), bỏ qua mục sentinel có `ieee[0] == '\0'`; hiện bảng chỉ có sentinel `{"",""}`.

### Phase 5 — Cloud & dịch vụ
| ID | Module / file | Mô tả cụ thể | Input → Output | Phụ thuộc | DoD |
|---|---|---|---|---|---|
| T-501 | `net_wifi` + `lib_fusion/logic/backoff` + test | Spec §5.5.7a: STA, sự kiện, backoff 1→60 s | — → cờ `wifi_up` | T-004 | `ctest`: dãy backoff 1,2,4,8,16,32,60,60; **HIL:** tắt router 2 phút → tự nối lại ≤ 60 s sau khi bật |
| T-502 | `net_time` | Spec §5.5.7b: SNTP, `time_epoch()` trả 0 nếu chưa đồng bộ | — → epoch | T-501 | **HIL:** sau khi có mạng ≤ 30 s có epoch hợp lý; mất mạng không làm epoch nhảy lùi |
| T-503 | `net_mqtt` | Spec §5.5.7c: TLS bundle, LWT, subscribe `cmd/gateway`, hàng đợi publish có giới hạn | `q_pub` → broker | T-501 | **HIL:** MQTT Explorer thấy `status/gateway` online (retain); rút nguồn gateway → LWT `online:false` sau keepalive; hàng đợi không tràn heap |
| T-504 | `payload` + test | Spec §5.5.7d: dựng JSON `sensor/*`, `alert/*`, `status/gateway` bằng `snprintf` có kiểm tràn | struct → chuỗi | T-004, T-306 | `ctest`: đúng khóa/kiểu, escape đúng, không quá `PAYLOAD_MAX`; khớp mẫu GatewayPlan §5.2 |
| T-505 | `cmd_handler` | Spec §5.5.8: danh sách trắng lệnh, giới hạn 1 lệnh/giây | MQTT → hành động | T-503 | **HIL:** gửi từng lệnh hợp lệ → thực thi; lệnh lạ/JSON hỏng/quá nhanh → bỏ + log, không crash |
| T-506 | `cloud/dashboard` | Spec §5.6.1: MQTT over WSS, thẻ Node, biểu đồ, bảng cảnh báo, phát hiện "stale" | MQTT → UI | T-503 | Mở trang: dữ liệu cập nhật ≤ 2 s; `alert` hiện đúng màu; ngắt gateway → UI báo offline ≤ 90 s; **không** có credential publish trong mã |
| T-507 | `cloud/telegram_bot` | Spec §5.6.2: subscribe `alert/#` + `status/gateway`, chống trùng, lặp lại khi FIRE, polling | MQTT → Telegram | T-503 | Gửi `alert` giả → tin nhắn tới ≤ 10 s (NFR-3); cùng trạng thái trong 60 s không gửi lại; leo thang thì gửi ngay |
| T-508 | đo độ trễ đầu-cuối | `tools/gw_sim.py --scenario fire` + đo mốc thời gian | — | T-506, T-507 | NFR-2 (≤ 5 s dashboard), NFR-3 (≤ 10 s Telegram) có số đo thật trong `tests/results/` |

### Phase 6 — Tích hợp & kiểm thử thực nghiệm
| ID | Nội dung | Phụ thuộc | DoD |
|---|---|---|---|
| T-601 | Mất điện lưới: rút adapter Node và Gateway khi đang chạy; lặp 20 lần | T-402, T-114 | Không reset ngoài ý muốn; `MAINS_LOST` ≤ 5 s; MQTT `power:"battery"`; (T-PWR-01/02/05) |
| T-602 | Mất mạng: tắt WiFi, tắt broker, rút UART, tắt Node | T-503, T-205 | Mọi ca theo bảng §1.3; LED/còi không bị ảnh hưởng (T-FLT-01..03, 05, 06) |
| T-603 | Thử nghiệm buồng nhiệt/khói giả lập (hộp kín thoáng, máy sấy, nhang, giấy cháy chậm; **an toàn theo GatewayPlan §11.4**) | T-306, T-402 | Thu log CSV có nhãn; T-SYS-01..03 |
| T-604 | Chống báo giả 24 h (nấu ăn, hơi nước, máy sấy tóc) | T-603 | 0 lần FIRE (NFR-7) |
| T-605 | Hiệu chỉnh & đóng băng tham số: CAL-01/02, replay log thật, so sánh fusion vs từng cảm biến đơn | T-603, T-604 | Bảng tham số kèm tên log nguồn; bảng so sánh trễ/báo giả (T-SYS-05) |
| T-606 | Đo thời gian chạy pin dự phòng | T-601 | Ghi số giờ thật (NFR-6 ≥ 2 h) |
| T-607 | Tổng hợp kết quả, cập nhật `GatewayPlan.md`/Errata, gắn tag `v0.1.0` | tất cả | Mọi `[VERIFY]` đã chuyển thành giá trị đã đo hoặc được ghi là chưa xác minh |

---

## 5. Đặc tả chi tiết từng module và từng hàm

> **Quy ước:** `[VERIFY]` = chữ ký API phải được xác minh trong header cài thật (H11). Các chữ ký ESP-IDF/Zigbee dưới đây là **ý định thiết kế**, không phải bằng chứng API tồn tại. Hàm tự viết (`proto_*`, `ma_*`, `fusion_*`...) là chữ ký **cố định**: agent không được đổi.

### 5.1 `firmware/common/proto_ndjson` (C99 thuần, không phụ thuộc IDF)

**Kiểu dữ liệu**
```c
#define PROTO_MAX_LINE 256
#define PROTO_IEEE_LEN 17                 /* 16 hex + NUL */
typedef enum { PROTO_OK=0, PROTO_E_TOO_LONG=-1, PROTO_E_SYNTAX=-2,
               PROTO_E_TYPE=-3, PROTO_E_FIELD=-4, PROTO_E_BUF=-5 } proto_err_t;
typedef enum { M_NONE=0, M_HELLO, M_HB, M_JOIN, M_LEAVE, M_RPT, M_CMD, M_ACK } msg_type_t;
typedef enum { OP_NONE=0, OP_PERMIT_JOIN, OP_RESET_NET, OP_PING } cmd_op_t;
#define RPT_HAS_TEMP  (1u<<0)   /* HUM=1, ECO2=2, TVOC=3, MQ2=4, FLAME=5, FLAGS=6 */
typedef struct { char ieee[PROTO_IEEE_LEN]; uint16_t sa; uint32_t seq;
                 float temp, hum, eco2, tvoc, mq2_mv, flame_mv;
                 uint16_t flags; int16_t rssi; uint8_t lqi; uint32_t valid; } rpt_t;
typedef struct { msg_type_t type;
  union { struct{char fw[24]; uint8_t ch; uint16_t pan;} hello;
          struct{uint32_t up; uint8_t nodes;} hb;
          struct{char ieee[PROTO_IEEE_LEN]; uint16_t sa;} join, leave;
          rpt_t rpt;
          struct{cmd_op_t op; uint16_t sec;} cmd;
          struct{cmd_op_t op;} ack; } u; } proto_msg_t;
```
**Bộ mã hóa** — chữ ký chung `int proto_enc_xxx(char *buf, size_t cap, ...)`; trả **độ dài (gồm `\n`, không gồm NUL)** hoặc `PROTO_E_BUF` nếu `cap` không đủ (khi đó `buf[0]=0`, tuyệt đối không ghi quá `cap`).
| Hàm | Đầu ra mẫu / quy tắc |
|---|---|
| `proto_enc_hello(buf,cap,fw,ch,pan)` | `{"t":"hello","fw":"gwh2-0.1.0","ch":25,"pan":"0x1A62"}\n` |
| `proto_enc_hb(buf,cap,up,nodes)` | `{"t":"hb","up":123,"nodes":1}\n` |
| `proto_enc_join/leave(buf,cap,ieee,sa)` | `{"t":"join","ieee":"0011223344556677","sa":"0x1234"}\n` |
| `proto_enc_rpt(buf,cap,const rpt_t*)` | Chỉ in trường có bit trong `valid`. `temp,hum` `%.2f`; `eco2,tvoc` `%.0f`; `mq2_mv,flame_mv` `%.1f`; **giá trị không hữu hạn (NaN/Inf) bị bỏ và xóa bit `valid`**. Luôn có `t`,`ieee`,`seq` |
| `proto_enc_cmd(buf,cap,op,sec)` | `{"t":"cmd","op":"permit_join","sec":180}\n` (`sec` chỉ in với `permit_join`) |
| `proto_enc_ack(buf,cap,op)` | `{"t":"ack","op":"ping"}\n` |

**Bộ giải mã**
| Hàm | Hành vi bắt buộc |
|---|---|
| `proto_err_t proto_dec(const char *line, size_t len, proto_msg_t *out)` | (1) cắt `\r`/`\n` cuối; (2) `len==0` hoặc `>=PROTO_MAX_LINE` → `TOO_LONG`; (3) phải bắt đầu `{`, kết thúc `}` → else `SYNTAX`; (4) lấy khóa `t` (chuỗi) → ánh xạ `msg_type_t`, lạ → `TYPE`; (5) với từng loại lấy trường **bắt buộc** (`rpt`: `ieee`,`seq`; `cmd`: `op`; `join/leave`: `ieee`) — thiếu → `FIELD`; (6) trường tùy chọn thiếu → không đặt bit `valid`; (7) số parse bằng `strtod/strtol` và **kiểm `endptr`**; (8) chuỗi không chứa `\` và không vượt giới hạn; (9) **khóa lạ bị bỏ qua** (tương thích tiến) |
| `int flat_find(const char *s, size_t n, const char *key, const char **vs, size_t *vl)` | Máy trạng thái duyệt ký tự, theo dõi `in_string` và escape; chỉ khớp khóa ở **độ sâu 1**, sau khóa là `:` (cho phép khoảng trắng). Không khớp nhầm khi chuỗi `key` xuất hiện **bên trong giá trị chuỗi**. Trả 1 nếu tìm thấy |
| `int flat_get_num(...,double*)`, `flat_get_str(...,char *out,size_t cap)` | Dựa trên `flat_find`; `flat_get_str` bỏ dấu nháy, từ chối nếu quá `cap` |

**Bộ ghép dòng**
```c
typedef void (*line_cb_t)(const char *line, size_t len, void *ctx);
typedef struct { char buf[PROTO_MAX_LINE]; size_t len; bool overflow;
                 uint32_t n_lines, n_overflow; } line_asm_t;
void line_asm_init(line_asm_t *la);
size_t line_asm_feed(line_asm_t *la, const uint8_t *d, size_t n, line_cb_t cb, void *ctx);
```
- Gặp `\n`: nếu không tràn và `len>0` → gọi `cb(buf,len,ctx)`; reset `len`.
- Vượt `PROTO_MAX_LINE-1` byte mà chưa có `\n`: `overflow=true`, **bỏ mọi byte cho tới `\n` kế tiếp**, `n_overflow++`, **không** gọi `cb` cho dòng đó.
- Bỏ qua dòng rỗng. Trả về số dòng đã gọi `cb`.

### 5.2 `firmware/lib_fusion` (C99 thuần; mọi hàm test được trên PC)

#### 5.2.1 `filters.h/.c`
```c
#define MA_MAX_WIN 16
typedef struct { float buf[MA_MAX_WIN]; uint8_t win, n, head; } ma_t;
void  ma_init(ma_t*, uint8_t win);        /* win kẹp vào [1,16] */
void  ma_push(ma_t*, float x);            /* bỏ qua NaN/Inf */
float ma_value(const ma_t*);              /* trung bình n mẫu; 0 nếu n==0; TÍNH LẠI tổng mỗi lần (không tích lũy sai số) */
bool  ma_ready(const ma_t*);              /* n==win */
void  ma_reset(ma_t*);

typedef struct { float v[3]; uint8_t n; } med3_t;
void  med3_init(med3_t*);
float med3_push(med3_t*, float x);        /* n<3 → trả x; n>=3 → trung vị 3 mẫu gần nhất */

#define RATE_MAX_SAMPLES 64
#define RATE_MIN_SPAN_MS 20000u
typedef struct { uint32_t t[RATE_MAX_SAMPLES]; float v[RATE_MAX_SAMPLES];
                 uint8_t n, head; uint32_t window_ms; } rate_t;
void rate_init(rate_t*, uint32_t window_ms);
void rate_push(rate_t*, uint32_t t_ms, float v);
bool rate_get(const rate_t*, float *per_min);
```
- `rate_push`: ghi vòng; **loại mẫu cũ hơn `window_ms`** bằng phép so sánh `(int32_t)(t_ms - t_i) > (int32_t)window_ms` (an toàn khi `millis` tràn 32 bit).
- `rate_get`: cần `n>=3` **và** khoảng thời gian (mẫu mới nhất − cũ nhất) `>= RATE_MIN_SPAN_MS`, nếu không trả `false`. Tính **hồi quy bình phương tối thiểu** với `x_i=(t_i−t_0)/1000.0` (giây), dùng `double`:
  `slope = (n·Σxy − Σx·Σy) / (n·Σx² − (Σx)²)`; nếu mẫu số `< 1e-9` trả `false`; `*per_min = slope·60`.

#### 5.2.2 `prealert.h/.c` (dùng trên **Node**)
```c
enum { PA_T=1<<0, PA_DTDT=1<<1, PA_MQ2=1<<2, PA_FLAME=1<<3, PA_ECO2=1<<4 };
typedef struct { float t_on_c, dtdt_on_c_min, mq2_rel_on, eco2_on_ppm; } prealert_params_t;   /* mặc định 45, 5, 0.5, 1000 [PLACEHOLDER] */
typedef struct { uint32_t t_ms; float temp, eco2, mq2_mv; bool flame_do; uint16_t flags; } prealert_in_t;
typedef struct { prealert_params_t p; float mq2_base_mv; uint8_t clear_n, clear_cnt; bool active;
                 med3_t m3; rate_t rate; } prealert_t;
void prealert_init(prealert_t*, const prealert_params_t*, float mq2_base_mv, uint8_t clear_n);
void prealert_set_baseline(prealert_t*, float mq2_base_mv);
bool prealert_update(prealert_t*, const prealert_in_t*, uint8_t *reasons);
```
`prealert_update` theo thứ tự:
1. Nếu cờ `bit0` (SHT lỗi) = 0: `tf = med3_push(temp)`; `rate_push(t_ms, tf)`; điều kiện `PA_T` = `tf >= t_on_c`; nếu `rate_get` được và `>= dtdt_on_c_min` → `PA_DTDT`.
2. MQ-2: chỉ xét khi `mq2_base_mv>0`, `bit2` (sấy)=0, `bit5` (ADC lỗi)=0: `rel=(mq2−base)/base`; `rel >= mq2_rel_on` → `PA_MQ2`.
3. `flame_do` → `PA_FLAME`. `eco2` chỉ xét khi `bit1`=0: `>= eco2_on_ppm` → `PA_ECO2`.
4. `mask != 0` → `active=true; clear_cnt=0`. `mask == 0` → `clear_cnt++`; nếu `active && clear_cnt>=clear_n` → `active=false`.
5. `*reasons = mask` (của lần đánh giá hiện tại); trả `active`.

#### 5.2.3 `fusion_params.h` và chấm điểm
```c
enum { CH_T, CH_DT, CH_CO2, CH_TVOC, CH_MQ2, CH_FL, CH_COUNT };
typedef enum { ST_NORMAL=0, ST_CAUTION, ST_WARNING, ST_FIRE } fstate_t;
typedef struct {
  float lo[CH_COUNT], hi[CH_COUNT], w[CH_COUNT];
  float up[4], down[4];            /* ngưỡng vào/ra theo mức; chỉ dùng chỉ số 1..3 */
  uint8_t n_up, n_down;            /* 2, 3 */
  float t_hard_c; uint8_t t_hard_n;            /* 65, 2   (R1) */
  uint8_t flame_n; float flame_gate;           /* 3, 0.2  (R2) */
  float r3_dtdt, r3_t_min; uint32_t r3_hold_ms;/* 10, 40, 30000 (R3) */
  uint32_t latch_clear_ms;                     /* 60000 */
  float min_valid_weight;                      /* 0.30 */
  uint8_t ma_win_gas, ma_win_ana;              /* 3 (eCO2,TVOC), 5 (mq2, flame) */
  uint32_t rate_window_ms;                     /* 60000 */
} fusion_params_t;
void fusion_params_default(fusion_params_t*);  /* nạp đúng bảng GatewayPlan §7.2–§7.4, toàn bộ [PLACEHOLDER] */
static inline float score_lin(float x, float lo, float hi);  /* clamp((x-lo)/(hi-lo),0,1); hi<=lo → 0 */
```
Mặc định: `lo/hi/w` = T 35/65/0,25 · DT 2/8/0,15 · CO2 800/1500/0,15 · TVOC 50/400/0,10 · MQ2 0,2/1,0/0,20 · FL 0,1/0,5/0,15. `up` = {-,0,25 ; 0,45 ; 0,65}, `down` = {-,0,15 ; 0,35 ; 0,55}.

#### 5.2.4 `fusion.h/.c` — hàm và thứ tự xử lý
```c
enum { FL_SHT_FAIL=1<<0, FL_CCS_BAD=1<<1, FL_MQ2_WARM=1<<2, FL_FLAME_DO=1<<3, FL_PREALERT=1<<4, FL_ADC_FAULT=1<<5 };
enum { RS_SCORE=1<<0, RS_R1=1<<1, RS_R2=1<<2, RS_R3=1<<3, RS_INSUFF=1<<4 };
typedef struct { uint32_t t_ms; float temp, hum, eco2, tvoc, mq2_mv, flame_mv; uint16_t flags; uint32_t valid; } fusion_in_t;
typedef struct { fstate_t state, prev; bool changed, latched, insufficient;
                 float score, s[CH_COUNT], dtdt, valid_weight; uint32_t reasons; } fusion_out_t;
typedef struct fusion_s fusion_t;   /* định nghĩa đầy đủ trong .h để cấp phát tĩnh */
void fusion_init(fusion_t*, const fusion_params_t*, float mq2_base_mv, float flame_base_mv);
void fusion_set_baselines(fusion_t*, float mq2_base_mv, float flame_base_mv);
void fusion_update(fusion_t*, const fusion_in_t*, fusion_out_t *out);   /* gọi mỗi report */
void fusion_tick(fusion_t*, uint32_t now_ms);                           /* gọi mỗi 1 s */
bool fusion_reset_latch(fusion_t*);                                     /* true nếu được phép */
fstate_t fusion_state(const fusion_t*);
float fusion_ma_mq2(const fusion_t*); float fusion_ma_flame(const fusion_t*); bool fusion_ma_ready(const fusion_t*);
```
`fusion_update`, **đúng thứ tự**:
1. **Hợp lệ từng kênh:** `T,DT` hợp lệ khi `!(flags&FL_SHT_FAIL)` và bit `valid` có `temp`; `CO2,TVOC` khi `!(flags&FL_CCS_BAD)`; `MQ2` khi `!(flags&(FL_MQ2_WARM|FL_ADC_FAULT))` và `base>0`; `FL` khi `!(flags&FL_ADC_FAULT)` và `base>0` (hoặc `FL_FLAME_DO` đặt).
2. **Lọc:** `med3` rồi `rate_push` cho `temp`; `ma_push` cho `eco2,tvoc` (cửa sổ `ma_win_gas`) và `mq2_mv,flame_mv` (`ma_win_ana`). Chỉ đẩy khi kênh hợp lệ.
3. **Đại lượng:** `mq2_rel=(ma_mq2−base)/base`; `flame_drop=(base−ma_flame)/base` (module LM393 thường **giảm** áp khi có lửa — hằng `FLAME_AO_INVERTED` đảo dấu nếu thực nghiệm ngược); `dtdt` từ `rate_get` (không có → kênh DT không hợp lệ).
4. **Điểm kênh:** `s[i]=score_lin(x_i, lo[i], hi[i])`.
5. **Tổng:** `vw = Σ w[i]` của kênh hợp lệ. Nếu `vw < min_valid_weight` → `insufficient=true`, **giữ nguyên `state`**, `reasons|=RS_INSUFF`, thoát. Ngược lại `score = Σ w[i]·s[i] / vw`.
6. **Ứng viên:** `cand_up` = mức cao nhất `L` có `score >= up[L]`; `cand_stay` = mức cao nhất có `score >= down[L]`.
7. **Override** (cập nhật bộ đếm trước khi quyết định):
   - R1: `temp >= t_hard_c` → `r1_cnt++` else `r1_cnt=0`; `r1_cnt >= t_hard_n` ⇒ ép `FIRE`.
   - R2: `flame_active = (flags&FL_FLAME_DO) || s[CH_FL] >= 0.5`; đúng → `r2_cnt++` else `0`; `r2_cnt >= flame_n` **và** (`s[CH_T] > flame_gate` hoặc `s[CH_MQ2] > flame_gate` hoặc `s[CH_CO2] > flame_gate`) ⇒ ép `FIRE`.
   - R3: `dtdt >= r3_dtdt` **và** `temp >= r3_t_min` → cộng dồn `r3_hold_ms`; đủ `r3_hold_ms` ⇒ sàn `WARNING`. Điều kiện ngắt → xóa bộ cộng.
8. **Leo/hạ mức** (đếm theo **report**):
   - Nếu `cand_up > state` → `up_cnt++; down_cnt=0`; khi `up_cnt >= n_up` → `state=cand_up`.
   - Else nếu `cand_stay < state` và `state != FIRE hoặc !latched` → `down_cnt++; up_cnt=0`; khi `down_cnt >= n_down` → `state = state−1` (**mỗi lần hạ một mức**).
   - Else → `up_cnt=down_cnt=0`.
   - Override ép mức **ngay lập tức** (bỏ qua `n_up`) và đặt `reasons` tương ứng. Sàn R3 áp dụng sau cùng: `state = max(state, WARNING)` khi R3 đang hoạt động.
9. **Latch:** vào `FIRE` → `latched=true`, `clear_ms=0`. Khi `latched` mọi bước hạ mức (8) bị chặn.
10. Điền `out` (`changed = (state != prev)`, `reasons` có `RS_SCORE` nếu mức đến từ điểm tổng).

`fusion_tick(now)`: nếu `latched`: tính `dt = now−last_tick`; nếu `score < down[ST_FIRE]` **và** `r1_cnt==0` **và** `r2_cnt==0` → `clear_ms += dt` else `clear_ms=0`.
`fusion_reset_latch`: chỉ khi `latched && clear_ms >= latch_clear_ms`: `state=NORMAL; latched=false; up_cnt=down_cnt=0; r1/r2/r3=0; return true`. Ngược lại `return false` và **không** đổi gì.

#### 5.2.5 Quy ước baseline
`mq2_base`, `flame_base` do `fusion_ctrl_cal_baseline()` lưu NVS (§5.5.3). Khi chưa có baseline (=0) → kênh `MQ2`/`FL` không hợp lệ (fusion vẫn chạy bằng kênh còn lại) và bật cờ `RS_INSUFF` nếu còn quá ít kênh.

#### 5.2.6 `logic/alarm_pattern`, `logic/button_logic`
```c
typedef enum { C_OFF=0, C_RED, C_GREEN, C_BLUE, C_YELLOW, C_MAGENTA, C_WHITE, C_TRANSPARENT=0xFF } color_t;
typedef struct { uint32_t dur_ms; uint8_t out; } pstep_t;       /* LED: out=color_t ; còi: 0/1 ; 0xFF = trong suốt (lớp phủ) */
typedef struct { const pstep_t *steps; uint8_t n, idx; uint32_t t_start; } pengine_t;
void    pengine_start(pengine_t*, const pstep_t*, uint8_t n, uint32_t now_ms);
uint8_t pengine_out(pengine_t*, uint32_t now_ms);   /* tiến idx theo (now - t_start) modulo tổng chu kỳ; xử lý khoảng nhảy thời gian lớn; không trôi */
```
**Bảng mẫu** (nguồn chân lý cho cảnh báo, thay bảng §8 của GatewayPlan):
| Mẫu | Bước `(dur_ms, out)` |
|---|---|
| `LED_NORMAL` | `(1000,C_GREEN)` |
| `LED_CAUTION` | `(1000,C_YELLOW)` |
| `LED_WARNING` | `(250,C_YELLOW)(250,C_OFF)` |
| `LED_FIRE` | `(125,C_RED)(125,C_OFF)` |
| `LED_FAULT` | `(500,C_BLUE)(500,C_OFF)` |
| `LED_OVL_MAINS` | `(300,C_MAGENTA)(4700,C_TRANSPARENT)` |
| `BZ_OFF` | `(1000,0)` |
| `BZ_WARN` | `(100,1)(4900,0)` |
| `BZ_FIRE` | `(500,1)(500,0)` |
| `BZ_FAULT` | `(100,1)(150,0)(100,1)(59650,0)` |
| `BZ_OVL_MAINS` | `(100,1)(299900,0)` |
```c
typedef struct { fstate_t state; uint32_t faults; bool mute_active, mains_lost; } alarm_in_t;
typedef struct { uint8_t led_pat, buz_pat; bool led_overlay, buz_overlay; } alarm_sel_t;
void alarm_resolve(const alarm_in_t*, alarm_sel_t*);
```
`alarm_resolve`: (1) `FIRE` → `LED_FIRE`,`BZ_FIRE`; (2) else nếu `faults & (NODE_OFFLINE|H2_LINK_LOST|SENSOR_FAULT)` → `LED_FAULT`,`BZ_FAULT`; (3) else theo `WARNING/CAUTION/NORMAL`; (4) `mute_active` ⇒ `buz_pat=BZ_OFF` và `buz_overlay=false` (**LED không đổi**); (5) `led_overlay = mains_lost && state != FIRE`; `buz_overlay = mains_lost && !mute_active && buz_pat==BZ_OFF`.

```c
typedef enum { BTN_EV_NONE=0, BTN_EV_SHORT, BTN_EV_RESET, BTN_EV_JOIN } btn_ev_t;
typedef struct { uint8_t integ; bool stable, was_stable; uint32_t press_t; } btn_t;
void      btn_init(btn_t*);
btn_ev_t  btn_update(btn_t*, bool raw_pressed, uint32_t now_ms);   /* gọi mỗi 10 ms */
uint8_t   btn_hold_cue(const btn_t*, uint32_t now_ms);             /* 0; 1 khi giữ >=3s; 2 khi >=5s */
```
`btn_update`: bộ tích phân `integ += raw?+1:-1`, kẹp `[0,3]`; `stable=true` khi `integ==3`, `false` khi `0`. Khi `stable` chuyển lên → ghi `press_t`. Khi `stable` chuyển xuống (**nhả**) → `dur = now−press_t`: `<50` → NONE (nhiễu); `50..999` → SHORT; `1000..2999` → NONE; `3000..4999` → RESET; `>=5000` → JOIN.

#### 5.2.7 `logic/fault`
```c
enum { F_NODE_OFFLINE=1<<0, F_H2_LINK_LOST=1<<1, F_SENSOR_FAULT=1<<2, F_MAINS_LOST=1<<3, F_FUSION_INSUFF=1<<4 };
bool fault_is_display(uint32_t mask);   /* mask & (OFFLINE|H2_LINK|SENSOR) */
bool node_is_offline(uint32_t now_ms, uint32_t last_rpt_ms, uint32_t interval_hint_ms);  /* (uint32_t)(now-last) > max(30000, 3*interval_hint) */
bool link_is_lost(uint32_t now_ms, uint32_t last_hb_ms, uint32_t timeout_ms);
```
#### 5.2.8 `logic/backoff`, `logic/pubq`
```c
uint32_t backoff_next(uint32_t *attempt, uint32_t min_ms, uint32_t max_ms, uint32_t rnd16);
void     backoff_reset(uint32_t *attempt);
```
`delay = min(max, min·2^attempt)`; `attempt` tăng đến trần (không tràn số); trải ±10 % bằng `rnd16` (0..65535): `delay += delay·((rnd16−32768)/32768.0)·0.10`.
```c
#define PUBQ_CAP 16
typedef enum { PRIO_TELEM=0, PRIO_ALERT=1, PRIO_STATUS=2 } pub_prio_t;
typedef struct { char topic[48]; char payload[320]; uint8_t qos, retain, prio; } pub_item_t;
void pubq_init(void);  bool pubq_push(const pub_item_t*); bool pubq_pop(pub_item_t*); bool pubq_push_front(const pub_item_t*); uint32_t pubq_drops(void);
```
`pubq_push` khi đầy: **bỏ mục `PRIO_TELEM` cũ nhất**; nếu không có mục TELEM → bỏ mục `ALERT` cũ nhất (giữ cái mới); mục `STATUS` chỉ giữ bản mới nhất (đẩy bản mới thay bản cũ cùng topic). Bảo vệ bằng mutex (có hook để PC test dùng `NULL` mutex).

### 5.3 `firmware/node_h2` — các component

#### 5.3.1 `i2c_bus`
| Hàm | Hành vi |
|---|---|
| `esp_err_t i2c_bus_init(int sda,int scl,uint32_t hz)` | Tạo master bus bằng driver I2C mới của IDF 5.x `[VERIFY]` (`i2c_new_master_bus`); `glitch_ignore_cnt=7`; bật pull-up nội bộ **chỉ** nếu `I2C_INTERNAL_PULLUP=1`; tạo mutex đệ quy |
| `esp_err_t i2c_bus_probe(uint8_t addr)` | `i2c_master_probe` `[VERIFY]`, timeout 50 ms |
| `i2c_dev_write / i2c_dev_write_read / i2c_dev_read` | Lấy mutex → thêm thiết bị vào bus nếu chưa có (cache ≤ 4) → truyền, timeout 100 ms → nhả mutex. Lỗi: trả `esp_err_t`, **không** `ESP_ERROR_CHECK` |
| `esp_err_t i2c_bus_recover(void)` | Nếu SDA bị giữ thấp: chuyển chân thành GPIO, xung SCL 9 lần, STOP; xóa bus và tạo lại, thêm lại thiết bị đã cache |
CCS811 có clock stretching: kiểm tra tham số chờ SCL trong cấu hình thiết bị (`scl_wait_us`) `[VERIFY]`.

#### 5.3.2 `sht31` (địa chỉ 0x44)
Dữ kiện datasheet (**phải đưa vào prompt**): lệnh đo một lần độ lặp cao, **không** kéo dãn xung: `0x24 0x00`; chờ **≥ 16 ms**; đọc 6 byte `T_MSB T_LSB CRC RH_MSB RH_LSB CRC`; soft reset `0x30 0xA2`; đọc status `0xF3 0x2D`. CRC-8: đa thức `0x31`, khởi tạo `0xFF`, không đảo bit; **vector thử: `{0xBE,0xEF}` → `0x92`**. `T = −45 + 175·raw/65535`; `RH = 100·raw/65535`.
| Hàm | Hành vi |
|---|---|
| `uint8_t sht31_crc8(const uint8_t*d,size_t n)` | Như trên (tĩnh, có test PC) |
| `esp_err_t sht31_init(void)` | probe → soft reset → chờ 2 ms → đọc status (kiểm CRC) |
| `esp_err_t sht31_read(float *t, float *rh)` | ghi lệnh → chờ 16 ms (`/* conv-wait <=20ms */`) → đọc 6 byte → kiểm **cả hai** CRC → quy đổi → kẹp `RH∈[0,100]`; thử lại tối đa 2 lần; lỗi trả `ESP_ERR_INVALID_CRC`/`ESP_FAIL`, không ghi vào `*t,*rh` |

#### 5.3.3 `ccs811` (địa chỉ 0x5A, nWAKE nối GND)
Dữ kiện (**đưa vào prompt**): `STATUS=0x00` (bit7 FW_MODE, bit4 APP_VALID, bit3 DATA_READY, bit0 ERROR); `MEAS_MODE=0x01`; `ALG_RESULT_DATA=0x02` (8 byte: `eCO2_hi eCO2_lo TVOC_hi TVOC_lo STATUS ERROR_ID raw_hi raw_lo`); `ENV_DATA=0x05`; `HW_ID=0x20` (kỳ vọng `0x81`); `ERROR_ID=0xE0`; `APP_START=0xF4` (ghi không dữ liệu); `SW_RESET=0xFF` + `{0x11,0xE5,0x72,0x8A}`. `MEAS_MODE` chế độ 1 (đo mỗi giây, không ngắt) = `0x10`. `ENV_DATA`: `hum_raw=(uint16)(RH·512+0.5)`, `temp_raw=(uint16)((T+25)·512+0.5)`, ghi **big-endian** `{hum_hi,hum_lo,temp_hi,temp_lo}`.
| Hàm | Hành vi |
|---|---|
| `esp_err_t ccs811_init(void)` | probe → đọc `HW_ID`≠0x81 → `ESP_ERR_NOT_FOUND`; `SW_RESET`, chờ 20 ms; đọc `STATUS`, nếu `APP_VALID=0` → lỗi; ghi `APP_START`, chờ 20 ms; đọc `STATUS` yêu cầu `FW_MODE=1`; ghi `MEAS_MODE=0x10`; ghi `t_start` |
| `esp_err_t ccs811_set_env(float t_c,float rh)` | Kẹp `T∈[-25,50]`, `RH∈[0,100]`, ghi 4 byte như trên |
| `esp_err_t ccs811_read(uint16_t *eco2,uint16_t *tvoc,bool *ready)` | Đọc `STATUS`; nếu `ERROR=1` → đọc `ERROR_ID`, đếm lỗi; nếu `DATA_READY=0` → `*ready=false`, trả `ESP_OK`; ngược lại đọc 8 byte, ghép, kiểm dải (`eco2∈[400,8192]`, `tvoc∈[0,1187]`) → ngoài dải coi là lỗi mẫu |
| `bool ccs811_stable(void)` | `now−t_start >= CCS811_STABLE_MS` (20 phút; `BENCH_MODE`: 2 phút) |
| Phục hồi | 3 lỗi liên tiếp → `ccs811_init()` lại, giãn cách 5 s (`backoff`), trong thời gian đó cờ `FL_CCS_BAD` |

#### 5.3.4 `adc_sensors`
| Hàm | Hành vi |
|---|---|
| `esp_err_t adc_sensors_init(void)` | `adc_oneshot_new_unit(ADC_UNIT_1)`; ánh xạ GPIO→kênh bằng `adc_oneshot_io_to_channel` (**không** viết cứng số kênh); `ADC_ATTEN_DB_12`; tạo hiệu chuẩn `adc_cali_create_scheme_curve_fitting` `[VERIFY]` (fallback: tính mV tuyến tính và ghi cảnh báo) |
| `int adc_read_pin_mv(sns_id_t id)` | Đọc `ADC_SAMPLES`=16 lần (`adc_oneshot_read` → `adc_cali_raw_to_voltage`), **bỏ mẫu min và max**, trung bình phần còn lại; lỗi đọc → trả `-1` |
| `esp_err_t adc_read_sensor_mv(sns_id_t id,float *mv,uint8_t *adc_flags)` | `mv_pin` → `*mv = mv_pin * ADC_DIV_RATIO`; `adc_flags`: `ADC_F_LOW` nếu `mv_pin < ADC_FAULT_LOW_MV`(20); `ADC_F_HIGH` nếu `mv_pin >= ADC_FAULT_HIGH_MV`(3050) |
`sns_id_t`: `SNS_MQ2` (GPIO1), `SNS_FLAME` (GPIO4).

#### 5.3.5 `flame_gpio`
| Hàm | Hành vi |
|---|---|
| `esp_err_t flame_gpio_init(QueueHandle_t q_evt)` | GPIO14 input, ngắt `ANYEDGE`, `gpio_install_isr_service` `[VERIFY]` |
| `static void IRAM_ATTR flame_isr(void*)` | Đọc mức; `xQueueSendFromISR` một `node_evt_t{NODE_EVT_FLAME, level}`; `portYIELD_FROM_ISR` nếu cần. **Không** log, không debounce trong ISR |
| `bool flame_do_active(void)` | Mức ổn định đã debounce (sự kiện cách nhau < 50 ms bị bỏ); `FLAME_DO_ACTIVE_LEVEL` (LM393 thường = 0) |

#### 5.3.6 `node_state`
```c
typedef struct { uint32_t t_ms; float temp, hum; uint16_t eco2, tvoc; float mq2_mv, flame_mv;
                 bool flame_do, prealert; uint16_t flags; uint32_t seq; } node_snapshot_t;
void     node_state_init(void);                     /* mutex + nạp bộ đếm sấy từ NVS */
void     node_state_update(const node_snapshot_t*); /* giữ khóa < 1 ms */
void     node_state_get(node_snapshot_t*);
uint16_t node_flags_compute(bool sht_fail,bool ccs_bad,bool mq2_warm,bool flame_do,bool prealert,bool adc_fault);
void     node_warmup_tick_1min(void);               /* cộng 60000 ms; ghi NVS mỗi 10 lần gọi (giảm mòn flash) */
bool     node_mq2_warm(void);                       /* tổng < MQ2_WARMUP_MS */
```
Bit cờ đúng GatewayPlan §4.3 (`bit0..5`).

#### 5.3.7 `zb_node` (ED không ngủ)
| Hàm | Hành vi |
|---|---|
| `esp_err_t zb_node_start(void)` | Tạo task `zb_task` (stack 8192, ưu tiên 5) |
| `static void zb_task(void*)` | `esp_zb_platform_config` (radio native) → `esp_zb_init` với cấu hình ED `rx_on_when_idle=true` → `create_endpoints()` → `esp_zb_set_primary_network_channel_set(ALL_CHANNELS)` → `esp_zb_start(false)` → `esp_zb_stack_main_loop()` `[VERIFY toàn bộ]` |
| `static void create_endpoints(void)` | Tạo EP10 (Basic + Identify + Temperature Meas `0x0402` + Rel. Humidity `0x0405`), EP11..15 (Basic + Analog Input `0x000C` có `Description`). Dùng hàm tạo cluster có sẵn; nếu không có cho độ ẩm thì tạo bằng danh sách attribute tùy chỉnh `[VERIFY]` |
| `void esp_zb_app_signal_handler(esp_zb_app_signal_t*)` | Bảng xử lý: `SKIP_STARTUP`→khởi tạo BDB; `DEVICE_FIRST_START/REBOOT`→nếu chưa tham gia mạng: bắt đầu steering; `STEERING` thành công→`joined=true`, log PAN/kênh; thất bại→`esp_zb_scheduler_alarm` thử lại sau `backoff_next(5 s..60 s)`; `LEAVE`→`joined=false`, xóa dữ liệu mạng, steering lại |
| `bool zb_node_joined(void)` | cờ atomic |
| `esp_err_t zb_node_publish(const node_snapshot_t*)` | Nếu `!joined` → `ESP_ERR_INVALID_STATE`. Lấy khóa Zigbee (`esp_zb_lock_acquire` `[VERIFY]`); đặt giá trị: `temp` → `int16(round(T·100))`, `hum` → `uint16(round(RH·100))`, các Analog Input → `float` (`mq2_mv, flame_mv, eco2, tvoc, flags`); với từng (EP,cluster,attr) gọi lệnh report tới đích `0x0000`, EP `COORD_EP` `[VERIFY]`; nhả khóa. Trả lỗi nếu lệnh nào thất bại (đếm `zb_tx_fail`) |
Việc gọi API Zigbee từ task khác `zb_task` **bắt buộc** nằm trong cặp khóa/nhả khóa.

#### 5.3.8 `report_sched`
```c
typedef enum { RPT_TRIG_PREALERT_ON=1, RPT_TRIG_FLAME_EVT, RPT_TRIG_TALERT } rpt_trig_t;
void report_sched_start(void);                       /* tạo q_trig(4) + task, đăng ký TWDT */
void report_request_now(rpt_trig_t reason);          /* an toàn từ task; từ ISR dùng bản FromISR */
void report_sched_stats(uint32_t *sent,uint32_t *skipped,uint32_t *tx_fail);
```
Vòng lặp: `period = snap.prealert ? REPORT_PREALERT_MS : REPORT_NORMAL_MS`; `wait = max(0, last_tx + period − now)`; `xQueueReceive(q_trig, &trig, wait)`.
- Hết thời gian chờ → gửi.
- Có `trig`: nếu `now − last_tx >= REPORT_MIN_GAP_MS` (1 s) → gửi ngay; nếu chưa đủ → hẹn gửi tại `last_tx + REPORT_MIN_GAP_MS`. **Xả hết** các trig dư trong queue (gộp).
- Gửi: `node_state_get` → `snap.seq = ++seq` → `zb_node_publish`; nếu chưa join → `skipped++` (không dồn hàng đợi). `last_tx = now`. `esp_task_wdt_reset()` mỗi vòng.

#### 5.3.9 `node_cli` (console USB-Serial-JTAG)
Lệnh: `status` (in snapshot + thống kê `sent/skipped/tx_fail`, `joined`), `cal` (lưu `mq2_base` = trung bình MA 60 mẫu gần nhất, chỉ khi `!prealert`), `warm_skip` (chỉ khi `BENCH_MODE`), `zb_erase` (xóa phân vùng Zigbee + khởi động lại), `reboot`.

#### 5.3.10 `app_main` (node) và `sensor_task`
Thứ tự: `NVS` → `i2c_bus_init` → `sht31_init`/`ccs811_init`/`adc_sensors_init`/`flame_gpio_init` (**lỗi từng cảm biến không dừng boot**, chỉ đặt cờ) → `node_state_init` → `zb_node_start` → `report_sched_start` → `sensor_task` → `node_cli`.
`sensor_task` mỗi 1000 ms (`vTaskDelayUntil`):
1. `sht31_read` → thành công: cập nhật `temp,hum`, `sht_fail=0`, `ccs811_set_env`; thất bại: `sht_fail++`; ≥3 → `i2c_bus_recover()` + `sht31_init()`.
2. `ccs811_read` khi `ready` → cập nhật `eco2,tvoc`; `ccs_bad = !ccs811_stable() || lỗi`.
3. `adc_read_sensor_mv` cho `SNS_MQ2`, `SNS_FLAME`; `adc_fault = flags!=0`.
4. `flame_do = flame_do_active()`.
5. `flags = node_flags_compute(...)`; `pa = prealert_update(...)`.
6. `node_state_update`; nếu `pa` chuyển `false→true` hoặc có `NODE_EVT_FLAME` → `report_request_now`.
7. Mỗi 60 vòng → `node_warmup_tick_1min()`. `esp_task_wdt_reset()`.

### 5.4 `firmware/gw_h2` — các component

#### 5.4.1 `zb_coord`
| Hàm | Hành vi |
|---|---|
| `esp_err_t zb_coord_start(QueueHandle_t q_evt)` | Tạo `zb_task` (coordinator): `esp_zb_init` cấu hình ZC, đăng ký 1 endpoint (EP `COORD_EP`, Basic), `esp_zb_set_primary_network_channel_set(1<<ZB_CHANNEL)`, đặt PAN ID `ZB_PAN_ID`, `esp_zb_core_action_handler_register(zb_action_handler)`, `esp_zb_start(false)`, `esp_zb_stack_main_loop()` `[VERIFY]` |
| `void esp_zb_app_signal_handler(...)` | `SKIP_STARTUP`→`BDB init`; `FIRST_START`→**FORMATION** (lập mạng); `FORMATION` xong → log kênh/PAN; nếu chưa có node nào → mở cửa sổ `JOIN_WINDOW_S`; `DEVICE_ANNCE`→đẩy `ZB_EVT_JOIN{sa,ieee}`; `LEAVE_INDICATION`→`ZB_EVT_LEAVE`; `PERMIT_JOIN_STATUS`→cập nhật LED onboard |
| `static esp_err_t zb_action_handler(callback_id, const void *msg)` | `ESP_ZB_CORE_REPORT_ATTR_CB_ID` → `on_report_attr`; callback khác → bỏ qua (log mức DEBUG). **Không** ghi UART, **không** block |
| `static void on_report_attr(const esp_zb_zcl_report_attr_message_t*)` | Bóc `src short addr, src EP, cluster, attr id, kiểu, giá trị`; đổi sang `float`; tạo `zb_evt_t{ZB_EVT_ATTR,...}`; `xQueueSend(q,&evt,0)`; đầy → `drop_evt++` |
| `esp_err_t zb_coord_open_network(uint16_t sec)` | Giữ khóa Zigbee → `esp_zb_bdb_open_network(sec)` → nhả khóa |
| `esp_err_t zb_coord_reset_network(void)` | Xóa dữ liệu mạng (local reset), khởi động lại sau 1 s (qua scheduler, không `delay`) |

#### 5.4.2 `node_cache`
```c
#define NC_MAX 4
#define EP_MASK_ALL 0x3Fu   /* 6 nhóm: temp, hum, mq2, flame, eco2, tvoc (+flags) theo bảng ánh xạ */
typedef struct { bool used; uint16_t sa; char ieee[PROTO_IEEE_LEN]; rpt_t cur; uint32_t got, first_ms, seq; } nc_entry_t;
void        nc_init(void);
nc_entry_t* nc_on_join(uint16_t sa,const char *ieee);
void        nc_on_leave(const char *ieee);
void        nc_apply_attr(const zb_evt_t *e, uint32_t now_ms);   /* ánh xạ (EP,cluster,attr)→trường rpt_t; đặt bit valid; ghi first_ms khi got==0 */
bool        nc_ready(const nc_entry_t*, uint32_t now_ms);         /* got==ALL || (now-first_ms)>=COALESCE_MS(300) */
void        nc_take(nc_entry_t*, rpt_t *out);                     /* ++seq, sao chép, got=0 */
```
Bảng ánh xạ: `(EP10,0x0402,0x0000)`→`temp=val/100`; `(EP10,0x0405,0x0000)`→`hum=val/100`; `(EP11,0x000C,0x0055)`→`mq2_mv`; `(EP12,…)`→`flame_mv`; `(EP13,…)`→`eco2`; `(EP14,…)`→`tvoc`; `(EP15,…)`→`flags`. **Gói không biết IEEE** (sau reboot H2): giữ lại tối đa 5 s chờ `DEVICE_ANNCE`/yêu cầu IEEE; hết thời gian → bỏ + `drop_unknown++`.

#### 5.4.3 `uart_bridge`
| Hàm | Hành vi |
|---|---|
| `esp_err_t bridge_init(QueueHandle_t q_evt)` | `uart_driver_install(UART_NUM_1, rx=1024, tx=1024)`, `uart_param_config` 115200 8N1 không điều khiển luồng, `uart_set_pin(TX=10, RX=11)`; tạo mutex TX; gửi `hello` |
| `static int bridge_send_line(const char*,size_t)` | Giữ mutex → `uart_write_bytes` → nhả; đếm `tx_lines`, `tx_err` |
| `static void bridge_task(void*)` | Mỗi 50 ms: (1) rút hết `q_zb_evt`: `JOIN/LEAVE` → `nc_*` + gửi bản tin tức thì; `ATTR` → `nc_apply_attr`; (2) với mỗi entry `nc_ready` → `nc_take` → `proto_enc_rpt` → gửi; (3) nếu `now−last_hb>=2000` → gửi `hb`; (4) `uart_read_bytes(...,0)` → `line_asm_feed` → `on_line`; (5) `esp_task_wdt_reset()` |
| `static void on_line(const char*,size_t,void*)` | `proto_dec`; `M_CMD`: `OP_PERMIT_JOIN` (kẹp `sec∈[1,254]`) → `zb_coord_open_network` → `ack`; `OP_RESET_NET` → `zb_coord_reset_network` → `ack`; `OP_PING` → `ack`. Lỗi giải mã → `parse_err++` (không phản hồi) |

#### 5.4.4 `app_main` (gw_h2)
`NVS` → `q_zb_evt(32)` → `nc_init` → `bridge_init` → `zb_coord_start` → `xTaskCreate(bridge_task)` + TWDT. **Mọi log qua USB-Serial-JTAG; UART1 chỉ NDJSON (H10).**

### 5.5 `firmware/gw_host` — các component

#### 5.5.1 `uart_link`
| Hàm | Hành vi |
|---|---|
| `esp_err_t uart_link_init(QueueHandle_t q_rpt)` | `UART_NUM_2`, TX=17, RX=16, 115200 8N1, buffer RX 1024; `last_hb_ms = now` (ân hạn 6 s sau boot) |
| `static void uart_rx_task(void*)` | Vòng: `uart_read_bytes(..., 128, 100 ms)` → `line_asm_feed(on_line)`; sau mỗi vòng: `if link_is_lost(now,last_hb,HB_TIMEOUT_MS)` → `sys_faults_set(F_H2_LINK_LOST)` + log 1 lần; `esp_task_wdt_reset()` |
| `static void on_line(...)` | `proto_dec`: `M_HELLO`→log phiên bản; `M_HB`→`last_hb=now; sys_faults_clear(F_H2_LINK_LOST)`; `M_JOIN/LEAVE`→`node_registry_on_join/leave`; `M_RPT`→`xQueueSend(q_rpt,&rpt,0)` (đầy → `drop_rpt++`); `M_ACK`→log; lỗi → `parse_err++` |
| `esp_err_t uart_link_send_cmd(cmd_op_t op,uint16_t sec)` | `proto_enc_cmd` → `uart_write_bytes` (mutex) |

#### 5.5.2 `node_registry`
```c
#define NR_MAX 4
typedef struct { bool used; char ieee[17]; char name[16]; uint32_t last_rpt_ms, last_seq, lost, interval_hint_ms; bool offline; fstate_t state; } nr_entry_t;
int  node_registry_on_report(const rpt_t*, uint32_t now_ms);  /* trả chỉ số node; tạo mới nếu chưa có */
void node_registry_on_join(const char *ieee); void node_registry_on_leave(const char *ieee);
uint32_t node_registry_tick(uint32_t now_ms);                 /* trả mặt nạ F_NODE_OFFLINE nếu có node offline; cập nhật offline */
const nr_entry_t* node_registry_get(int idx);
```
Tên node: tra `NODES_MAP[]` trong `config.h` (`{ieee, "Node_01"}`); chưa có → `Node_<4 ký tự cuối IEEE>`. `interval_hint_ms = (flags & FL_PREALERT) ? 3000 : 10000`. Mất gói: nếu `seq > last_seq` → `lost += seq−last_seq−1`; nếu `seq < last_seq` (khởi động lại) → đặt lại `last_seq`, không tính mất.

#### 5.5.3 `fusion_ctrl`
| Hàm | Hành vi |
|---|---|
| `void fusion_ctrl_start(QueueHandle_t q_rpt)` | `fusion_params_default`; nạp baseline từ NVS (`mq2_base`,`flame_base`); tạo `fusion_t` cho mỗi `NR_MAX`; tạo task |
| `static void fusion_task(void*)` | `xQueueReceive(q_rpt,&r,1000 ms)`. Có `r`: `idx=node_registry_on_report`; dựng `fusion_in_t` (bit `valid` từ `r.valid`, `flags` từ `r.flags`); `fusion_update`; `payload_sensor` → `pubq_push(PRIO_TELEM)`; nếu `out.changed` → `payload_alert` → `pubq_push(PRIO_ALERT)` + cập nhật `sys_state`. Mỗi vòng (cả khi timeout): `fusion_tick` mọi node, `m = node_registry_tick(now)` → đặt/xóa `F_NODE_OFFLINE`; `sys state = max(state các node)`; **nếu node offline và state ≥ WARNING → không hạ (đã bảo đảm vì `fusion` không bị gọi)**; `esp_task_wdt_reset()` |
| `bool fusion_ctrl_cal_baseline(void)` | Từ chối (`false`) nếu có node ≠ NORMAL hoặc `!fusion_ma_ready`; ngược lại lưu NVS `mq2_base=ma_mq2`, `flame_base=ma_flame`, gọi `fusion_set_baselines` |
| `uint8_t fusion_ctrl_reset_latch(void)` | Gọi `fusion_reset_latch` mọi node; trả số node **bị từ chối** |

#### 5.5.4 `sys_state`
```c
typedef struct { fstate_t state; uint32_t faults; bool mute_active, mains_ok, wifi_up, mqtt_up, h2_link_ok;
                 uint32_t mute_until_ms; int reset_reason; uint8_t n_nodes; struct{char name[16]; fstate_t st; bool online;} node[NR_MAX]; } sys_snapshot_t;
void sys_state_init(void); void sys_get(sys_snapshot_t*);
void sys_set_state(fstate_t); void sys_faults_set(uint32_t); void sys_faults_clear(uint32_t);
void sys_mute_start(uint32_t sec); void sys_mute_cancel(void); bool sys_mute_active(uint32_t now_ms);
void sys_set_net(bool wifi,bool mqtt); void sys_set_mains(bool ok);
```
Mutex ngắn (< 1 ms); `sys_get` sao chép nguyên khối.

#### 5.5.5 `alarm_out`
**a) RGB:** `rgb_init()`: LEDC timer (low-speed, 8 bit, 5 kHz), 3 kênh GPIO25/26/27, duty ban đầu 0 **trước khi gán chân** `[VERIFY]`. `rgb_set_color(color_t)`: bảng duty `{R,G,B}`: ĐỎ {255,0,0}, XANH LÁ {0,255,0}, XANH DƯƠNG {0,0,255}, VÀNG {`RGB_YELLOW_R_DUTY`,`RGB_YELLOW_G_DUTY`,0}, TÍM {255,0,255}, TRẮNG {255,255,255}, TẮT {0,0,0}. Cathode chung ⇒ duty cao = sáng.
**b) Còi:** `buzzer_init()` đặt GPIO33 = OUTPUT, mức TẮT, **là lệnh đầu tiên của `app_main`**. `buzzer_set(bool on)`. Nếu `BUZZER_PASSIVE=1`: dùng LEDC `BUZZER_PASSIVE_FREQ_HZ`=2700, duty 50 % khi bật.
**c) Nút:** `input_task` mỗi 10 ms: `raw = (gpio_get_level(32)==0)`; `ev = btn_update(...)`; `ev!=NONE` → `xQueueSend(q_btn,&ev,0)`; `cue = btn_hold_cue(...)` đổi giá trị → `alarm_request_cue(CUE_HOLD_3S/CUE_HOLD_5S)`. `esp_task_wdt_reset()`.
**d) `alarm_task`** mỗi 50 ms (`vTaskDelayUntil`): `sys_get` → `alarm_resolve` → nếu `led_pat/buz_pat` đổi thì `pengine_start` mẫu mới → `led = pengine_out(base)`; nếu `led_overlay` và `pengine_out(ovl) != C_TRANSPARENT` thì lấy màu overlay; tương tự còi (overlay + `sys_mute_active`) → `rgb_set_color`, `buzzer_set`. Xử lý `q_btn` (không chờ): `SHORT` → nếu `sys_mute_active` thì `sys_mute_cancel` else `sys_mute_start(MUTE_DEFAULT_S)`; `RESET` → `n=fusion_ctrl_reset_latch()`; `n>0` → `cue(REFUSED)` (3 chớp đỏ 100 ms) else `cue(OK)` (xanh lá 300 ms); `JOIN` → `uart_link_send_cmd(OP_PERMIT_JOIN, JOIN_WINDOW_S)` + `cue(JOIN)` (xanh dương 300 ms). **Cue là máy trạng thái có hẹn giờ, không block**, ghi đè LED tối đa 600 ms.
**e) Self-test lúc boot:** `alarm_task` chạy 1,2 s đầu: TRẮNG liên tục + 1 bíp 100 ms rồi chuyển sang chế độ thường. Giữ nút lúc boot (`input` đọc ở `app_main`) → self-test mở rộng: ĐỎ,XANH LÁ,XANH DƯƠNG mỗi 700 ms kèm bíp. Còn `alarm_request_test(ms)` (lệnh `test_alarm`) → buộc mẫu `LED_FIRE+BZ_FIRE` trong `ms` ms rồi trả về.

#### 5.5.6 `power_mon`
`power_mon_init`: ADC1 kênh GPIO35 (`adc_oneshot_io_to_channel`), atten 12 dB, hiệu chuẩn `[VERIFY]` (ESP32: line fitting). `power_task` mỗi 1000 ms: trung bình cắt 16 mẫu → `mv_pin`; `v_adp_mv = mv_pin · PS_DIV_NUM / PS_DIV_DEN` (25/15). Trễ + khử dội: `!mains_ok && mv_pin >= PS_ON_MV(2300)` → `cnt_on++` (≥3 ⇒ `mains_ok=true`); `mains_ok && mv_pin <= PS_OFF_MV(1500)` → `cnt_off++` (≥3 ⇒ `false`); vùng giữa → xóa bộ đếm. Cập nhật `sys_set_mains`, đặt/xóa `F_MAINS_LOST`; phát `status` khi đổi.

#### 5.5.7 `net_*`
**a) `net_wifi`:** `net_wifi_start()`: `esp_netif_init`, vòng sự kiện, `esp_wifi_init`, STA từ `secrets.h`, `esp_wifi_set_ps(WIFI_PS_NONE)` (nguồn lưới, ưu tiên độ trễ). `WIFI_EVENT_STA_START`→`esp_wifi_connect`; `STA_DISCONNECTED`→`wifi_up=false`; hẹn `esp_timer` sau `backoff_next(&attempt,1000,60000,esp_random()&0xFFFF)` rồi `esp_wifi_connect`; `IP_EVENT_STA_GOT_IP`→`backoff_reset`, `wifi_up=true`, gọi `net_time_start()` và `net_mqtt_start()` (nếu chưa).
**b) `net_time`:** `net_time_start()` khởi tạo SNTP (`pool.ntp.org`) `[VERIFY]`; `uint32_t time_epoch(void)`: `t=time(NULL)`; `t < 1700000000` → trả 0 (chưa đồng bộ); đã đồng bộ thì **không** trả giá trị nhỏ hơn lần trước.
**c) `net_mqtt`:** `net_mqtt_start()`: cấu hình `esp_mqtt_client` `[VERIFY]`: URI `mqtts://MQTT_HOST:MQTT_PORT`, `username/password/client_id`, `keepalive=30`, **LWT** `status/gateway` ← `{"online":false}` QoS1 retain, `crt_bundle_attach=esp_crt_bundle_attach`. Sự kiện: `CONNECTED`→`mqtt_up=true`; đẩy `status` online (retain) vào `pubq`; subscribe `cmd/gateway` QoS1. `DISCONNECTED`→`mqtt_up=false`. `DATA`→ chỉ xử lý khi `total_data_len == data_len` (không phân mảnh), sao chép vào buffer cục bộ ≤ 128 byte → `cmd_handler_on_mqtt`. `mqtt_pub_task`: `pubq_pop`; nếu `mqtt_up` → `esp_mqtt_client_publish(topic,payload,0,qos,retain)`; trả `-1` → `pubq_push_front` và chờ 200 ms; không `mqtt_up` → chờ 500 ms (không busy-loop).
**d) `payload`** (hàm thuần, test PC): trả độ dài hoặc `-1` nếu cắt cụt.
```c
int payload_sensor(char*,size_t,const char *node_id,const rpt_t*,const fusion_out_t*,uint32_t faults,uint32_t epoch);
int payload_alert (char*,size_t,const char *node_id,const fusion_out_t*,float temp,uint32_t epoch);
int payload_status(char*,size_t,const sys_snapshot_t*,uint32_t uptime_s,const char *fw,int zb_ch,const char *last_cmd_ack);
```
Khóa đúng GatewayPlan §5.2; `state` là chuỗi `"NORMAL|CAUTION|WARNING|FIRE"`; `reason` là mảng chuỗi từ `reasons` (`"score"`,`"r1_temp"`,`"r2_flame"`,`"r3_rate"`); trường không hợp lệ **bị bỏ**; `epoch==0` → `"ts":0`. Mọi chuỗi động qua hàm escape (`"` và `\`); `PAYLOAD_MAX`=320.

#### 5.5.8 `cmd_handler`
`void cmd_handler_on_mqtt(const char *topic,const char *data,size_t len)`: (1) `topic == "cmd/gateway"`; (2) `len<=128`; (3) giới hạn tốc độ: bỏ nếu `now − last_cmd < 1000 ms`; (4) `flat_get_str(...,"cmd")` → danh sách trắng: `permit_join`(`sec` mặc định 180, kẹp 1..254) → `uart_link_send_cmd`; `mute`(`sec` 1..3600) → `sys_mute_start`; `test_alarm` → `alarm_request_test(5000)`; `reset_latch` → `fusion_ctrl_reset_latch`; `cal_baseline` → `fusion_ctrl_cal_baseline`; (5) khác → log + bỏ; (6) ghi `last_cmd_ack="<cmd>:ok|refused|bad"` đưa vào `status` kế tiếp.

#### 5.5.9 `app_main` (host) — thứ tự bắt buộc
```
1. buzzer_init(); rgb_init();                 // ĐẦU RA TẮT TRƯỚC TIÊN
2. đọc esp_reset_reason(); nvs_flash_init(); sys_state_init()
3. tạo q_rpt(8), q_btn(4); pubq_init()
4. xTaskCreate alarm_task, input_task         // bắt đầu self-test
5. power_mon_start()
6. fusion_ctrl_start(q_rpt)                   // nạp baseline NVS
7. uart_link_init(q_rpt) + uart_rx_task
8. net_wifi_start()                           // KHÔNG chờ; hệ thống chạy offline
9. status_task: publish status/gateway mỗi 30 s và khi (mains, h2_link, wifi/mqtt, mute) đổi
10. esp_task_wdt_add: alarm, input, uart_rx, fusion
11. nếu reset_reason ∈ {TASK_WDT, INT_WDT, PANIC, BROWNOUT}: bíp 3 lần, ghi vào status
```

### 5.6 `cloud/`

#### 5.6.1 `dashboard` (HTML + JS thuần, không build)
Kết nối `wss://<host>:8884/mqtt` `[VERIFY cổng WSS của HiveMQ Cloud]`. Thư viện lưu **cục bộ** trong `vendor/` (không tải từ CDN lúc chạy). Credential subscribe-only đặt trong `config.js` (ghi chú rõ: công khai).
| Hàm | Hành vi |
|---|---|
| `connect()` | `mqtt.connect(url,{username,password,clean:true,reconnectPeriod:2000})`; `on('connect')` → subscribe `sensor/#`,`alert/#`,`status/#` |
| `onMessage(topic,payload)` | `JSON.parse` trong `try/catch` (hỏng → bỏ + đếm); định tuyến theo tiền tố topic |
| `updateNodeCard(id,data)` | Cập nhật T, RH, eCO₂, TVOC, MQ-2, Flame, `state` (màu: xanh/vàng/cam/đỏ), `lastSeen=Date.now()` (dùng đồng hồ trình duyệt để phát hiện cũ, không tin `ts` của thiết bị) |
| `pushChart(series,ts,value)` | Vòng đệm 300 điểm/chuỗi; Chart.js `update('none')` |
| `renderAlert(a)` | Thêm hàng đầu bảng; tối đa 100 hàng; `FIRE` nháy đỏ |
| `markStale()` | Mỗi 5 s: node không có `sensor` > 30 s → "mất dữ liệu"; gateway không cập nhật `status` > 90 s hoặc `online:false` → "Gateway offline" |
**Chỉ hiển thị**, không có nút điều khiển (credential không có quyền publish).

#### 5.6.2 `telegram_bot` (Node.js 20, polling)
Biến môi trường: `MQTT_URL, MQTT_USER, MQTT_PASS, TG_TOKEN, TG_CHAT_IDS, ALERT_MIN_STATE (mặc định WARNING), RENOTIFY_FIRE_S (120)`.
| Hàm | Hành vi |
|---|---|
| `connectMqtt()` | Subscribe `alert/#`, `status/gateway`, `sensor/#` (để tra cứu `/status`) |
| `onAlert(nodeId,a)` | Bỏ nếu `rank(a.state) < rank(ALERT_MIN_STATE)`. `dedupe.shouldSend(nodeId,a.state)`: cùng trạng thái trong 60 s → không gửi; **leo thang → gửi ngay**. Gửi tin có: node, trạng thái, nhiệt độ, lý do, giờ |
| `fireRenotify()` | Khi node ở `FIRE`: nhắc lại mỗi `RENOTIFY_FIRE_S`; dừng khi nhận `alert` hạ mức |
| `onGatewayStatus(s)` | `online:false` → đợi 60 s (chống chập chờn) rồi gửi "Gateway offline"; `power:"battery"` → gửi cảnh báo mất điện lưới 1 lần |
| `bot.onText(/\/status/)` | Trả trạng thái cuối cùng của từng node + gateway; chỉ phục vụ chat có trong `TG_CHAT_IDS` |
| `sendSafe(text)` | Gửi có thử lại 3 lần (backoff 1,2,4 s); lỗi cuối cùng → log, **không** làm sập tiến trình |

---

## 6. Kế hoạch kiểm thử & chiến lược gỡ lỗi

### 6.1 Kiểm thử đơn vị trên PC (không cần phần cứng)
Chạy: `cmake -S tests/host -B build_host -DCMAKE_C_FLAGS="-fsanitize=address,undefined -Wall -Wextra" && cmake --build build_host && ctest --test-dir build_host --output-on-failure`.
Mọi test dùng `test_util.h` (không thư viện ngoài). **Không merge nếu có test fail hoặc sanitizer báo lỗi.**

| File test | Ca bắt buộc (tên ca = tên hàm test) |
|---|---|
| `test_proto.c` | `enc_hello_exact`, `enc_hb_exact`, `enc_rpt_full`, `enc_rpt_omits_invalid`, `enc_rpt_nan_removed`, `enc_buf_too_small`, `dec_rpt_ok`, `dec_missing_ieee`, `dec_missing_seq`, `dec_bad_type`, `dec_not_object`, `dec_too_long`, `dec_empty`, `dec_key_order_swapped`, `dec_extra_spaces`, `dec_unknown_key_ignored`, `dec_key_inside_string_value`, `dec_number_trailing_garbage`, `dec_string_with_backslash`, `dec_cmd_permit_join_sec`, `dec_cmd_unknown_op`, `asm_split_across_feeds`, `asm_two_lines_one_feed`, `asm_overflow_recovers`, `asm_blank_lines_ignored`, `roundtrip_rpt_random_1000` |
| `test_filters.c` | `ma_basic`, `ma_ignores_nan`, `ma_window_clamp`, `ma_no_drift_1e6_pushes`, `med3_removes_spike` (`{20,90,21}`→`21`), `med3_short_returns_x`, `rate_slope_10_per_min` (±0,5), `rate_needs_3_samples`, `rate_needs_20s_span`, `rate_drops_old_samples`, `rate_millis_wraparound`, `rate_flat_is_zero` |
| `test_prealert.c` | `enters_on_temp`, `enters_on_dtdt`, `enters_on_mq2_rel`, `enters_on_flame_do`, `ignores_mq2_when_warm`, `ignores_temp_when_sht_fail`, `clear_after_n_normal`, `no_clear_if_one_bad_between`, `reasons_mask_correct` |
| `test_fusion.c` | `vd1..vd4` (§6.2), `s1..s10` (§6.2), `weights_renormalize`, `insufficient_holds_state`, `no_double_count_override`, `latch_blocks_downgrade`, `reset_latch_refused_before_60s`, `reset_latch_ok_after_60s`, `downgrade_one_level_at_a_time` |
| `test_logic.c` | `pengine_period_exact`, `pengine_large_time_jump`, `pengine_transparent_step`, `resolve_truth_table` (mọi tổ hợp state×fault×mute×mains), `resolve_fire_wins`, `resolve_mute_only_buzzer`, `btn_bounce_5ms_ignored`, `btn_short_300ms`, `btn_dead_zone_2s_none`, `btn_reset_3500ms`, `btn_join_6000ms`, `btn_hold_cue`, `fault_node_offline_boundary` (đúng 30 000 ms), `fault_offline_3x_interval`, `fault_millis_wrap`, `backoff_sequence` (1,2,4,8,16,32,60,60 s ±10 %), `backoff_no_overflow`, `pubq_drop_oldest_telem`, `pubq_keep_alerts`, `pubq_status_replaces`, `pubq_push_front_order`, `payload_sensor_keys`, `payload_escape_quote`, `payload_truncation_returns_neg`, `payload_omits_invalid` |

### 6.2 Ví dụ tính tay và kịch bản fusion (nguồn chân lý cho `test_fusion.c`)
Tham số mặc định `fusion_params_default` (GatewayPlan §7). Mọi số dưới đây agent **phải tái tạo đúng**, sai lệch ≤ 0,001.

| Ví dụ | Đầu vào (đã lọc) | Tính | Kết quả |
|---|---|---|---|
| **VD-1** | T=50 °C, các kênh khác ở nền | `s_T=(50−35)/30=0,5`; `S=0,25·0,5/1,0=0,125` | `NORMAL` (0,125 < 0,25) |
| **VD-2** | T=65 °C, kênh khác nền | `s_T=1`; `S=0,25` | Theo điểm: `CAUTION` sau 2 report. **Nhưng R1** (T≥65 liên tiếp 2 lần) → **`FIRE` tại report thứ 2** |
| **VD-3** | T=50 °C, MQ-2 cờ sấy (loại) | trọng số hợp lệ `vw=1−0,20=0,80`; `S=0,125/0,80=0,15625` | `NORMAL`; `valid_weight=0,80` |
| **VD-4** | mọi `s_i=0,5` (T=50; dT/dt=5; eCO₂=1150; TVOC=225; mq2_rel=0,6; flame_drop=0,3) | `S=0,5·Σw=0,5` | `WARNING` sau 2 report (0,45 ≤ 0,5 < 0,65) |

| Kịch bản | Mô tả đầu vào | Kết quả mong đợi |
|---|---|---|
| S1 | 100 report ở nền | `NORMAL`, `score<0,05` suốt |
| S2 | T tăng 26→70 °C, kênh khác nền | `FIRE` đúng report thứ 2 có T≥65 (`RS_R1`) |
| S3 | `flame_drop=0,8` + `FL_FLAME_DO` liên tục 20 report, kênh khác nền | `S=0,15` → **luôn `NORMAL`** (R2 không kích vì không có tín hiệu khác > 0,2). **Lửa một mình không bao giờ FIRE** |
| S4 | như S3 nhưng T=42 °C (`s_T=0,233>0,2`) | `FIRE` tại report lửa thứ 3 (`RS_R2`) |
| S5 | `S`=0,46 hai report rồi xen kẽ 0,44/0,46 × 40 | **đúng 1** lần chuyển vào `WARNING`, không dao động sau đó |
| S6 | VD-3 kéo dài 20 report | trạng thái ổn định, `S` chuẩn hóa đúng |
| S7 | Vào `FIRE`, rồi dữ liệu nền; `fusion_tick` mỗi 1 s | reset tại 59 s → `false`; tại 61 s → `true`, `state=NORMAL` |
| S8 | T=40 °C tăng 12 °C/phút, report mỗi 3 s | sau ≥ 20 s (đủ span) và giữ 30 s → tối thiểu `WARNING` (`RS_R3`); sai số ±1 report |
| S9 | Mọi kênh không hợp lệ (`SHT_FAIL|CCS_BAD|ADC_FAULT`) | `state` giữ nguyên, `insufficient=true` |
| S10 | Đang `FIRE` (latched), kênh chuyển sang không hợp lệ | **vẫn `FIRE`** (fail-safe) |

### 6.3 Công cụ mô phỏng / replay
**Định dạng CSV đầu vào:** `t_ms,temp,hum,eco2,tvoc,mq2_mv,flame_mv,flags` (flags thập phân). **Golden:** thêm `state,score,reasons`.
| Công cụ | Chức năng | Tham số chính |
|---|---|---|
| `tests/host/fusion_replay.c` | Đọc CSV từ stdin, chạy `fusion_update/tick` theo `t_ms`, in CSV kết quả | `--base-mq2 X --base-flame Y` |
| `tools/replay.py` | Chạy `fusion_replay` trên mọi `tests/vectors/*.csv`, so với `*.golden.csv`, in sai khác từng dòng | `--update-golden` (chỉ người được dùng) |
| `tools/gw_sim.py` | **Đóng vai H2** gửi NDJSON vào UART2 của WROOM (qua cáp USB-UART 3,3 V: `TX→GPIO16`, `RX←GPIO17`, `GND`): gửi `hello`, `hb` mỗi 2 s, `rpt` từ CSV theo thời gian thực × `--speed` | `--port --csv --speed --ieee --scenario {normal,smolder,fire,node_offline,link_lost,garbage}` |
| `tools/serial_check.py` | Đọc serial, kiểm `--expect REGEX` (lặp lại được) và `--forbid REGEX` trong `--timeout` giây; mã thoát 0/1; `--log FILE` | `--selftest` |
| `tools/pdr_report.py` | Từ log có `seq`: PDR, khoảng trống lớn nhất, jitter chu kỳ | `--period 10` |
Nhờ `gw_sim.py`, **Phase 3–5 làm được khi chưa có Zigbee** (không phụ thuộc G0).

### 6.4 Kiểm thử tại bàn (Hardware-in-the-Loop)
Định dạng dòng máy đọc: `BU|<TEST>|k=v|k=v|PASS` hoặc `BU|<TEST>|...|FAIL|reason=<mã>`. Mỗi ứng dụng bring-up kết thúc bằng một dòng `PASS/FAIL`; `serial_check.py` quyết định mã thoát.

| Đối tượng | Thiết bị đo | Quy trình | Tiêu chí |
|---|---|---|---|
| I2C bus | — | `bu_i2c_scan` | Thấy 0x44, 0x5A |
| SHT31 | Nhiệt kế tham chiếu | Đặt cạnh nhau 10 phút; thổi hơi ấm | Sai số ≤ ±1 °C; 0 lỗi CRC / 100 lần |
| CCS811 | — | Chạy 30 phút; đưa cồn/ethanol lại gần (lượng nhỏ, thoáng) | TVOC tăng rõ rồi hạ; không treo; hồi phục sau ngắt nguồn |
| ADC | Đồng hồ vạn năng, 2 điện trở bằng nhau | Cấp 1,65 V và 0 V, 3,3 V | ±3 %; cờ LOW/HIGH đúng |
| MQ-2 | Khói nhang (thoáng) | Sau sấy (hoặc `BENCH_MODE`) | `mq2_mv` tăng ≥ 20 % khi có khói, hồi lại khi hết |
| Flame | Bật lửa ở 30 cm, đèn sợi đốt, ánh nắng | Ghi tỷ lệ kích đúng/sai | Dữ liệu cho luật R2; không dội |
| UART H2↔WROOM | Logic analyzer (nếu có) | `bu_uart_*` 10 000 vòng | 0 lỗi; chèn rác/tràn dòng vẫn phục hồi |
| Zigbee | — | `T-207` PDR 1 h | ≥ 99 % |
| Còi/LED | Đồng hồ đo dòng | `bu_rgb`, `bu_buzzer` | Dòng/chân ≤ 8 mA (LED), còi không kêu khi boot |
| Nút | — | `bu_button` giữ 0,3/3,5/6 s | SHORT/RESET/JOIN đúng |
| PWR_SENSE | Adapter, UPS | Cắm/rút | Đổi trạng thái ≤ 5 s, không dao động |
| Gateway end-to-end | Đồng hồ bấm giờ | `gw_sim.py --scenario fire` | NFR-1 ≤ 3 s (sau xác nhận), NFR-2 ≤ 5 s, NFR-3 ≤ 10 s |

Lưu **mọi** log HIL thành `tests/results/<TaskID>_<yyyymmdd>.log` (`idf.py -p PORT monitor | tee ...`). Không có log = chưa xong.

### 6.5 Chiến lược gỡ lỗi
**Thang gỡ lỗi (đi từng bậc, không nhảy bậc):** cảm biến → `node_snapshot` (console Node) → report Zigbee (console GW-H2) → NDJSON trên UART (terminal/logic analyzer) → `uart_link` (log host) → fusion → `pubq` → MQTT (MQTT Explorer) → dashboard/Telegram. Lỗi nằm ở **bậc đầu tiên mà dữ liệu sai**.

| Triệu chứng | Kiểm tra theo thứ tự |
|---|---|
| I2C không thấy thiết bị | Nguồn 3V3; SDA/SCL đổi chỗ; điện trở kéo lên (tổng ≥ 1,5 kΩ, không bị thiếu/thừa); GND chung; địa chỉ (ADDR); dây breadboard lỏng |
| SHT31 lỗi CRC | Cáp dài/lỏng; hạ tốc độ 100→50 kHz; thời gian chờ ≥ 16 ms; đo lại sau `i2c_bus_recover` |
| CCS811 NACK / `HW_ID` sai | **nWAKE phải nối GND**; ADDR; sau `SW_RESET` chờ ≥ 20 ms; clock stretching → thử hạ tốc độ hoặc tăng `scl_wait_us` |
| eCO₂ kẹt 400, TVOC 0 | Chưa qua 20 phút ổn định; chưa `APP_START`; `MEAS_MODE` sai; chưa ghi `ENV_DATA` |
| ADC sai lệch lớn | `ADC_DIV_RATIO` đúng 2,0? chia áp 22 kΩ/22 kΩ đúng? có tụ 100 nF? attenuation 12 dB? hiệu chuẩn bật? GPIO đúng kênh ADC1? |
| Node không join | Xóa flash Node và phân vùng `zb_storage`; kiểm kênh/PAN; **cửa sổ join đã mở?**; bảng phân vùng có `zb_storage/zb_fct`?; khoảng cách; nguồn 3V3 đủ khi phát |
| `rpt` thiếu trường | Node chưa gửi đủ EP10–15 trong < 300 ms; bảng ánh xạ `(EP,cluster,attr)` sai; callback không nhận cluster → xác minh bằng log ATTR thô |
| UART ra rác | Baud; **TX/RX chưa chéo**; GND chung; log lọt vào UART1 (đổi console sang USB-Serial-JTAG); mức logic 3,3 V |
| Host reset ngẫu nhiên | `esp_reset_reason()` ghi trong `status`; **brownout** (nguồn yếu, UPS sụt áp khi chuyển → tăng tụ); TWDT (task nào không `reset`); tràn stack (`CONFIG_FREERTOS_CHECK_STACKOVERFLOW`); heap (log `esp_get_minimum_free_heap_size()` mỗi 60 s) |
| TLS/MQTT không kết nối | Giờ hệ thống chưa đồng bộ (SNTP) trước khi bắt tay TLS; bundle CA bật chưa; cổng 8883; user/pass; dung lượng stack task; broker giới hạn kết nối |
| Còi kêu lúc boot | Thiếu điện trở 10 kΩ base→GND; `buzzer_init()` chưa phải lệnh đầu; mức kích |
| Màu LED sai | Thứ tự R/G/B của LED module; cathode chung/anode chung; điện trở; duty vàng |
| `%f` in ra rỗng | Kiểm `CONFIG_NEWLIB_NANO_FORMAT` `[VERIFY]` (tắt để in float) |

**Quy tắc log khi gỡ lỗi:** bật `ESP_LOG_DEBUG` theo từng TAG (`esp_log_level_set("ZB", ESP_LOG_DEBUG)`), không bật toàn cục; **log không được làm thay đổi thời gian** (không `printf` dài trong vòng 50 ms của `alarm_task`); gỡ xong phải trả về mức INFO.

---

## 7. Thứ tự thực hiện và rủi ro phần mềm

### 7.1 Đường găng và luồng song song
```
Đường găng:  T-001→T-002→T-006(G0)→T-202/203→T-204→T-205→T-206→T-306→T-402→T-503→T-601
Luồng A (không chờ G0, breadboard): T-101→T-102→T-103→T-104→T-105→T-106
Luồng B (không chờ G0, gateway):    T-111→T-112→T-113→T-114 ;  T-201→T-115
Luồng C (PC thuần, không phần cứng): T-004→T-301→T-302/303→T-304→T-305→T-401→T-501(backoff)→T-504
```
**Tuần đầu tiên nên chạy:** T-001..T-005 → (T-006 ∥ Luồng A) → Luồng C bắt đầu khi có `test_util.h`. Không có ước lượng ngày vì chưa biết quỹ thời gian của bạn; hãy đo thời gian thực của T-001..T-006 rồi ngoại suy.

### 7.2 Rủi ro phần mềm
| ID | Rủi ro | Giảm thiểu |
|---|---|---|
| SW-1 | API `esp-zigbee-sdk` khác phiên bản/tài liệu ít | G0; `API_NOTES.md`; bám ví dụ chính thức của SDK; ghim phiên bản; phương án lùi |
| SW-2 | Agent bịa API/thanh ghi | Cấp dữ kiện datasheet trong prompt (§5.3); cổng build; H11; người duyệt |
| SW-3 | Agent "sửa" test cho qua | H8; golden file do người khóa; `--update-golden` chỉ người dùng |
| SW-4 | Mòn flash do ghi NVS quá thường | Ghi bộ đếm sấy mỗi 10 phút; baseline chỉ ghi khi hiệu chỉnh |
| SW-5 | Tràn stack/heap khi TLS | Stack mqtt 8 KB; đo `uxTaskGetStackHighWaterMark`; log heap tối thiểu |
| SW-6 | Race giữa task Zigbee và task ứng dụng | Mọi API Zigbee ngoài `zb_task` phải trong khóa; callback chỉ đẩy queue |
| SW-7 | Fusion báo giả khi hiệu chỉnh kém | Replay log thật; N_UP/N_DOWN; R1–R3; test 24 h (T-604) |
| SW-8 | Lệch hợp đồng giữa 3 firmware | Hợp đồng ở `common/` + `GatewayPlan.md`; test mã hóa/giải mã dùng chung |
| SW-9 | Hai kênh vô tuyến 2,4 GHz nhiễu nhau | Kênh Zigbee 25; khoảng cách ≥ 10 cm; T-NET-03/06 |

---

## 8. Phụ lục

### 8.1 Mẫu prompt cho mỗi task (DeepSeek trên OpenCode)
```
VAI TRÒ: Bạn là kỹ sư firmware ESP-IDF. Chỉ làm đúng task dưới đây.

TASK: <Task ID> — <tên>
MỤC TIÊU: <1–2 câu>

NGỮ CẢNH (chỉ đọc, KHÔNG sửa):
- docs/GatewayPlan.md §<mục>   - docs/DEV_CODING_PLAN.md §5.x   - <file header liên quan>

TẠO/SỬA ĐÚNG CÁC FILE SAU (không file nào khác):
- <path1>  - <path2>

GIAO DIỆN (chép NGUYÊN VĂN, không đổi tên/chữ ký):
<dán chữ ký hàm/struct từ §5>

DỮ KIỆN DATASHEET (nguồn chân lý, không suy đoán thêm):
<dán bảng thanh ghi/địa chỉ/công thức từ §5.3.x>

RÀNG BUỘC: Tuân thủ H1–H11 (DEV_CODING_PLAN §1.1). Không delay dài, không sprintf/strcpy,
không malloc trong ISR, mọi esp_err_t được xử lý. Mọi hằng số ở config.h.
API ESP-IDF/Zigbee: grep header thật trước khi dùng; ghi vào docs/API_NOTES.md.
Không tìm thấy API → DỪNG và báo, đừng đoán.

NGHIỆM THU (chạy và DÁN ĐẦU RA THẬT):
1) <lệnh build>   2) <lệnh test/ctest>   3) <điều kiện quan sát được>

NGOÀI PHẠM VI: <liệt kê rõ những gì KHÔNG làm>

KHI XONG: tóm tắt file đã đổi, lệnh đã chạy + kết quả, các mục [VERIFY] còn tồn.
```
**Ví dụ điền sẵn — T-102 (driver SHT31):**
```
TASK: T-102 — Driver SHT31 + ứng dụng bring-up
NGỮ CẢNH: DEV_CODING_PLAN §5.3.1, §5.3.2; firmware/node_h2/components/i2c_bus/include/i2c_bus.h
TẠO: firmware/node_h2/components/sht31/{CMakeLists.txt,include/sht31.h,sht31.c}
     firmware/bringup/bu_sht31/{CMakeLists.txt,main/main.c}
     tests/host/test_sht31_crc.c  (chỉ test hàm sht31_crc8, tách khỏi phụ thuộc IDF)
GIAO DIỆN: uint8_t sht31_crc8(const uint8_t*,size_t); esp_err_t sht31_init(void);
           esp_err_t sht31_read(float *t,float *rh);
DỮ KIỆN: địa chỉ 0x44; đo: ghi 0x24 0x00, chờ >=16 ms, đọc 6 byte (T_hi T_lo CRC RH_hi RH_lo CRC);
         soft reset 0x30 0xA2; CRC-8 poly 0x31 init 0xFF; vector thử {0xBE,0xEF} -> 0x92;
         T=-45+175*raw/65535; RH=100*raw/65535.
NGHIỆM THU: cd firmware/bringup/bu_sht31 && idf.py set-target esp32h2 && idf.py build  (dán kết quả)
            ctest --test-dir build_host -R sht31_crc  (dán kết quả)
            Khi người nạp board: 100 mẫu, 0 lỗi CRC, dòng cuối BU|SHT31|...|PASS
NGOÀI PHẠM VI: CCS811, ADC, Zigbee, mọi thay đổi ở i2c_bus.
```

### 8.2 Nội dung `AGENTS.md` (đặt ở gốc repo — OpenCode đọc làm luật dự án `[VERIFY tên file theo phiên bản OpenCode]`)
```markdown
# Luật cho agent — Fire Alarm IoT
1. Đọc docs/GatewayPlan.md (hợp đồng) và docs/DEV_CODING_PLAN.md (kế hoạch) trước khi làm. Hợp đồng ở common/ và GatewayPlan §4–§5 KHÔNG được đổi nếu không được người dùng cho phép.
2. Mỗi lần chỉ làm MỘT task; chỉ sửa file được liệt kê trong prompt.
3. Toolchain: ESP-IDF 5.x CLI (idf.py). Không PlatformIO, không Arduino, không thư viện ngoài chưa có trong DEV_CODING_PLAN §3.
4. Cấm: delay dài/vòng bận, sprintf/strcpy/strcat/gets, malloc/log trong ISR, số ma thuật ngoài config.h, bí mật trong git.
5. API ESP-IDF/Zigbee: phải grep header thật và ghi docs/API_NOTES.md. Không tìm thấy → DỪNG và hỏi.
6. Không sửa/xóa/nới test để cho qua. Test fail → sửa code, hoặc dừng và báo.
7. Không báo "xong" nếu chưa chạy lệnh nghiệm thu và dán đầu ra thật. Không có thiết bị → ghi rõ "chưa kiểm HIL".
8. Đầu ra (còi/LED) phải ở mức TẮT khi khởi động và khi lỗi.
9. Khi không chắc về pin/thanh ghi/API: hỏi, không đoán.
```

### 8.3 Lỗi hay gặp ở code ESP-IDF do LLM sinh (người duyệt soát nhanh)
`driver/i2c.h` kiểu cũ (nên dùng driver I2C master mới) · gọi `vTaskDelay` trong ISR · `ESP_LOGI` trong ISR · `portMAX_DELAY` ở nơi cần timeout · quên `esp_zb_lock_acquire/release` · quên bảng phân vùng Zigbee · ngưỡng so sánh `float ==` · tràn số `uint32_t` khi trừ thời gian không ép kiểu · `xQueueSend` từ ISR thay vì `...FromISR` · stack task quá nhỏ cho TLS · đặt `esp_task_wdt_reset()` ngoài vòng lặp · quên `static`/`IRAM_ATTR` cho ISR · khởi tạo GPIO đầu ra **sau** NVS/WiFi.




