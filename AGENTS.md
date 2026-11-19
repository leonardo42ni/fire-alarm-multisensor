# Agent Rules — Fire Alarm IoT (OpenCode / DeepSeek)

## 0. Context Recovery (New Session)
When starting a new chat or continuing work:
1. Read `STATE.md` to get the current Task ID (`T-xxx`), active Git branch, and latest status[cite: 1].
2. Read `review.md`. If reviewer feedback exists, fix all issues and re-verify before starting a new task[cite: 1].
3. Read ONLY the sections in `docs/DEV_CODING_PLAN.md` and `docs/GatewayPlan.md` relevant to the current task — never scan the entire repo[cite: 1].

## 1. Coding Mindset & Anti-Overengineering
Bias toward **caution and simplicity** over speed.
1. **Think Before Coding:**
   - State assumptions explicitly before writing code; if uncertain, stop and ask.
   - If multiple interpretations or a simpler approach exist, present them briefly — do not pick silently.
   - If docs and existing code conflict, stop and surface the discrepancy — do not guess.
2. **Simplicity First:**
   - Write the minimum code that solves the current task. Nothing speculative.
   - No features, configurability, or "flexibility" beyond the task scope.
   - No abstractions, wrappers, callback registries, or extra structs for single-use logic.
   - No error handling for impossible scenarios.
   - Check before finishing: *"If I wrote 200 lines and 50 can do it cleanly, rewrite it."*
3. **Surgical Changes:**
   - One task at a time; touch ONLY files listed in the task scope[cite: 1]. Every changed line in `git diff` must trace directly to the task request.
   - Do not "improve", reformat, or refactor adjacent working code/comments; match existing style.
   - Remove only `#include`s/variables/functions orphaned by YOUR changes. Mention pre-existing dead code — do not delete it.

## 2. Hard Embedded & Source Rules (`DEV_CODING_PLAN.md` §1.1 & §8.2)
1. **Contract-First:** Never alter interfaces in `firmware/common/`, `GatewayPlan.md` §4–§5, or function signatures in `DEV_CODING_PLAN.md` §5 without user approval[cite: 1].
2. **Toolchain:** Use **ESP-IDF 5.5.x CLI (`idf.py`)** for all 3 firmwares (`node_h2`, `gw_h2`, `gw_host`) and **CMake/CTest** for `tests/host/`[cite: 1]. No PlatformIO, Arduino, or unapproved external libraries[cite: 1].
3. **Strict Bans (H1–H10):**
   - Blocking `delay()`/`vTaskDelay()` in logic flows (use `vTaskDelayUntil` tagged `/* periodic */` or queue/timeout waits)[cite: 1].
   - `sprintf`, `strcpy`, `strcat`, `gets` (use `snprintf` with bounds checks)[cite: 1].
   - Heap allocation (`malloc`), `printf`, or `ESP_LOGx` inside ISRs[cite: 1].
   - Magic numbers outside `config.h` / `fusion_params.h` / `shared_config.h`[cite: 1].
   - Secrets (WiFi/MQTT/tokens) in Git[cite: 1].
   - Files > ~300 lines or functions > ~60 lines[cite: 1].
4. **No Hallucinated APIs (H11):** Before using any ESP-IDF/Zigbee API for the first time, verify its exact signature in installed headers and log it in `docs/API_NOTES.md`[cite: 1]. If not found → **STOP and ask; never guess**[cite: 1].
5. **Hardware Safety:** Drive outputs (`buzzer_init()`, `rgb_init()`) OFF at the very start of `app_main`, before NVS/WiFi[cite: 1].

## 3. Goal-Driven Execution & Task Closeout
1. **Verify Plan:** For multi-step tasks, state a brief plan first: `1. [Step] → verify: [build/test command]`.
2. **Protect Tests (H8):** Never modify, delete, or loosen assertions in `tests/host/` to make tests pass[cite: 1]. Fix application code or report failure[cite: 1].
3. **Real Verification:** Never claim "done" without running `tools/build_all.ps1` / `idf.py build` or `ctest` and pasting actual output[cite: 1]. For hardware-dependent tasks without a board attached, state: *"Build/CTest passed, awaiting HIL check."*[cite: 1]
4. **Closeout:** Update `STATE.md`, clear `review.md` (if fixing review issues), and remind the user to run `.\export_diff.ps1`[cite: 1].