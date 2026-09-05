#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import struct
import zlib
from dataclasses import dataclass
from pathlib import Path
from typing import Iterator

TAG_END = 0
TAG_SHOW_FRAME = 1
TAG_DEFINE_SHAPE = {2, 22, 32, 83}
TAG_DEFINE_BUTTON = 7
TAG_DEFINE_BUTTON2 = 34
TAG_DEFINE_SPRITE = 39
TAG_DEFINE_TEXT = {11, 33}
TAG_DEFINE_EDIT_TEXT = 37
TAG_DEFINE_MORPH_SHAPE = {46, 84}
TAG_EXPORT_ASSETS = 56
TAG_PLACE_OBJECT2 = 26
TAG_PLACE_OBJECT3 = 70
TAG_REMOVE_OBJECT2 = 28

DEFAULT_HUD = {
    "playerInfo": {"export": "symbol3550", "class": "playerUI.PlayerInfoUIMC", "x": 0.0, "y": 0.0, "anchor": "top-left"},
    "gameInfo": {"export": "symbol4343", "class": "playerUI.GameInfoUIMC", "x": 0.0, "y": 570.0, "anchor": "bottom-left"},
    "controlBar": {"export": "symbol4131", "class": "playerUI.ControlBarUIMC", "x": 600.0, "y": 640.0, "anchor": "bottom-center"},
    "smallMap": {"export": "symbol1825", "class": "playerUI.SmallMapUIMC", "x": 1200.0, "y": 0.0, "anchor": "top-right"},
    "taskTracer": {"export": "symbol4135", "class": "playerUI.TaskTracerUIMC", "x": 960.0, "y": 230.0, "anchor": "top-right"},
}


class BitReader:
    def __init__(self, data: bytes, offset: int):
        self.data = data
        self.bit_pos = offset * 8

    def read_bits(self, count: int) -> int:
        value = 0
        for _ in range(count):
            byte = self.data[self.bit_pos >> 3]
            shift = 7 - (self.bit_pos & 7)
            value = (value << 1) | ((byte >> shift) & 1)
            self.bit_pos += 1
        return value

    def read_sbits(self, count: int) -> int:
        value = self.read_bits(count)
        if count and value & (1 << (count - 1)):
            value -= 1 << count
        return value

    def align(self) -> None:
        self.bit_pos = (self.bit_pos + 7) // 8 * 8

    @property
    def offset(self) -> int:
        return self.bit_pos // 8


@dataclass(slots=True)
class Tag:
    code: int
    start: int
    end: int


