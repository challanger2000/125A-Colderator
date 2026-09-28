#!/usr/bin/env python3
import hashlib
import json
import pathlib
import time
import urllib.error
import urllib.request

import av
import numpy as np
import soundfile as sf

ROOT = pathlib.Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "assets" / "real_fixtures.json"
CACHE = ROOT / "build" / "real-fixture-cache"
OUT = ROOT / "build" / "real-fixtures"

def fetch(url: str, dest: pathlib.Path):
    dest.parent.mkdir(parents=True, exist_ok=True)
    if dest.exists() and dest.stat().st_size > 0:
        return
    last_error = None
    for attempt in range(5):
        try:
            req = urllib.request.Request(
                url,
                headers={"User-Agent": "125A-Colderator-real-fixtures/1.0", "Accept": "*/*"},
            )
            with urllib.request.urlopen(req, timeout=90) as r, open(dest, "wb") as f:
                f.write(r.read())
            return
        except (urllib.error.HTTPError, urllib.error.URLError) as exc:
            last_error = exc
            time.sleep(2.0 * (attempt + 1))
    raise RuntimeError(f"Failed to fetch {url}: {last_error}")

def decode(path: pathlib.Path):
    try:
        audio, sr = sf.read(path, always_2d=True, dtype="float32")
        return np.asarray(audio, dtype=np.float32), int(sr)
    except Exception:
        container = av.open(str(path))
        stream = next(s for s in container.streams if s.type == "audio")
        chunks = []
        sr = int(stream.rate or 48000)
        for frame in container.decode(stream):
            arr = frame.to_ndarray()
            if arr.ndim == 1:
                arr = arr[np.newaxis, :]
            arr = arr.astype(np.float32)
            if np.issubdtype(frame.to_ndarray().dtype, np.integer):
                info = np.iinfo(frame.to_ndarray().dtype)
                arr /= float(max(abs(info.min), info.max))
            chunks.append(arr.T)
        container.close()
        if not chunks:
            raise RuntimeError(f"No audio decoded from {path}")
        return np.concatenate(chunks, axis=0), sr

def resample(audio, sr, target_sr):
    if sr == target_sr:
        return audio
    n = max(1, int(round(len(audio) * target_sr / float(sr))))
    old = np.linspace(0.0, 1.0, len(audio), endpoint=False)
    new = np.linspace(0.0, 1.0, n, endpoint=False)
    channels = [
        np.interp(new, old, audio[:, ch]).astype(np.float32)
        for ch in range(audio.shape[1])
    ]
    return np.stack(channels, axis=1)

def highest_rms_window(audio, target_sr, seconds):
    length = min(len(audio), max(1, int(round(seconds * target_sr))))
    if len(audio) <= length:
        return audio.copy()
    mono = np.mean(audio, axis=1)
    hop = max(1024, length // 64)
    best_i, best_e = 0, -1.0
    for i in range(0, len(audio) - length + 1, hop):
        w = mono[i:i+length]
        e = float(np.mean(w * w))
        if e > best_e:
            best_i, best_e = i, e
    return audio[best_i:best_i+length].copy()

def main():
    data = json.loads(MANIFEST.read_text(encoding="utf-8"))
    target_sr = int(data["target_sample_rate"])
    OUT.mkdir(parents=True, exist_ok=True)
    provenance = []

    for src in data["fixtures"]:
        raw = CACHE / src["file"]
        fetch(src["download"], raw)
        digest = hashlib.sha256(raw.read_bytes()).hexdigest()
        audio, sr = decode(raw)

        # Preserve the source's channel structure up to stereo; no normalization.
        if audio.shape[1] > 2:
            audio = audio[:, :2]
        audio = resample(audio, sr, target_sr)
        audio = highest_rms_window(audio, target_sr, float(src["seconds"]))
        audio = np.clip(audio, -1.0, 1.0)

        out = OUT / f"{src['id']}.wav"
        sf.write(out, audio, target_sr, subtype="PCM_16")

        provenance.append({
            **src,
            "source_sha256": digest,
            "prepared_file": out.name,
            "prepared_sample_rate": target_sr,
            "prepared_channels": int(audio.shape[1]),
            "prepared_frames": int(audio.shape[0]),
            "processing": "decode; preserve gain; preserve mono/stereo; resample to 48 kHz; deterministic highest-RMS excerpt; PCM16",
        })
        print(
            f"REAL_FIXTURE {src['id']} sha256={digest} sr={sr} "
            f"channels={audio.shape[1]} frames={audio.shape[0]} license={src['license']}"
        )

    (OUT / "PROVENANCE.json").write_text(
        json.dumps({"fixtures": provenance}, indent=2, ensure_ascii=False),
        encoding="utf-8",
    )

if __name__ == "__main__":
    main()
