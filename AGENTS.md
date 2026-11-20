# Agent Rules — Fire Alarm IoT (OpenCode / DeepSeek)

## 0. Context Recovery & Precedence
When starting a new session or continuing work:
1. Read `STATE.md` to get the current Stage/Task ID, active Git branch, and latest status.
2. Read `review.md`. If reviewer feedback exists, fix all issues and re-verify before starting a new task.
3. **Precedence:** `docs/DEV_CODING_PLAN.md` (v2.0 Thesis MVP) is the **Single Source of Truth**. If `docs/GatewayPlan.md` conflicts with `DEV_CODING_PLAN.md` (e.g., buzzer GPIO4, simple mute button GPIO32, dropped `PWR_SENSE`/UPS/fault-injection/bringup), **follow `DEV_CODING_PLAN.md` v2.0**.
4. Read ONLY the sections in `docs/DEV_CODING_PLAN.md` relevant to the current task — never scan the entire repo or `_ref_*/`.

## 1. Coding Mindset & Anti-Overengineering
Bias toward **simplicity, flat C, and Happy Path stability** (this is an undergraduate thesis focused on Data Fusion, not a commercial product).
1. **Think Before Coding:**
   - State assumptions explicitly before writing code; if uncertain, stop and ask.
   - If docs and existing code conflict, stop and surface the discrepancy — do not guess.
2. **Simplicity & File Consolidation:**
   - Write the minimum code that solves the task. Nothing speculative.
   - **Pack each new module into a SINGLE `.c`/`.h` pair** (≤ 250 lines/file). Do NOT split a single component into 4–5 micro-files (`*_internal.h`, `*_stack.c`, etc.) like earlier tasks.
   - No abstractions, wrappers, callback registries, or error handling for impossible scenarios.
   - Check before finishing: *"If I wrote 200 lines and 50 can do it cleanly, rewrite it."*
3. **Surgical Changes:**
   - One task at a time; touch ONLY files listed in the task scope. Every changed line in `git diff` must trace directly to the request.
   - Do not reformat or refactor adjacent working code (especially completed Phase 0, `T-201`, and `T-202`).
   - Remove only `#include`s/variables/functions orphaned by YOUR changes.

## 2. Hard Embedded & Source Rules (`DEV_CODING_PLAN.md` v2.0)
1. **Contract-First:** Never alter existing contracts in `firmware/common/proto_ndjson.h` or function signatures in `DEV_CODING_PLAN.md` §5 without user approval.
2. **Toolchain:** Use **ESP-IDF 5.5.x CLI (`idf.py`)** for all 3 firmwares (`node_h2`, `gw_h2`, `gw_host`) and **CMake/CTest** for `tests/host/`. No PlatformIO, Arduino, or unapproved external libraries.
3. **Strict Bans:**
   - Blocking `delay()`/`vTaskDelay()` in logic flows (use `vTaskDelayUntil` tagged `/* periodic */` or queue/timeout waits).
   - `sprintf`, `strcpy`, `strcat`, `gets` (use `snprintf` with bounds checks).
   - Heap allocation (`malloc`), `printf`, or `ESP_LOGx` inside ISRs.
   - Magic numbers outside `config.h` / `fusion_params.h` / `shared_config.h`.
   - Secrets (`secrets.h`, `.env`) in Git.
4. **No Hallucinated APIs:** Before using any ESP-IDF/Zigbee API for the first time, verify its exact signature in installed headers and log it in `docs/API_NOTES.md`. If not found → **STOP and ask; never guess**.
5. **Hardware Safety:** Drive outputs (`buzzer_init()` on GPIO4, `rgb_init()` on GPIO25/26/27) OFF at the very start of `app_main`, before NVS/WiFi.

## 3. Goal-Driven Execution & Task Closeout
1. **Verify Plan:** For multi-step tasks, state a brief plan first: `1. [Step] → verify: [build/test command]`.
2. **Protect Tests:** Never modify, delete, or loosen assertions in `tests/host/` to make tests pass. Fix application code or report failure.
3. **Real Verification:** Never claim "done" without running `tools/build_all.ps1` / `idf.py build` or `ctest` and pasting actual output. For hardware-dependent tasks without a board attached, state: *"Build/CTest passed, awaiting HIL check."*
4. **Closeout:** Update `STATE.md`, clear `review.md` (if fixing review issues), and remind the user to run `.\export_diff.ps1`.

## 4. # Agent Rules — Fire Alarm IoT (v2.0 Final Thesis MVP)

1. **Single Source of Truth:** `docs/DEV_CODING_PLAN.md` (v2.0 Final) overrides `GatewayPlan.md` and `GW_SCHEMATIC_SPEC.md` on all conflicts (GPIO4 buzzer, GPIO32 simple mute, no PWR_SENSE, no bringup/, fixed 2 s report, Node-RED dashboard).
2. **Context Recovery:** Read `STATE.md` and `review.md` first. Read ONLY the task-relevant sections of `docs/DEV_CODING_PLAN.md`. `_ref_*/` is strictly read-only.
3. **Frozen Contracts:** Never break `firmware/common/proto_ndjson.h` or Zigbee `EP10–EP15` mapping without explicit user approval.
4. **Lean Flat C:** Execute one atomic task at a time; touch ONLY files listed in the prompt. Pack each new module into a SINGLE `.c`/`.h` pair with minimal boilerplate — do not split features into micro-files.
5. **Toolchain & Hard Rules (R1–R7):** ESP-IDF 5.5.5 CLI (`idf.py`) + CMake/CTest only. No busy-wait/long `vTaskDelay()`, no `sprintf`/`strcpy`/`strcat`/`gets`, no `malloc`/logging in ISR, no secrets in Git, no magic numbers outside `config.h`/`shared_config.h`/`fusion_params.h`, set Buzzer (GPIO4) and RGB LED (25/26/27) OFF first in `gw_host` `app_main()`.
6. **Zero Hallucinations:** Verify unfamiliar ESP-IDF/Zigbee APIs in installed headers and log in `docs/API_NOTES.md`. If missing or uncertain -> STOP and ask.
7. **Verification Gate:** Never alter tests to pass. Never report done without running `ctest` and/or `.\tools\build_all.ps1` and pasting actual output. If no board is attached, state: *"Build/CTest passed, awaiting HIL check."* Update `STATE.md` (4 lines) on closeout.