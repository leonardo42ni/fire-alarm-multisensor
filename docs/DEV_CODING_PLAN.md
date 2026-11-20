# DEV_CODING_PLAN.md — Firmware & Software Implementation Plan (v2.0 Final — Thesis MVP)

**System:** Wireless Multi-Sensor Early Fire Detection System using **Fuzzy Logic Data Fusion** (upgradable to ML/AI).  
**Pipeline:** Node (`ESP32-H2`) $\rightarrow$ Zigbee 3.0 $\rightarrow$ Gateway (`ESP32-H2` Coordinator + `ESP32 WROOM-32` Host) $\rightarrow$ Edge Data Fusion (`lib_fusion`) $\rightarrow$ Local Alarm + MQTT/TLS (HiveMQ Cloud) $\rightarrow$ **Node-RED Dashboard**.

| Field | Specification |
|---|---|
| **Version** | `2.0 Final` (Supersedes v0.1; Single Source of Truth) |
| **Remaining Timeline** | 3–4 Weeks |
| **Scope Level** | **Moderate Reduction (Thesis MVP):** Happy-Path reliability + Deep focus on `lib_fusion` & experimental validation |
| **Primary Algorithm** | Zero-Order **Sugeno Fuzzy Logic** (decoupled 3-layer architecture ready for Decision Tree / ML drop-in) |
| **Toolchain** | ESP-IDF `v5.5.5` (`esp32h2`, `esp32`), `esp-zigbee-lib 1.6.8`, CMake/CTest (PC Host), Python 3.10+, Node-RED |
| **Executor** | Human Engineer + AI Coding Agent (DeepSeek on OpenCode) |
| **Preserved from v0.1** | `firmware/common/proto_ndjson*` (frozen contract), Zigbee Endpoints `EP10–EP15`, Phase 0 G0 results |

---

## 0. Current Baseline & Scope Boundaries

### 0.1 Completed & Frozen (DO NOT Rewrite or Break)

| Item | Status | Frozen Artifacts / Metrics |
|---|---|---|
| **Phase 0 (`T-001`–`T-006`)** | **DONE (`phase-0-done`)** | ESP-IDF `v5.5.5`, repo skeleton, `tools/build_all.ps1`, CTest PC harness. **G0 Passed:** join $2.87\text{ s}$, packet loss $0.8\%$ over $1000$ reports, GW-only S5 recovery $5.6\text{ s}$. |
| **`T-201` (`proto_ndjson`)** | **DONE & Committed** | `firmware/common/proto_ndjson.h` + implementation + CTest 5/5 (41 cases) & ASAN/UBSAN passing. Call only; never alter schema. |
| **`T-202` (`zb_node`)** | **Build PASS (Awaiting HIL)** | `firmware/node_h2/components/zb_node/` (`EP10–EP15`, `seq` on `EP15` `0x0100`). Review fixes `C1–C8` and HIL verification are handled in `S1-1`. |

### 0.2 Permanently Cut from v0.1
* Separate `firmware/bringup/*` apps and `tools/serial_check.py`.
* Software power/UPS monitoring (`PWR_SENSE`, `GPIO35`, `power_mon`, `T-114`, `T-601`, `T-606`).
* Fault-injection torture tests (`T-602`, `T-208`, `T-FLT-*`) and mandatory 24-hour stability tests.
* Node-side `prealert` state machine (replaced by fixed `REPORT_MS = 2000`), NVS warmup persistence, standalone `fault.c`/`backoff.c`/`pubq.c`, priority publish queues, MQTT `cmd_handler`, `node_cli`, mandatory TWDT on non-critical tasks, mandatory ASAN/UBSAN on every commit.
* **Telegram Bot:** Dropped from MVP scope (implement only if spare time remains after `S3-3`).

### 0.3 Out of Scope (Document under "Limitations" in Thesis)
Single sensor node; single test location; non-commercial power redundancy; no OTA updates; no advanced network security beyond MQTT TLS.

---

## 1. Confirmed Hardware Architecture & Pinout (Overrides v0.1 & `GatewayPlan.md`)

### 1.1 Sensor Node (`ESP32-H2 SuperMini`)
* **Power:** Continuous `5V` DC wall adapter (no battery, no sleep mode).
  * `5V` rail feeds `ESP32-H2` (`5V` pin), `MQ-2` (`VCC`, add a $470\ \mu\text{F}$ decoupling capacitor close to `MQ-2` power pins), and `Flame Sensor` (`VCC`).
  * `3V3` rail (ESP32-H2 onboard LDO) feeds `SHT31` and `CCS811`/`ENS160`. **Never connect 5V to I2C sensors.**
* **Fixed Reporting Cadence:** `REPORT_MS = 2000` ($2\text{ s}$). No dynamic pre-alert period switching.

```text
[NODE — ESP32-H2 SuperMini]
  3V3     ──► SHT31 VIN (0x44, ADDR→GND), Gas Sensor VCC (CCS811 0x5A nWAKE→GND, or ENS160 0x52/0x53)
  GPIO10  ──► I2C SDA (SHT31 + Gas Sensor, 100 kHz, external module pull-ups)
  GPIO11  ──► I2C SCL (SHT31 + Gas Sensor)
  5V      ──► MQ-2 VCC (+ 470uF cap to GND), Flame VCC
  MQ-2 AO ──[22k]──┬──► GPIO1 (ADC1, ADC_DIV_RATIO = 2.0, max <= 2.5V)
                   ├──[22k]──► GND
                   └──[100nF]──► GND
  Flame AO──[22k]──┬──► GPIO4 (ADC1, ADC_DIV_RATIO = 2.0, max <= 2.5V)
                   ├──[22k]──► GND
                   └──[100nF]──► GND
  Flame DO─────────► GPIO14 (Optional digital input)
  USB-C   ─────────► Console Log (CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y)
```

### 1.2 Gateway (`ESP32-H2` Coordinator + `ESP32 WROOM-32` Host)
* **Power:** Passive dual power handled purely in hardware (`5V` main adapter + `3x AA` $4.5\text{ V}$ backup battery box via Schottky diode OR-ing). **No software power monitoring.**
* **UART Link (`115200 8N1`, NDJSON ONLY):** Cross-wired `GW-H2 TX (GPIO10)` $\rightarrow$ `WROOM RX2 (GPIO16)` and `GW-H2 RX (GPIO11)` $\leftarrow$ `WROOM TX2 (GPIO17)` (optional $220\ \Omega$ series resistors) + common `GND`. **Do NOT bridge 5V rails when both boards are plugged into PC USB.**

