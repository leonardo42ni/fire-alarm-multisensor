# KẾT QUẢ CODE REVIEW TỪ PRINCIPAL REVIEWER & CLAUDE

- **Mã Task:** `T-002` — Khởi tạo cây thư mục, cấu hình SDK, khung 3 firmware và script tự động hóa build.
- **Nhánh Git:** `feat/T-002-skeleton`
- **Thời điểm duyệt:** 2026-10-30

---

## 1. Phán quyết
**[ĐẠT - CHO PHÉP COMMIT]** (Kèm 5 sửa đổi nhỏ mục 3 cần gộp trực tiếp vào commit này trước khi merge). Không có lỗi logic chặn (No blocking bug).

---

## 2. Đánh giá Checklist §1.8 & Quy tắc cứng H1–H11

* **Quy tắc cứng H1–H11:**
  - **H1 (Delay dài / Vòng bận):** ĐẠT. Quét tự động 0 kết quả; cả 3 file `app_main.c` chỉ in log rồi kết thúc luồng khởi tạo, không có `while(1)` bận hay `vTaskDelay` trái phép.
  - **H2 (Giám sát TWDT):** ĐẠT. Cấu hình TWDT 10s (`CONFIG_ESP_TASK_WDT_TIMEOUT_S=10`, `CONFIG_ESP_TASK_WDT_PANIC=y`) đã nạp đầy đủ trong `sdkconfig.defaults` đúng §3.
  - **H3 (An toàn ISR):** ĐẠT. Chưa phát sinh mã nguồn ISR.
  - **H4 (Cấm hàm chuỗi không an toàn):** ĐẠT. 0 vi phạm (không dùng `sprintf`, `strcpy`, `strcat`, `gets`).
  - **H5 (Xử lý lỗi / `esp_err_t`):** ĐẠT. `esp_reset_reason()` trả về enum đã được ép kiểu `(int)` an toàn khi format log.
  - **H6 (Bảo mật bí mật):** ĐẠT. `.gitignore` đã chặn toàn bộ `secrets.h`, `.env`, `.env.*` và ngoại lệ duy nhất là `!.env.example`. File mẫu `secrets.example.h` chỉ chứa dữ liệu giả lập.
  - **H7 (Hằng số ma thuật):** ĐẠT. Định danh firmware và phiên bản được tách biệt thành công vào `config.h` và `version.h`.
  - **H8 (Toàn vẹn bài test):** ĐẠT. Chưa can thiệp bài test.
  - **H9 (Kích thước file/hàm):** ĐẠT. Toàn bộ file đều < 50 dòng, hàm `app_main()` < 10 dòng.
  - **H10 (Kênh UART tách biệt Log):** ĐẠT. Cả hai bo H2 đã cấu hình `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y`, giải phóng hoàn toàn UART1 cho giao thức NDJSON.
  - **H11 (Sổ tay API `API_NOTES.md`):** CẦN BỔ SUNG NHẸ (Xem R2 bên dưới).

---

## 3. Bảng phân tích rủi ro & Lệch quy chuẩn

| # | Mức độ | Vấn đề phát hiện | Phân tích & Biện pháp khắc phục |
|---|:---:|---|---|
| **R1** | **Trung bình** | Cấu hình Kconfig Zigbee chưa được xác minh | `sdkconfig.defaults` của `node_h2` và `gw_h2` chứa `CONFIG_ZB_ENABLED`, `CONFIG_ZB_RADIO_NATIVE`, `CONFIG_ZB_ZED`, `CONFIG_ZB_ZCZR`. Hiện tại project chưa nạp component `esp-zigbee-lib` nên IDF chưa có Kconfig tương ứng. Lệnh build thành công nhiều khả năng do hệ thống bỏ qua im lặng các symbol này. Cần đánh dấu cờ `[VERIFY T-006]` để đối chiếu lại ở cổng G0. |
| **R2** | **Thấp** | Quy tắc H11 chưa bao quát macro `ESP_LOGI` | Hàm/macro `ESP_LOGI` (trong `esp_log.h`) được dùng lần đầu ở cả 3 project nhưng chưa được khai báo vào `docs/API_NOTES.md`. Theo quy tắc H11: mọi API/macro ESP-IDF mới xuất hiện đều phải ghi chép chữ ký thật. |
| **R3** | **Thấp** | Lệch phạm vi phân rã Task | Các file `error.h`, `shared_config.h` (rỗng) và `secrets.example.h` thuộc phạm vi của `T-003`. Việc tạo sẵn là vô hại nhưng cần ghi chú rõ trong `STATE.md` để tránh xung đột lịch sử commit. |
| **R4** | **Thấp** | Tài liệu mâu thuẫn đường dẫn nạp môi trường | `README.md` đang hướng dẫn chạy `$HOME\esp\esp-idf\export.ps1`, trong khi máy nghiệm thu thực tế dùng `E:\Espressif\...` và bắt buộc phải đưa Python 3.11 lên đầu PATH để tránh xung đột với Python 3.15 của Windows Store. `docs/ENV.md` còn sót dòng thừa từ T-001. |
| **R5** | **Thấp** | An toàn điều hướng trong `tools/build_all.ps1` | `build_all.ps1` dùng `Push-Location` và `Pop-Location` rải rác mà không bọc trong khối `try/finally`. Nếu có exception phát sinh giữa chừng, thư mục làm việc của PowerShell sẽ bị lệch. |
| **R6** | **Ghi nhận** | Trạng thái an toàn chân còi lúc khởi động | `app_main` của `gw_host` chưa kéo còi (GPIO33) về mức TẮT. Việc hoãn sang Task `T-112` là hợp lý vì chưa có driver `alarm_out`, hiện tại chân GPIO33 hoàn toàn an toàn nhờ điện trở 10 kΩ kéo xuống GND trên phần cứng. |

