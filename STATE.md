# TRẠNG THÁI THỰC THI DỰ ÁN (PROJECT STATE)

- **Phase hiện tại:** Phase 0 — Môi trường, khung dự án, cổng G0
- **Nhánh Git:** `feat/T-002-skeleton`
- **Task vừa hoàn thành:** `T-002` — Đã tạo cây thư mục, `.gitignore`, 3 project ESP-IDF khung (`node_h2`, `gw_h2`, `gw_host`), `docs/API_NOTES.md`, `docs/ENV.md`, `docs/pinmap_verified.md` và `tools/build_all.ps1`.
  - **Phạm vi app_main:** mỗi project chỉ in 3 dòng: tên firmware, `FW_VERSION`, `esp_reset_reason()`. Không có code cảm biến/Zigbee/Fusion và không khởi tạo đầu ra.
  - **Trạng thái nghiệm thu:** `tools/build_all.ps1` đã chạy với ESP-IDF **v5.5.5** → `[PASS] node_h2`, `[PASS] gw_h2`, `[PASS] gw_host`, `[PASS] Ca 3 firmware build thanh cong.` (exit 0). `secrets.h` không bị track và đã được `.gitignore`.
  - **Môi trường:** ESP-IDF v5.5.5 tại `E:\Espressif\frameworks\esp-idf-v5.5.5`, venv Python `idf5.5_py3.11_env`. `PATH` mặc định có Python 3.15 (Windows Store) nên phải đưa `E:\Espressif\tools\idf-python\3.11.2` lên đầu `PATH` trước `export.ps1` (đã ghi trong `docs/ENV.md`).
  - **Dọn dẹp:** Đã xóa `build/` cũ (được cấu hình cho `X:\firmware\...`) rồi build lại từ đầu.
  - **Quyết định của người dùng:** giữ `EXTRA_COMPONENT_DIRS` dạng tương đối `"../common"`, `"../lib_fusion"`; thêm `.clangd`, `.devcontainer/`, `CMakePresets.json` vào `.gitignore`; cập nhật `docs/ENV.md` sang v5.5.5; commit T-002.
- **Task tiếp theo cần làm:**
  - `T-003`: Tạo/hoàn thiện `config.h` (×3), `shared_config.h`, `secrets.example.h`, `error.h`, `version.h`; thêm `#error` khi thiếu `secrets.h` cho `gw_host`.
  - `T-004`: Thiết lập `tests/host/CMakeLists.txt` và `test_util.h` để chạy `ctest` trên PC.
- **Ghi chú kỹ thuật:**
  - Target: `node_h2`/`gw_h2` = `esp32h2`; `gw_host` = `esp32`.
  - `esp_reset_reason()` đã được đối chiếu chữ ký tại `esp_system.h:85` trên bản cài thật v5.5.5 và ghi vào `docs/API_NOTES.md`.
  - `sdkconfig.defaults` và `partitions.csv` copy nguyên văn từ `docs/DEV_CODING_PLAN.md` §3. Kích thước `zb_storage`/`zb_fct` vẫn mang dấu `[VERIFY]` của kế hoạch, sẽ kiểm tra ở T-006 khi thêm Zigbee.
  - Cây thư mục rỗng theo §2 đã được tạo (components/, bringup/, cloud/, tests/host, tests/vectors); các thư mục rỗng không được Git theo dõi.
  - Việc tắt đầu ra (còi/LED) theo §1.3 sẽ làm ở task `alarm_out`/`buzzer` (T-112), không nằm trong T-002.
