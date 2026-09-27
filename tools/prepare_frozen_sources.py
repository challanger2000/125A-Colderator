#!/usr/bin/env python3
import json
import hashlib
import math
import pathlib
import urllib.request
import urllib.error
import time

import numpy as np
import soundfile as sf
import av

ROOT = pathlib.Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "assets" / "frozen_sources.json"
OUT = ROOT / "generated" / "frozen_sources.h"
CACHE = ROOT / "build" / "frozen-source-cache"
TARGET_SR = 16000

def fetch(url: str, dest: pathlib.Path):
    dest.parent.mkdir(parents=True, exist_ok=True)
    if dest.exists() and dest.stat().st_size > 0:
        return

    last_error = None
    for attempt in range(5):
        try:
            req = urllib.request.Request(
                url,
                headers={
                    "User-Agent": "125A-Colderator-source-prep/1.0",
                    "Accept": "*/*"
                }
            )
            with urllib.request.urlopen(req, timeout=60) as r, open(dest, "wb") as f:
                f.write(r.read())
            return
        except urllib.error.HTTPError as exc:
            last_error = exc
            if exc.code != 429:
                raise
            time.sleep(2.0 * (attempt + 1))
        except urllib.error.URLError as exc:
            last_error = exc
            time.sleep(2.0 * (attempt + 1))

    raise RuntimeError(f"Failed to fetch {url}: {last_error}")

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

    active_ids = {"storm_wind", "ice_crackle"}
    for src in data["sources"]:
        if src["id"] not in active_ids:
            print(f"SKIP {src['id']} reserved source not embedded yet")
            continue
        raw = CACHE / src["file"]
        fetch(src["download"], raw)
        digest = hashlib.sha256(raw.read_bytes()).hexdigest()
        print(f"SOURCE {src['id']} sha256={digest} bytes={raw.stat().st_size} license={src['license']}")
        try:
            audio, sr = sf.read(raw, always_2d=False, dtype="float32")
        except Exception:
            container = av.open(str(raw))
            stream = next(s for s in container.streams if s.type == "audio")
            chunks = []
            sr = int(stream.rate or 48000)
            for frame in container.decode(stream):
                arr = frame.to_ndarray()
                if arr.ndim == 2:
                    arr = np.mean(arr.astype(np.float32), axis=0)
                else:
                    arr = arr.astype(np.float32)
                if np.issubdtype(arr.dtype, np.integer):
                    info = np.iinfo(arr.dtype)
                    arr = arr.astype(np.float32) / float(max(abs(info.min), info.max))
                chunks.append(np.asarray(arr, dtype=np.float32))
            container.close()
            if not chunks:
                raise RuntimeError(f"No audio frames decoded from {raw}")
            audio = np.concatenate(chunks)
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
