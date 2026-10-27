# ENV — Môi trường build và phiên bản

## Phiên bản yêu cầu

| Hạng mục | Phiên bản | Trạng thái trên máy nghiệm thu |
|---|---|---|
| ESP-IDF | 5.5.5 | Đã cài tại `E:\Espressif\frameworks\esp-idf-v5.5.5`; `idf.py --version` báo `ESP-IDF v5.5.5` |
| Python | 3.11.2 (venv `idf5.5_py3.11_env`) | Đã xác minh; PATH mặc định đang có Python 3.15 của Windows Store (xem lưu ý bên dưới) |
| CMake | IDF bundled | Đã dùng để build thành công cả 3 firmware |
| Ninja | IDF bundled | Đã dùng để build thành công cả 3 firmware |
| Node.js | 20 LTS | Chưa xác minh (chưa cần cho T-002) |

Cập nhật cột "Trạng thái" sau khi hoàn tất T-001.

## Cài ESP-IDF 5.5.x trên Windows (PowerShell)

```powershell
New-Item -ItemType Directory -Force -Path $HOME\esp | Out-Null
git clone -b v5.5.5 --recursive https://github.com/espressif/esp-idf.git $HOME\esp\esp-idf
Set-Location $HOME\esp\esp-idf
.\install.ps1 esp32,esp32h2
```

Máy nghiệm thu hiện tại cài bằng ESP-IDF Installer tại `E:\Espressif\frameworks\esp-idf-v5.5.5`
(với `IDF_TOOLS_PATH=E:\Espressif`), venv Python là `E:\Espressif\python_env\idf5.5_py3.11_env`.

Nạp môi trường cho mỗi phiên làm việc:

```powershell
. E:\Espressif\frameworks\esp-idf-v5.5.5\export.ps1
idf.py --version
```

> **Lưu ý Python 3.11 vs 3.15:** `export.ps1` chọn venv theo `python` đầu tiên trong `PATH`.
> Nếu `PATH` trỏ tới Python 3.15 (Windows Store) trong khi venv đã cài là `idf5.5_py3.11_env`,
> export sẽ báo `ESP-IDF Python virtual environment "...idf5.5_py3.15_env..." not found`.
> Đưa Python đi kèm IDF lên đầu `PATH` trước khi export:
>
> ```powershell
> $env:PATH = "E:\Espressif\tools\idf-python\3.11.2;" + $env:PATH
> . E:\Espressif\frameworks\esp-idf-v5.5.5\export.ps1
> idf.py --version
> ```

## Build

Build cả 3 firmware (sau khi đã export môi trường ở mục trên):

```powershell
.\tools\build_all.ps1
```

Build riêng từng project:

```powershell
idf.py -C firmware/node_h2 build
idf.py -C firmware/gw_h2 build
idf.py -C firmware/gw_host build
```

Target được khai báo trong `sdkconfig.defaults`:

| Project | Target |
|---|---|
| `firmware/node_h2` | `esp32h2` |
| `firmware/gw_h2` | `esp32h2` |
| `firmware/gw_host` | `esp32` |

Nếu cần đặt lại target:

```powershell
idf.py -C firmware/node_h2 set-target esp32h2
idf.py -C firmware/gw_h2 set-target esp32h2
idf.py -C firmware/gw_host set-target esp32
```

## Nạp và giám sát

```powershell
idf.py -C firmware/node_h2 -p COMx flash monitor
idf.py -C firmware/gw_h2 -p COMy flash monitor
idf.py -C firmware/gw_host -p COMz flash monitor
```

- Node và GW-H2 dùng console USB-Serial-JTAG.
- GW-HOST dùng UART0 qua cầu USB của DevKit.
- UART giao thức chỉ chứa NDJSON: GW-H2 dùng UART1 (GPIO10/11), GW-HOST dùng UART2 (GPIO16/17).

## Kiểm thử trên PC

```powershell
cmake -S tests/host -B build_host
cmake --build build_host
ctest --test-dir build_host --output-on-failure
```

## Ghi chú

- Không dùng PlatformIO cho firmware.
- Ghim phiên bản `esp-zigbee-lib`/`esp-zboss-lib` trong `dependencies.lock` và commit file đó khi bắt đầu dùng Component Manager.
