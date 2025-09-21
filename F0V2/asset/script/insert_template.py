#!/usr/bin/env python3
# -*- coding: utf-8 -*-
from __future__ import annotations

import sys
import os
from pathlib import Path

# 設定
target_dir = "F0V2"


def ascend_to(dir_name: str, start: Path) -> Path:
    cur = start.resolve()
    while True:
        if cur.name == dir_name:
            return cur
        parent = cur.parent
        if parent == cur:
            raise FileNotFoundError(f'Could not find target_dir "{dir_name}" upwards from: {start}')
        cur = parent


def is_text_empty(p: Path) -> bool:
    """BOM/空白/改行のみなら空とみなす。存在しない場合は False。"""
    try:
        s = p.read_text(encoding="utf-8-sig", errors="ignore")
    except FileNotFoundError:
        return False
    s = s.replace("\ufeff", "")
    return s.strip() == ""


def read_template_replace(tpl_path: Path, stem: str) -> str:
    """
    テンプレ本文中の {tpl_stem} を対象 stem に置換。
    BOM は除去。
    """
    text = tpl_path.read_text(encoding="utf-8-sig", errors="ignore").replace("\ufeff", "")
    tpl_stem = tpl_path.name

    while "." in tpl_stem:
        tpl_stem = Path(tpl_stem).stem

    text = text.replace(tpl_stem, stem)
    return text


def pick_any_template(dirp: Path, pattern: str) -> Path | None:
    """対象ディレクトリ内で pattern に一致するテンプレを探す。1つ見つければそれを使う。"""
    for p in dirp.glob(pattern):
        if p.is_file():
            return p
    return None


def process_header(h_path: Path, base_dir: Path) -> None:
    stem = h_path.stem
    dirp = h_path.parent

    # ディレクトリ内の *_template.h.txt を探す
    h_tpl = pick_any_template(dirp, "*_template.h.txt")
    if not h_tpl:
        return

    if is_text_empty(h_path):
        content = read_template_replace(h_tpl, stem)
        with h_path.open("a", encoding="utf-8", newline="\n") as f:
            f.write(content)
        print(f"[HEADER] wrote: {h_tpl.relative_to(base_dir)} -> {h_path.relative_to(base_dir)}")
    else:
        return

    # 対応する cpp
    cpp_path = dirp / f"{stem}.cpp"
    cpp_tpl = pick_any_template(dirp, "*_template.cpp.txt")
    if cpp_tpl:
        # if (not cpp_path.exists()) or is_text_empty(cpp_path):
        cpp_content = read_template_replace(cpp_tpl, stem)
        with cpp_path.open("a", encoding="utf-8", newline="\n") as f:
            f.write(cpp_content)
        print(f"[SOURCE] wrote: {cpp_tpl.relative_to(base_dir)} -> {cpp_path.relative_to(base_dir)}")
        # else:
        #     print(f"[SOURCE] skip (non-empty): {cpp_path.relative_to(base_dir)}")
    else:
        print(f"[SOURCE] no template: {dirp.relative_to(base_dir)}/*_template.cpp.txt")


def main() -> int:
    try:
        start_dir = Path.cwd()
        base_dir = ascend_to(target_dir, start_dir)
    except FileNotFoundError as e:
        print(str(e), file=sys.stderr)
        return 1

    os.chdir(base_dir)
    print(f"Base directory: {base_dir}")

    count_total = 0
    count_applied = 0
    for h in base_dir.rglob("*.h"):
        count_total += 1
        if is_text_empty(h):
            before = h.read_text(encoding="utf-8-sig", errors="ignore") if h.exists() else ""
            process_header(h, base_dir)
            after = h.read_text(encoding="utf-8-sig", errors="ignore") if h.exists() else ""
            if before != after:
                count_applied += 1

    print(f"Done. headers scanned: {count_total}, headers updated: {count_applied}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
