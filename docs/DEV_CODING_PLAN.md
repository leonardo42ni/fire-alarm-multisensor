# DEV_CODING_PLAN.md — Firmware & Software Implementation Plan (v2.0 Thesis MVP)

**System:** Wireless Multi-Sensor Early Fire Detection System for Smart Home Ecosystems  
**Pipeline:** Node (`ESP32-H2`) $\rightarrow$ Zigbee 3.0 $\rightarrow$ Gateway (`ESP32-H2` Coordinator + `ESP32 WROOM-32` Host) $\rightarrow$ Edge Data Fusion (`lib_fusion`) $\rightarrow$ Local Alarm + MQTT/TLS (HiveMQ Cloud) $\rightarrow$ Web Dashboard & Telegram Bot.

| Field | Value |
|---|---|
| **Version** | `2.0` (Thesis MVP — Consolidated & Focused on Data Fusion) |
| **Remaining Timeline** | 3–4 Weeks |
| **Target Scope** | Graduation Thesis (Happy-Path stability + Rigorous Data Fusion evaluation) |
| **Primary Algorithm** | **Fuzzy Logic** (with pluggable ML/AI-ready architecture & CSV data logging) |
| **Toolchain** | ESP-IDF `v5.5.5` (`esp32h2`, `esp32`), `esp-zigbee-lib 1.6.8`, CMake/CTest (PC Host) |
| **Executor** | Human Engineer + AI Coding Agent (DeepSeek on OpenCode) |

---

## 0. Agent Workflow & Lean Coding Discipline

### 0.1 Per-Task Execution Loop
1. **Branch:** `git switch -c feat/<TaskID>-<short-name>`
2. **Context Loading:** Provide ONLY `AGENTS.md`, `STATE.md`, the relevant `§5.x` subsection of this file, and existing target headers.
3. **Implementation:** Write clean, flat C99/C11 code. Avoid over-abstraction, redundant wrappers, and unnecessary file splitting.
4. **Build & Test Gate:** Run `ctest --test-dir build_host` (for PC-testable logic) and `.\tools\build_all.ps1` (for all 3 ESP-IDF firmwares). Paste real command output; never claim completion without passing builds.
5. **Review & Merge:** Verify against `§1.1` rules, update `STATE.md` and `docs/API_NOTES.md`, commit, and merge.

### 0.2 Mandatory Stop Rules (Ask Human Before Proceeding)
- An ESP-IDF or Zigbee API is **not found** in local installed headers (`docs/API_NOTES.md` rule `H10`).
- A hardware pin differs from the verified pinout table in `§3.2`.
- A unit test fails and the only apparent fix is relaxing thresholds or deleting assertions.
- Any change is needed to frozen contracts (`proto_ndjson.h`, ZCL Endpoints `EP10–EP15`, or `zb_node.h`).

---

## 1. Core Engineering Rules & Simplified Architecture

### 1.1 Hard Rules (H1–H10)
| ID | Rule | Verification |
|---|---|---|
| **H1** | **No blocking `delay()` / long `vTaskDelay()`** in logic paths. Periodic loops use `vTaskDelayUntil` tagged `/* periodic */` or queue timeouts. | Grep check |
| **H2** | **Watchdog (TWDT):** Core firmware loops call `esp_task_wdt_reset()`. | Code review |
| **H3** | **ISR Safety:** No heap allocation (`malloc`), no `ESP_LOGx`, no blocking calls inside ISRs. Use `...FromISR` only. | Code review |
| **H4** | **Buffer Safety:** Zero usage of `sprintf`, `strcpy`, `strcat`, `gets`. Always use bounded `snprintf` / length checks. | Grep = 0 |
| **H5** | **Explicit Error Handling:** Check returned `esp_err_t` / `proto_err_t`. Do not silently swallow errors. | Code review |
| **H6** | **No Secrets in Git:** `secrets.h` and `.env` must remain git-ignored; keep `#error` guard in `gw_host/main/config.h`. | `git status` |
| **H7** | **No Magic Numbers:** All GPIOs, intervals, dividers, and thresholds live in `config.h`, `shared_config.h`, or `fusion_params.h`. | Code review |
| **H8** | **Fail-Safe Boot:** On `gw_host`, set Buzzer (`GPIO4`) and RGB LED (`GPIO25/26/27`) to **OFF** as the very first step in `app_main()`. | Code review |
| **H9** | **Clean UART Separation & Lean Files:** `GW-H2` logs go to USB-Serial-JTAG; `UART1`/`UART2` carry **only** NDJSON. Keep each `.c`/`.h` file $\le 250$ lines. | Line count & monitor |
| **H10** | **Zero Hallucinated APIs:** Verify every newly used ESP-IDF/Zigbee API in installed headers and log its exact signature in `docs/API_NOTES.md`. | `API_NOTES.md` diff |

### 1.2 Simplified FreeRTOS Concurrency Model
- **`node_h2` (ESP32-H2):**
  - `zb_node` task (Stack `8192`, Prio `5`) — Zigbee End Device stack (`T-202`, already built).
  - `sensor_report` task (Stack `4096`, Prio `4`, TWDT) — Samples sensors every `1000 ms`, computes local pre-alert flag, publishes Zigbee snapshot every `10 s` (normal) or `3 s` (pre-alert).
- **`gw_h2` (ESP32-H2 Coordinator):**
  - `zb_coord` task (Stack `8192`, Prio `5`) — Forms Zigbee network on Channel 25, PAN `0x1A62`; pushes ZCL attribute events into `q_zb_evt` (never writes UART inside Zigbee callbacks).
  - `uart_bridge` task (Stack `4096`, Prio `4`, TWDT) — Drains `q_zb_evt`, coalesces attributes (`300 ms`), emits NDJSON over `UART1`, sends heartbeat (`hb`) every `2 s`.