```text
[GATEWAY — ESP32-H2 Coordinator <-> ESP32 WROOM-32 Host]
  H2 GPIO10 (TX1) ──(220Ω)──► WROOM GPIO16 (RX2)   | Common GND
  H2 GPIO11 (RX1) ◄──(220Ω)── WROOM GPIO17 (TX2)   | H2 logs via USB-Serial-JTAG only

  WROOM GPIO25 ──[220Ω]──► LED RGB (BKGR Common-Cathode: R)
  WROOM GPIO26 ──[100Ω]──► LED RGB (G)                    ; K (Cathode) ──► GND
  WROOM GPIO27 ──[100Ω]──► LED RGB (B)
  WROOM GPIO4  ──[1kΩ]───► NPN Base (S8050/2N2222)        ; [10kΩ] Base──►GND
                           Emitter──►GND; Collector──►Buzzer(-); Buzzer(+)──►+5V + 1N4148 flyback
                           (Note: Direct GPIO4 drive allowed ONLY if 3.3V buzzer draws <= 10mA)
  WROOM GPIO32 ◄── Pushbutton ──► GND (INPUT_PULLUP; press-and-release ONLY mutes Buzzer & LED)
```

*Note on `gw_host/main/config.h`:* Update `BUZZER_GPIO` from `33` (v0.1) to **`4`**, and remove `PWR_SENSE` definitions during `S1-4`/`S3-1`.

### 1.3 Swappable Gas Sensor Abstraction (`CCS811` $\leftrightarrow$ `ENS160`)
Place this clean interface in `firmware/node_h2/components/sensors/gas_sensor.h`:

```c
esp_err_t gas_init(void);
esp_err_t gas_set_env(float t_c, float rh);
esp_err_t gas_read(uint16_t *eco2, uint16_t *tvoc, bool *ready);
bool      gas_stable(void);
```
Select driver via `node_h2/main/config.h`: `#define GAS_DRIVER_CCS811 1` (default) or `#define GAS_DRIVER_ENS160 1`. Each driver lives in its own `.c` file (`gas_ccs811.c` or `gas_ens160.c`). The rest of the firmware never depends on sensor vendor specifics.

### 1.4 Unified Node Status Flags (`firmware/common/shared_config.h`)
Supersedes `GatewayPlan.md` §4.3:
```c
#define NODE_FLAG_FLAME_DO   (1u << 0)  /* Digital flame output active */
#define NODE_FLAG_SHT_FAIL   (1u << 1)  /* SHT31 read/CRC failure */
#define NODE_FLAG_GAS_BAD    (1u << 2)  /* Gas sensor warming up or read error */
#define NODE_FLAG_MQ2_WARM   (1u << 3)  /* MQ-2 still within warmup uptime window */
#define NODE_FLAG_ADC_FAULT  (1u << 4)  /* ADC out-of-range (<20 mV or >=3050 mV at pin) */
```

---

## 2. Engineering Rules (7 Core Rules)

| ID | Rule |
|---|---|
| **R1** | **Non-blocking loops:** No busy-wait or long `vTaskDelay()` in logic paths. Periodic loops must use `vTaskDelayUntil` tagged `/* periodic */` or queue waits with timeout. |
| **R2** | **Buffer safety:** Zero usage of `sprintf`, `strcpy`, `strcat`, `gets`. Always use bounded `snprintf` and check lengths. |
| **R3** | **Error handling:** Use `ESP_ERROR_CHECK` only during unrecoverable boot init. Inside runtime loops, log errors and continue — never silently swallow `esp_err_t`. |
| **R4** | **Secrets hygiene:** Never commit Wi-Fi/MQTT credentials. Keep `secrets.h` in `.gitignore` with `#error` guard in `config.h`. |
| **R5** | **Centralized constants:** All GPIOs, intervals, calibration ratios, and fuzzy parameters live strictly in `config.h`, `shared_config.h`, or `fusion_params.h`. |
| **R6** | **Test integrity:** Never delete, modify, or weaken unit test assertions to make failing code pass. Fix the code or stop and report. |
| **R7** | **Clean UART & Safe Boot:** Inter-MCU UART carries **only** NDJSON. Drive Buzzer (`GPIO4`) and RGB LED (`GPIO25/26/27`) **OFF** at the very start of `gw_host` `app_main()`. |

**Lean File & Stop Discipline:**
* **One `.c`/`.h` pair per new module** (flat C99, minimal boilerplate; do not split a single feature into 4–5 micro-files).
* **Stop & Ask:** If an ESP-IDF/Zigbee API is missing from installed headers, a pin mismatches §1, or a frozen contract (`proto_ndjson.h`, `EP10–15`) needs changing $\rightarrow$ **STOP and ask**. Log tricky APIs (Zigbee, ADC calibration) in `docs/API_NOTES.md`.
* **Review Cadence:** Formal diff review once at the end of each Stage (3 reviews total) checking: Build PASS? DoD met? `R1–R7` clean? Contracts intact? `STATE.md` updated?

---

## 3. Architecture & Streamlined Repository Tree

**Data Flow:** `Node (sensors)` $\xrightarrow{\text{Zigbee}}$ `GW-H2 (zb_coord -> uart_bridge)` $\xrightarrow{\text{NDJSON UART}}$ `GW-HOST (uart_link -> fusion_task -> alarm_out / CSV / MQTT)` $\rightarrow$ `Node-RED`.  
**Key Principle:** All Data Fusion executes on **`GW-HOST`**. Node and `GW-H2` transmit raw telemetry only.

