#!/usr/bin/env python3
import argparse
import csv
from datetime import datetime
from pathlib import Path
import sys
import time

CSV_HEADER = [
    "t_ms", "node", "seq", "temp", "hum", "eco2", "tvoc", "mq2_mv", "flame_mv",
    "flags", "rssi", "lqi", "n_T", "n_R", "n_S", "n_G", "n_F", "n_H", "risk",
    "state", "pc_time", "run_id", "label", "event",
]
PAYLOAD_COLS = 20
DEFAULT_BAUD = 115200
SERIAL_TIMEOUT_S = 0.2


def read_hotkey():
    if not sys.stdin.isatty():
        return None
    if sys.platform == "win32":
        import msvcrt
        if msvcrt.kbhit():
            ch = msvcrt.getwch()
            return ch.lower() if ch else None
        return None
    import select
    r, _, _ = select.select([sys.stdin], [], [], 0)
    if r:
        return sys.stdin.read(1).lower()
    return None


def iter_file(path):
    with path.open("r", encoding="utf-8-sig", errors="replace") as handle:
        for line in handle:
            yield line.rstrip("\r\n")


def iter_serial(port, baud, duration):
    import serial
    handle = serial.Serial(port=port, baudrate=baud, timeout=SERIAL_TIMEOUT_S)
    deadline = (time.monotonic() + duration) if duration is not None else None
    try:
        while deadline is None or time.monotonic() < deadline:
            raw = handle.readline()
            if not raw:
                yield None
                continue
            yield raw.decode("utf-8", errors="replace").rstrip("\r\n")
    finally:
        handle.close()


def parse_args():
    repo_root = Path(__file__).resolve().parent.parent
    parser = argparse.ArgumentParser(description="Capture gw_host CSV telemetry to data/raw/<run_id>.csv")
    src = parser.add_mutually_exclusive_group(required=True)
    src.add_argument("--port", help="Serial port (e.g. COM10)")
    src.add_argument("--input", type=Path, help="Offline log file for replay/testing")
    parser.add_argument("--baud", type=int, default=DEFAULT_BAUD)
    parser.add_argument("--run-id", default=datetime.now().strftime("%Y%m%d_%H%M%S"))
    parser.add_argument("--out-dir", type=Path, default=repo_root / "data" / "raw")
    parser.add_argument("--label", default="n", choices=["n", "f", "u"])
    parser.add_argument("--duration", type=float, default=None, help="Capture duration in seconds")
    return parser.parse_args()


def main():
    args = parse_args()
    args.out_dir.mkdir(parents=True, exist_ok=True)
    out_path = args.out_dir / f"{args.run_id}.csv"

    label = args.label
    pending_event = ""
    rows = 0

    lines = iter_serial(args.port, args.baud, args.duration) if args.port else iter_file(args.input)
    print(f"[log_csv] writing={out_path} label={label} keys=[n/f/u=label, i=ignite, e=end, q=quit]")

    with out_path.open("w", newline="", encoding="utf-8") as csv_file:
        writer = csv.writer(csv_file)
        writer.writerow(CSV_HEADER)
        csv_file.flush()

        try:
            for line in lines:
                key = read_hotkey()
                if key in ("n", "f", "u"):
                    label = key
                    print(f"[key] label={label}")
                elif key == "i":
                    pending_event = "ignite"
                    print("[key] event=ignite (next CSV row)")
                elif key == "e":
                    pending_event = "end"
                    print("[key] event=end (next CSV row)")
                elif key == "q":
                    print("[key] quit requested")
                    break

                if line is None:
                    continue
                if "BU|" in line:
                    print(line)
                if not line.startswith("CSV,"):
                    continue

                parts = line.split(",")
                if len(parts) != PAYLOAD_COLS + 1:
                    print(f"[warn] skip malformed CSV ({len(parts) - 1} cols): {line}")
                    continue

                pc_time = datetime.now().isoformat(timespec="milliseconds")
                writer.writerow(parts[1:] + [pc_time, args.run_id, label, pending_event])
                csv_file.flush()
                rows += 1
                pending_event = ""
        except KeyboardInterrupt:
            print("\n[log_csv] interrupted by user")

    print(f"[log_csv] done rows={rows} file={out_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
