# export_diff.ps1 - Đóng gói Diff + Tự động quét Rule H1-H11 gửi cho Google Argon 4
# LƯU Ý: file phải lưu UTF-8 CÓ BOM để PowerShell 5.1 không hiểu nhầm ký tự tiếng Việt thành dấu ngoặc.
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
$OutputEncoding = [System.Text.Encoding]::UTF8

Write-Host "=== [1/3] Đang quét tự động các quy tắc cứng (H1, H4, H6, H11)... ===" -ForegroundColor Cyan

# Ép Git nhận diện các file mới tạo (Untracked)
git add -N . | Out-Null

# Quét H4: Cấm sprintf, strcpy, strcat, gets
$h4_violations = git grep -nE '\b(sprintf|strcpy|strcat|gets)\(' -- "firmware/" 2>$null
if (-not $h4_violations) { $h4_violations = "PASS (0 vi phạm)" }

# Quét H1: Kiểm tra vTaskDelay
$h1_check = git grep -n "vTaskDelay(" -- "firmware/" 2>$null
if (-not $h1_check) { $h1_check = "PASS (Không dùng vTaskDelay)" }

# Quét H6: Kiểm tra lộ mật khẩu
$h6_violations = git grep -nE '(PASSWORD|PASS|TOKEN)[[:space:]]*=[[:space:]]*"[^"]+"' -- ':!*.example.*' ':!export_diff.ps1' 2>$null
if (-not $h6_violations) { $h6_violations = "PASS (Không lộ bí mật)" }

Write-Host "=== [2/3] Đang thu thập Git Diff và STATE.md... ===" -ForegroundColor Cyan

$stateContent = if (Test-Path "STATE.md") { Get-Content "STATE.md" -Raw -Encoding UTF8 } else { "Chưa có STATE.md" }
$apiNotes = if (Test-Path "docs/API_NOTES.md") { Get-Content "docs/API_NOTES.md" -Raw -Encoding UTF8 } else { "Chưa có docs/API_NOTES.md" }

$exclude = @(":!build*", ":!managed_components", ":!review.md", ":!export_diff.ps1")

$diffContent = git diff HEAD -- . @exclude | Out-String
$diffLabel = "git diff HEAD (working tree)"
$changedFiles = git status --short | Out-String

if ([string]::IsNullOrWhiteSpace($diffContent)) {
    git rev-parse --verify HEAD~1 2>$null | Out-Null
    if ($LASTEXITCODE -eq 0) {
        $diffContent = git diff HEAD~1 HEAD -- . @exclude | Out-String
        $diffLabel = "git diff HEAD~1 HEAD (commit gần nhất; working tree sạch)"
        $changedFiles = git diff --name-status HEAD~1 HEAD | Out-String
    }
}

if ([string]::IsNullOrWhiteSpace($diffContent)) {
    Write-Host "[!] Không có thay đổi mã nguồn nào để xuất!" -ForegroundColor Red
    exit 0
}

Write-Host "=== [3/3] Đang đóng gói vào Clipboard cho Google Argon 4... ===" -ForegroundColor Cyan

$bt = '```'

$payload = @"
Bạn là Principal Embedded Architect & Reviewer kiểm duyệt code theo tài liệu DEV_CODING_PLAN.md.
Hãy kiểm tra bản thay đổi mã nguồn (PR) dưới đây theo đúng 10 mục trong Checklist §1.8 và Quy tắc cứng H1–H11 (§1.1).

### 1. TRẠNG THÁI HIỆN TẠI (STATE.md):
$stateContent

### 2. KẾT QUẢ QUÉT TỰ ĐỘNG QUY TẮC CỨNG:
- H4 (Cấm sprintf/strcpy/strcat/gets): $h4_violations
- H1 (Kiểm tra vTaskDelay): $h1_check
- H6 (Kiểm tra lộ Secret): $h6_violations

### 3. BẢNG docs/API_NOTES.md HIỆN TẠI (Quy tắc H11):
$apiNotes

### 4. DANH SÁCH FILE THAY ĐỔI ($diffLabel):
$changedFiles

### 5. CHI TIẾT CODE THAY ĐỔI (git diff):
${bt}diff
$diffContent
${bt}

### 6. YÊU CẦU ĐẦU RA (ĐỂ COPY DÁN VÀO review.md):
Hãy đối chiếu khắt khe với đặc tả hàm trong §5 của DEV_CODING_PLAN.md và trả lời ngắn gọn:
1. Phán quyết: [ĐẠT - CHO PHÉP COMMIT] hoặc [TỪ CHỐI - CẦN SỬA LỖI]
2. Vi phạm Quy tắc cứng (H1-H11) hoặc sai chữ ký hàm §5 (nếu có):
3. Hướng dẫn sửa chi tiết từng lỗi: (Chỉ rõ tên file, tên hàm, dòng code và đoạn mã sửa lại để Agent DeepSeek thực thi ngay).
"@

try {
    Set-Clipboard -Value $payload
} catch {
    $payload | clip.exe
}
$lineCount = ($payload -split "`n").Count
Write-Host "[HOÀN TẤT] Đã copy $lineCount dòng vào Clipboard!" -ForegroundColor Green
Write-Host "-> Hãy sang trình duyệt Google Argon 4 và nhấn Ctrl + V." -ForegroundColor Yellow