```text
fire-alarm-multisensor/
├─ AGENTS.md                           # Concise agent rules (§9.2)
├─ STATE.md                            # Live 4-line progress tracker
├─ docs/
│  ├─ DEV_CODING_PLAN.md               # This document (v2.0 Final — Single Source of Truth)
│  ├─ API_NOTES.md                     # Verified tricky ESP-IDF/Zigbee signatures
│  └─ ENV.md                           # Local ESP-IDF v5.5.5 environment notes
├─ firmware/
│  ├─ common/                          # FROZEN: proto_ndjson*.c/.h (DONE), shared_config.h, error.h
│  ├─ lib_fusion/                      # CORE THESIS FOCUS (Pure C99, PC & ESP32 portable)
│  │  ├─ fusion.h                      # Public facade API
│  │  ├─ fusion_params.h               # Tunable normalization bounds, MF breakpoints, rules, thresholds
│  │  ├─ fusion_features.h / .c        # Layer 1: Med3, MA, OLS dT/dt, adaptive baseline, normalization
│  │  ├─ fusion_infer.h                # Layer 2: Pluggable inference engine interface (infer_engine_t)
│  │  ├─ infer_fuzzy.c                 # Engine 1: Zero-order Sugeno Fuzzy Logic (15 rules)
│  │  ├─ infer_tree.c                  # (Optional S2-4) Engine 2: Auto-generated Decision Tree if/else
│  │  ├─ fusion_decide.h / .c          # Layer 3: Hysteresis, N_UP/N_DOWN, FIRE latch, 65 C safety net
│  │  └─ CMakeLists.txt
│  ├─ node_h2/                         # Target: esp32h2 (Continuous 5V Adapter)
│  │  ├─ main/                         # app_main.c, config.h, idf_component.yml
│  │  └─ components/
│  │     ├─ zb_node/                   # DONE (T-202, polish & HIL in S1-1)
│  │     └─ sensors/                   # S1-2: sensors.h/.c, gas_sensor.h, gas_ccs811.c (or gas_ens160.c)
│  ├─ gw_h2/                           # Target: esp32h2 (Zigbee Coordinator + UART1 Bridge)
│  │  ├─ main/                         # app_main.c, config.h, idf_component.yml
│  │  └─ components/
│  │     ├─ zb_coord/                  # S1-3: zb_coord.h/.c
│  │     └─ uart_bridge/               # S1-3: uart_bridge.h/.c (300 ms coalesce + NDJSON)
│  └─ gw_host/                         # Target: esp32 WROOM-32 (Edge Fusion + Alarm + Cloud)
│     ├─ main/                         # app_main.c, config.h, secrets.example.h, secrets.h (git-ignored)
│     └─ components/
│        ├─ uart_link/                 # S1-4: uart_link.h/.c (UART2 RX + node timeout + CSV print)
│        ├─ fusion_task/               # S3-1: fusion_task.h/.c (runs lib_fusion on q_rpt)
│        ├─ alarm_out/                 # S3-1: alarm_out.h/.c (RGB LED 25/26/27 + Buzzer GPIO4 + Mute GPIO32)
│        └─ net_mqtt/                  # S3-2: net_mqtt.h/.c (Wi-Fi STA + SNTP + HiveMQ TLS MQTT)
├─ cloud/
│  └─ Node_RED_DATN.json               # S3-2: Updated Node-RED flow for live monitoring & alerts
├─ tests/
│  ├─ host/                            # CMakeLists.txt, test_util.h, test_proto*.c (DONE)
│  │                                   # + test_features.c, test_fuzzy.c, test_decide.c, fusion_replay.c
│  └─ results/                         # HIL logs (S1-1_*.log, etc.)
├─ data/raw/                           # Experiment CSV recordings (.gitkeep)
└─ tools/
   ├─ build_all.ps1                    # Automated 3-firmware ESP-IDF build
   ├─ log_csv.py                       # S1-4: Interactive Serial CSV logger with labeling hotkeys
   ├─ eval.py                          # S2-3: Fuzzy vs. 3 single-sensor baselines evaluator
   └─ train_tree.py                    # (Optional S2-4) scikit-learn Decision Tree -> infer_tree.c
```

---

## 4. Atomic Task Breakdown (10 Tasks across 3 Stages)

Each task targets **one MCU or one isolated logic layer** so AI sessions remain small, fast, and bug-free. End every task by updating `STATE.md` (4 lines: Completed / Command / Result / Next).

> **Flexible Execution Order:** `S2-1` and `S2-2` run purely on PC (`ctest`) and can be developed in parallel with Stage 1 at any time. Within Stage 1, you may run **`S1-1` $\rightarrow$ `S1-3` $\rightarrow$ `S1-4` first** (to complete the entire Node $\rightarrow$ Gateway communication pipe using dummy data before wiring breadboard sensors in `S1-2`).

### Stage 1 — End-to-End Data Pipeline (Week 1 $\rightarrow$ Mid Week 2)

#### **`S1-1` — Finalize `T-202` (`zb_node`) & HIL Verification**
* **Scope:** Apply the T-202 review fixes below in `firmware/node_h2/components/zb_node/`, set `REPORT_MS = 2000`, and run a 30-minute 2-board ESP32-H2 HIL test with dummy values (including one $90\text{ s}$ Gateway power-cut test).
* **T-202 Review Fix Table:**

| Item | Action |
|---|---|
| `C1` Repeated `LEAVE` resets | **Fix:** Add `s_reset_pending` guard flag to prevent overlapping resets. |
| `C2` `scheduler_alarm` outside Zigbee lock | **Fix:** Wrap call inside `esp_zb_lock_acquire` / `esp_zb_lock_release`. |
| `C3` `ZB_REJOIN_FAIL_THRESHOLD = 1` | **Fix:** Set to `3`. |
| `C4` Missing NULL checks on cluster create | **Fix:** Add concise NULL checks. |
| `C5` `ESP_OK ? ESP_OK : ESP_FAIL` hides error | **Fix:** Propagate actual `esp_err_t`. |
| `C6` Identify cluster on `EP11–15` | Document actual code behavior; no rewrite needed. |
| `C7` `flame_do`/`prealert` omitted in publish | **Fix:** Populate `flags` using `NODE_FLAG_*` (§1.4) in `S1-2`; leave prealert `0`. |
| `C8` 60 s backoff cap vs. S5 recovery | **No code change;** measure actual recovery time in HIL. |
| `N1–N8`, `H11 keep_alive` | **Drop** unless an actual failure occurs in HIL. |

* **DoD:** `.\tools\build_all.ps1` PASS; HIL log saved in `tests/results/S1-1_*.log` showing continuous `JOINED` + `ZB_RECV`, packet loss $\le 3\%$ by `seq`, and automatic rejoin $\le 120\text{ s}$ after GW power cycle.

