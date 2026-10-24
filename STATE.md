# TRẠNG THÁI THỰC THI DỰ ÁN (PROJECT STATE)

- **Phase hiện tại:** Phase 0 — Môi trường, khung dự án, cổng G0[cite: 2]
- **Nhánh Git:** `main` (Chuẩn bị tách nhánh cho từng Task theo quy tắc `feat/<TaskID>-<ten-ngan>`)[cite: 2]
- **Task vừa hoàn thành:** Chưa có
- **Task tiếp theo cần làm:**
  - `T-002`: Tạo cây thư mục repo theo §2 của `docs/DEV_CODING_PLAN.md`, `.gitignore`, 3 project IDF rỗng (`node_h2`, `gw_h2`, `gw_host`) và script `tools/build_all.ps1`[cite: 2].
  - `T-003`: Tạo `config.h` (x3), `shared_config.h`, `secrets.example.h`, `error.h`, `version.h`[cite: 2].
  - `T-004`: Thiết lập `tests/host/CMakeLists.txt` và `test_util.h` để chạy `ctest` trên PC[cite: 2].
- **Ghi chú kỹ thuật:** Tuân thủ sơ đồ chân Phase 1 trong `docs/DEV_CODING_PLAN.md` (Node I2C: SDA=GPIO10, SCL=GPIO11; MQ2 AO=GPIO1; Flame AO=GPIO4, DO=GPIO14)[cite: 2].