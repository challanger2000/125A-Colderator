#!/usr/bin/env python3
import json
import hashlib
import math
import pathlib
import urllib.request

import numpy as np
import soundfile as sf

ROOT = pathlib.Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "assets" / "frozen_sources.json"
OUT = ROOT / "generated" / "frozen_sources.h"
CACHE = ROOT / "build" / "frozen-source-cache"
TARGET_SR = 16000

def fetch(url: str, dest: pathlib.Path):
    dest.parent.mkdir(parents=True, exist_ok=True)
    if dest.exists() and dest.stat().st_size > 0:
        return
    req = urllib.request.Request(url, headers={"User-Agent": "125A-Colderator-source-prep/1.0"})
    with urllib.request.urlopen(req, timeout=60) as r, open(dest, "wb") as f:
        f.write(r.read())

def mono_resample(x, sr):
    x = np.asarray(x, dtype=np.float32)
    if x.ndim == 2:
        x = np.mean(x, axis=1)
    if sr == TARGET_SR:
        return x
    n = max(1, int(round(len(x) * TARGET_SR / float(sr))))
    old = np.linspace(0.0, 1.0, len(x), endpoint=False)
    new = np.linspace(0.0, 1.0, n, endpoint=False)
    return np.interp(new, old, x).astype(np.float32)

def highest_rms_window(x, seconds):
    length = min(len(x), max(1, int(round(seconds * TARGET_SR))))
    if len(x) <= length:
        return x.copy()
    hop = max(256, length // 32)
    best_i, best_e = 0, -1.0
    for i in range(0, len(x) - length + 1, hop):
        w = x[i:i+length]
        e = float(np.mean(w * w))
        if e > best_e:
            best_i, best_e = i, e
    return x[best_i:best_i+length].copy()

def normalize_int16(x):
    if len(x) == 0:
        return np.zeros(1, dtype=np.int16)
    x = x - float(np.mean(x))
    peak = float(np.max(np.abs(x)))
    if peak > 1e-8:
        x = x * (0.92 / peak)
    return np.round(np.clip(x, -1.0, 1.0) * 32767.0).astype(np.int16)

def c_array(name, values):
    rows = []
    vals = values.tolist()
    for i in range(0, len(vals), 20):
        rows.append("    " + ", ".join(str(int(v)) for v in vals[i:i+20]))
    return "inline constexpr std::int16_t " + name + "[] = {\n" + ",\n".join(rows) + "\n};\n"

def main():
    data = json.loads(MANIFEST.read_text(encoding="utf-8"))
    OUT.parent.mkdir(parents=True, exist_ok=True)
    blocks = [
        "#pragma once",
        "#include <cstddef>",
        "#include <cstdint>",
        "",
        "namespace Colderator::FrozenSources {",
        f"inline constexpr int kSampleRate = {TARGET_SR};",
        ""
    ]

    for src in data["sources"]:
        raw = CACHE / src["file"]
        fetch(src["download"], raw)
        digest = hashlib.sha256(raw.read_bytes()).hexdigest()
        print(f"SOURCE {src['id']} sha256={digest} bytes={raw.stat().st_size} license={src['license']}")
        audio, sr = sf.read(raw, always_2d=False, dtype="float32")
        audio = mono_resample(audio, sr)
        audio = highest_rms_window(audio, float(src["seconds"]))
        pcm = normalize_int16(audio)

        ident = "".join(ch if ch.isalnum() else "_" for ch in src["id"])
        blocks.append(c_array("k_" + ident, pcm))
        blocks.append(f"inline constexpr std::size_t k_{ident}_count = sizeof(k_{ident}) / sizeof(k_{ident}[0]);")
        blocks.append("")

    blocks += ["} // namespace Colderator::FrozenSources", ""]
    OUT.write_text("\n".join(blocks), encoding="utf-8")
    print(f"Wrote {OUT}")

if __name__ == "__main__":
    main()