#### **`S1-2` — Node Real Sensors (`node_h2/components/sensors/`)**
* **Scope:** Implement `sensors.h/.c` + `gas_sensor.h` + `gas_ccs811.c` (and optional `gas_ens160.c`) using ESP-IDF I2C master driver and `adc_oneshot` + curve-fitting calibration:
  * **SHT31 (`0x44`):** Write `0x24 0x00`, wait `16 ms`, read 6 bytes (`T_hi, T_lo, CRC, RH_hi, RH_lo, CRC`), verify CRC-8 (poly `0x31`, init `0xFF`, check `{0xBE, 0xEF} -> 0x92`), $T = -45 + 175 \cdot \text{raw}/65535$, $RH = 100 \cdot \text{raw}/65535$.
  * **CCS811 (`0x5A`, `nWAKE=GND`):** Verify `HW_ID(0x20) == 0x81`, `SW_RESET(0xFF) = {0x11, 0xE5, 0x72, 0x8A}` (wait `20 ms`), `APP_START(0xF4)` (wait `20 ms`), `MEAS_MODE(0x01) = 0x10` (1 Hz), write `ENV_DATA(0x05)` big-endian `(uint16)(RH*512+0.5)` and `(uint16)((T+25)*512+0.5)`, read 8 bytes from `ALG_RESULT_DATA(0x02)`. **Critical:** If referencing `ot_cli`, **remove its `T > 24 C` hack** (which zeros CCS811 readings below $24^\circ\text{C}$ and corrupts fusion data).
  * **ADC (`MQ-2` on `GPIO1`, `Flame` on `GPIO4`):** `ADC_ATTEN_DB_12`, 16 samples, drop min & max, average remaining 14, multiply by `ADC_DIV_RATIO = 2.0f`. Set `NODE_FLAG_ADC_FAULT` if pin voltage $< 20\text{ mV}$ or $\ge 3050\text{ mV}$.
  * **Flame DO (`GPIO14`):** Optional digital read into `NODE_FLAG_FLAME_DO`.
  * **Warmup:** Track via uptime (`MQ2_WARMUP_MS`, `GAS_STABLE_MS` in `config.h`; no NVS writes). Support `BENCH_MODE` flag for short dev warmup, but disable it for real thesis data collection.
  * **Task:** `sensor_task` runs every `2000 ms` (`vTaskDelayUntil /* periodic */`), populates `node_snapshot_t` + `NODE_FLAG_*`, calls `zb_node_publish()`.
* **DoD:** 30-minute run with valid live readings (no `NaN`, not stuck at `0`). Test lighter near Flame sensor and record whether `flame_mv` increases or decreases in `STATE.md` to lock `FLAME_AO_INVERTED`.

#### **`S1-3` — Gateway Coordinator & UART1 Bridge (`gw_h2`)**
* **Scope:**
  * `zb_coord.h/.c`: Form Zigbee network on Channel `25`, PAN `0x1A62`; open join window (`JOIN_WINDOW_S`) on boot; handle `DEVICE_ANNCE` / `LEAVE` (map `short_addr` $\leftrightarrow$ 16-hex `ieee`); push attribute reports from `ESP_ZB_CORE_REPORT_ATTR_CB_ID` to FreeRTOS queue `q_zb_evt` (never write UART inside Zigbee callback).
  * `uart_bridge.h/.c`: Map `(EP10–EP15, cluster, attr)` into `rpt_t`; emit **exactly one** `proto_enc_rpt()` over `UART1` (`TX=GPIO10, RX=GPIO11, 115200 8N1`) when `seq` arrives or after `300 ms` coalesce timeout; emit `proto_enc_hello()` on boot and `proto_enc_hb()` every `2000 ms`; parse incoming `ping` / `permit_join` via `line_asm_feed()` + `proto_dec()`.
* **DoD:** 1-hour run with Node. Exactly 1 complete `rpt` NDJSON line per Node cycle, monotonically increasing `seq`, loss $\le 3\%$, zero console log bleed onto `UART1` (`R7`).

#### **`S1-4` — Gateway Host UART2 Link & CSV Data Logger (`gw_host` + `tools/log_csv.py`)**
* **Scope:**
  * Update `gw_host/main/config.h`: change `BUZZER_GPIO` to `4`, drop `PWR_SENSE`.
  * `uart_link.h/.c`: Read `UART2` (`RX=GPIO16, TX=GPIO17, 115200 8N1`), run `line_asm_feed()` + `proto_dec()`, push `rpt_t` to `q_rpt`. Monitor `hb` ($6\text{ s}$ timeout) and minimal node table (`NODE_TIMEOUT_MS = 30000` $\rightarrow$ `NODE_OFFLINE`). Print raw `CSV,...` line (§6.1) to USB console UART0.
  * `tools/log_csv.py`: PC serial recorder capturing `CSV,` lines into `data/raw/<run_id>.csv` with non-blocking labeling hotkeys (§6.1).
* **DoD:** Record 30 minutes to `data/raw/*.csv` via `log_csv.py`; continuous `seq`; loads cleanly in `pandas`. **Real sensor data collection can begin immediately after this task.**

---

### Stage 2 — `lib_fusion` Core & Experimental Calibration (CORE FOCUS: Week 2 $\rightarrow$ Week 3)

#### **`S2-1` — Layer 1: Feature Extraction (`fusion_features.h/.c`)**
* **Scope:** Implement `Med3`, `MA`, overflow-safe OLS linear regression $dT/dt$ (window $60\text{ s}$, min $3$ samples & $20\text{ s}$ span), adaptive baseline ($120\text{ s}$ init average + EMA $\tau = 600\text{ s}$, **frozen whenever `state != ST_NORMAL`**), channel validity masking, and $[0, 1]$ normalization per §5.2.
* **DoD (`tests/host/test_features.c` via `ctest`):** $10\ ^\circ\text{C}/\text{min}$ ramp yields $10.0 \pm 0.5$; $<3$ samples or $<20\text{ s}$ span returns invalid; `uint32_t` timestamp wraparound passes; baseline freezes when `state != ST_NORMAL`; invalid channels clear valid bit and output $0$; normalized outputs clamp to $[0, 1]$.

