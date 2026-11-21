# Agent Rules — Fire Alarm IoT (v2.0 Final Thesis MVP)

## 0. Context Recovery & Single Source of Truth
1. **Single Source of Truth:** `docs/DEV_CODING_PLAN.md` **(v2.0 Final)** overrides `GatewayPlan.md` and `GW_SCHEMATIC_SPEC.md` on all conflicts (GPIO4 buzzer, GPIO32 simple mute, no `PWR_SENSE`, no `bringup/`, fixed 2 s report, Node-RED dashboard).
2. **Session Start:** Read `STATE.md` first to get the current Stage/Task ID, active Git branch, and completed modules.
3. **Scoped Reading:** Read ONLY the task-relevant sections of `docs/DEV_CODING_PLAN.md`. Never scan the entire repo; `_ref_*/` is strictly read-only.

## 1. Coding Mindset & Anti-Overengineering
Bias toward **caution, flat C99, and Happy Path stability** (undergraduate thesis focused on Data Fusion, not a commercial product).
1. **Think Before Coding:**
   - State assumptions explicitly before writing code; if uncertain or docs conflict with existing code, stop and ask.
   - If a simpler approach exists, propose it briefly — do not over-design silently.
2. **Simplicity & File Consolidation:**
   - Write the minimum code that solves the task. Nothing speculative.
   - **Pack each new module into a SINGLE `.c`/`.h` pair** (minimum lines/file) with minimal boilerplate — do NOT split a feature into 4–5 micro-files (`*_internal.h`, `*_stack.c`, etc.).
   - No abstractions, wrappers, callback registries, or error handling for impossible scenarios.
   - Check before finishing: *"If I wrote 200 lines and 50 can do it cleanly, rewrite it."*
3. **Surgical Changes:**
   - Execute one atomic task at a time; touch ONLY files listed in the task scope. Every changed line in `git diff` must trace directly to the request.
   - Do not reformat or refactor adjacent working code (especially completed Phase 0, `T-201`, and `T-202`).
   - Remove only `#include`s/variables/functions orphaned by YOUR changes. Mention pre-existing dead code — do not delete it.

## 2. Hard Embedded & Hardware Rules (R1–R7)
1. **Frozen Contracts:** Never alter `firmware/common/proto_ndjson.h`, Zigbee `EP10–EP15` mapping, or `DEV_CODING_PLAN.md` §5 signatures without explicit user approval.
2. **Toolchain:** Use **ESP-IDF 5.5.x CLI (`idf.py`)** for `node_h2` (`esp32h2`), `gw_h2` (`esp32h2`), `gw_host` (`esp32`), and **CMake/CTest** for `tests/host/`. No PlatformIO, Arduino, or unapproved libraries.
3. **Strict Bans:**
   - Blocking `delay()`/`vTaskDelay()` in logic flows (use `vTaskDelayUntil` tagged `/* periodic */` or queue/timeout waits).
   - `sprintf`, `strcpy`, `strcat`, `gets` (use `snprintf` with bounds checks).
   - Heap allocation (`malloc`), `printf`, or `ESP_LOGx` inside ISRs.
   - Magic numbers outside `config.h` / `fusion_params.h` / `shared_config.h`.
   - Secrets (`secrets.h`, `.env`) in Git.
4. **Zero Hallucinated APIs:** Verify unfamiliar ESP-IDF/Zigbee API signatures in installed headers (`E:\Espressif\frameworks\esp-idf-v5.5.5`) and log them in `docs/API_NOTES.md`. If missing or uncertain → **STOP and ask**.
5. **Hardware Safety:** In `gw_host` `app_main()`, drive Buzzer (`GPIO4`) and RGB LED (`GPIO25/26/27`) OFF at the very first step, before NVS/Wi-Fi.

## 3. Verification Gate & Task Closeout
1. **Plan First:** For multi-step tasks, state a brief plan before editing: `1. [Step] → verify: [build/test command]`.
2. **Protect Tests:** Never modify, delete, or loosen assertions in `tests/host/` to force a pass. Fix application code or report failure.
3. **Self-Review & Real Proof:** Before reporting done, inspect `git diff` against Sections 1 & 2, run `ctest` and/or `.\tools\build_all.ps1`, and paste actual output. If no board is attached for hardware tasks, state: *"Build/CTest passed, awaiting HIL check."*
4. **Closeout:** Update `STATE.md` (4 lines: current stage/task, branch, last completed, exact next step).