# PINMAP VERIFIED — Đối chiếu chân thực tế

Trạng thái chung: **CHƯA ĐO TRÊN PHẦN CỨNG**. Bảng dưới ghi giá trị thiết kế từ
`docs/DEV_CODING_PLAN.md` và `docs/GW_SCHEMATIC_SPEC.md`; cột "Đã đo" phải được điền
khi kiểm HIL (T-001 và các task bring-up).

## Node — ESP32-H2 SuperMini (`firmware/node_h2`)

| Chức năng | GPIO thiết kế | Nguồn tài liệu | Đã đo |
|---|---|---|---|
| I2C SDA (SHT31, CCS811) | 10 | DEV_CODING_PLAN §5.3.1 | Chưa |
| I2C SCL (SHT31, CCS811) | 11 | DEV_CODING_PLAN §5.3.1 | Chưa |
| MQ-2 AO (qua chia 22k/22k) | 1 | DEV_CODING_PLAN §5.3.4 | Chưa |
| Flame AO (qua chia 22k/22k) | 4 | DEV_CODING_PLAN §5.3.4 | Chưa |
| Flame DO | 14 | DEV_CODING_PLAN §5.3.5 | Chưa |
| SHT31 ADDR | GND (0x44) | DEV_CODING_PLAN §5.3.2 | Chưa |
| CCS811 ADDR | GND (0x5A) | DEV_CODING_PLAN §5.3.3 | Chưa |
| CCS811 nWAKE | GND | DEV_CODING_PLAN §5.3.3 | Chưa |

## GW-H2 — ESP32-H2 SuperMini (`firmware/gw_h2`)

| Chức năng | GPIO thiết kế | Nguồn tài liệu | Đã đo |
|---|---|---|---|
| UART1 TX sang GW-HOST | 10 | DEV_CODING_PLAN §5.4.3, §1.6 | Chưa |
| UART1 RX từ GW-HOST | 11 | DEV_CODING_PLAN §5.4.3, §1.6 | Chưa |

## GW-HOST — ESP32 DevKit WROOM-32 (`firmware/gw_host`)

| Chức năng | GPIO thiết kế | Nguồn tài liệu | Đã đo |
|---|---|---|---|
| UART2 RX (từ H2 TX) | 16 | DEV_CODING_PLAN §1.6 | Chưa |
| UART2 TX (sang H2 RX) | 17 | DEV_CODING_PLAN §1.6 | Chưa |
| LED RGB — R | 25 | DEV_CODING_PLAN §1.7 (X2) | Chưa |
| LED RGB — G | 26 | DEV_CODING_PLAN §1.7 (X2) | Chưa |
| LED RGB — B | 27 | DEV_CODING_PLAN §1.7 (X2) | Chưa |
| Còi (qua NPN) | 4 | DEV_CODING_PLAN §1.7 (X3), §1.7.1 | Chưa |
| Nút mute/test | 32 | DEV_CODING_PLAN §5.5.5c | Chưa |
| PWR_SENSE (ADC1) | 35 | DEV_CODING_PLAN §5.5.6 | Chưa |

## Xung đột cần xử lý trước khi coi là "verified"

- `docs/GW_SCHEMATIC_SPEC.md` §3.3 vẫn ghi 3 LED rời: GPIO25 = LED_R, GPIO26 = LED_Y,
  GPIO27 = LED_G. `docs/DEV_CODING_PLAN.md` §1.7 (X2) đã đổi sang 1 LED RGB cathode chung:
  GPIO25 = R, GPIO26 = G, GPIO27 = B. Bảng trên theo DEV_CODING_PLAN; cần cập nhật
  GW_SCHEMATIC_SPEC và xác nhận thứ tự R/G/B trên board thật.
- Các chân strapping/nội bộ không dùng: GPIO0, 2, 5, 12, 15 và GPIO6–11 trên ESP32 DevKit
  (GPIO6–11 nối flash). Không cấp 5 V trực tiếp vào bất kỳ GPIO nào.