#### **`S2-2` — Layer 2 (Sugeno Fuzzy Engine) & Layer 3 (Decision Supervisor)**
* **Scope:** Implement `fusion_infer.h`, `infer_fuzzy.c`, `fusion_decide.h/.c`, `fusion_params.h`, and facade `fusion.h` per §5.3–§5.4. Include `tests/host/test_fuzzy.c` and `tests/host/test_decide.c`.
* **DoD (`ctest` 100% PASS):**
  * All 3 hand-calculated vectors (§5.3: `VD A = 80.0`, `VD B = 0.0`, `VD C = 30.0`) match within $\pm 0.5$.
  * Steam stays at `ST_NORMAL`/`ST_CAUTION`; hairdryer caps at `ST_CAUTION`; smoke + gas + warming triggers `ST_FIRE`.
  * Oscillating score around threshold triggers $\le 1$ transition (hysteresis + `N_UP`/`N_DOWN`); `ST_FIRE` stays latched until `risk < 25` for $60\text{ s}$; $T \ge 65^\circ\text{C} \times 2$ overrides to `ST_FIRE`.
  * Engine-swap test confirms replacing `INFER_FUZZY` with a mock `infer_engine_t` works without modifying Layer 1 or Layer 3.

#### **`S2-3` — Dataset Collection, Offline Replay & Parameter Tuning**
* **Scope:**
  * Collect $\ge 20$ labeled runs per §7.2 using `tools/log_csv.py`.
  * Implement `tests/host/fusion_replay.c` (reads raw CSV $\rightarrow$ runs `lib_fusion` $\rightarrow$ outputs predicted `risk, state` CSV) and `tools/eval.py` (computes §7.3 metrics comparing Fuzzy Logic against 3 fair single-sensor baselines `B1/B2/B3`).
  * Tune and freeze `fusion_params.h` using **run-level** train/validation split (never sample-level split).
* **DoD:** Quantitative comparison table (Fuzzy vs. `B1`, `B2`, `B3`) generated; `fusion_params.h` frozen with source CSV filenames recorded in comments.

#### **`(Optional) S2-4` — Pluggable ML Engine (`infer_tree.c`)**
* **Scope:** Write `tools/train_tree.py` (scikit-learn shallow Decision Tree trained on `data/raw/*.csv`) exporting static C `if/else` code into `firmware/lib_fusion/infer_tree.c` implementing `extern const infer_engine_t INFER_TREE;` returning `risk = P(fire) * 100.0f`.
* **DoD:** Drop-in swap via `fusion_init(..., &INFER_TREE)` requires zero changes outside `lib_fusion`; `tools/eval.py` compares Fuzzy vs. Decision Tree on held-out runs.

---

### Stage 3 — Full System Integration & Thesis Closeout (Week 3 $\rightarrow$ Week 4)

#### **`S3-1` — Gateway Host Integration (`fusion_task` + `alarm_out`)**
* **Scope:**
  * `fusion_task.h/.c`: Wait on `q_rpt` (timeout $1000\text{ ms}$) $\rightarrow$ `fusion_update()` + `fusion_tick()` $\rightarrow$ update system state, print full `CSV,...` line, forward telemetry/alerts to MQTT queue.
  * `alarm_out.h/.c`: `50 ms` `vTaskDelayUntil /* periodic */` task driving BKGR RGB LED (`GPIO25/26/27` via LEDC), Active Buzzer (`GPIO4`), and Mute Button (`GPIO32`) per §6.2. First line of `app_main()` turns Buzzer and LED OFF (`R7`).
* **DoD:** Live HIL bench test with Node using hairdryer, steam, and incense stick. LED/Buzzer reflect exact state within $\le 1\text{ s}$; press-and-release mutes both Buzzer and LED cleanly; zero buzzer chirp on MCU reset.

#### **`S3-2` — Wi-Fi, HiveMQ MQTT TLS & Node-RED Dashboard**
* **Scope:**
  * `net_mqtt.h/.c`: Wi-Fi STA (`secrets.h`), SNTP epoch time, `esp_mqtt_client` over TLS (`mqtts://MQTT_HOST:8883` using `esp_crt_bundle_attach`, relying on ESP-IDF built-in auto-reconnect). Publish JSON to `sensor/<node>` on every report (`node_id, temp, hum, eco2, tvoc, mq2_mv, flame_mv, risk, state, ts`) and `alert/<node>` on state transitions.
  * Update `cloud/Node_RED_DATN.json` to parse the new JSON fields and display live charts + Fuzzy risk score + alarm banner.
* **DoD:** Node-RED Dashboard updates live; Wi-Fi disconnect/reconnect recovers automatically; local `lib_fusion`, LED, and Buzzer continue operating normally while offline. *(Ensure fresh Wi-Fi/HiveMQ credentials per Risk `K6`.)*

#### **`S3-3` — Held-Out Final Experiments, Demo Video & Thesis Report**
* **Scope:** Run complete end-to-end tests on **unseen validation runs** (not used during `S2-3` tuning), measure Node $\rightarrow$ Node-RED latency, film demo video, compile final metrics (G0 network stats + §7.3 fusion tables + Limitations section), and tag `v2.0.0`.
* **DoD:** Final comparison tables, confusion matrix, demo video, and `v2.0.0` Git tag complete.

**Suggested 4-Week Schedule:** Week 1: `S1-1`, `S1-2`, `S2-1` $\cdot$ Week 2: `S1-3`, `S1-4`, `S2-2`, start CSV collection $\cdot$ Week 3: `S2-3`, `S3-1`, `S3-2` $\cdot$ Week 4: `S3-3`, optional `S2-4`, buffer. *(If Stage 1 slips, reduce run count in `S2-3` slightly — never cut the quantitative evaluation.)*

---

## 5. Detailed `lib_fusion` Specification (Pure C99, PC-Testable)

### 5.1 Decoupled 3-Layer Architecture

```text
rpt_t ──► [1. FEATURES] ──► feat_t ──► [2. INFER ENGINE] ──► risk [0..100] ──► [3. DECIDE] ──► fstate_t
          (Med3, MA, OLS,              (INFER_FUZZY default;                   (Hysteresis,
           adaptive base,               swappable with                          N_UP/N_DOWN,
           [0,1] scaling)               INFER_TREE / TinyML)                    Latch, 65 C guard)
```

