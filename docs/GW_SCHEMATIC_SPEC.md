# GW_SCHEMATIC_SPEC.md — Đặc tả schematic khối Gateway

Bám theo `GatewayPlan.md` v0.1 (§3.3 Gateway, §5.1 UART, §3.4 giao diện nguồn).
Mục đích: để bạn vẽ trực tiếp trong KiCad theo sheet, và để PCB về sau dễ đi dây.

| | |
|---|---|
| Trạng thái | Nháp, phụ thuộc quyết định D1 (nguồn) |
| Phạm vi | Gateway: GW-H2 (ESP32-H2 SuperMini), GW-HOST (ESP32 DevKit WROOM-32), nút/LED/còi, cảm biến điện lưới |
| Ngoài phạm vi | Khối nguồn chi tiết (chỉ định nghĩa giao diện) |

---

## 1. Nguyên tắc vẽ (để sau này làm PCB dễ)

1. **Board dev dùng header cái (female), không hàn trực tiếp.** Trên schematic, mỗi board là một ký hiệu connector với đúng số chân của header. Khi làm PCB, footprint là hai hàng header cái đặt đúng khoảng cách 2,54 mm (hoặc 1 hàng cho SuperMini).
2. **Mọi net có tên.** Không để net vô danh. Tên net trùng với tên trong bảng §4.
3. **Không đi 5 V vào bất kỳ chân GPIO nào.** Đây là lỗi nguy hiểm nhất của ESP32. Chỉ có `PWR_SENSE` nhận 5 V, và nó đi qua chia áp.
4. **Chia áp và tụ đặt đúng sheet với chân nó phục vụ**, để khi đi PCB không bị nhầm.
5. **Chân dự phòng**: ghi "NC" thay vì để trống.

---

## 2. Cấu trúc sheet

```
Gateway_Main.sch  (root)
├── 01_PWR_IF.sch          Giao diện nguồn: adapter, chia áp PWR_SENSE, đầu ra cho UPS và board
├── 02_HOST_WROOM32.sch    Header ESP32 DevKit WROOM-32 (GW-HOST) + tụ tách nguồn
├── 03_ZB_H2.sch           Header ESP32-H2 SuperMini (GW-H2)
├── 04_UART_LINK.sch       Đường UART H2 ↔ WROOM (điện trở nối tiếp)
└── 05_IND_BTN_BUZ.sch     LED R/Y/G, còi active, nút mute, điện trở hạn dòng
```

Nếu muốn gọn hơn, có thể gộp 04 vào 02 và 03. Giữ tách để kiểm tra từng phần.

---

## 3. Chi tiết từng sheet

### 3.1 `01_PWR_IF.sch` — Giao diện nguồn

**Ký hiệu:**
| Ref | Ký hiệu | Ghi chú |
|---|---|---|
| J_ADP | Conn_01x02 | Đầu vào adapter 5 V: pin1 `+5V_ADP`, pin2 `GND` |
| R_PS1 | R, 10 kΩ | Từ `+5V_ADP` xuống node `PWR_SENSE` |
| R_PS2 | R, 15 kΩ | Từ `PWR_SENSE` xuống `GND` |
| C_PS1 | C, 100 nF | `PWR_SENSE` → `GND`, lọc nhiễu |
| C_IN1 | C, 470 µF / 10 V (điện phân) | `+5V_ADP` → `GND`, gần đầu vào |
| J_UPS | Conn_01x02 | Đầu vào/ra module UPS: pin1 `+5V_ADP` (vào UPS), pin2 `GND` |
| J_OUT | Conn_01x02 | Đầu ra rail hệ thống: pin1 `+5V`, pin2 `GND` |
| C_OUT1 | C, 470 µF / 10 V | `+5V` → `GND`, gần đầu ra |
| C_OUT2 | C, 100 nF | `+5V` → `GND`, gần đầu ra |

**Phép tính chia áp (kiểm tra lại khi vẽ):**
`V_PWR_SENSE = V_ADP × R_PS2 / (R_PS1 + R_PS2) = 5 × 15 / 25 = 3,0 V`
Với adapter 5,3 V: `3,18 V`. Vẫn dưới 3,3 V. Với adapter 5,5 V: `3,3 V`, sát ngưỡng. **Dùng adapter 5 V là đủ.**

