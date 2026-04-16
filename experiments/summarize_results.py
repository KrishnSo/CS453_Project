#!/usr/bin/env python3
"""Extract high-signal lines from experiment console logs into short summaries."""
import argparse
import re
from pathlib import Path


def main():
    p = argparse.ArgumentParser()
    p.add_argument("kind", choices=["partition", "scale"])
    p.add_argument("log_file", type=Path)
    p.add_argument("out_file", type=Path)
    args = p.parse_args()

    text = args.log_file.read_text(errors="replace")
    lines = text.splitlines()

    def grab(pattern: str):
        rx = re.compile(pattern)
        return [ln for ln in lines if rx.search(ln)]

    out = []
    out.append(f"# Summary ({args.kind})")
    out.append("")
    out.extend(grab(r"cut_edges|strategy="))
    out.extend(grab(r"Leader:|Leader rounds:|Messages sent|Bytes sent|Dijkstra iterations:"))
    out.append("")
    args.out_file.parent.mkdir(parents=True, exist_ok=True)
    args.out_file.write_text("\n".join(out) + "\n")
    print(f"Wrote {args.out_file}")


if __name__ == "__main__":
    main()