Only Layer 2 changes when upgrading from Fuzzy Logic to Machine Learning. Public facade (`firmware/lib_fusion/fusion.h`):

```c
typedef enum { ST_NORMAL = 0, ST_CAUTION, ST_WARNING, ST_FIRE } fstate_t;

typedef struct {
    uint32_t t_ms;
    float    temp, hum, eco2, tvoc, mq2_mv, flame_mv;
    uint16_t flags; /* NODE_FLAG_* */
} fusion_in_t;

typedef struct {
    fstate_t state;
    float    risk;         /* 0.0 .. 100.0 */
    feat_t   feat;         /* Exposed for CSV logging & ML training */
    bool     warmup;
    bool     insufficient;
    uint32_t fired;        /* Bitmask of triggered fuzzy rules / safety overrides */
} fusion_out_t;

void fusion_init(fusion_t *f, const fusion_params_t *p, const infer_engine_t *eng);
void fusion_update(fusion_t *f, const fusion_in_t *in, fusion_out_t *out); /* Call every report (2 s) */
void fusion_tick(fusion_t *f, uint32_t now_ms);                            /* Call every 1 s (latch)  */
```

### 5.2 Layer 1 — Feature Extractor (`fusion_features.h/.c`)

```c
enum { F_T_D = 0, F_T_RATE, F_HUM_D, F_SMOKE, F_ECO2_D, F_TVOC_D, F_FLAME, F_COUNT };

typedef struct {
    float   x[F_COUNT]; /* Physical delta / rate units */
    float   n[F_COUNT]; /* Clamped normalized [0.0 .. 1.0] */
    uint8_t valid;      /* Bit i = 1 if feature i is valid */
} feat_t;
```

| Feature | Computation (Placeholder defaults; tuned in `S2-3`) | Norm `lo` $\rightarrow$ `hi` |
|---|---|---|
| `F_T_D` | $T_{\text{filt}} - T_{\text{base}}$ (Med3 $\rightarrow$ EMA) | $2 \rightarrow 20\ ^\circ\text{C}$ |
| `F_T_RATE` | OLS linear regression over $60\text{ s}$ window ($\ge 3$ samples, $\ge 20\text{ s}$ span), $^\circ\text{C}/\text{min}$ | $1 \rightarrow 10\ ^\circ\text{C}/\text{min}$ |
| `F_HUM_D` | $\max(0,\ RH - RH_{\text{base}})$ (Steam raises RH; fire lowers RH) | $2 \rightarrow 15\ \%\text{RH}$ |
| `F_SMOKE` | $(V_{\text{mq2,MA5}} - V_{\text{mq2,base}}) / V_{\text{mq2,base}}$ | $0.1 \rightarrow 0.8$ |
| `F_ECO2_D` / `F_TVOC_D` | $\text{MA3} - \text{base}$ | $100 \rightarrow 800\text{ ppm}$ / $30 \rightarrow 300\text{ ppb}$ |
| `F_FLAME` | $(V_{\text{fl,base}} - V_{\text{fl,MA5}}) / V_{\text{fl,base}}$ (flip sign if `FLAME_AO_INVERTED`); `NODE_FLAG_FLAME_DO` forces `n = 1.0` | $0.1 \rightarrow 0.5$ |

* **Adaptive Baseline:** Average first `BASE_INIT_S = 120 s`, then track via slow EMA (`BASE_TAU_S = 600 s`). **Update ONLY when `state == ST_NORMAL`** (freeze immediately on any elevation).
* **Warmup & Missing Channels:** If `NODE_FLAG_MQ2_WARM` or `NODE_FLAG_GAS_BAD` is set, or baseline is uninitialized $\rightarrow$ set `warmup = true`, clear corresponding `valid` bits, hold `ST_NORMAL`. If `F_T_D` is invalid OR all three (`F_SMOKE`, `F_ECO2_D`, `F_TVOC_D`) are invalid $\rightarrow$ `insufficient = true` and hold current state.
* **Derived Gas Linguistic Input:** $G = \max(n[\text{ECO2\_D}],\ n[\text{TVOC\_D}])$ (since CCS811 estimates $\text{eCO}_2$ directly from VOCs; document this correlation in thesis).

### 5.3 Layer 2 — Pluggable Inference Engine (`fusion_infer.h`, `infer_fuzzy.c`)

```c
typedef struct {
    float    risk;  /* 0.0 .. 100.0 */
    uint32_t fired; /* Bitmask of rules with firing strength > 0 */
} infer_out_t;

typedef struct infer_engine {
    const char *name;
    void  (*reset)(void *ctx);
    float (*infer)(void *ctx, const feat_t *f, infer_out_t *out);
    void  *ctx;
} infer_engine_t;

extern const infer_engine_t INFER_FUZZY;
/* extern const infer_engine_t INFER_TREE;  -- Optional S2-4 ML engine */
```

**Zero-Order Sugeno Fuzzy Logic Specification (`infer_fuzzy.c`):**
* **Normalized Inputs ($\in [0, 1]$):** $T = n[\text{T\_D}]$, $R = n[\text{T\_RATE}]$, $S = n[\text{SMOKE}]$, $G = \max(n[\text{ECO2\_D}], n[\text{TVOC\_D}])$, $F = n[\text{FLAME}]$, $H = n[\text{HUM\_D}]$.
* **Trapezoidal Membership Functions $(a, b, c, d)$:**
  * `L` (Low) $= (0.0,\ 0.0,\ 0.2,\ 0.4)$
  * `M` (Medium) $= (0.2,\ 0.4,\ 0.6,\ 0.8)$
  * `H` (High) $= (0.6,\ 0.8,\ 1.0,\ 1.0)$
  * `NL` (Not Low) $= \max(\mu_M, \mu_H)$
* **Operators & Singletons:** `AND` $= \min$. Output singletons: `SAFE` $= 0$, `LOW` $= 30$, `MED` $= 60$, `HIGH` $= 90$.
* **Defuzzification:**
  $$\text{risk} = \begin{cases} \dfrac{\sum_i w_i z_i}{\sum_i w_i} & \text{if } \sum_i w_i \ge 10^{-3} \\ 0 & \text{otherwise} \end{cases}$$

