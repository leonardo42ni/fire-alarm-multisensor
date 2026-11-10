#!/usr/bin/env python3
import argparse
import re
import sys
import tempfile
import time
from pathlib import Path

SAMPLE_LOG = Path(__file__).resolve().parent.parent / "tests" / "vectors" / "serial_sample.log"
DEFAULT_BAUD = 115200
DEFAULT_TIMEOUT = 20.0
SERIAL_POLL_TIMEOUT = 0.2


class CheckError(Exception):
    pass


def compile_patterns(values):
    compiled = []
    for value in values:
        try:
            compiled.append(re.compile(value))
        except re.error as exc:
            raise CheckError(f"invalid regex {value!r}: {exc}") from exc
    return compiled


def iter_file(path):
    with path.open("r", encoding="utf-8", errors="replace") as handle:
        for line in handle:
            yield line.rstrip("\r\n")


def iter_serial(port, baud, timeout):
    try:
        import serial
    except ImportError as exc:
        raise CheckError(
            "pyserial is required for serial mode; install with: python -m pip install pyserial"
        ) from exc

    try:
        handle = serial.Serial(port=port, baudrate=baud, timeout=SERIAL_POLL_TIMEOUT)
    except (serial.SerialException, ValueError) as exc:
        raise CheckError(f"cannot open serial port {port}: {exc}") from exc

    deadline = time.monotonic() + timeout
    try:
        while time.monotonic() < deadline:
            raw = handle.readline()
            if not raw:
                continue
            yield raw.decode("utf-8", errors="replace").rstrip("\r\n")
    finally:
        handle.close()


def close_iterator(iterator):
    close = getattr(iterator, "close", None)
    if close is not None:
        close()


def run_check(lines, expects, forbids, log_path=None, quiet=False, stop_on_match=True):
    matched = [False] * len(expects)
    log_handle = None
    iterator = iter(lines)

    try:
        if log_path is not None:
            log_path.parent.mkdir(parents=True, exist_ok=True)
            log_handle = log_path.open("w", encoding="utf-8")

        for line in iterator:
            if log_handle is not None:
                log_handle.write(line + "\n")
                log_handle.flush()

            if "BU|" not in line:
                continue

            for index, pattern in enumerate(expects):
                if not matched[index] and pattern.search(line):
                    matched[index] = True

            for pattern in forbids:
                if pattern.search(line):
                    if not quiet:
                        print(f"[FORBID] {pattern.pattern} matched: {line}")
                    return 1

            if stop_on_match and not forbids and matched and all(matched):
                break
    finally:
        close_iterator(iterator)
        if log_handle is not None:
            log_handle.close()

    if not quiet:
        for index, pattern in enumerate(expects):
            state = "MATCHED" if matched[index] else "MISSING"
            print(f"[EXPECT] {pattern.pattern} -> {state}")

    if all(matched):
        if not quiet:
            print("[PASS]")
        return 0

    if not quiet:
        print("[FAIL] timeout or missing expectation")
    return 1


def load_sample_lines():
    if not SAMPLE_LOG.is_file():
        raise CheckError(f"sample log not found: {SAMPLE_LOG}")
    return list(iter_file(SAMPLE_LOG))


def run_case(name, lines, expect_values, forbid_values, want):
    try:
        expects = compile_patterns(expect_values)
        forbids = compile_patterns(forbid_values)
        rc = run_check(lines, expects, forbids, quiet=True)
    except CheckError as exc:
        print(f"[SELFTEST] {name}: ERROR {exc}")
        return False

    ok = rc == want
    print(f"[SELFTEST] {name}: {'OK' if ok else 'FAIL'}")
    return ok


def check_forbid_fail(lines):
    fail_lines = lines + ["BU|SHT31|T=99.0|RH=0.0|FAIL|reason=CRC"]
    return run_case("forbid_fail", fail_lines, [], [r"BU\|.*\|FAIL"], 1)


