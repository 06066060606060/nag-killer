#!/usr/bin/env python3
import gzip
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "dashboard_source.html"
OUTPUT = ROOT / "index_html.h"


def minify(text: str) -> str:
    text = re.sub(r"<!--.*?-->", "", text, flags=re.S)
    text = re.sub(r">\s+<", "><", text)
    text = re.sub(r"[ \t]+", " ", text)
    return "\n".join(line.strip() for line in text.splitlines() if line.strip()) + "\n"


def main() -> None:
    source = minify(SOURCE.read_text(encoding="utf-8")).encode("utf-8")
    payload = gzip.compress(source, compresslevel=9, mtime=0)
    rows = []
    for offset in range(0, len(payload), 16):
        rows.append("  " + ", ".join(f"0x{byte:02X}" for byte in payload[offset:offset + 16]) + ",")
    header = "\n".join([
        "#pragma once",
        "#include <Arduino.h>",
        "",
        "static const uint8_t INDEX_HTML_GZ[] PROGMEM = {",
        *rows,
        "};",
        f"static const size_t INDEX_HTML_GZ_LEN = {len(payload)}u;",
        "",
    ])
    with OUTPUT.open("w", encoding="utf-8", newline="\n") as stream:
        stream.write(header)


if __name__ == "__main__":
    main()