**Initial 15-Rule Base (stored as constant table in `fusion_params.h`, tuned in `S2-3`):**

| Rule | Antecedent | Consequent | Physical Meaning |
|---|---|---|---|
| `1a` | $F=\text{H} \land T=\text{H}$ | `HIGH` ($90$) | Flame + High heat |
| `1b` | $F=\text{H} \land S=\text{H}$ | `HIGH` ($90$) | Flame + Heavy smoke |
| `1c` | $F=\text{H} \land G=\text{H}$ | `HIGH` ($90$) | Flame + High combustion gas |
| `2` | $T=\text{H} \land R=\text{H} \land S=\text{NL}$ | `HIGH` ($90$) | Rapid heating with smoke |
| `2b` | $T=\text{H} \land R=\text{H} \land S=\text{L} \land G=\text{L}$ | `LOW` ($30$) | Rapid heating, clean air (**Hairdryer rejection**) |
| `3` | $S=\text{H} \land G=\text{H}$ | `HIGH` ($90$) | Heavy smoke + High gas (Smoldering fire) |
| `4` | $S=\text{H} \land T=\text{M}$ | `HIGH` ($90$) | Heavy smoke + Warming |
| `5` | $S=\text{M} \land G=\text{M}$ | `MED` ($60$) | Early smoldering cues |
| `6` | $T=\text{M} \land R=\text{M}$ | `MED` ($60$) | Steady temperature buildup |
| `7` | $T=\text{H} \land S=\text{L} \land G=\text{L}$ | `LOW` ($30$) | Static heat (Heater / hot room) |
| `8` | $S=\text{H} \land G=\text{L} \land T=\text{L}$ | `LOW` ($30$) | MQ-2 spike alone (Cooking / solvent) |
| `9` | $G=\text{H} \land S=\text{L} \land T=\text{L}$ | `LOW` ($30$) | VOC spike alone (Perfume / aerosol) |
| `10` | $F=\text{H} \land T=\text{L} \land S=\text{L} \land G=\text{L}$ | `LOW` ($30$) | Flame alone (**Sunlight / lamp rejection**) |
| `11` | $R=\text{H} \land H=\text{H} \land S=\text{L}$ | `SAFE` ($0$) | **Steam rejection** (Temp + Humidity rise, no smoke) |
| `12` | $T=\text{L} \land S=\text{L} \land G=\text{L} \land F=\text{L}$ | `SAFE` ($0$) | Ambient normal |

**Mandatory Hand-Calculated Unit Test Vectors (`tests/host/test_fuzzy.c`, tolerance $\pm 0.5$):**

| Vector | Normalized Input $(T, R, S, G, F, H)$ | Firing Rules $(w_i \rightarrow z_i)$ | Expected `risk` |
|---|---|---|---|
| **VD A** (Smoldering) | $(0.3,\ 0,\ 0.7,\ 0.7,\ 0,\ 0)$ | `#3` $(0.5 \rightarrow 90)$, `#4` $(0.5 \rightarrow 90)$, `#5` $(0.5 \rightarrow 60)$ | $(45 + 45 + 30) / 1.5 =$ **`80.0`** |
| **VD B** (Steam) | $(0.5,\ 0.9,\ 0,\ 0,\ 0,\ 0.9)$ | `#11` $(1.0 \rightarrow 0)$ only | **`0.0`** |
| **VD C** (Hairdryer) | $(0.8,\ 0.9,\ 0,\ 0,\ 0,\ 0)$ | `#2b` $(1.0 \rightarrow 30)$, `#7` $(1.0 \rightarrow 30)$ | $(30 + 30) / 2.0 =$ **`30.0`** |

### 5.4 Layer 3 — Decision Supervisor (`fusion_decide.h/.c`, Engine-Agnostic)
* **Escalation Thresholds:** `CAUTION` $\ge 25$, `WARNING` $\ge 50$, `FIRE` $\ge 75$.
* **Hysteresis (De-escalation):** Threshold $-$ `10` (`15 / 40 / 65`).
* **Confirmation Counters:** `N_UP = 2` consecutive reports to escalate; `N_DOWN = 5` consecutive reports to step down **one level at a time**.
* **FIRE Latch:** Hold `ST_FIRE` until `risk < 25` continuously for `LATCH_CLEAR_S = 60 s` (tracked via `fusion_tick()`).
* **Hard Safety Net (Engine-Independent):** Raw `temp >= T_HARD_C` ($65^\circ\text{C}$) for `2` consecutive reports $\rightarrow$ immediate `ST_FIRE`.

---

## 6. CSV Telemetry Format & Local Alarm Behavior

### 6.1 Gateway Serial CSV Format (`gw_host` $\rightarrow$ `tools/log_csv.py`)
`gw_host` prints one prefixed CSV line per report on USB console UART0:
```text
CSV,t_ms,node,seq,temp,hum,eco2,tvoc,mq2_mv,flame_mv,flags,rssi,lqi,n_T,n_R,n_S,n_G,n_F,n_H,risk,state
```
`tools/log_csv.py --port COMx` filters lines starting with `CSV,`, appends `pc_time,run_id,label,event`, and writes `data/raw/<run_id>.csv`.  
**Non-blocking Keyboard Hotkeys:** `n` = normal/ambient, `f` = fire, `u` = nuisance/false-alarm, `i` = mark **ignition start ($t_{\text{ignite}}$)**, `e` = end event.

### 6.2 Local Alarm Specification (`alarm_out.h/.c`, 50 ms `vTaskDelayUntil`)

| System State | RGB LED (`25=R, 26=G, 27=B`) | Active Buzzer (`GPIO4`) |
|---|---|---|
| `WARMUP` | Solid Blue | OFF |
| `NORMAL` | Solid Green | OFF |
| `CAUTION` | Solid Yellow (`R+G`) | OFF |
| `WARNING` | Blinking Yellow ($2\text{ Hz}$) | Short beep every $5\text{ s}$ |
| `FIRE` | Fast Blinking Red ($4\text{ Hz}$) | $0.5\text{ s}$ ON / $0.5\text{ s}$ OFF |
| `NODE_OFFLINE` | Blinking Blue ($1\text{ Hz}$) | OFF |