def check_forbid_after_match():
    case_lines = [
        "BU|SHT31|T=24.0|RH=50.0|PASS",
        "BU|SHT31|T=99.0|RH=0.0|FAIL|reason=CRC",
    ]
    return run_case(
        "forbid_after_match",
        case_lines,
        [r"BU\|SHT31\|.*\|PASS"],
        [r"BU\|.*\|FAIL"],
        1,
    )


def check_log_capture(lines):
    with tempfile.TemporaryDirectory() as temp_dir:
        log_path = Path(temp_dir) / "capture.log"
        try:
            rc = run_check(
                lines,
                compile_patterns([r"BU\|SHT31\|.*\|PASS"]),
                compile_patterns([]),
                log_path=log_path,
                quiet=True,
            )
            text = log_path.read_text(encoding="utf-8") if log_path.is_file() else ""
            ok = rc == 0 and "BU|SHT31" in text
        except (CheckError, OSError) as exc:
            print(f"[SELFTEST] log_capture: ERROR {exc}")
            return False

    print(f"[SELFTEST] log_capture: {'OK' if ok else 'FAIL'}")
    return ok


def run_selftest():
    try:
        lines = load_sample_lines()
    except (CheckError, OSError) as exc:
        print(f"SELFTEST FAIL: {exc}")
        return 1

    cases = [
        ("dod_expect", lines, [r"BU\|SHT31\|.*\|PASS"], [], 0),
        ("i2c_expect", lines, [r"BU\|I2C\|found=0x44,0x5A\|PASS"], [], 0),
        ("missing_expect", lines, [r"BU\|NO_SUCH\|PASS"], [], 1),
        ("forbid_clean", lines, [], [r"BU\|.*\|FAIL"], 0),
    ]

    results = [
        run_case(name, case_lines, expects, forbids, want)
        for name, case_lines, expects, forbids, want in cases
    ]
    results.append(check_forbid_fail(lines))
    results.append(check_forbid_after_match())
    results.append(check_log_capture(lines))

    failures = results.count(False)
    if failures:
        print(f"SELFTEST FAIL ({failures} case(s))")
        return 1

    print("SELFTEST PASS")
    return 0


def build_parser():
    parser = argparse.ArgumentParser(
        description="Check BU|... lines from a serial port or a saved log."
    )
    source = parser.add_mutually_exclusive_group()
    source.add_argument("--port", help="serial port, for example COM5")
    source.add_argument("--input", type=Path, help="read lines from a log file")
    parser.add_argument("--baud", type=int, default=DEFAULT_BAUD, help="serial baud rate")
    parser.add_argument(
        "--timeout",
        type=float,
        default=DEFAULT_TIMEOUT,
        help="read timeout in seconds (only applies to --port mode)",
    )
    parser.add_argument(
        "--expect",
        action="append",
        default=[],
        metavar="REGEX",
        help="expected regex pattern (can be repeated)",
    )
    parser.add_argument(
        "--forbid",
        action="append",
        default=[],
        metavar="REGEX",
        help="forbidden regex pattern; in --port mode, runs for full --timeout to verify no violations",
    )
    parser.add_argument("--log", type=Path, help="write raw lines to this file")
    parser.add_argument("--selftest", action="store_true", help="run built-in checks")
    return parser


def main(argv=None):
    parser = build_parser()
    args = parser.parse_args(argv)

    if args.timeout <= 0:
        parser.error("--timeout must be greater than 0")
    if args.baud <= 0:
        parser.error("--baud must be greater than 0")
    if args.selftest:
        return run_selftest()
    if not args.expect and not args.forbid:
        parser.error("provide at least one --expect or --forbid")
    if args.port is None and args.input is None:
        parser.error("provide --port or --input (or use --selftest)")

    try:
        expects = compile_patterns(args.expect)
        forbids = compile_patterns(args.forbid)

        if args.input is not None:
            if not args.input.is_file():
                raise CheckError(f"input file not found: {args.input}")
            lines = iter_file(args.input)
        else:
            lines = iter_serial(args.port, args.baud, args.timeout)

        return run_check(
            lines,
            expects,
            forbids,
            log_path=args.log,
            stop_on_match=args.input is None,
        )
    except (CheckError, OSError) as exc:
        print(f"[ERROR] {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