class Swf:
    def __init__(self, path: Path):
        self.path = path
        self.data = self._read(path)
        self.version = self.data[3]
        self.file_length = struct.unpack_from("<I", self.data, 4)[0]
        self.stage_rect_twips, offset = self._read_rect(8)
        self.frame_rate_raw = struct.unpack_from("<H", self.data, offset)[0]
        offset += 2
        self.frame_count = struct.unpack_from("<H", self.data, offset)[0]
        offset += 2
        self.tag_offset = offset
        self.tags = list(self._iter_tags(offset, len(self.data)))
        self.definitions: dict[int, Tag] = {}
        self.exports: dict[str, int] = {}
        self._index()

    @staticmethod
    def _read(path: Path) -> bytes:
        raw = path.read_bytes()
        signature = raw[:3]
        if signature == b"FWS":
            return raw
        if signature == b"CWS":
            return b"FWS" + raw[3:8] + zlib.decompress(raw[8:])
        raise ValueError(f"Unsupported SWF signature: {signature!r}. FWS/CWS are supported.")

    def _read_rect(self, offset: int) -> tuple[list[int], int]:
        reader = BitReader(self.data, offset)
        bits = reader.read_bits(5)
        values = [reader.read_sbits(bits) for _ in range(4)]
        reader.align()
        return values, reader.offset

    def _read_matrix(self, offset: int) -> tuple[dict[str, float], int]:
        reader = BitReader(self.data, offset)
        scale_x = scale_y = 1.0
        rotate_0 = rotate_1 = 0.0
        if reader.read_bits(1):
            bits = reader.read_bits(5)
            scale_x = reader.read_sbits(bits) / 65536.0
            scale_y = reader.read_sbits(bits) / 65536.0
        if reader.read_bits(1):
            bits = reader.read_bits(5)
            rotate_0 = reader.read_sbits(bits) / 65536.0
            rotate_1 = reader.read_sbits(bits) / 65536.0
        bits = reader.read_bits(5)
        tx = reader.read_sbits(bits) / 20.0 if bits else 0.0
        ty = reader.read_sbits(bits) / 20.0 if bits else 0.0
        reader.align()
        return {
            "scaleX": scale_x,
            "scaleY": scale_y,
            "rotateSkew0": rotate_0,
            "rotateSkew1": rotate_1,
            "x": tx,
            "y": ty,
        }, reader.offset

    def _skip_cxform_alpha(self, offset: int) -> int:
        reader = BitReader(self.data, offset)
        has_add = reader.read_bits(1)
        has_mult = reader.read_bits(1)
        bits = reader.read_bits(4)
        if has_mult:
            for _ in range(4):
                reader.read_sbits(bits)
        if has_add:
            for _ in range(4):
                reader.read_sbits(bits)
        reader.align()
        return reader.offset

    @staticmethod
    def _read_cstring(data: bytes, offset: int, end: int) -> tuple[str, int]:
        zero = data.find(b"\0", offset, end)
        if zero < 0:
            return "", end
        return data[offset:zero].decode("utf-8", "replace"), zero + 1

    def _iter_tags(self, start: int, end: int) -> Iterator[Tag]:
        offset = start
        while offset + 2 <= end:
            record = struct.unpack_from("<H", self.data, offset)[0]
            offset += 2
            code = record >> 6
            length = record & 0x3F
            if length == 0x3F:
                length = struct.unpack_from("<I", self.data, offset)[0]
                offset += 4
            tag = Tag(code, offset, offset + length)
            yield tag
            offset += length
            if code == TAG_END:
                break

    def _index(self) -> None:
        character_tags = TAG_DEFINE_SHAPE | TAG_DEFINE_TEXT | TAG_DEFINE_MORPH_SHAPE | {
            TAG_DEFINE_BUTTON,
            TAG_DEFINE_BUTTON2,
            TAG_DEFINE_EDIT_TEXT,
            TAG_DEFINE_SPRITE,
        }
        character_tags |= {6, 10, 14, 17, 20, 21, 35, 36, 48, 60, 61, 62, 73, 74, 75, 87, 90, 91}

        for tag in self.tags:
            if tag.code in character_tags and tag.end - tag.start >= 2:
                character_id = struct.unpack_from("<H", self.data, tag.start)[0]
                self.definitions[character_id] = tag
            elif tag.code == TAG_EXPORT_ASSETS:
                count = struct.unpack_from("<H", self.data, tag.start)[0]
                offset = tag.start + 2
                for _ in range(count):
                    character_id = struct.unpack_from("<H", self.data, offset)[0]
                    offset += 2
                    name, offset = self._read_cstring(self.data, offset, tag.end)
                    self.exports[name] = character_id

    def _parse_place2(self, tag: Tag) -> dict:
        offset = tag.start
        flags = self.data[offset]
        offset += 1
        depth = struct.unpack_from("<H", self.data, offset)[0]
        offset += 2
        result = {"depth": depth, "move": bool(flags & 0x01)}
        if flags & 0x02:
            result["characterId"] = struct.unpack_from("<H", self.data, offset)[0]
            offset += 2
        if flags & 0x04:
            result["transform"], offset = self._read_matrix(offset)
        if flags & 0x08:
            offset = self._skip_cxform_alpha(offset)
        if flags & 0x10:
            offset += 2
        if flags & 0x20:
            result["name"], offset = self._read_cstring(self.data, offset, tag.end)
        if flags & 0x40:
            offset += 2
        return result

    def _parse_place3(self, tag: Tag) -> dict:
        offset = tag.start
        flags1 = self.data[offset]
        flags2 = self.data[offset + 1]
        offset += 2
        depth = struct.unpack_from("<H", self.data, offset)[0]
        offset += 2
        result = {"depth": depth, "move": bool(flags1 & 0x01)}
        if (flags2 & 0x08) or ((flags2 & 0x10) and (flags1 & 0x02)):
            result["className"], offset = self._read_cstring(self.data, offset, tag.end)
        if flags1 & 0x02:
            result["characterId"] = struct.unpack_from("<H", self.data, offset)[0]
            offset += 2
        if flags1 & 0x04:
            result["transform"], offset = self._read_matrix(offset)
        if flags1 & 0x08:
            offset = self._skip_cxform_alpha(offset)
        if flags1 & 0x10:
            offset += 2
        if flags1 & 0x20:
            result["name"], offset = self._read_cstring(self.data, offset, tag.end)
        if flags1 & 0x40:
            offset += 2
        if flags2 & 0x20:
            result["visible"] = bool(self.data[offset])
        return result

    def first_frame(self, sprite_id: int) -> list[dict]:
        definition = self.definitions[sprite_id]
        if definition.code != TAG_DEFINE_SPRITE:
            raise ValueError(f"Character {sprite_id} is not a DefineSprite")
        offset = definition.start + 4
        display_list: dict[int, dict] = {}
        for tag in self._iter_tags(offset, definition.end):
            if tag.code == TAG_PLACE_OBJECT2:
                item = self._parse_place2(tag)
            elif tag.code == TAG_PLACE_OBJECT3:
                item = self._parse_place3(tag)
            elif tag.code == TAG_REMOVE_OBJECT2:
                depth = struct.unpack_from("<H", self.data, tag.start)[0]
                display_list.pop(depth, None)
                continue
            elif tag.code == TAG_SHOW_FRAME:
                break
            else:
                continue

            depth = item["depth"]
            if item["move"] and depth in display_list:
                merged = dict(display_list[depth])
                merged.update(item)
                display_list[depth] = merged
            else:
                display_list[depth] = item

        result = []
        for depth in sorted(display_list):
            item = dict(display_list[depth])
            item.pop("move", None)
            result.append(item)
        return result

    def character_type(self, character_id: int) -> str:
        definition = self.definitions.get(character_id)
        if not definition:
            return "unknown"
        return {
            2: "shape", 22: "shape2", 32: "shape3", 83: "shape4",
            7: "button", 34: "button2", 39: "sprite",
            11: "text", 33: "text2", 37: "editText",
            46: "morphShape", 84: "morphShape2",
        }.get(definition.code, f"tag{definition.code}")

    def sprite_payload(self, export_name: str) -> dict:
        sprite_id = self.exports[export_name]
        children = self.first_frame(sprite_id)
        for child in children:
            cid = child.get("characterId")
            if cid is not None:
                child["characterType"] = self.character_type(cid)
        return {
            "export": export_name,
            "characterId": sprite_id,
            "children": children,
        }


