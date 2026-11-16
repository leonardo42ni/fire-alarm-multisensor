# Luật cho agent — Fire Alarm IoT (OpenCode / DeepSeek)

## 0. KHÔI PHỤC NGỮ CẢNH KHI MỞ CHAT MỚI (CONTEXT RECOVERY)
Khi bắt đầu phiên chat mới hoặc khi người dùng yêu cầu làm tiếp:
1. Đọc ngay `STATE.md` để biết Task ID hiện tại (`T-xxx`), nhánh Git đang mở và trạng thái gần nhất.
2. Đọc `review.md`. Nếu có nhận xét lỗi từ Reviewer (Argon 4), BẮT BUỘC sửa triệt để các lỗi đó trước, chạy lại lệnh nghiệm thu rồi mới chuyển sang Task mới.
3. Chỉ đọc đúng các mục trong `docs/DEV_CODING_PLAN.md` và `docs/GatewayPlan.md` được chỉ định cho Task đó (không đọc tràn lan toàn bộ repo).

## 1. KỶ LUẬT BẤT BIẾN (THEO DEV_CODING_PLAN.md §1.1 & §8.2)
1. Đọc `docs/GatewayPlan.md` (hợp đồng) và `docs/DEV_CODING_PLAN.md` (kế hoạch) trước khi làm[cite: 2]. Hợp đồng ở `firmware/common/` và `GatewayPlan.md` §4–§5 KHÔNG được đổi nếu không được người dùng cho phép[cite: 2, 5].
2. Mỗi lần chỉ làm MỘT task; chỉ tạo/sửa đúng các file được liệt kê trong prompt của task đó[cite: 2]. Giữ nguyên chữ ký hàm trong §5[cite: 2].
3. Toolchain: **ESP-IDF 5.5.x CLI (`idf.py`)** cho cả 3 firmware (`node_h2`, `gw_h2`, `gw_host`) và **CMake/CTest** cho `tests/host/`[cite: 2]. Không PlatformIO, không Arduino, không thư viện ngoài chưa có trong `DEV_CODING_PLAN.md` §3[cite: 2].
4. **Cấm tuyệt đối (H1–H10):**
   - `delay()`/`vTaskDelay()` dài trong luồng xử lý logic (chỉ dùng `vTaskDelayUntil` có chú thích `/* periodic */` hoặc chờ trên queue/timeout)[cite: 2].
   - `sprintf`, `strcpy`, `strcat`, `gets` (chỉ dùng `snprintf`, kiểm tra tràn buffer)[cite: 2].
   - Cấp phát heap (`malloc`), `printf` hoặc `ESP_LOGx` bên trong ISR[cite: 2].
   - Số ma thuật ngoài `config.h` / `fusion_params.h` / `shared_config.h`[cite: 2].
   - Bí mật (WiFi/MQTT/Token) trong git[cite: 2].
   - Mỗi file ≤ ~300 dòng, mỗi hàm ≤ ~60 dòng[cite: 2].
5. **Chống bịa API (H11):** Trước khi dùng bất kỳ API ESP-IDF/Zigbee nào lần đầu, PHẢI tìm xác minh chữ ký thật trong file header đã cài và ghi vào `docs/API_NOTES.md`[cite: 2]. Nếu không tìm thấy → **DỪNG và hỏi người dùng, tuyệt đối không đoán**[cite: 2].
6. Không sửa/xóa/nới ngưỡng bài test trong `tests/host/` để cho qua (H8)[cite: 2]. Test fail → sửa code ứng dụng hoặc dừng báo cáo[cite: 2].
7. Không báo "xong" nếu chưa chạy lệnh build / `ctest` nghiệm thu và dán đầu ra thật[cite: 2]. Nếu task cần phần cứng (HIL) mà chưa cắm mạch → ghi rõ "Đã pass build/CTest, chờ người dùng kiểm HIL"[cite: 2].
8. Đầu ra (`buzzer_init()`, `rgb_init()`) phải đưa về mức TẮT ở ngay dòng đầu tiên của `app_main` trước cả NVS/WiFi[cite: 2].
9. Sau khi hoàn thành hoặc sửa xong 1 Task: Cập nhật ngay file `STATE.md` và nhắc người dùng chạy `.\export_diff.ps1` để kiểm duyệt.
10. Khi không chắc về pin/thanh ghi/API: hỏi, không đoán.
11. Không tự push/merge/tag lên remote public khi chưa được người dùng cho phép; quy trình remote ở `docs/DEV_CODING_PLAN.md` §0.4 và `docs/ENV.md`.