#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import struct
from dataclasses import dataclass
from pathlib import Path

from swf_ui_payload import (
    Swf,
    TAG_DEFINE_BUTTON,
    TAG_DEFINE_BUTTON2,
    TAG_DEFINE_EDIT_TEXT,
    TAG_DEFINE_MORPH_SHAPE,
    TAG_DEFINE_SHAPE,
    TAG_DEFINE_SPRITE,
    TAG_DEFINE_TEXT,
)


@dataclass(frozen=True)
class Bounds:
    left: float
    top: float
    right: float
    bottom: float

    @property
    def width(self) -> float:
        return self.right - self.left

    @property
    def height(self) -> float:
        return self.bottom - self.top


def union(a: Bounds | None, b: Bounds | None) -> Bounds | None:
    if a is None:
        return b
    if b is None:
        return a
    return Bounds(
        min(a.left, b.left),
        min(a.top, b.top),
        max(a.right, b.right),
        max(a.bottom, b.bottom),
    )


def transform_bounds(bounds: Bounds, transform: dict) -> Bounds:
    sx = float(transform.get("scaleX", 1.0))
    sy = float(transform.get("scaleY", 1.0))
    r0 = float(transform.get("rotateSkew0", 0.0))
    r1 = float(transform.get("rotateSkew1", 0.0))
    tx = float(transform.get("x", 0.0))
    ty = float(transform.get("y", 0.0))

    points = []
    for x, y in (
        (bounds.left, bounds.top),
        (bounds.right, bounds.top),
        (bounds.left, bounds.bottom),
        (bounds.right, bounds.bottom),
    ):
        points.append((sx * x + r0 * y + tx, r1 * x + sy * y + ty))

    return Bounds(
        min(p[0] for p in points),
        min(p[1] for p in points),
        max(p[0] for p in points),
        max(p[1] for p in points),
    )


class BoundsResolver:
    def __init__(self, swf: Swf):
        self.swf = swf
        self.cache: dict[int, Bounds | None] = {}
        self.active: set[int] = set()

    def _button_up_bounds(self, character_id: int) -> Bounds | None:
        tag = self.swf.definitions[character_id]
        data = self.swf.data
        result: Bounds | None = None

        if tag.code == TAG_DEFINE_BUTTON:
            offset = tag.start + 2
            while offset < tag.end:
                flags = data[offset]
                offset += 1
                if flags == 0:
                    break
                child_id = struct.unpack_from("<H", data, offset)[0]
                offset += 2
                offset += 2
                matrix, offset = self.swf._read_matrix(offset)
                if flags & 0x01:
                    child_bounds = self.resolve(child_id)
                    if child_bounds is not None:
                        result = union(result, transform_bounds(child_bounds, matrix))
            return result

        if tag.code == TAG_DEFINE_BUTTON2:
            offset = tag.start + 2
            offset += 1
            offset += 2
            while offset < tag.end:
                flags = data[offset]
                offset += 1
                if flags == 0:
                    break
                child_id = struct.unpack_from("<H", data, offset)[0]
                offset += 2
                offset += 2
                matrix, offset = self.swf._read_matrix(offset)
                offset = self.swf._skip_cxform_alpha(offset)

                if flags & 0x01:
                    child_bounds = self.resolve(child_id)
                    if child_bounds is not None:
                        result = union(result, transform_bounds(child_bounds, matrix))

                if flags & 0x10:
                    return result
                if flags & 0x20:
                    offset += 1
            return result

        return None

    def resolve(self, character_id: int) -> Bounds | None:
        if character_id in self.cache:
            return self.cache[character_id]
        if character_id in self.active:
            return None

        tag = self.swf.definitions.get(character_id)
        if tag is None:
            self.cache[character_id] = None
            return None

        self.active.add(character_id)
        try:
            result: Bounds | None = None
            if tag.code in TAG_DEFINE_SHAPE or tag.code in TAG_DEFINE_TEXT or tag.code == TAG_DEFINE_EDIT_TEXT:
                rect, _ = self.swf._read_rect(tag.start + 2)
                result = Bounds(rect[0] / 20.0, rect[2] / 20.0, rect[1] / 20.0, rect[3] / 20.0)
            elif tag.code in TAG_DEFINE_MORPH_SHAPE:
                rect, _ = self.swf._read_rect(tag.start + 2)
                result = Bounds(rect[0] / 20.0, rect[2] / 20.0, rect[1] / 20.0, rect[3] / 20.0)
            elif tag.code == TAG_DEFINE_SPRITE:
                for child in self.swf.first_frame(character_id):
                    child_id = child.get("characterId")
                    if child_id is None:
                        continue
                    child_bounds = self.resolve(int(child_id))
                    if child_bounds is None:
                        continue
                    transformed = transform_bounds(child_bounds, child.get("transform", {}))
                    result = union(result, transformed)
            elif tag.code in {TAG_DEFINE_BUTTON, TAG_DEFINE_BUTTON2}:
                result = self._button_up_bounds(character_id)

            self.cache[character_id] = result
            return result
        finally:
            self.active.remove(character_id)


