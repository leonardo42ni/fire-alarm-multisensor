# Fire Alarm Multisensor IoT

Hệ thống cảnh báo cháy đa cảm biến dùng ESP32-H2 (Zigbee) và ESP32 (Gateway host).

## Thành phần

- `firmware/node_h2`: Node end-device Zigbee, target `esp32h2`.
- `firmware/gw_h2`: Coordinator Zigbee + cầu UART, target `esp32h2`.
- `firmware/gw_host`: Gateway trung tâm, fusion, cảnh báo, WiFi/MQTT, target `esp32`.
- `firmware/common`: Header dùng chung (C thuần).
- `firmware/lib_fusion`: Lõi logic thuần, kiểm thử trên PC.
- `tests/host`: Kiểm thử đơn vị bằng CMake/CTest.
- `tools`: Script build và kiểm tra serial.
- `docs`: Hợp đồng kỹ thuật, kế hoạch, ghi chú API và môi trường.

## Yêu cầu môi trường

Xem `docs/ENV.md`. Toolchain bắt buộc: ESP-IDF 5.5.x (`idf.py`), CMake/CTest cho test PC.

## Build firmware

```powershell
. $HOME\esp\esp-idf\export.ps1
.\tools\build_all.ps1
```

Build từng project:

```powershell
idf.py -C firmware/node_h2 build
idf.py -C firmware/gw_h2 build
idf.py -C firmware/gw_host build
```

Nạp và giám sát:

```powershell
idf.py -C firmware/node_h2 -p <PORT> flash monitor
```

## Tài liệu

- `docs/DEV_CODING_PLAN.md`: kế hoạch triển khai và quy tắc mã nguồn.
- `docs/GatewayPlan.md`: hợp đồng giao thức và kiến trúc.
- `docs/GW_SCHEMATIC_SPEC.md`: đặc tả sơ đồ nguyên lý.
- `docs/API_NOTES.md`: chữ ký API đã xác minh (H11).
- `docs/pinmap_verified.md`: đối chiếu pinout thực tế.