def main() -> int:
    parser = argparse.ArgumentParser(description="Extract Crystal Saga legacy UI display-list payloads from assets.swf")
    parser.add_argument("swf", type=Path)
    parser.add_argument("--output", type=Path, default=Path("generated/ui/hud.legacy.json"))
    parser.add_argument("--exports", nargs="*", help="Override exported symbols. Defaults to the confirmed main HUD.")
    args = parser.parse_args()

    swf = Swf(args.swf)
    roots = DEFAULT_HUD if not args.exports else {
        name: {"export": name, "class": None, "x": 0, "y": 0, "anchor": "top-left"}
        for name in args.exports
    }

    components = []
    for component_id, root in roots.items():
        if root["export"] not in swf.exports:
            raise KeyError(f"SWF export not found: {root['export']}")
        payload = swf.sprite_payload(root["export"])
        payload.update({
            "id": component_id,
            "legacyClass": root["class"],
            "root": {"x": root["x"], "y": root["y"], "anchor": root["anchor"]},
        })
        components.append(payload)

    xmin, xmax, ymin, ymax = swf.stage_rect_twips
    output = {
        "schemaVersion": 1,
        "source": str(args.swf),
        "swf": {
            "version": swf.version,
            "frameRate": swf.frame_rate_raw / 256.0,
            "frameCount": swf.frame_count,
            "stage": {
                "width": (xmax - xmin) / 20.0,
                "height": (ymax - ymin) / 20.0,
            },
        },
        "legacyGameStage": {"width": 1200, "height": 640},
        "components": components,
    }

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(output, ensure_ascii=False, indent=2), encoding="utf-8")
    print(f"Wrote {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