**Lưu ý thiết kế:**
- `PWR_SENSE` phải lấy **trước** module UPS (tức là từ `+5V_ADP`), để báo mất điện lưới ngay cả khi UPS đang cấp.
- Nếu bỏ module UPS (chỉ dùng nguồn lưới, không pin), `J_UPS` được nối tắt: `+5V_ADP` → `+5V`.

**Net xuất ra:** `+5V`, `GND`, `PWR_SENSE`.

---

### 3.2 `02_HOST_WROOM32.sch` — GW-HOST (ESP32 DevKit WROOM-32, 38 chân)

**Ký hiệu:** `J_HOST` — hai connector 1×19 (trái, phải), hoặc một symbol "ESP32-DevKitC" nếu thư viện có. Đặt tên pin theo GPIO (silkscreen), không đặt theo số thứ tự chân.

**Các chân được dùng và net gắn vào:**

| Chân (tên GPIO) | Net | Chức năng |
|---|---|---|
| 5V (VIN) | `+5V` | Cấp nguồn DevKit |
| GND | `GND` | Đất (đấu ít nhất 2 chân GND) |
| GPIO16 (RX2) | `UART_RX_HOST` | Nhận từ H2 |
| GPIO17 (TX2) | `UART_TX_HOST` | Gửi sang H2 |
| GPIO25 | `LED_R` | LED đỏ (FIRE) |
| GPIO26 | `LED_Y` | LED vàng |
| GPIO27 | `LED_G` | LED xanh |
| GPIO33 | `BUZ_OUT` | Điều khiển còi |
| GPIO32 | `BTN_MUTE` | Nút mute/test (kéo lên nội bộ) |
| GPIO35 | `PWR_SENSE` | Chỉ nhập (input-only), đọc ADC1 |

**Chân không dùng:** đánh dấu NC hoặc để trống, không nối gì. Đặc biệt **không** dùng GPIO 0, 2, 5, 12, 15 (chân strapping), và không dùng GPIO 6–11 (nối nội bộ với flash).

**Lưu ý thiết kế:**
- Trên DevKit có sẵn LDO 3,3 V và mạch nạp USB, không cần thêm LDO ngoài.
- Tụ trên `+5V` đã có ở `01_PWR_IF`. Có thể thêm 100 nF gần chân 5V của header nếu thấy cần.
- Kiểm tra lại từng GPIO bằng silkscreen của DevKit thực tế trước khi vẽ (`[VERIFY]` theo GatewayPlan §3.1).

---

### 3.3 `03_ZB_H2.sch` — GW-H2 (ESP32-H2 SuperMini)

**Ký hiệu:** `J_H2` — connector 1×N (hoặc hai hàng) theo đúng số chân header của SuperMini. Chỉ đấu các chân sau:

| Chân (tên GPIO) | Net | Chức năng |
|---|---|---|
| 5V | `+5V` | Cấp nguồn |
| GND | `GND` | Đất |
| GPIO10 | `UART_TX_H2` | Gửi sang WROOM (đi qua R trong sheet 04) |
| GPIO11 | `UART_RX_H2` | Nhận từ WROOM (đi qua R trong sheet 04) |

**Chân không dùng:** để NC. Không đấu thêm chân nào cho GW-H2, vì coordinator Zigbee không cần cảm biến hay nút (LED onboard và nút BOOT/RESET có sẵn trên board).

**Lưu ý thiết kế:**
- SuperMini **không có chip USB-UART**, cổng USB-C nối thẳng chip. Khi nạp firmware cần giữ BOOT khi cắm USB-C (theo mô tả của nhà bán).
- Tuyệt đối không đưa 5 V vào chân 3V3 của SuperMini. Chỉ đưa 5 V vào chân `5V`.
- Anten PCB của H2 phải nằm ở mép board, không đặt dây hay linh kiện bằng đồng bên dưới (xem GatewayPlan §3.5).

---

### 3.4 `04_UART_LINK.sch` — Đường UART H2 ↔ WROOM

