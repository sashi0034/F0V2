#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Generate `Asset.generated.h` by scanning asset directories.

Default targets (easy to extend via ASSET_SPECS):
- F0V2/asset/shader/**.hlsl  -> namespace Asset_shader
- F0V2/asset/model/**.obj    -> namespace Asset_model

Searches upward from the current directory until a directory named "F0V2" is found.
Writes the header into that root directory.

Entry point: __name__ in {"main", "__main__"}.
"""

from __future__ import annotations
import os
from pathlib import Path
import re
from dataclasses import dataclass
from typing import Dict, List, Tuple, Iterable

ROOT_DIR_NAME = "F0V2"
OUTPUT_FILENAME = "Asset.generated.h"

CPP_VALUE_DECL = "static const inline"
CPP_INCLUDE = ["<string>", "\"ResourcePathWrapper.h\""]


# -----------------------------------------------
# 設定: ここにカテゴリを足すだけで拡張できます

@dataclass(frozen=True)
class AssetSpec:
    namespace: str  # 出力するC++のnamespace名
    typename: str  # パスの型名
    subdir: Tuple[str, ...]  # ルートからの相対ディレクトリ（分割）
    globs: Tuple[str, ...]  # 収集するglobパターン（複数可）


# 例: テクスチャ系を追加したい場合
# AssetSpec("Asset_texture", ("asset", "texture"), ("**/*.png", "**/*.jpg"))
ASSET_SPECS: List[AssetSpec] = [
    AssetSpec("Asset_shader", "GraphicsShaderPathWrapper", ("asset", "shader"), ("**/*.hlsl",)),
    AssetSpec("Asset_image", "ImagePathWrapper", ("asset", "image"), ("**/*.png",)),
    AssetSpec("Asset_model", "ModelPathWrapper", ("asset", "model"), ("**/*.obj",)),
    AssetSpec("Asset_sound", "SoundAudioPathWrapper", ("asset", "sound"), ("**/*.mp3",)),
    AssetSpec("Asset_music", "MusicAudioPathWrapper", ("asset", "music"), ("**/*.mp3",)),
]


def _shader_typename_from_stem(stem: str, default: str) -> str:
    """
    ファイル名末尾のサフィックスから shader 用のラッパ型を決定する
    """
    low = stem.lower()
    if low.endswith("_ps"):
        return "PixelShaderPathWrapper"
    if low.endswith("_vs"):
        return "VertexShaderPathWrapper"
    if low.endswith("_cs"):
        return "ComputeShaderPathWrapper"
    return default


# -----------------------------------------------
# Utilities

def find_root_dir(start: Path, target_name: str) -> Path:
    """Ascend from `start` until a directory whose name equals `target_name` is found."""
    start = start.resolve()
    for p in [start, *start.parents]:
        if p.name == target_name:
            return p
    raise FileNotFoundError(f'Root directory "{target_name}" not found from "{start}".')


_ident_rx = re.compile(r"[^A-Za-z0-9_]")


def make_identifier(stem: str, used: set[str]) -> str:
    """
    Convert filename stem to a valid C++ identifier.
    - Replace non [A-Za-z0-9_] with '_'
    - Prefix '_' if it starts with a digit
    - Ensure uniqueness within its namespace by appending _2, _3, ...
    """
    name = _ident_rx.sub("_", stem)
    if name and name[0].isdigit():
        name = "_" + name
    if not name:
        name = "_"
    base = name
    counter = 2
    while name in used:
        name = f"{base}_{counter}"
        counter += 1
    used.add(name)
    return name


def to_posix_rel(root: Path, p: Path) -> str:
    """Return a forward-slash relative path string from root."""
    return p.resolve().relative_to(root.resolve()).as_posix()


def iter_files(base: Path, patterns: Iterable[str]) -> Iterable[Path]:
    """base 以下を patterns で列挙（存在しない場合は空）。"""
    if not base.is_dir():
        return []
    out: List[Path] = []
    for pat in patterns:
        out.extend(base.rglob(pat))
    # 重複除去して安定化
    return sorted(set(out))


# -----------------------------------------------
# Emission

def emit_namespace(namespace: str, entries: List[Tuple[str, str, str]]) -> str:
    """
    Build C++ namespace block.
    entries: list of (identifier, path_string, typename) それぞれのエントリが個別の型を持てる
    """
    lines = []
    lines.append(f"namespace {namespace}")
    lines.append("{")
    for ident, relpath, typename in entries:
        lines.append(f'    {CPP_VALUE_DECL} {typename} {ident}{{"{relpath}"}};')
    lines.append("}")
    lines.append("")  # trailing newline
    return "\n".join(lines)


# -----------------------------------------------
# Main

def build_entries_for_spec(root: Path, spec: AssetSpec) -> List[Tuple[str, str, str]]:
    """AssetSpec に基づいて (identifier, relpath, typename) のリストを作る。"""
    base = root.joinpath(*spec.subdir)
    files = iter_files(base, spec.globs)

    used: set[str] = set()
    entries: List[Tuple[str, str, str]] = []
    for p in files:
        ident = make_identifier(p.stem, used)
        rel = to_posix_rel(root, p)
        # shader の場合のみ末尾による型分岐
        if spec.namespace == "Asset_shader":
            typename = _shader_typename_from_stem(p.stem, spec.typename)
        else:
            typename = spec.typename
        entries.append((ident, rel, typename))

    # ソートはパスで安定化
    entries.sort(key=lambda kv: kv[1])
    return entries


def main() -> None:
    cwd = Path(os.getcwd())
    root = find_root_dir(cwd, ROOT_DIR_NAME)

    # すべてのカテゴリを処理
    ns_to_entries: Dict[str, List[Tuple[str, str, str]]] = {}
    for spec in ASSET_SPECS:
        ns_to_entries[spec.namespace] = build_entries_for_spec(root, spec)

    # ヘッダー構築
    header_lines: List[str] = []
    header_lines.append("// This file is AUTO-GENERATED. Do not edit manually.")
    header_lines.append("#pragma once")
    for inc in CPP_INCLUDE:
        header_lines.append(f"#include {inc}")
    header_lines.append("")

    for spec in ASSET_SPECS:
        entries = ns_to_entries[spec.namespace]
        if entries:
            header_lines.append(emit_namespace(spec.namespace, entries))
        else:
            header_lines.append(f"namespace {spec.namespace} {{}}\n")

    content = "\n".join(header_lines).rstrip() + "\n"

    out_path = root / OUTPUT_FILENAME
    out_path.write_text(content, encoding="utf-8", newline="\n")

    # Optional console log（各カテゴリの件数も表示）
    counts = ", ".join(
        f"{ns}: {len(ns_to_entries[ns])}" for ns in (spec.namespace for spec in ASSET_SPECS)
    )
    print(f'Generated: {out_path} ({counts})')


# Support both the user's request and the conventional entrypoint.
if __name__ == "__main__":
    main()
