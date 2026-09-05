#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import re
import struct
import sys
import zlib
from collections import Counter, defaultdict
from dataclasses import asdict, dataclass, field
from pathlib import Path
from typing import Any, Iterable

TILE_RE = re.compile(r"^(?P<row>\d+)_(?P<col>\d+)\.(?:jpg|jpeg|png)$", re.IGNORECASE)
MINIMAP_RE = re.compile(r"^p(?P<legacy_id>.+)\.(?:jpg|jpeg|png)$", re.IGNORECASE)


@dataclass(slots=True)
class AssetRecord:
    legacy_id: str
    kind: str
    path: str
    extension: str
    size: int
    sha256: str | None = None
    width: int | None = None
    height: int | None = None
    metadata: dict[str, Any] = field(default_factory=dict)


def sha256_file(path: Path, chunk_size: int = 1024 * 1024) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(chunk_size), b""):
            digest.update(chunk)
    return digest.hexdigest()


def image_size(path: Path) -> tuple[int | None, int | None]:
    try:
        with path.open("rb") as handle:
            header = handle.read(32)
            if header.startswith(b"\x89PNG\r\n\x1a\n") and len(header) >= 24:
                width, height = struct.unpack(">II", header[16:24])
                return width, height

            if header[:2] != b"\xff\xd8":
                return None, None

            handle.seek(2)
            while True:
                marker_start = handle.read(1)
                if not marker_start:
                    return None, None
                if marker_start != b"\xff":
                    continue

                marker = handle.read(1)
                while marker == b"\xff":
                    marker = handle.read(1)
                if not marker:
                    return None, None

                code = marker[0]
                if code in {0xD8, 0xD9}:
                    continue

                raw_length = handle.read(2)
                if len(raw_length) != 2:
                    return None, None
                segment_length = struct.unpack(">H", raw_length)[0]
                if segment_length < 2:
                    return None, None

                if code in {
                    0xC0, 0xC1, 0xC2, 0xC3,
                    0xC5, 0xC6, 0xC7,
                    0xC9, 0xCA, 0xCB,
                    0xCD, 0xCE, 0xCF,
                }:
                    payload = handle.read(segment_length - 2)
                    if len(payload) >= 5:
                        height, width = struct.unpack(">HH", payload[1:5])
                        return width, height
                    return None, None

                handle.seek(segment_length - 2, 1)
    except OSError:
        return None, None


def _read_bits(data: bytes, bit_offset: int, count: int) -> tuple[int, int]:
    value = 0
    for _ in range(count):
        byte_index = bit_offset // 8
        if byte_index >= len(data):
            raise ValueError("Unexpected end of SWF bit stream")
        bit_index = 7 - (bit_offset % 8)
        value = (value << 1) | ((data[byte_index] >> bit_index) & 1)
        bit_offset += 1
    return value, bit_offset


def _signed(value: int, width: int) -> int:
    sign_bit = 1 << (width - 1)
    return value - (1 << width) if value & sign_bit else value


def _parse_swf_rect(body: bytes) -> tuple[dict[str, float], int]:
    bit_offset = 0
    nbits, bit_offset = _read_bits(body, bit_offset, 5)
    values: list[int] = []
    for _ in range(4):
        raw, bit_offset = _read_bits(body, bit_offset, nbits)
        values.append(_signed(raw, nbits))
    byte_offset = (bit_offset + 7) // 8
    xmin, xmax, ymin, ymax = values
    return {
        "xmin": xmin / 20.0,
        "xmax": xmax / 20.0,
        "ymin": ymin / 20.0,
        "ymax": ymax / 20.0,
        "width": (xmax - xmin) / 20.0,
        "height": (ymax - ymin) / 20.0,
    }, byte_offset


def _iter_swf_tags(data: bytes, start: int) -> Iterable[tuple[int, bytes]]:
    offset = start
    while offset + 2 <= len(data):
        header = struct.unpack_from("<H", data, offset)[0]
        offset += 2
        code = header >> 6
        length = header & 0x3F
        if length == 0x3F:
            if offset + 4 > len(data):
                return
            length = struct.unpack_from("<I", data, offset)[0]
            offset += 4
        end = offset + length
        if end > len(data):
            return
        payload = data[offset:end]
        yield code, payload
        offset = end
        if code == 0:
            return


def _read_c_string(payload: bytes, offset: int) -> tuple[str, int]:
    end = payload.find(b"\x00", offset)
    if end < 0:
        end = len(payload)
    value = payload[offset:end].decode("utf-8", errors="replace")
    return value, min(end + 1, len(payload))