- **`gw_host` (ESP32 WROOM-32):**
  - `alarm_task` (Stack `3072`, Prio `7`, `50 ms` periodic, TWDT) — Drives RGB LED, Active Buzzer, and polls Mute button (`GPIO32`). **Never blocks on Wi-Fi/MQTT.**
  - `uart_rx` task (Stack `4096`, Prio `6`, TWDT) — Reads `UART2`, feeds `line_asm_feed` + `proto_dec`, pushes valid `rpt_t` to `q_rpt`.
  - `fusion_task` (Stack `4096`, Prio `5`, TWDT) — Pops `q_rpt`, runs `lib_fusion` (Fuzzy Logic), updates system state, prints CSV log line to USB console, pushes telemetry/alerts to MQTT queue.
  - `mqtt_task` (Stack `6144`, Prio `4`, No TWDT) — Publishes JSON payloads to HiveMQ Cloud over TLS when Wi-Fi/MQTT is up.

---

## 2. Frozen Baseline & Scope Reduction Matrix

### 2.1 Already Completed (DO NOT Break or Rewrite)
| Item | Status | Frozen Artifacts |
|---|---|---|
| **Phase 0 (`T-001`–`T-006`)** | **DONE & Tagged (`phase-0-done`)** | ESP-IDF `v5.5.5`, repo skeleton, `build_all.ps1`, `tests/host/` CMake+CTest, Zigbee G0 passed (Ch 25, PAN `0x1A62`, PDR $\ge 99\%$, rejoin $\le 60\text{ s}$). |
| **`T-201` (`proto_ndjson`)** | **DONE & Committed** | `firmware/common/proto_ndjson.h` + implementation + 5/5 CTest & ASAN/UBSAN passing. Contract is **frozen**. |
| **`T-202` (`zb_node`)** | **Code & Build DONE (Awaiting HIL)** | `firmware/node_h2/components/zb_node/` (`zb_node_start`, `zb_node_joined`, `zb_node_publish`, `node_snapshot_t`, EP10–EP15 + `seq` on EP15 `0x0100`). Currently feeds `SPIKE_*` dummy values until Stage 2. |

### 2.2 Explicit Scope Cuts (Removed from v0.1)
To finish comfortably within 3–4 weeks while maximizing depth in Data Fusion, the following v0.1 items are **permanently dropped**:
- **Removed:** Separate `firmware/bringup/` test applications (`bu_*`) and `tools/serial_check.py`. Drivers are verified directly inside `node_h2` / `gw_host` via `idf.py monitor`.
- **Removed:** Software mains/battery power monitoring (`PWR_SENSE`, `GPIO35`, `power_mon`, `T-114`, `T-601`, `T-606`). Dual power on Gateway is passive hardware only.
- **Removed:** Fault-injection test suites (`T-602`, `T-FLT-*`), 24-hour commercial torture tests, and `node_cli` command shell.
- **Removed:** Multi-duration button hold state machine (`3s` reset / `5s` permit-join), LED/buzzer overlay matrices, and complex `MAINS_LOST` blink overlays.
- **Action Required in Stage 3:** Update `firmware/gw_host/main/config.h` (from `T-003`) to change `BUZZER_GPIO` from `33` to **`4`**, remove `PWR_SENSE` macros, and simplify button/alarm configs.

---

## 3. Repository Structure & Verified Hardware Pinout

### 3.1 Streamlined Repository Tree
```text
fire-alarm-multisensor/
├─ AGENTS.md                         # Updated lean agent rules (§7.2)
├─ STATE.md                          # Live project progress tracker
├─ docs/
│  ├─ DEV_CODING_PLAN.md             # This document (v2.0)
│  ├─ GatewayPlan.md                 # Reference protocol contract
│  ├─ API_NOTES.md                   # Verified ESP-IDF/Zigbee API signatures (H10)
│  └─ ENV.md                         # Local ESP-IDF v5.5.5 environment setup
├─ firmware/
│  ├─ common/                        # FROZEN: proto_ndjson.*, shared_config.h, error.h, version.h
│  ├─ lib_fusion/                    # CORE THESIS FOCUS (Pure C99, PC & ESP32 portable)
│  │  ├─ filters.h / filters.c       # MA, Med3, overflow-safe Linear Regression dT/dt
│  │  ├─ fusion_features.h / .c      # Feature extraction, baseline normalization, validity
│  │  ├─ fusion_engine.h             # Pluggable inference strategy interface (Fuzzy <-> ML/AI)
│  │  ├─ fusion_fuzzy.h / .c         # Fuzzy Logic engine (MF + Rule Base + Defuzzification)
│  │  ├─ fusion_ml_stub.h / .c       # Ready-to-implement ML/AI model adapter stub
│  │  ├─ fusion_params.h             # Tunable MF points, weights, thresholds, override limits
│  │  ├─ fusion.h / fusion.c         # 4-level State Machine, R1-R3 overrides, Latch, CSV format
│  │  └─ CMakeLists.txt
│  ├─ node_h2/                       # Target: esp32h2 (Continuous 5V Adapter)
│  │  ├─ main/                       # app_main.c, config.h, idf_component.yml
│  │  └─ components/
│  │     ├─ zb_node/                 # DONE (T-202): EP10-15 Zigbee End Device
│  │     └─ sensors/                 # NEW (Stage 2): Consolidated I2C (SHT31, Gas) + ADC (MQ2, Flame)
│  ├─ gw_h2/                         # Target: esp32h2 (Zigbee Coordinator + UART1 Bridge)
│  │  ├─ main/                       # app_main.c, config.h, idf_component.yml
│  │  └─ components/
│  │     ├─ zb_coord/                # Zigbee Coordinator + attribute callback
│  │     └─ uart_bridge/             # node_cache (300 ms coalesce) + NDJSON over UART1
│  └─ gw_host/                       # Target: esp32 WROOM-32 (Edge Fusion + Alarm + Cloud)
│     ├─ main/                       # app_main.c, config.h, secrets.example.h, secrets.h (git-ignored)
│     └─ components/
│        ├─ uart_link/               # UART2 NDJSON RX + simple node tracking
│        ├─ fusion_ctrl/             # Runs lib_fusion per node + emits CSV telemetry to Serial
│        ├─ alarm_out/               # RGB LED (25/26/27) + Active Buzzer (GPIO4) + Mute Button (GPIO32)
│        └─ net_cloud/               # Wi-Fi STA + HiveMQ MQTT TLS + JSON payload builder
├─ cloud/
│  ├─ dashboard/                     # Static HTML/JS + Chart.js + MQTT over WSS
│  └─ telegram_bot/                  # Lightweight Node.js alert bot (alert/# -> Telegram)
├─ tests/
│  ├─ host/                          # CMakeLists.txt, test_util.h, test_proto_*.c, test_fusion.c, fusion_replay.c
│  └─ vectors/                       # Synthetic & real experiment CSV datasets + benchmark outputs
└─ tools/
   ├─ build_all.ps1                  # Automated 3-firmware ESP-IDF build script
   └─ eval_fusion.py                 # Thesis evaluation script (Fuzzy Logic vs. Single-Sensor metrics)
```

