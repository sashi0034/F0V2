#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Generate `AssetPath.generated.h` by scanning:
- F0V2/asset/shader/**.hlsl
- F0V2/asset/model/**.obj

Searches upward from the current directory until a directory named "F0V2" is found.
Writes the header into that root directory.

Entry point: __name__ == "main" (also supports the usual "__main__").
"""

from __future__ import annotations
import os
from pathlib import Path
import re
from typing import Dict, List, Tuple

ROOT_DIR_NAME = "F0V2"
OUTPUT_FILENAME = "Assets.generated.h"


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


def collect_paths(root: Path) -> Tuple[List[Path], List[Path]]:
    """Collect shader .hlsl paths and model .obj paths under root."""
    shader_root = root / "asset" / "shader"
    model_root = root / "asset" / "model"

    shader_files: List[Path] = []
    model_files: List[Path] = []

    if shader_root.is_dir():
        shader_files = sorted(shader_root.rglob("*.hlsl"))
    if model_root.is_dir():
        model_files = sorted(model_root.rglob("*.obj"))

    return shader_files, model_files


def to_posix_rel(root: Path, p: Path) -> str:
    """Return a forward-slash relative path string from root."""
    return p.resolve().relative_to(root.resolve()).as_posix()


def emit_namespace(namespace: str, entries: List[Tuple[str, str]]) -> str:
    """
    Build C++ namespace block.
    entries: list of (identifier, path_string)
    """
    lines = []
    lines.append(f"namespace {namespace}")
    lines.append("{")
    for ident, relpath in entries:
        lines.append(f'    static const inline std::string {ident} = "{relpath}";')
    lines.append("}")
    lines.append("")  # trailing newline
    return "\n".join(lines)


# -----------------------------------------------
# Main

def main() -> None:
    cwd = Path(os.getcwd())
    root = find_root_dir(cwd, ROOT_DIR_NAME)

    shader_files, model_files = collect_paths(root)

    # Prepare entries
    shader_used: set[str] = set()
    model_used: set[str] = set()

    shader_entries: List[Tuple[str, str]] = []
    for p in shader_files:
        ident = make_identifier(p.stem, shader_used)
        rel = to_posix_rel(root, p)
        shader_entries.append((ident, rel))

    model_entries: List[Tuple[str, str]] = []
    for p in model_files:
        ident = make_identifier(p.stem, model_used)
        rel = to_posix_rel(root, p)
        model_entries.append((ident, rel))

    # Sort entries by their relative path for deterministic output
    shader_entries.sort(key=lambda kv: kv[1])
    model_entries.sort(key=lambda kv: kv[1])

    # Build header content
    header_lines: List[str] = []
    header_lines.append("// This file is AUTO-GENERATED. Do not edit manually.")
    header_lines.append("#pragma once")
    header_lines.append("#include <string>")
    header_lines.append("")
    if shader_entries:
        header_lines.append(emit_namespace("Asset_shader", shader_entries))
    else:
        header_lines.append("namespace Asset_shader {}\n")
    if model_entries:
        header_lines.append(emit_namespace("Asset_model", model_entries))
    else:
        header_lines.append("namespace Asset_model {}\n")

    content = "\n".join(header_lines).rstrip() + "\n"

    out_path = root / OUTPUT_FILENAME
    out_path.write_text(content, encoding="utf-8", newline="\n")

    # Optional console log
    print(f'Generated: {out_path} ({len(shader_entries)} shaders, {len(model_entries)} models)')


# Support both the user's request and the conventional entrypoint.
if __name__ == "__main__":
    main()