def _parse_symbol_pairs(payload: bytes) -> list[dict[str, Any]]:
    if len(payload) < 2:
        return []
    count = struct.unpack_from("<H", payload, 0)[0]
    offset = 2
    result: list[dict[str, Any]] = []
    for _ in range(count):
        if offset + 2 > len(payload):
            break
        symbol_id = struct.unpack_from("<H", payload, offset)[0]
        offset += 2
        name, offset = _read_c_string(payload, offset)
        result.append({"id": symbol_id, "name": name})
    return result


def swf_metadata(path: Path) -> dict[str, Any]:
    try:
        raw = path.read_bytes()
    except OSError as exc:
        return {"error": str(exc)}

    if len(raw) < 8:
        return {"error": "file too small"}

    signature = raw[:3].decode("ascii", errors="replace")
    version = raw[3]
    declared_size = struct.unpack_from("<I", raw, 4)[0]
    metadata: dict[str, Any] = {
        "signature": signature,
        "version": version,
        "declaredSize": declared_size,
    }

    if signature == "FWS":
        body = raw[8:]
    elif signature == "CWS":
        try:
            body = zlib.decompress(raw[8:])
        except zlib.error as exc:
            metadata["error"] = f"zlib: {exc}"
            return metadata
    elif signature == "ZWS":
        metadata["compressed"] = "lzma"
        metadata["note"] = "ZWS header catalogued; tag parsing is intentionally deferred"
        return metadata
    else:
        metadata["error"] = "not a SWF signature"
        return metadata

    try:
        stage, offset = _parse_swf_rect(body)
        if offset + 4 > len(body):
            raise ValueError("missing SWF frame header")
        frame_rate_raw = struct.unpack_from("<H", body, offset)[0]
        frame_rate = ((frame_rate_raw >> 8) & 0xFF) + ((frame_rate_raw & 0xFF) / 256.0)
        frame_count = struct.unpack_from("<H", body, offset + 2)[0]
        tag_start = offset + 4

        exports: list[dict[str, Any]] = []
        symbol_classes: list[dict[str, Any]] = []
        tag_counts: Counter[int] = Counter()
        for code, payload in _iter_swf_tags(body, tag_start):
            tag_counts[code] += 1
            if code == 56:
                exports.extend(_parse_symbol_pairs(payload))
            elif code == 76:
                symbol_classes.extend(_parse_symbol_pairs(payload))

        metadata.update({
            "stage": stage,
            "frameRate": frame_rate,
            "frameCount": frame_count,
            "exports": exports,
            "symbolClasses": symbol_classes,
            "tagCounts": {str(key): value for key, value in sorted(tag_counts.items())},
        })
    except (ValueError, struct.error) as exc:
        metadata["error"] = str(exc)

    return metadata


def infer_kind(root: Path, path: Path) -> tuple[str, str, dict[str, Any]]:
    relative = path.relative_to(root)
    extension = path.suffix.lower()
    stem = path.stem
    parts = [part.lower() for part in relative.parts]
    metadata: dict[str, Any] = {}

    if extension == ".swf":
        return "swf", stem, metadata

    if extension in {".json", ".txt"}:
        return "config", stem, metadata

    if extension in {".jpg", ".jpeg", ".png"}:
        parent_name = path.parent.name
        tile_match = TILE_RE.match(path.name)
        if parent_name.lower().startswith("d_") and tile_match:
            map_id = parent_name[2:]
            metadata.update({
                "mapId": map_id,
                "row": int(tile_match.group("row")),
                "column": int(tile_match.group("col")),
            })
            return "map_tile", f"{map_id}:{tile_match.group('row')}:{tile_match.group('col')}", metadata

        minimap_match = MINIMAP_RE.match(path.name)
        if minimap_match:
            legacy_id = minimap_match.group("legacy_id")
            metadata["configId"] = legacy_id
            return "minimap", legacy_id, metadata

        return "image", stem, metadata

    if "raw" in parts:
        return "legacy_raw", stem, metadata

    return "other", stem, metadata


def load_json_summary(path: Path) -> dict[str, Any]:
    try:
        with path.open("r", encoding="utf-8-sig") as handle:
            data = json.load(handle)
    except (OSError, UnicodeDecodeError, json.JSONDecodeError) as exc:
        return {"parseError": str(exc)}

    if isinstance(data, dict):
        return {
            "jsonType": "object",
            "keys": sorted(str(key) for key in data.keys())[:200],
            "keyCount": len(data),
        }
    if isinstance(data, list):
        return {"jsonType": "array", "length": len(data)}
    return {"jsonType": type(data).__name__}