### 3.2 Confirmed Hardware Architecture & Pinout (Overrides v0.1)

#### A. Sensor Node (`ESP32-H2 SuperMini`)
- **Power:** Continuous `5V` DC adapter (no battery).
  - `5V` rail powers `ESP32-H2` (`5V` pin), `MQ-2` (`VCC`, with $470\ \mu\text{F}$ decoupling capacitor close to pin), and `Flame Sensor` (`VCC`).
  - `3V3` rail (from ESP32-H2 onboard LDO) powers `SHT31` and `CCS811/ENS160`. **Never connect 5V to I2C sensors.**

| Subsystem | Signal / Pin | ESP32-H2 GPIO | Hardware Notes |
|---|---|---|---|
| **I2C Bus** | `SDA` / `SCL` | `GPIO10` / `GPIO11` | `100 kHz`; pull-ups already on breakout boards (`I2C_INTERNAL_PULLUP=0`). |
| **SHT31** | Addr `0x44` | `GPIO10`, `GPIO11` | `ADDR` $\rightarrow$ `GND`, `VIN` $\rightarrow$ `3V3`. |
| **Gas Sensor** | Addr `0x5A` (CCS811) or `0x53` (ENS160) | `GPIO10`, `GPIO11` | `VCC` $\rightarrow$ `3V3`. For CCS811: `nWAKE` $\rightarrow$ `GND`, `ADDR` $\rightarrow$ `GND`. |
| **MQ-2 Smoke** | `AO` (0–5V) | `GPIO1` (ADC1) | Voltage divider $22\text{ k}\Omega / 22\text{ k}\Omega$ (`ADC_DIV_RATIO = 2.0`) + $100\text{ nF}$ cap to `GND` (max pin voltage $\le 2.5\text{ V}$). |
| **Flame Sensor** | `AO` (0–5V) / `DO` (opt) | `GPIO4` (ADC1) / `GPIO14` | `AO` via $22\text{ k}\Omega / 22\text{ k}\Omega$ divider + $100\text{ nF}$ cap to `GND`. `DO` optional digital input. |
| **Console Log** | USB-Serial-JTAG | Onboard USB-C | `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y`. |

#### B. Gateway (`ESP32-H2` Coordinator + `ESP32 WROOM-32` Host)
- **Power:** Passive dual power (`5V` primary adapter + `3x AA` `4.5V` backup battery box via hardware diode OR-ing). **No software power-sense GPIO.**

| Subsystem | Signal / Pin | MCU & GPIO | Hardware Notes |
|---|---|---|---|
| **UART Bridge** | `TX` $\rightarrow$ `RX2` | `GW-H2 GPIO10` $\rightarrow$ `WROOM GPIO16` | `115200 8N1`, NDJSON only. Common `GND` required. Optional $220\ \Omega$ series resistors. |
| **UART Bridge** | `RX` $\leftarrow$ `TX2` | `GW-H2 GPIO11` $\leftarrow$ `WROOM GPIO17` | Reserved for optional commands (`permit_join` / `ping`). |
| **LED RGB (BKGR)** | `R` / `G` / `B` | `WROOM GPIO25` / `GPIO26` / `GPIO27` | Common-cathode (`K` $\rightarrow$ `GND`). Series resistors: `R` $220\ \Omega$, `G`/`B` $100\ \Omega$. Driven via LEDC PWM. |
| **Active Buzzer** | `SIG` (**Updated**) | **`WROOM GPIO4`** | Simple active buzzer (replaces `GPIO33` from v0.1). Driven LOW/OFF first in `app_main()`. |
| **Mute Button** | `BTN` | `WROOM GPIO32` | Pushbutton to `GND` (`GPIO_PULLUP_ENABLE`). **Press-and-release ONLY silences/turns off Buzzer and LED.** |

---

## 4. Consolidated 4-Stage Roadmap (3–4 Weeks)

All remaining work is consolidated into **4 focused stages (8 tasks total)** prioritized so the core thesis deliverable (`lib_fusion`) is completed and validated first.

