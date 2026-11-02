## 1. Phán quyết
**[ĐẠT - CHO PHÉP COMMIT]**, kèm 3 ghi chú nhỏ (mục 3) có thể làm cùng commit hoặc để sau. Không có lỗi chặn.

Các lỗi V1–V9 đã được xử lý đúng hướng. Riêng V1/V8 (`NODES_MAP`) đã chuyển sang `extern` + một định nghĩa duy nhất trong `nodes_map.c`, đúng như đề xuất.

## 2. Vi phạm và rủi ro

Không vi phạm H1–H11 được quét, và không có chữ ký hàm §5 nào để đối chiếu (T-003 chỉ là header và bảng hằng số).

| # | Mức | Vấn đề |
|---|---|---|
| R1 | Thấp | `shared_config.h` có `#ifndef ZB_CHANNEL`, nhưng `gw_h2/config.h` và `node_h2/config.h` còn `#define ZB_TASK_STACK`, `ZB_TASK_PRIO` lặp nhau (đã ghi nhận ở V5, không lỗi). |
| R2 | Thấp | `RELEASE_BUILD` được kiểm bằng `#if BENCH_MODE && RELEASE_BUILD` trong `shared_config.h`, nhưng `node_h2/config.h` dùng `#if BENCH_MODE` để chọn thời gian warm-up. Đúng, vì `shared_config.h` luôn được include trước. Ghi nhận, không sửa. |
| R3 | Thấp | `gw_host/config.h`: `#if !__has_include("secrets.h")` rồi `#include "secrets.h"`. Đúng với GCC/IDF. Không cần sửa. |
| R4 | Thấp | `NODES_MAP_LEN` là `const size_t` có `extern`. Hợp lệ C. Khi T-404 dùng, nên kiểm `NODES_MAP[0].ieee[0] != '\0'` để bỏ qua sentinel. Ghi vào plan T-404. |
| R5 | Ghi nhận | Chưa có task nào dùng `FW_VERSION` ghép chuỗi trong build thật ngoài `app_main`. Macro stringify đã được build kiểm chứng ở lần chạy trước. |

## 3. Hướng dẫn (chỉ gợi ý, không sửa code)

Không bắt buộc sửa trước commit. Gợi ý cho các task sau:

- **R1:** khi có task thứ ba dùng `ZB_TASK_STACK`/`ZB_TASK_PRIO`, chuyển cả hai về `shared_config.h` và ghi chú trong plan.
- **R4 (T-404):** khi viết code đọc `NODES_MAP`, bỏ qua mục có `ieee` rỗng. Ghi điều này vào mô tả T-404 trong `DEV_CODING_PLAN.md` để agent không quên.
- **Trước khi chuyển sang T-004:** cập nhật `STATE.md` để ghi rõ `NODES_MAP` hiện chỉ có sentinel, và `RELEASE_BUILD` chưa được đặt ở cấu hình build nào.

## 4. Điều kiện
- Commit được ngay. Các mục R1–R4 không chặn.
- Kiểm lại trước khi tag release: `RELEASE_BUILD=1` với `BENCH_MODE=1` phải báo `#error`.