**Ký hiệu:** 2 điện trở 220 Ω.

| Ref | Ký hiệu | Nối |
|---|---|---|
| R_U1 | R, 220 Ω | `UART_TX_H2` → (nối tiếp) → net `UART_RX_HOST` |
| R_U2 | R, 220 Ω | `UART_TX_HOST` → (nối tiếp) → net `UART_RX_H2` |

**Ghi chú về cấu trúc chéo** (dễ sai nhất trong khối này):
- TX của H2 nối với RX của WROOM. TX của WROOM nối với RX của H2.
- Hai dây còn lại (GND) đấu chung.

**Ghi chú về điện trở:** 220 Ω nối tiếp hạn dòng và giảm nhiễu khi chân bị chạm nhầm. Đây là tùy chọn, không bắt buộc để chạy được (`[PLACEHOLDER]`).

**Quy tắc ERC:** trong sheet này, các net `UART_*_H2` và `UART_*_HOST` phải có đúng một bên là chân output và một bên là input. Nếu ERC báo xung đột TX-TX hoặc RX-RX thì đấu chéo đang sai.

---

### 3.5 `05_IND_BTN_BUZ.sch` — LED, còi, nút

**Ký hiệu và nối:**

| Ref | Ký hiệu | Nối |
|---|---|---|
| R_LR | R, 220 Ω | `LED_R` → R → Anode(D_R) |
| D_R | LED đỏ | Cathode → `GND` |
| R_LY | R, 220 Ω | `LED_Y` → R → Anode(D_Y) |
| D_Y | LED vàng | Cathode → `GND` |
| R_LG | R, 220 Ω | `LED_G` → R → Anode(D_G) |
| D_G | LED xanh | Cathode → `GND` |
| BZ1 | Còi active, module 3 chân (VCC/GND/I/O) | VCC → `+5V`, GND → `GND`, I/O → `BUZ_OUT` |
| SW_MUTE | Nút nhấn (thường mở) | một đầu `BTN_MUTE`, đầu kia `GND` |
| C_BTN | C, 100 nF | `BTN_MUTE` → `GND` (chống dội, tùy chọn) |

**Ghi chú quan trọng:**
- **Còi:** nhiều module còi 5 V là *low-level trigger*, nghĩa là kêu khi I/O = LOW. Khi đó phải đặt `BUZZER_ACTIVE_LEVEL` trong firmware và đảm bảo GPIO33 ở mức tắt ngay lúc boot. Kiểm tra datasheet module thực tế trước khi đấu (`GatewayPlan §3.3.1`).
- **Nếu dùng còi trần 2 chân** thay vì module: cần transistor NPN (S8050 hoặc 2N2222), điện trở nền 1 kΩ, và diode 1N4148 ngược chiều song song còi nếu còi là loại điện từ. Sơ đồ khi đó: `BUZ_OUT` → 1 kΩ → Base; Emitter → `GND`; Collector → còi(−), còi(+) → `+5V`; diode song song còi. Đây là phương án dự phòng, không dùng nếu có module.
- **LED:** dòng khoảng 6 mA mỗi LED, nằm trong giới hạn an toàn của GPIO. Không cần transistor.
- **Nút mute:** dùng điện trở kéo lên nội bộ của ESP32 (`INPUT_PULLUP`), không cần điện trở kéo ngoài. Nhấn = LOW.
- **Vị trí LED và nút trên vỏ:** nếu muốn đặt LED ra mặt trước vỏ, tách sheet này thành board nhỏ nối bằng header (xem GatewayPlan BOM). Điện trở LED có thể đặt trên board chính hoặc trên board nhỏ, nhưng chọn **một chỗ duy nhất** và ghi rõ trên sheet.

**Net xuất ra:** `LED_R`, `LED_Y`, `LED_G`, `BUZ_OUT`, `BTN_MUTE` (đi đến `02_HOST_WROOM32`).

---

## 4. Bảng net tổng hợp (dùng để kiểm tra chéo)