| Stage & Target Week | Task ID | Scope & Deliverables | Dependencies | Definition of Done (DoD) |
|---|---|---|---|---|
| **Stage 1** *(Week 1 — PC First)* | **`S1-1`** | **`lib_fusion` Core Implementation:** `filters.c`, `fusion_features.c`, `fusion_engine.h`, `fusion_fuzzy.c`, `fusion_ml_stub.c`, `fusion_params.h`, `fusion.c`. | `T-004` | Pure C99, compiles with CMake on PC (`-Wall -Wextra`) and ESP-IDF (`.\tools\build_all.ps1`). Zero IDF dependencies inside `lib_fusion`. |
| **Stage 1** *(Week 1 — PC First)* | **`S1-2`** | **Fusion Verification & Benchmark Tooling:** `tests/host/test_fusion.c`, `tests/host/fusion_replay.c`, synthetic CSV vectors, and `tools/eval_fusion.py`. | `S1-1` | `ctest` + ASAN/UBSAN 100% PASS (fuzzy math, R1–R3 overrides, hysteresis, latch, pluggable engine swap test). `eval_fusion.py` outputs Fuzzy vs. Single-Sensor comparison metrics. |
| **Stage 2** *(Week 2 — Node & Zigbee)* | **`S2-1`** | **Consolidated Node Sensors & `T-202` Integration:** Single component `firmware/node_h2/components/sensors/` (`i2c_bus` + `SHT31` + `CCS811/ENS160` + `ADC MQ2/Flame` + local pre-alert) wired directly into `node_h2/main/app_main.c` replacing `SPIKE_*`. | `T-202` | **HIL:** Flash `node_h2`; monitor prints real live `T, RH, eCO2, TVOC, MQ2_mV, Flame_mV`; breathing/smoke/lighter visibly shifts readings; Zigbee sends real packets every `10 s` (`3 s` on pre-alert). |
| **Stage 2** *(Week 2 — Node & Zigbee)* | **`S2-2`** | **`gw_h2` Coordinator + UART1 NDJSON Bridge:** `zb_coord` + `node_cache` (`300 ms` coalesce) + `uart_bridge` emitting `proto_enc_rpt()` lines on `UART1` (`GPIO10/11`). | `T-201`, `S2-1` | **HIL:** 2 ESP32-H2 boards running; `gw_h2` receives Node reports and emits valid NDJSON `{"t":"rpt",...}` lines on `UART1` with incrementing `seq`. |
| **Stage 3** *(Week 3 — Gateway Host)* | **`S3-1`** | **`gw_host` Core Integration & Local Alarm:** Update `config.h` (`BUZZER_GPIO=4`, drop `PWR_SENSE`); implement `uart_link` (`UART2`), `fusion_ctrl` (runs `lib_fusion` + prints `CSV|...` log), and `alarm_out` (BKGR RGB `25/26/27`, Buzzer `GPIO4`, simple Mute button `GPIO32`). | `S1-1`, `S2-2` | **HIL:** Wire `GW-H2` $\leftrightarrow$ `WROOM-32`; end-to-end smoke/heat test transitions `NORMAL` $\rightarrow$ `CAUTION` $\rightarrow$ `WARNING` $\rightarrow$ `FIRE`, driving RGB + Buzzer; pressing `GPIO32` immediately turns off Buzzer & LED. |
| **Stage 4** *(Week 3–4 — Cloud)* | **`S4-1`** | **IoT Cloud & Remote Alerting:** `net_cloud` (Wi-Fi STA + HiveMQ TLS MQTT + JSON payloads) + `cloud/dashboard/` (WSS live charts) + `cloud/telegram_bot/` (alerts on `WARNING`/`FIRE`). | `S3-1` | **HIL:** Live sensor charts update on Web Dashboard within $\le 3\text{ s}$; `WARNING`/`FIRE` triggers Telegram message within $\le 10\text{ s}$; auto-reconnects if Wi-Fi drops. |
| **Stage 4** *(Week 4 — Thesis Data)* | **`S4-2`** | **Real-World Experiments & Thesis Evaluation:** Record CSV logs via Gateway Serial for 3 scenarios: (1) Smoldering paper/incense, (2) Open lighter flame, (3) Hairdryer/steam false-alarm test. Run `eval_fusion.py` to tune Fuzzy parameters and produce thesis tables/charts. | `S1-2`, `S4-1` | Recorded CSV logs saved in `tests/vectors/exp_*.csv`; quantitative table proving Fuzzy Logic reduces false alarms and detects smoldering fire earlier than fixed single-sensor thresholds. *(Optional bonus: train Decision Tree/Random Forest on collected CSV and plug into `fusion_ml_stub.c`).* |

---

## 5. Detailed Module & Interface Specifications

### 5.1 Frozen Interfaces (Reference Only — Do Not Modify)
- **`firmware/common/proto_ndjson.h` (`T-201`):** Use `rpt_t`, `proto_msg_t`, `proto_enc_rpt()`, `proto_enc_hb()`, `proto_enc_hello()`, `proto_dec()`, and `line_asm_feed()` as-is.
- **`firmware/node_h2/components/zb_node/include/zb_node.h` (`T-202`):** Use `node_snapshot_t`, `zb_node_start()`, `zb_node_joined()`, and `zb_node_publish()` as-is.
- **ZCL Endpoint Mapping (Frozen from G0/T-202):**
  - `EP10`: Temperature (`0x0402`, `int16` = $T \times 100$), Humidity (`0x0405`, `uint16` = $RH \times 100$).
  - `EP11`: `mq2_mv` (`0x000C` Analog Input, `float`).
  - `EP12`: `flame_mv` (`0x000C`, `float`).
  - `EP13`: `eco2` (`0x000C`, `float`).
  - `EP14`: `tvoc` (`0x000C`, `float`).
  - `EP15`: `flags` (`0x000C`, `float`) + `seq` (`0x0100` ApplicationType, `uint32`).

---

### 5.2 Core Thesis Module: `firmware/lib_fusion/` (Fuzzy Logic + ML-Ready Architecture)

`lib_fusion` is written in **strict C99** with zero dynamic allocation (`malloc`) and zero ESP-IDF dependencies so it runs identically on PC (`ctest` / CSV replay) and on `ESP32 WROOM-32`.

#### Architecture Overview (Decoupled for Future ML/AI Upgrade)
```text
Raw rpt_t ──► [1. Feature Extractor] ──► fusion_features_t ──► [2. Pluggable Engine] ──► score [0..1] ──► [3. State Supervisor] ──► fstate_t
                (filters.c +               (T, dT/dt, eCO2,       Default: Fuzzy Logic                      (R1-R3 overrides,
                 fusion_features.c)         TVOC, mq2_rel,        Future:  Decision Tree /                   Hysteresis, Latch)
                                            flame_drop, valid)             Random Forest / TinyML
```

#### 5.2.1 `filters.h / filters.c`
```c
#define MA_MAX_WIN       16
#define RATE_MAX_SAMPLES 64
#define RATE_MIN_SPAN_MS 20000u

typedef struct { float buf[MA_MAX_WIN]; uint8_t win, n, head; } ma_t;
void  ma_init(ma_t *m, uint8_t win);
void  ma_push(ma_t *m, float x);       /* ignores NaN/Inf */
float ma_value(const ma_t *m);         /* recomputes sum each call (zero drift) */
bool  ma_ready(const ma_t *m);

typedef struct { float v[3]; uint8_t n; } med3_t;
void  med3_init(med3_t *m);
float med3_push(med3_t *m, float x);   /* returns median of last 3 samples */

typedef struct {
    uint32_t t[RATE_MAX_SAMPLES];
    float    v[RATE_MAX_SAMPLES];
    uint8_t  n, head;
    uint32_t window_ms;
} rate_t;
void rate_init(rate_t *r, uint32_t window_ms);
void rate_push(rate_t *r, uint32_t t_ms, float v); /* drop samples with (int32_t)(t_ms - t_i) > window_ms */
bool rate_get(const rate_t *r, float *per_min);    /* OLS linear regression in double -> degC/min */
```

