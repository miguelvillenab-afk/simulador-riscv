#!/usr/bin/env python3
"""Empaqueta los .word documentados en los ejemplos como binarios little-endian.

No es un ensamblador: cada fuente contiene las palabras de máquina explícitas.
"""

from pathlib import Path
import re
import struct


ROOT = Path(__file__).resolve().parent
ALLOWED_LINE = re.compile(r"(?:\.text|\.globl\s+\w+|[A-Za-z_][A-Za-z0-9_]*:)")
WORD_LINE = re.compile(r"\.word\s+(0[xX][0-9a-fA-F]{1,8})\b")


def export(source: Path) -> Path:
    words = []
    for line_number, original in enumerate(source.read_text(encoding="utf-8").splitlines(), 1):
        line = original.split("#", 1)[0].strip()
        if not line or ALLOWED_LINE.fullmatch(line):
            continue
        match = WORD_LINE.fullmatch(line)
        if not match:
            raise ValueError(f"{source.name}:{line_number}: línea no reconocida: {line}")
        words.append(int(match.group(1), 16))

    if not words:
        raise ValueError(f"{source.name}: no contiene palabras de máquina")

    output = source.with_suffix(".bin")
    output.write_bytes(b"".join(struct.pack("<I", word) for word in words))
    print(f"{source.name} -> {output.name}: {len(words)} instrucciones, {4 * len(words)} bytes")
    return output


if __name__ == "__main__":
    for source_file in sorted(ROOT.glob("*.s")):
        export(source_file)