def signed_draw_rect(bounds: Bounds, transform: dict) -> tuple[float, float, float, float] | None:
    r0 = float(transform.get("rotateSkew0", 0.0))
    r1 = float(transform.get("rotateSkew1", 0.0))
    if abs(r0) > 1e-6 or abs(r1) > 1e-6:
        return None

    sx = float(transform.get("scaleX", 1.0))
    sy = float(transform.get("scaleY", 1.0))
    tx = float(transform.get("x", 0.0))
    ty = float(transform.get("y", 0.0))
    return (
        tx + bounds.left * sx,
        ty + bounds.top * sy,
        bounds.width * sx,
        bounds.height * sy,
    )


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate exact static visual manifest for PlayerFullInfoUIMC symbol1998")
    parser.add_argument("swf", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument(
        "--exclude-ids",
        nargs="*",
        type=int,
        default=[],
        help="Character IDs already reconstructed manually; keep them out of the auto layer.",
    )
    args = parser.parse_args()

    swf = Swf(args.swf)
    sprite_id = swf.resolve_symbol("symbol1998")

    excluded = set(args.exclude_ids)
    resolver = BoundsResolver(swf)
    rows: list[dict[str, str]] = []

    for child in swf.first_frame(sprite_id):
        character_id = child.get("characterId")
        if character_id is None:
            continue

        character_id = int(character_id)
        if character_id in excluded:
            continue

        character_type = swf.character_type(character_id)
        if character_type not in {"shape", "shape2", "shape3", "shape4", "sprite", "button", "button2"}:
            continue
        if child.get("visible") is False:
            continue

        bounds = resolver.resolve(character_id)
        if bounds is None or bounds.width <= 0.0 or bounds.height <= 0.0:
            continue

        transform = child.get("transform", {})
        rect = signed_draw_rect(bounds, transform)
        if rect is None:
            continue

        draw_x, draw_y, draw_width, draw_height = rect
        depth = int(child["depth"])
        asset_name = f"d{depth}_c{character_id}.png"
        if character_type in {"button", "button2"}:
            source_frame = "up"
        else:
            source_frame = "100" if character_id == 361 else "1"

        rows.append({
            "depth": str(depth),
            "name": str(child.get("name", "")),
            "characterId": str(character_id),
            "characterType": character_type,
            "drawX": f"{draw_x:.6f}",
            "drawY": f"{draw_y:.6f}",
            "drawWidth": f"{draw_width:.6f}",
            "drawHeight": f"{draw_height:.6f}",
            "asset": asset_name,
            "sourceFrame": source_frame,
        })

    args.output.parent.mkdir(parents=True, exist_ok=True)
    with args.output.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(
            handle,
            fieldnames=[
                "depth",
                "name",
                "characterId",
                "characterType",
                "drawX",
                "drawY",
                "drawWidth",
                "drawHeight",
                "asset",
                "sourceFrame",
            ],
            delimiter="\t",
        )
        writer.writeheader()
        writer.writerows(rows)

    print(f"Wrote {args.output} ({len(rows)} static visuals)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