#### 5.2.2 `fusion_features.h / fusion_features.c` (Layer 1 — Feature Extraction)
```c
enum { CH_T = 0, CH_DT, CH_CO2, CH_TVOC, CH_MQ2, CH_FL, CH_COUNT };
enum {
    FL_SHT_FAIL  = 1u << 0,
    FL_CCS_BAD   = 1u << 1,
    FL_MQ2_WARM  = 1u << 2,
    FL_FLAME_DO  = 1u << 3,
    FL_PREALERT  = 1u << 4,
    FL_ADC_FAULT = 1u << 5
};

typedef struct {
    uint32_t t_ms;
    float    temp, hum, eco2, tvoc, mq2_mv, flame_mv;
    uint16_t flags;
    uint32_t valid;   /* RPT_HAS_* bitmask from proto_ndjson.h */
} fusion_in_t;

/* Clean, model-agnostic feature vector consumed by Fuzzy Logic OR any future ML/AI model */
typedef struct {
    float    temp_c;        /* Med3-filtered temperature (degC) */
    float    hum_pct;       /* Raw/filtered relative humidity (%) */
    float    dtdt_c_min;    /* Temperature rise rate (degC/min) */
    float    eco2_ppm;      /* MA-filtered eCO2 (ppm) */
    float    tvoc_ppb;      /* MA-filtered TVOC (ppb) */
    float    mq2_rel;       /* (ma_mq2 - mq2_base) / mq2_base */
    float    flame_drop;    /* (flame_base - ma_flame) / flame_base (inverted if FLAME_AO_INVERTED=1) */
    bool     flame_do;      /* Digital flame trigger */
    uint8_t  valid_mask;    /* Bit i = 1 if channel CH_i is valid */
} fusion_features_t;
```

#### 5.2.3 `fusion_engine.h` (Layer 2 — Pluggable Inference Strategy)
```c
typedef struct {
    const char *name; /* e.g., "fuzzy_v1", "rf_tinyml_v1" */
    float (*infer)(const fusion_features_t *feat, const void *cfg,
                   float out_ch_scores[CH_COUNT], float *out_valid_weight);
    const void *cfg;
} fusion_engine_t;
```
*Upgrade path to ML/AI:* To replace Fuzzy Logic with a trained Decision Tree, Random Forest (`emlearn`/`micromlgen`), or TinyML neural net later, implement a single function matching `infer(...)` in `fusion_ml_stub.c` that maps `fusion_features_t` $\rightarrow$ fire probability $\in [0.0, 1.0]$ and pass that `fusion_engine_t` into `fusion_set_engine()`. Zero changes required in `gw_host`, UART, MQTT, or alarm logic.

#### 5.2.4 `fusion_fuzzy.h / fusion_fuzzy.c` (Primary Fuzzy Logic Inference Engine)
Implements a transparent, explainable **Sugeno Zero-Order Fuzzy Inference System (FIS)** tailored for early fire detection and false-alarm rejection:

1. **Membership Functions (Trapezoidal / Triangular):**
   For each channel $i \in \{\text{T}, dT/dt, \text{eCO}_2, \text{TVOC}, \text{MQ2}, \text{Flame}\}$, compute three linguistic fuzzy degrees $\mu_{\text{LOW}}(x), \mu_{\text{MED}}(x), \mu_{\text{HIGH}}(x) \in [0, 1]$ using tunable breakpoints $(a, b, c, d)$ in `fusion_params.h`:
   - **Temperature (`T`, $^\circ\text{C}$):** `LOW` $\le 35$, `MED` peaks at $48$, `HIGH` $\ge 62$.
   - **Temp Rise Rate (`dT/dt`, $^\circ\text{C}/\text{min}$):** `LOW` $\le 1.5$, `MED` peaks at $4.5$, `HIGH` $\ge 8.0$.
   - **Smoke (`MQ2_rel`):** `LOW` $\le 0.15$, `MED` peaks at $0.45$, `HIGH` $\ge 0.90$.
   - **Gas (`eCO2` ppm / `TVOC` ppb):** Combined into an Air Quality Anomaly linguistic variable (`GAS_LOW`, `GAS_MED`, `GAS_HIGH`) using `max` of normalized `eCO2` ($700..1500$) and `TVOC` ($50..400$).
   - **Flame (`flame_drop`):** `LOW` $\le 0.10$, `MED` peaks at $0.30$, `HIGH` $\ge 0.50$ (or `flame_do == true` $\rightarrow \mu_{\text{HIGH}} = 1.0$).