---

## 4. Hướng dẫn sửa đổi cụ thể (Thực hiện trước khi Commit T-002)

### Sửa R1: Đánh dấu xác minh Kconfig Zigbee
Mở `firmware/node_h2/sdkconfig.defaults` và `firmware/gw_h2/sdkconfig.defaults`, bổ sung dòng chú thích ngay trên cụm `CONFIG_ZB_*`:
```ini
# [VERIFY T-006] Ten symbol ZB chua duoc kiem chung (chua co esp-zigbee-lib). Doi chieu voi Kconfig that khi them component.
CONFIG_ZB_ENABLED=y
CONFIG_ZB_RADIO_NATIVE=y
CONFIG_ZB_ZED=y        # gw_h2: CONFIG_ZB_ZCZR=y
Cập nhật mục Ghi chú kỹ thuật trong STATE.md: "Các symbol CONFIG_ZB_ trong sdkconfig.defaults chưa được kiểm chứng; T-006 phải xác nhận bằng idf.py menuconfig/grep Kconfig của esp-zigbee-lib."

Sửa R2: Bổ sung ESP_LOGI vào docs/API_NOTES.md
Chạy lệnh PowerShell sau để lấy định nghĩa thật từ header cài đặt:

PowerShell
Select-String -Path "E:\Espressif\frameworks\esp-idf-v5.5.5\components\log\include\esp_log.h" -Pattern "define ESP_LOGI"
Sau đó thêm 1 hàng mới vào bảng trong docs/API_NOTES.md tương ứng cho Task T-002.

Sửa R3: Cập nhật ngữ cảnh trong STATE.md
Thêm dòng sau vào mục Ghi chú kỹ thuật của STATE.md:

"Các file error.h, shared_config.h, secrets.example.h là stub tạm từ T-002; Task T-003 sẽ hoàn thiện nội dung chi tiết."

Sửa R4: Đồng bộ hóa tài liệu
Trong README.md, sửa lại mục Build firmware:

Markdown
Nạp môi trường theo `docs/ENV.md` (lưu ý thứ tự PATH Python 3.11), rồi chạy:
.\tools\build_all.ps1
Trong docs/ENV.md, xóa bỏ dòng: "Cập nhật cột 'Trạng thái' sau khi hoàn tất T-001."

Sửa R5: Khắc phục điều hướng trong tools/build_all.ps1
Mở tools/build_all.ps1, thay thế khối try/catch bên trong vòng lặp foreach bằng khối có finally:

PowerShell
    Push-Location -LiteralPath $projectDir
    try {
        & idf.py build
        $exitCode =$LASTEXITCODE
    }
    catch {
        Write-Host "[FAIL] $name : $($_.Exception.Message)" -ForegroundColor Red
        exit 1
    }
    finally {
        Pop-Location
    }
5. Tiêu chí nghiệm thu bắt buộc cho Task tiếp theo (T-003)
Kiểm soát bí mật: gw_host bắt buộc phải có chỉ thị tiền xử lý chặn biên dịch nếu thiếu secrets.h (§1.5):

C
#if !__has_include("secrets.h")
#error "Thieu main/secrets.h: sao chep tu secrets.example.h va dien thong tin"
#endif
#include "secrets.h"
Nghiệm thu: Xóa thử file secrets.h → build phải báo lỗi #error rõ ràng; tạo lại secrets.h từ bản mẫu → build thành công.

Khai báo hằng số hệ thống: Hoàn thiện toàn bộ hằng số chân GPIO, chu kỳ, tỷ lệ phân áp ADC trong shared_config.h và config.h (x3) mà không để số ma thuật trong code (§1.1 H7).


---