* **Mute Button (`GPIO32`, `INPUT_PULLUP`, $30\text{ ms}$ debounce, action on release):** Mutes **both Buzzer and RGB LED** for `MUTE_S = 300 s`. Automatically un-mutes when `MUTE_S` expires, when a second press occurs, OR **immediately if state escalates above the level at mute time** (prevents hiding a worsening fire).
* **Safe Boot (`R7`):** Initialize `GPIO4` LOW and LEDC duties to `0` at line 1 of `app_main()`. Zero chirp on reset.

---

## 7. Thesis Experimental & Evaluation Protocol

### 7.1 Research Question & Definitions
* **Question:** Does multi-sensor Sugeno Fuzzy Logic (combining $T, dT/dt, \text{RH}, \text{eCO}_2/\text{TVOC}, \text{MQ-2}, \text{Flame}$) detect early fires **faster** and with **fewer false alarms** than fixed single-sensor thresholds?
* **Detection Time:** $t_{\text{detect}} = t(\text{state} \ge \text{WARNING}) - t_{\text{ignite}}$ (marked by hotkey `i`). Do not claim "prediction before ignition" — measure strictly from ignition.
* **False Alarm Definition:** Any run reaching `state >= WARNING` during a nuisance (`u`) or ambient (`n`) session.

### 7.2 Experimental Runs ($\ge 20$ runs; $\sim 10\text{ min/run}$: $3\text{ min}$ baseline $\rightarrow$ stimulus $\rightarrow$ $5\text{ min}$ recovery)
Conduct in a ventilated area with fireproof tray, water/extinguisher, minimal fuel, adult supervision, fixed measured Node distance, and full sensor warmup (`BENCH_MODE = 0`).

| Group | Scenario | Min Runs | Label |
|---|---|---|---|
| Smoldering Fire | Incense stick / smoldering paper | 5 | `f` |
| Flaming Fire | Small paper flame / alcohol lamp | 4 | `f` |
| Nuisance | Steam (kettle / hot soup) | 3 | `u` |
| Nuisance | Hairdryer hot air | 3 | `u` |
| Nuisance | Aerosol spray / perfume | 3 | `u` |
| Nuisance | Incandescent bulb / sunlight on Flame sensor | 2 | `u` |
| Ambient | Quiet room baseline ($\ge 2\text{ h}$ total) | — | `n` |

### 7.3 Evaluation Metrics & Fair Baselines (`tools/eval.py`)
* **Targets:** Detection Rate (Recall) $\ge 80\%$; False Alarms $= 0$ (or $\le 1$ per $2\text{ h}$ ambient); Median $t_{\text{detect}} \le$ best baseline at equal false-alarm constraint; per-scenario Confusion Matrix.
* **Offline Baselines (evaluated on identical CSVs):** `B1` MQ-2 single threshold; `B2` Temperature single threshold; `B3` Logical OR of single thresholds. Select baseline thresholds for zero false alarms on the tuning split for fair comparison.
* **Data Splitting:** Split train vs. validation **strictly by whole run (`run_id`)** (or Leave-One-Run-Out) to prevent temporal leakage. If Fuzzy underperforms a baseline on any scenario, report honestly with root-cause analysis.

---

## 8. Key Risks & Mitigations

| ID | Risk | Mitigation |
|---|---|---|
| `K1` | No prior dataset (initial fuzzy rules are domain estimates) | Start logging right after `S1-4`; gate parameter tuning on $\ge 12$ real runs. |
| `K2` | Gateway `3x AA` ($4.5\text{ V}$ fresh, $\sim 3.9\text{ V}$ depleted) minus Schottky drop ($\sim 4.2\text{ V}$) is near DevKit LDO brownout during Wi-Fi TX peaks | Bench-test under live Wi-Fi load; use low-$V_f$ Schottky diodes; keep 5V adapter primary for demo and document voltage headroom under Limitations. |
| `K3` | Muting both LED & Buzzer could obscure escalating fire | Auto-unmute on any state escalation or after `300 s` (§6.2). |
| `K4` | Fixed $2\text{ s}$ cadence increases Zigbee traffic vs. G0 | Measure loss in `S1-1`; if $> 3\%$, relax `REPORT_MS` to `3000`. |
| `K5` | MQ-2 drift/humidity sensitivity; CCS811 eCO2 derived from VOC; Flame IR false triggers | Enforce full warmup, include steam/perfume/light nuisance runs, document physics in thesis. |
| `K6` | **Credential Leak:** Reference files (`esp_ot_br.c`, `Node_RED_DATN.json`) contained plaintext Wi-Fi/HiveMQ passwords | Rotate Wi-Fi/HiveMQ passwords before deployment; keep all secrets strictly in git-ignored `secrets.h`. |
| `K7` | Legacy doc conflicts (`GatewayPlan.md` / `GW_SCHEMATIC_SPEC.md` still list GPIO33, PWR_SENSE, prealert) | This `DEV_CODING_PLAN.md` v2.0 Final is the authoritative Single Source of Truth. |
| `K8` | Fire testing safety | Minimal fuel, fireproof tray, ventilation, extinguisher/water on hand, supervision. |
| `K9` | Agent hallucinating registers or ESP-IDF APIs | Paste §4 `S1-2` register bytes directly in prompts; verify new ESP-IDF/Zigbee headers and log in `docs/API_NOTES.md`. |

---

## 9. Appendix: OpenCode Prompt Template & `AGENTS.md`

### 9.1 Compact Task Prompt Template (DeepSeek on OpenCode)
```markdown
TASK: <ID> — <Short Title>
GOAL: <1-2 sentences>
CONTEXT (Read-only): AGENTS.md, STATE.md, docs/DEV_CODING_PLAN.md §<sec>, <relevant headers>.
FILES TO CREATE/EDIT (Strictly limited to): <explicit file list>.
INTERFACE (Keep verbatim): <function signatures from §1.3 / §5>.
DATASHEET / CONSTANTS: <register bytes or pin constants if hardware task>.
CONSTRAINTS: Follow Rules R1–R7 (§2). Pack new modules into a single .c/.h pair; minimal code meeting DoD; no speculative wrappers or extra files.
VERIFICATION (Run and paste REAL output): <build / ctest command> + observable DoD. Update STATE.md (4 lines).
OUT OF SCOPE: <explicit exclusions>.
```