2. **Fuzzy Rule Base (Multi-Sensor Correlation & False-Alarm Rejection):**
   Each rule $r_k$ computes firing strength $\alpha_k$ using Zadeh operators (`AND` = $\min$, `OR` = $\max$) and maps to a Sugeno output singleton $z_k \in \{0.05\text{ (Safe)}, 0.35\text{ (Caution)}, 0.60\text{ (Warning)}, 0.95\text{ (Fire)}\}$:
   - **Rule 1 (Ambient Normal):** `T=LOW AND MQ2=LOW AND GAS=LOW AND FL=LOW` $\rightarrow$ `Safe` ($0.05$)
   - **Rule 2 (Hairdryer / Hot Air Rejection):** `T=MED AND dT/dt=MED AND MQ2=LOW AND GAS=LOW AND FL=LOW` $\rightarrow$ `Caution` ($0.30$, prevents jumping to `WARNING`/`FIRE` from heat alone)
   - **Rule 3 (Cooking Steam / Mild Gas):** `GAS=MED AND T=LOW AND MQ2=LOW` $\rightarrow$ `Caution` ($0.30$)
   - **Rule 4 (Sunlight / False IR Rejection):** `FL=HIGH AND T=LOW AND MQ2=LOW AND GAS=LOW` $\rightarrow$ `Caution` ($0.20$, flame sensor alone never triggers `FIRE`)
   - **Rule 5 (Early Smoldering Fire — Smoke + Gas):** `MQ2=MED AND GAS=MED` $\rightarrow$ `Warning` ($0.60$)
   - **Rule 6 (Smoldering Fire — Heavy Smoke + Rising Temp):** `MQ2=HIGH AND (T=MED OR dT/dt=MED)` $\rightarrow$ `Fire` ($0.90$)
   - **Rule 7 (Smoldering Fire — Heavy Smoke + High Gas):** `MQ2=HIGH AND GAS=HIGH` $\rightarrow$ `Fire` ($0.90$)
   - **Rule 8 (Rapid Thermal Buildup + Smoke/Gas):** `dT/dt=HIGH AND (MQ2=MED OR GAS=MED)` $\rightarrow$ `Fire` ($0.92$)
   - **Rule 9 (Confirmed Flame + Any Corroborating Cue):** `FL=HIGH AND (T=MED OR dT/dt=MED OR MQ2=MED OR GAS=MED)` $\rightarrow$ `Fire` ($0.95$)
   - **Rule 10 (Extreme Heat):** `T=HIGH AND (dT/dt=MED OR MQ2=MED OR GAS=MED)` $\rightarrow$ `Fire` ($0.95$)
   - **Rule 11 (High Smoke Standalone Warning):** `MQ2=HIGH AND T=LOW AND GAS=LOW` $\rightarrow$ `Warning` ($0.58$)
   - **Rule 12 (High Thermal Rise Standalone Warning):** `dT/dt=HIGH AND T=MED` $\rightarrow$ `Warning` ($0.55$)

3. **Defuzzification (Weighted Average) + Missing-Sensor Gracefulness:**
   $$S_{\text{fuzzy}} = \frac{\sum_k \alpha_k z_k}{\sum_k \alpha_k}$$
   - If a sensor channel is flagged invalid (e.g., `MQ2` warming up or `CCS811` not ready), its linguistic terms evaluate neutrally or rules requiring it are masked out, while computing `valid_weight` from the remaining active channels. If `valid_weight < min_valid_weight` ($0.30$), signal `RS_INSUFF` and hold previous state.

#### 5.2.5 `fusion.h / fusion.c` (Layer 3 — State Machine, Safety Overrides & CSV Export)
```c
typedef enum { ST_NORMAL = 0, ST_CAUTION, ST_WARNING, ST_FIRE } fstate_t;
enum { RS_FUZZY = 1u<<0, RS_R1 = 1u<<1, RS_R2 = 1u<<2, RS_R3 = 1u<<3, RS_INSUFF = 1u<<4 };

typedef struct {
    fstate_t state, prev;
    bool     changed, latched, insufficient;
    float    score;                   /* Crisp output [0.0..1.0] from active engine */
    float    ch_scores[CH_COUNT];     /* Per-channel normalized [0..1] for telemetry/debug */
    float    dtdt;
    float    valid_weight;
    uint32_t reasons;
    fusion_features_t features;       /* Exposed for CSV logging & ML dataset capture */
} fusion_out_t;

void     fusion_params_default(fusion_params_t *p);
void     fusion_init(fusion_t *f, const fusion_params_t *p, float mq2_base_mv, float flame_base_mv);
void     fusion_set_engine(fusion_t *f, const fusion_engine_t *eng); /* Default: fusion_fuzzy_engine() */
void     fusion_set_baselines(fusion_t *f, float mq2_base_mv, float flame_base_mv);
void     fusion_update(fusion_t *f, const fusion_in_t *in, fusion_out_t *out);
void     fusion_tick(fusion_t *f, uint32_t now_ms);
bool     fusion_reset_latch(fusion_t *f);
int      fusion_format_csv(char *buf, size_t cap, const fusion_in_t *in, const fusion_out_t *out);
```
- **Hysteresis & Anti-Chatter:** `up = {0, 0.25, 0.45, 0.70}`, `down = {0, 0.15, 0.35, 0.55}`; require `N_UP = 2` consecutive reports to escalate, `N_DOWN = 3` consecutive reports to de-escalate one level at a time.
- **Hard Safety Overrides (Bypass `N_UP`):**
  - **R1 (Hard Temp):** `temp >= 65.0` for `2` consecutive reports $\rightarrow$ immediate `ST_FIRE` (`RS_R1`).
  - **R2 (Corroborated Flame):** (`flame_do` or `flame_drop >= 0.35`) for `3` consecutive reports **AND** at least one other channel (`T`, `MQ2`, or `eCO2`) normalized score $> 0.20$ $\rightarrow$ immediate `ST_FIRE` (`RS_R2`).
  - **R3 (Rapid Thermal Rise):** `dtdt >= 10.0` $^\circ\text{C}/\text{min}$ AND `temp >= 40.0` $^\circ\text{C}$ sustained for `30 s` $\rightarrow$ floor state at `ST_WARNING` (`RS_R3`).
- **FIRE Latch:** Entering `ST_FIRE` sets `latched = true`. Prevents auto-downgrade until `score < down[ST_FIRE]` and R1/R2 are clear for `60 s` (or manual reset).

---

### 5.3 Consolidated Node Sensors (`firmware/node_h2/components/sensors/`)

Instead of 6 separate component folders and `bringup/` apps, implement one compact component `sensors` (`sensors.h`, `sensors_i2c.c`, `sensors_adc.c`, `sensors.c`):

```c
esp_err_t sensors_init(void);                           /* I2C master + SHT31 + Gas + ADC1 + optional Flame DO */
esp_err_t sensors_sample(node_snapshot_t *out);         /* Reads all sensors, updates flags & local prealert */
void      sensors_set_mq2_baseline(float base_mv);      /* Auto-captured after warmup or on boot in clean air */
```