| Net | Nguồn → đích | Điện áp | Ghi chú |
|---|---|---|---|
| `+5V` | `01` → `02` (5V), `03` (5V), `05` (VCC còi) | 5 V | Rail chính |
| `GND` | tất cả | 0 V | Đất chung |
| `PWR_SENSE` | `01` → `02` (GPIO35) | ~3,0 V | Đọc ADC, chỉ input |
| `UART_TX_H2` | `03` (GPIO10) → `04` R_U1 | 3,3 V | |
| `UART_RX_HOST` | `04` R_U1 → `02` (GPIO16) | 3,3 V | |
| `UART_TX_HOST` | `02` (GPIO17) → `04` R_U2 | 3,3 V | |
| `UART_RX_H2` | `04` R_U2 → `03` (GPIO11) | 3,3 V | |
| `LED_R` | `02` (GPIO25) → `05` | 3,3 V | |
| `LED_Y` | `02` (GPIO26) → `05` | 3,3 V | |
| `LED_G` | `02` (GPIO27) → `05` | 3,3 V | |
| `BUZ_OUT` | `02` (GPIO33) → `05` | 3,3 V | Mức logic theo module còi |
| `BTN_MUTE` | `02` (GPIO32) → `05` | 3,3 V | Kéo lên nội bộ |

**Quy tắc kiểm tra thủ công (làm trước khi chạy ERC):**
1. Không có net nào mang `+5V` mà đồng thời là chân GPIO.
2. `PWR_SENSE` chỉ đi qua chia áp, không đi thẳng từ `+5V`.
3. Mỗi net UART có đúng một output và một input ở hai đầu.
4. Chân strapping (0, 2, 5, 12, 15) không có net nào gắn vào.

---

## 5. Các bước vẽ trong KiCad

1. Tạo project `Gateway`. Tạo root `Gateway_Main.sch`.
2. Thêm 5 sheet theo §2. Đặt tên file đúng như trên.
3. Vẽ từng sheet theo §3. Dùng **Place → Global Label** cho các net đi giữa các sheet (`+5V`, `GND`, `PWR_SENSE`, `UART_*`, `LED_*`, `BUZ_OUT`, `BTN_MUTE`). Dùng **Local Label** cho net chỉ nằm trong một sheet.
4. Gán footprint ngay khi vẽ schematic để PCB không phải làm lại:
   - `J_HOST`: `PinHeader_1x19_P2.54mm_Vertical` (×2), hoặc đặt footprint `PinSocket` cho DevKit
   - `J_H2`: `PinSocket_1x10_P2.54mm_Vertical` (×2) hoặc theo số chân thực
   - Điện trở: `R_Axial_DIN0207` (THT) hoặc `R_0805` (SMD)
   - LED: `LED_D5.0mm` (THT)
   - Tụ điện phân: `CP_Radial_D8.0mm_P3.50mm`
5. Chạy **Inspect → Electrical Rules Checker**. Mục tiêu: 0 lỗi. Cảnh báo "pin not connected" được chấp nhận nếu đó là các chân NC đã đánh dấu.
6. Chạy **Tools → Generate Netlist** và so sánh với bảng §4.
7. Lưu, xuất PDF của từng sheet để đưa vào báo cáo.

---

## 6. Việc cần xác nhận trước khi vẽ

| # | Câu hỏi | Ảnh hưởng |
|---|---|---|
| Q1 | Khối nguồn bạn đã vẽ là phiên bản cũ hay phiên bản UPS (§3.4)? | Chỉ ảnh hưởng `01_PWR_IF`. Các sheet khác không đổi |
| Q2 | Đã kiểm tra pinout SuperMini và DevKit thực tế chưa? | Nếu khác, sửa cột "Chân" ở §3.2 và §3.3 trước khi vẽ |
| Q3 | Còi bạn mua là module 3 chân hay còi trần 2 chân? | Quyết định §3.5 dùng cấu hình nào |
| Q4 | LED đặt trên mainboard hay trên board mặt trước? | Quyết định vị trí điện trở LED |

---

*Đặc tả này chỉ mô tả đấu nối logic. Các giá trị R, C và chân GPIO đánh dấu `[PLACEHOLDER]` hoặc `[VERIFY]` trong GatewayPlan vẫn phải được xác nhận trên phần cứng thật.*