def scan(root: Path, include_hashes: bool, parse_swf: bool) -> dict[str, Any]:
    records: list[AssetRecord] = []
    by_kind: Counter[str] = Counter()
    config_ids: set[str] = set()
    minimap_ids: set[str] = set()
    map_tiles: dict[str, list[tuple[int, int]]] = defaultdict(list)

    files = (path for path in root.rglob("*") if path.is_file())
    for index, path in enumerate(files, start=1):
        kind, legacy_id, metadata = infer_kind(root, path)
        extension = path.suffix.lower()
        width = height = None

        if extension in {".jpg", ".jpeg", ".png"}:
            width, height = image_size(path)

        if kind == "config":
            config_ids.add(legacy_id)
            if extension == ".json":
                metadata.update(load_json_summary(path))
        elif kind == "minimap":
            minimap_ids.add(legacy_id)
        elif kind == "map_tile":
            map_tiles[metadata["mapId"]].append((metadata["row"], metadata["column"]))
        elif kind == "swf" and parse_swf:
            metadata.update(swf_metadata(path))

        record = AssetRecord(
            legacy_id=legacy_id,
            kind=kind,
            path=path.relative_to(root).as_posix(),
            extension=extension,
            size=path.stat().st_size,
            sha256=sha256_file(path) if include_hashes else None,
            width=width,
            height=height,
            metadata=metadata,
        )
        records.append(record)
        by_kind[kind] += 1

        if index % 1000 == 0:
            print(f"scanned {index:,} files...", file=sys.stderr)

    relationships = []
    for legacy_id in sorted(minimap_ids & config_ids):
        relationships.append({
            "type": "minimap_config",
            "minimapLegacyId": legacy_id,
            "configLegacyId": legacy_id,
        })

    map_catalog: dict[str, Any] = {}
    for map_id, coordinates in sorted(map_tiles.items()):
        rows = [row for row, _ in coordinates]
        columns = [column for _, column in coordinates]
        map_catalog[map_id] = {
            "tileCount": len(coordinates),
            "minRow": min(rows),
            "maxRow": max(rows),
            "minColumn": min(columns),
            "maxColumn": max(columns),
            "rows": len(set(rows)),
            "columns": len(set(columns)),
        }

    return {
        "schemaVersion": 1,
        "sourceRoot": str(root.resolve()),
        "summary": {
            "totalFiles": len(records),
            "totalBytes": sum(record.size for record in records),
            "byKind": dict(sorted(by_kind.items())),
            "minimapConfigMatches": len(minimap_ids & config_ids),
            "minimapsWithoutConfig": sorted(minimap_ids - config_ids),
            "configsWithoutMinimap": sorted(config_ids - minimap_ids),
        },
        "maps": map_catalog,
        "relationships": relationships,
        "assets": [asdict(record) for record in records],
    }


def write_outputs(result: dict[str, Any], output_dir: Path) -> None:
    output_dir.mkdir(parents=True, exist_ok=True)

    (output_dir / "legacy_catalog.json").write_text(
        json.dumps(result, ensure_ascii=False, indent=2), encoding="utf-8"
    )

    summary = {
        "schemaVersion": result["schemaVersion"],
        "sourceRoot": result["sourceRoot"],
        "summary": result["summary"],
        "maps": result["maps"],
    }
    (output_dir / "legacy_summary.json").write_text(
        json.dumps(summary, ensure_ascii=False, indent=2), encoding="utf-8"
    )

    (output_dir / "legacy_relationships.json").write_text(
        json.dumps(result["relationships"], ensure_ascii=False, indent=2), encoding="utf-8"
    )

    missing = {
        "minimapsWithoutConfig": result["summary"]["minimapsWithoutConfig"],
        "configsWithoutMinimap": result["summary"]["configsWithoutMinimap"],
    }
    (output_dir / "legacy_missing.json").write_text(
        json.dumps(missing, ensure_ascii=False, indent=2), encoding="utf-8"
    )


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Catalogue Crystal Saga legacy resources for Eudoria.")
    parser.add_argument("root", type=Path, help="Root of the local decoded resource folder")
    parser.add_argument("--output", type=Path, default=Path("generated/legacy"), help="Output directory")
    parser.add_argument("--hash", action="store_true", help="Calculate SHA-256 for every file (slower on multi-GB dumps)")
    parser.add_argument("--no-swf-parse", action="store_true", help="Skip SWF metadata and exported-symbol parsing")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    root = args.root.expanduser().resolve()
    if not root.is_dir():
        print(f"error: resource root does not exist: {root}", file=sys.stderr)
        return 2

    result = scan(root, include_hashes=args.hash, parse_swf=not args.no_swf_parse)
    write_outputs(result, args.output)
    print(json.dumps(result["summary"], ensure_ascii=False, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