- **Datasheet Facts (Do Not Hallucinate):**
  - **SHT31 (`0x44`):** Single-shot high repeatability no clock-stretching: write `0x24 0x00`, wait `16 ms`, read `6` bytes (`T_hi, T_lo, CRC, RH_hi, RH_lo, CRC`). CRC-8 poly `0x31`, init `0xFF` (`{0xBE, 0xEF} -> 0x92`). $T = -45 + 175 \times \text{raw}/65535$, $RH = 100 \times \text{raw}/65535$.
  - **Gas Sensor Interface (CCS811 default `0x5A`, ENS160 drop-in `0x53`):**
    - *CCS811:* `HW_ID(0x20)=0x81`, `SW_RESET(0xFF)={0x11,0xE5,0x72,0x8A}` (wait `20 ms`), `APP_START(0xF4)` (wait `20 ms`), `MEAS_MODE(0x01)=0x10` (Mode 1, 1 Hz), `ENV_DATA(0x05)` = 4 bytes big-endian `(RH*512, (T+25)*512)`, `ALG_RESULT_DATA(0x02)` = 8 bytes (`eCO2`, `TVOC`, `STATUS`, `ERROR_ID`).
    - *Drop-in Abstraction:* Wrap behind `gas_sensor_init()`, `gas_sensor_set_env(t, rh)`, `gas_sensor_read(&eco2, &tvoc)` selectable via `#define GAS_SENSOR_TYPE_CCS811 1` (or `ENS160`) in `node_h2/main/config.h`.
  - **ADC (`MQ-2` on `GPIO1`, `Flame` on `GPIO4`):** `adc_oneshot` + curve-fitting calibration, `ADC_ATTEN_DB_12`, 16-sample trimmed mean (drop min/max), multiply by `ADC_DIV_RATIO = 2.0f`.
  - **Auto-Baseline (Thesis Convenience):** On Node boot (after a short `30 s` stabilization window in `BENCH_MODE=1` or configurable warmup), average `16` clean-air samples for `mq2_base` if NVS has no stored baseline.
  - **Local Pre-Alert:** If `temp >= 45.0` OR `mq2_rel >= 0.4` OR `eco2 >= 1000` OR `flame_do`, set `FL_PREALERT` bit and switch report interval from `10 s` to `3 s`.

---

### 5.4 Gateway Zigbee Bridge (`firmware/gw_h2/`)
- **`zb_coord`:** Forms network on `ZB_CHANNEL = 25`, `ZB_PAN_ID = 0x1A62`, opens permit-join (`180 s`) on boot if no nodes bound. Callback `ESP_ZB_CORE_REPORT_ATTR_CB_ID` extracts `(short_addr, src_ep, cluster, attr, value)` and sends `zb_evt_t` to `q_zb_evt`.
- **`uart_bridge`:**
  - Maps incoming Zigbee attributes (`EP10–EP15`) into `rpt_t` per node; flushes via `proto_enc_rpt()` once all expected fields arrive OR `300 ms` (`COALESCE_MS`) elapses after the first field.
  - Emits `{"t":"hello",...}` on boot and `{"t":"hb",...}` every `2000 ms` on `UART1` (`TX=GPIO10, RX=GPIO11, 115200 8N1`).

---

### 5.5 Gateway Host (`firmware/gw_host/`)
- **`uart_link`:** Reads `UART2` (`RX=GPIO16, TX=GPIO17, 115200 8N1`), runs `line_asm_feed()` $\rightarrow$ `proto_dec()`, forwards `M_RPT` to `q_rpt`. Logs `H2_LINK_LOST` if no `hb` for `6 s`.
- **`fusion_ctrl`:**
  - Maintains `fusion_t` instance (defaulting to Fuzzy Logic engine). On first `10` normal reports (if baseline is `0`), auto-initializes `mq2_base` and `flame_base` so experiments work out-of-the-box without complex CLI calibration.
  - Prints a clean CSV record to USB UART0 on every report for dataset collection:
    `CSV,<t_ms>,<temp>,<hum>,<eco2>,<tvoc>,<mq2_mv>,<flame_mv>,<flags>,<dtdt>,<score>,<state>`
- **`alarm_out` (Simplified per Confirmed Hardware):**
  - **Boot Safety (`H8`):** First lines of `app_main()` configure `GPIO4` (Buzzer) as OUTPUT LOW and `GPIO25/26/27` (RGB LEDC) to duty `0`.
  - **LED RGB (BKGR Common-Cathode, `25=R, 26=G, 27=B`):**
    - `ST_NORMAL`: Solid Green (`0, 255, 0`)
    - `ST_CAUTION`: Solid Yellow (`255, 160, 0`)
    - `ST_WARNING`: Blinking Yellow (`250 ms` on/off)
    - `ST_FIRE`: Fast Blinking Red (`125 ms` on/off)
    - *(When Muted by Button)*: LED **OFF** (`0, 0, 0`)
  - **Active Buzzer (`GPIO4`):**
    - `ST_NORMAL` / `ST_CAUTION`: OFF
    - `ST_WARNING`: Short beep (`100 ms` ON / `1900 ms` OFF)
    - `ST_FIRE`: Fast beep (`500 ms` ON / `500 ms` OFF)
    - *(When Muted by Button)*: Buzzer **OFF**
  - **Mute Button (`GPIO32`, `INPUT_PULLUP`):**
    - Simple `30 ms` debounce. **Press-and-release toggles `local_silenced = true`** (turns OFF both Buzzer and LED immediately). Automatically re-arms (`local_silenced = false`) if a **new** escalation to `ST_FIRE` occurs or state returns to `ST_NORMAL`.
- **`net_cloud`:**
  - Wi-Fi STA (`secrets.h`) with standard exponential reconnect backoff (`1 s..60 s`).
  - `esp_mqtt_client` over TLS (`mqtts://MQTT_HOST:8883` with `esp_crt_bundle_attach`), LWT `{"online":false}` on `status/gateway`.
  - Publishes JSON to `sensor/<node_id>` (every report) and `alert/<node_id>` (on state change).

---

### 5.6 Cloud Layer (`cloud/`)
- **`cloud/dashboard/`:** Single-page static HTML/JS (`index.html`, `app.js`, `style.css`) connecting to HiveMQ Cloud via `wss://<host>:8884/mqtt` using a **subscribe-only** credential. Displays live sensor cards, Fuzzy risk score gauge ($0..100\%$), state badge, and rolling Chart.js trend lines (`T`, `MQ-2`, `eCO2`, `Score`).
- **`cloud/telegram_bot/`:** Compact Node.js script subscribing to `alert/#`. Sends formatted Telegram alert on `WARNING` and `FIRE` (with `60 s` deduplication for same state; immediate send on escalation).

---

## 6. Lean Test Plan & Thesis Evaluation Protocol

### 6.1 Host Unit Tests & CSV Replay (PC — `ctest`)
Run with:
```powershell
cmake -S tests/host -B build_host && cmake --build build_host && ctest --test-dir build_host --output-on-failure
```
*(Also verify ASAN/UBSAN with Clang MSYS2 as established in `T-201`.)*

**Required Test Cases in `tests/host/test_fusion.c`:**
1. `test_filters`: MA no-drift, Med3 spike rejection (`{25, 95, 26} -> 26`), `rate_get` accuracy ($10.0 \pm 0.5\ ^\circ\text{C}/\text{min}$), `uint32_t` timestamp wraparound.
2. `test_fuzzy_scenarios`:
   - **Ambient (`S1`):** Stable room data $\rightarrow$ `ST_NORMAL`, `score < 0.15`.
   - **Hairdryer False Alarm (`S2`):** High temp rise + medium temp, zero smoke/gas/flame $\rightarrow$ stays $\le$ `ST_CAUTION` (no false `FIRE`).
   - **Sunlight IR False Alarm (`S3`):** `FL_FLAME_DO = 1` + `flame_drop = 0.8` alone, normal temp/smoke $\rightarrow$ stays $\le$ `ST_CAUTION` (never `FIRE`).
   - **Early Smoldering Fire (`S4`):** Rising `MQ-2` + `eCO2`/`TVOC` with mild temperature ($38^\circ\text{C}$) $\rightarrow$ escalates to `ST_WARNING` / `ST_FIRE` **before** temp hits $55^\circ\text{C}$.
   - **Corroborated Open Flame (`S5`):** Flame + moderate heat/smoke $\rightarrow$ immediate `ST_FIRE` via Fuzzy Rule 9 / Override R2.
   - **Hard Thermal Override (`S6`):** $T \ge 65^\circ\text{C}$ twice $\rightarrow$ immediate `ST_FIRE` (`RS_R1`).
   - **Hysteresis & Latch (`S7`):** Oscillating score around `0.45` does not chatter; `ST_FIRE` remains latched until cleared for `60 s`.
3. `test_pluggable_ml_engine`: Swap in `fusion_ml_stub` via `fusion_set_engine()` and confirm state machine operates identically on stub score output.

### 6.2 Thesis Experimental Evaluation (`tools/eval_fusion.py`)
Collect 3 real CSV recordings from `gw_host` Serial (`CSV,...` lines) in a safe, ventilated bench setup:
1. **Scenario A — Smoldering Fire:** Incense stick or smoldering paper near Node (`tests/vectors/exp_smolder.csv`).
2. **Scenario B — Open Flame:** Lighter flame at `20–30 cm` (`tests/vectors/exp_flame.csv`).
3. **Scenario C — False Alarm Nuisance:** Hairdryer warm air / steam / flashlight (`tests/vectors/exp_nuisance.csv`).

`tools/eval_fusion.py` runs both **Fuzzy Logic (`fusion_replay`)** and **Baseline Single-Sensor Fixed Thresholds** (e.g., Temp $> 55^\circ\text{C}$, MQ-2 $> +80\%$, or Flame DO alone) over the identical CSV logs and outputs a thesis-ready Markdown comparison table:
- **Detection Latency (s)** for Smoldering & Flame scenarios.
- **False Alarm Count** on Nuisance scenarios.
- **Precision / Recall / F1-Score** across labeled windows.

---

## 7. Appendix: OpenCode Prompt Template & `AGENTS.md`

### 7.1 Compact OpenCode Prompt Template
```markdown
ROLE: ESP-IDF & C99 Embedded Engineer. Execute ONLY the task below.
MODE: Thesis MVP (Happy-Path stability, lean code, <= 250 lines/file, no boilerplate).

TASK: <TaskID> — <Short Title>
CONTEXT (Read-only):
- AGENTS.md, STATE.md
- docs/DEV_CODING_PLAN.md (§<section>)
- docs/API_NOTES.md (for ESP-IDF/Zigbee tasks)

FILES TO CREATE/EDIT (Strictly limited to):
- <path1>
- <path2>

VERIFICATION (Run and paste REAL output):
1) <build / ctest command>
2) Confirm zero H1–H10 violations and update STATE.md.
```

### 7.2 Recommended `AGENTS.md` Update (Root Repo)
```markdown
# Agent Rules — Fire Alarm IoT (v2.0 Thesis MVP)
1. Source of truth: `docs/DEV_CODING_PLAN.md` (v2.0) and `STATE.md`. Never break frozen contracts (`proto_ndjson.h`, `zb_node.h`, EP10–15).
2. Thesis MVP discipline: Keep code lean (prefer 1 `.c`/`.h` pair per module, <= 250 lines/file). Do NOT create `bringup/` apps, `PWR_SENSE` code, or commercial fault-injection suites.
3. Hardware overrides: Gateway RGB LED is BKGR (`R=25, G=26, B=27`), Active Buzzer is **`GPIO4`**, Button `GPIO32` press-and-release ONLY silences Buzzer & LED.
4. Safety rules: No blocking `delay()`, no `sprintf`/`strcpy`, no `malloc`/logging in ISR, set Buzzer & LED OFF first in `app_main()`.
5. Anti-hallucination: Grep local ESP-IDF v5.5.5 headers before using any new API and record its exact signature in `docs/API_NOTES.md`.
6. Build gate: Never report done without running `ctest` and/or `.\tools\build_all.ps1` and pasting actual output.
```